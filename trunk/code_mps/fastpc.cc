#include "fastpc.h"

#include <iomanip>
#include<fstream>
#include <ctime>
#include <cstdlib>

using namespace std;


nonzero_entry_t::nonzero_entry_t(double value, double eps, sampler_item_t* sampler, sampler_item_t* u_sampler):
  coeff(value),  
  sampler_pointer(sampler), 
  u_sampler_pointer(u_sampler) {
  //always round to make approximation an upper bd on value of coefficient
  if (eps > 0) //dual
    exponent = (int)floor(log_base(value, 1-eps));   // =log base 1-eps (value)
  else //primal
    exponent = (int)ceil(log_base(value, 1-eps));    // =log base 1-eps (value)
}

void solve_instance::bound_sort() {
  double b;
  bound_exponents(M, MT, b);
  // bound_exponents(M_copy, MT, b);//scale by original problem
  bound_exponents(MT, M_copy, b);

  if(sort_ratio <= 1) {
    exact_sort(M);
    exact_sort(MT);
  } else {
    pseudo_sort(M,MT.size(), b);
    pseudo_sort(MT, M.size(), b);
  }
}

void solve_instance::pseudo_sort( my_vector<line_element>& matrix, int n_cols, double b ){
  int n_rows = matrix.size();
 
  //create buckets for the  bucket sort
  int num_buckets = (int)ceil(log_base(n_cols/eps,sort_ratio))*2;
  my_vector<nonzero_entry_t*>*  *buckets = (my_vector<nonzero_entry_t*>**)(malloc( sizeof(my_vector<nonzero_entry_t*>*) * num_buckets));    
  for(int i = 0; i < num_buckets; i++)
    buckets[i] =  new my_vector<nonzero_entry_t*>();

  // bucket sort each row
  for (int i=0; i < n_rows; i++) {
    for(my_vector<nonzero_entry_t*>::iterator x = matrix[i].begin(); x != matrix[i].end(); ++x){
      //index is normalized such that index of smallest item will be 0 and index of largest item will be 2*log(n_cols/eps) = log(n_cols^2/eps^2)
      int index = (int)floor(log_base((*x)->coeff,sort_ratio) - log_base(b,sort_ratio) + log_base(n_cols/eps,sort_ratio));
      buckets[num_buckets-1-index]->push_back((*x));  //sorts in decreasing order
    }
    
    //add items from buckets back into the matrix
    //line_element::iterator row = p->begin();
    line_element::iterator row = matrix[i].begin();
       
    //scan through all sorted items and reset row items to these
    for(int i = 0; i < num_buckets; i++){
      for (my_vector<nonzero_entry_t *>::iterator x = buckets[i]->begin(); x != buckets[i]->end(); ++x){
	(*row) = (*x); //reset current pointer in row to pointer in bucket
	row++;
      }
    }
    //erase elements in buckets so they can be used for next row
    for(int i = 0; i<num_buckets; i++)
      buckets[i]->clear();
  }
  //deallocate memory for  buckets
  for(int i =0; i < num_buckets; i++)
    free(buckets[i]);
  free(buckets);
}

void 
solve_instance::exact_sort(my_vector<line_element>& matrix) { //uses c function to exactly sort matrices
  line_element* first = &matrix[0];
  line_element* last = &matrix[matrix.size()-1];
  for (line_element* p = first; p <= last; ++p) {
    p->sort_desc();
  }
}

void 
solve_instance::bound_exponents( my_vector<line_element>& matrix, my_vector<line_element>& matrix_T, double& b) {
  //find b
  int n_row = matrix.size();
  int n_col = matrix_T.size();

  b = -1; // sentinel to mark first loop
  //for (line_element *p = first_t; p <= last_t; ++p) {
  for (int i=0; i < n_col; ++i) {
    double max_temp = matrix_T[i].front()->coeff;    
    for(my_vector<nonzero_entry_t*>::iterator x = matrix_T[i].begin(); x != matrix_T[i].end(); ++x){ 
      if ((*x)->coeff > max_temp){
	max_temp = (*x)->coeff;   
      }
    }
    if (max_temp < b || b == -1){
      b = max_temp;
    }
  }
  //scale all  elements
  double bound = b*eps/n_col;
  double replace = b*n_col/eps;
  //for (line_element* p = first; p <= last; ++p) {
  for (int i=0; i < n_row; ++i) {
    for (int j = 0; j < matrix[i].size(); j++) {
      if (matrix[i][j]->coeff < bound) {
	matrix[i].remove(j);
      } else {
	matrix[i][j]->coeff = min(matrix[i][j]->coeff,replace);
      }
    }
  }
}

solve_instance::solve_instance(double EPSILON, string infile, int SORT_RATIO) :
  eps(0.9*EPSILON),
  epsilon(EPSILON),
  file_name(infile),
  sort_ratio(SORT_RATIO),
  p_shift_ratio(-1),
  d_shift_ratio(-1)
{
  // constructor reads the input and creates matrices M and MT (transpose)
  int row, col, total;
  double val;
  string s;

  //bookkeeping
  unsigned long preprocess_start = get_time();
  {
    //open input file
    ifstream in_file;
    in_file.open(file_name.c_str());

    if (in_file.fail()) {
      cout << "Error opening " << file_name << endl;
      exit(1);
    }

    cout << "INPUT FILE: " << file_name << endl;
    //read and parse 1st line of input (parameters)
    string s;
    in_file >> r >> c >> total;
    cout << "ROWS: " <<  r << " COLUMNS: " << c << " NON-ZEROS: " << total 
	 << " DENSITY: " << (double)total/((double)r*(double)c)<< endl;

    M.resize(r);
    MT.resize(c);
    M_copy.resize(r);

    N = int(ceil(2*(log(r)+log(c))/(eps*eps)));

    cout << "N = " << N << endl;
    
    int max_exp = N+10;
 
    p_p = new primal_sampler_t(r, eps, 0, max_exp);
    p_d = new dual_sampler_t(c, eps, 0, max_exp);
    p_pXuh = new primal_u_sampler_t(r, eps, 0, max_exp);
    p_dXu = new dual_u_sampler_t(c, eps, 0, max_exp);
    
    assert(p_p);
    assert(p_d);
    assert(p_pXuh);
    assert(p_dXu);

    p_p->init();
    p_d->init();
    p_pXuh->init();
    p_dXu->init();

   non_zero_entry_count = 0;
    while(true) {
      if (non_zero_entry_count > total) break; //stop scanning input if all nonzeros have been scanned
      in_file >> row  >> col >> val;  //took out string s 
      if (in_file.eof()) break;
      //cout <<row<<endl;
      //cout <<col<<endl;
      //cout <<val <<endl;
      //cout <<M.size()<<endl;
      M[row].push_back(new nonzero_entry_t(val,eps*(-1.0), p_d->get_ith(col), p_dXu->get_ith(col)));
      M_copy[row].push_back(new nonzero_entry_t(val, eps*(-1.0), p_d->get_ith(col), p_dXu->get_ith(col)));
      MT[col].push_back(new nonzero_entry_t(val, eps, p_p->get_ith(row), p_pXuh->get_ith(row)));
      non_zero_entry_count++;
    }
    
    //close file
    in_file.close();

    //sort or pseudo-sort M and MT, bounding their coefficients
    bound_sort();
   	
    //find the minimum exponent for MT and maximum exponent for M
    //this is done to normalize the exponents so that all exponents in 
    //dual samplers are positive and all exponents in primal samplers
    //are negative
    int min_u_exp = MT[0].front()->exponent;
    int max_uh_exp = M[0].front()->exponent;
    int min_uh_exp = M[0].front()->exponent;
    
    for (int j = 0; j < c; j++) {
      int temp = MT[j].front()->exponent;
      if (temp < min_u_exp)
				min_u_exp = temp;
    }
    for (int i = 0; i < r; i++) {
      int temp = M[i].front()->exponent;
      if (temp > max_uh_exp)
       	max_uh_exp = temp;
      if (temp < min_uh_exp)
	min_uh_exp = temp;
    }

    //shift the min exponent to the min exponent of the second bucket from left
    //in the primal sampler if it's too small
    int p_diff = 0;
    if (N+10-ceil(1/eps) > 0 && max_uh_exp - min_uh_exp > N+10-ceil(1/eps)) {
      p_diff = max_uh_exp - min_uh_exp - (N+10-(int)ceil(1/eps));
    }
   
    // re-initialize u_sampler_items with normalized exponents if necessary
    // min_u_exp may not be the actual min and so we compensate by adding the exponent for sort_ratio
    int d_exp_shift_init = min_u_exp + (int)ceil(log_base(sort_ratio, 1-eps));
    if (min_u_exp != 0) {
      for (int i = 0; i < c; i++) {
	sampler_item_t* item = p_dXu->get_ith(i);
	int exponent = MT[i].front()->exponent - d_exp_shift_init;
	if (exponent > max_exp) {
	  item->exponent_overflow = exponent - max_exp; //initialize overflow if necessary
	  exponent = max_exp;
	}
	p_dXu->update_item_exponent(item,exponent);
      }
      p_dXu->exp_shift -= d_exp_shift_init;  //initial shift of sampler-- can be negative
      p_dXu->exp_shift_updated = true;
    }

    int p_exp_shift_init = max_uh_exp - p_diff + (int)ceil(log_base(sort_ratio, 1+eps)); //total shift amt needed
    if (p_exp_shift_init != 0) {
      for (int j = 0; j < r; j++) {
	sampler_item_t* item = p_pXuh->get_ith(j);
	int exponent = M[j].front()->exponent - p_exp_shift_init;
	if (exponent > 0) {
	  item->exponent_overflow = exponent;
	  exponent = 0;
	}
	p_pXuh->update_item_exponent(item,exponent);
      }
      p_pXuh->exp_shift -= p_exp_shift_init;   //initial shift of sampler-- can be negative
      p_pXuh->exp_shift_updated = true;
    }

    unsigned long preprocess_time = get_time() - preprocess_start;
    cout << "preprocessing_time = " << preprocess_time/1000000.0 << " s" << endl;

    //print normalized exponents
    //print M
    //cout << "PRINTING M: SUBTRACTED " << max_uh_exp-p_diff << " TO NORMALIZE.\n";
//     for (int i = 0; i < r; ++i) {
//       for(my_vector<nonzero_entry_t*>::iterator x = M[i].begin(); x != M[i].end(); ++x){
// 	cout << "Coeff:" << (*x)->coeff << " "; //<< " Exponent:" << p_pXuh->get_ith(i)->exponent_entry->exponent 
// 	  //<< " Overflow:" << (*x)->u_sampler_pointer->exponent_overflow << endl;   //debug
//       }
//       cout << "\n";
//     }

//     //print MT
//     cout << "PRINTING MT: SUBTRACTED " << min_u_exp << " TO NORMALIZE.\n";
//     for (int j=0; j < c; ++j) {
//         for(my_vector<nonzero_entry_t*>::iterator x = MT[j].begin(); x != MT[j].end(); ++x){
// 	  cout << "Coeff:" << (*x)->coeff << " "; //" Exponent:" << p_dXu->get_ith(j)->exponent_entry->exponent 
// 	    //<< " Overflow:" << (*x)->u_sampler_pointer->exponent_overflow << endl;   //debug
//       }
// 	cout << "\n";
//     }
    //end print normalized exponents
  }
}

void
solve_instance::solve() {
  int J_size = c;	  // we have c active columns at the beginning

  bool done = false;
  unsigned iteration = 0;

  unsigned long n_samples = 0;
  unsigned long n_deletes = 0;
  unsigned long n_increments_p = 0;
  unsigned long n_increments_d = 0;
  unsigned long start_time = get_time();

  double assumed_ops_per_usec = 1.20e2; // ckou's machine
  double assumed_time_per_op = 1/assumed_ops_per_usec;

  cout << "predict at most " << double(N)*(r+c) << " increments" << endl;
  cout << "predict at most " << double(N)*eps/2 << " rebuilds of each sampler" << endl;
  cout << "predict " << (253.0/2*(double)(r+c)*log_base(non_zero_entry_count,2)/(eps*eps))*assumed_time_per_op/1000000 << " seconds ";

  //cout << "predict " << (365.0/2*(double)(r+c)*log(non_zero_entry_count)/(eps*eps))*assumed_time_per_op/1000000 << " seconds ";//do not know what the divide by 2is for.
  
  cout << "assuming " << assumed_ops_per_usec << "ops per usec" << endl;

  //srand(time(0));

  while (!done){
    iteration++;
    //cout <<"iteration "<<iteration<<endl;

    //call function to freeze samplers and test them
    // if (iteration == N){ //arbitrary freeze point in middle of alg 
//       int prob_reciprocal = 10;   //prob is less than 1/6 of x deviating from E[x] by eps-factor
//       freeze_and_sample(M, MT, r, c, p_d, p_p, p_dXu, p_pXuh, eps, prob_reciprocal);
//       //exit(0); //don't continue with alg; just stop after sampler test
//     }

    //wi and wj store the sampler items which are chosen
    //caveat-- could be from regular primal/dual samplers OR from p_pXuh/p_dXu, so the full
    //value of each x will be distributed between TWO samplers for both primal and dual variables
    sampler_item_t* wi;
    sampler_item_t* wj;
    random_pair(&wi, &wj, p_p, p_d, p_pXuh, p_dXu);
    
    int i = wi->i;
    int j = wj->i;
    ++n_samples;

    // line 6
    nonzero_entry_t* front_active = get_largest_active(&M[i]);
    double uh_i;
    if (front_active == NULL) {//if all columns in the selected row are marked for deletion
      uh_i = 0;
      p_p->remove(p_p->get_ith(i));
      p_pXuh->remove(p_pXuh->get_ith(i));
      continue;
    }
    else{
      uh_i = sort_ratio*(front_active->coeff); //include sort_ratio b/c of pseudo-sort
    }

    double u_j = MT[j].front()->coeff;
    double delta = 1/(uh_i + u_j);
    wj->x += delta;
    wi->x += delta;
 
    // line 7
    register double z = (rand()%1000)/999.0;
 
    //line 8
    {
      register my_vector<nonzero_entry_t*>::iterator mt_end = MT[j].end();

      
      for (register my_vector<nonzero_entry_t*>::iterator x = MT[j].begin(); x != mt_end; ++x) { 
	double increment = ((*x)->coeff)*delta;
	count_ops(10);  // 3 ops for floating-pt mult
	//if the row is not active any more
	if ((*x)->u_sampler_pointer->removed){
	  continue;
	}	

	if (sort_ratio*increment >= z) { //must include sort_ratio factor b/c elements could be out of order if pseudo-sorting
	  if (increment >= z) {
	    count_ops(6);
	    n_increments_p++;
	    p_pXuh->increment_exponent((*x)->u_sampler_pointer);
	    // stop when a packing constraint becomes tight
	    if (p_p->increment_exponent((*x)->sampler_pointer) >= N)
	      done = true;  
	  }
	} else {
	  break;
	}
      }

    }

    // line 9 
    {
      register my_vector<nonzero_entry_t*>::iterator m_end = M[i].end();
      int num_removed = 0; // number of items removed
      int index_removed = 0; // index of the last item deleted
      int index = 0; // number of elements iterated through



      for (register my_vector<nonzero_entry_t*>::iterator x = M[i].begin(); x != m_end; ++x) {
	count_ops(11);
	double increment = ((*x)->coeff)*delta;
	
	//if the column is not active any more
	if ((*x)->sampler_pointer->removed){
	  index_removed = index;
	  num_removed++;
	  index++;
	  continue;
	}	
	index++;
	if (sort_ratio*increment >= z) { //must include sort_ratio factor b/c elements could be out of order if pseudo-sorting
	  if (increment >= z) {
	    count_ops(13);

	    n_increments_d++;
	    p_dXu->increment_exponent((*x)->u_sampler_pointer);
	    // remove covering constraint when it's met
	    if (p_d->increment_exponent((*x)->sampler_pointer) >= N) {	    
	      //update p_pXuh if uh_i changed for that row
	      //to do this, iterate through dropped column in MT and update uh_i for each row if necessary
	      int colIndex = (*x)->sampler_pointer->i;
	      for (my_vector<nonzero_entry_t*>::iterator y = MT[colIndex].begin(); y != MT[colIndex].end(); ++y) {
		count_ops(9);
		int rowIndex = (*y)->sampler_pointer->i; //locate which row this entry is in
		nonzero_entry_t* row_active_first = NULL;
		nonzero_entry_t* row_active_second = NULL;
		get_two_largest_active(&M[rowIndex], &row_active_first, &row_active_second);
 
		//update exponents for new uh_i if y is first active element in row, 
		//i.e. current uh_i-value came from y and y is not the only remaining item
		if(row_active_first == *y && row_active_second != NULL) {    
		  int exp_diff = row_active_first->exponent - row_active_second->exponent; //dif b/t exponents of first and second active elements of row
		  if (exp_diff != 0) { //if y and next element don't have same exponent
		    p_pXuh->update_item_exponent(row_active_first->u_sampler_pointer, (row_active_first->u_sampler_pointer->exponent_entry->exponent - exp_diff));
		  }
		}
	      }
	      p_d->remove((*x)->sampler_pointer);
	      p_dXu->remove((*x)->u_sampler_pointer);
	    	    
	      --J_size;
	    }
	  }
	} else {
	  break;
	}
      }
      if (num_removed > 0) {
	compress_forward(&M[i], index_removed);
      }
    }
    if (J_size == 0) 
      done = true; 
    // stop when there is no active covering constraint (all are met)
  }

  // end of iterations. compute final x_p and x_d values as in line 10 of the algorithm
  // cout<<"end of iterations"<<endl;

  unsigned long main_loop_time = get_time() - start_time;
  
  count_ops(4*r);

  double sum_x_p=0, sum_x_d=0;

  double max_row = 0;
  for (int i=0; i<r; ++i){
    double tmp = 0;
    count_ops(7*M_copy[i].size());
    for (line_element::iterator iter = M_copy[i].begin(); 
	 iter != M_copy[i].end(); 
	 ++iter) {
      tmp += (*iter)->coeff * ((*iter)->sampler_pointer->x + (*iter)->u_sampler_pointer->x); //each sampler item stores part of var's value 
    }
    if (tmp > max_row)
      max_row = tmp;
    sum_x_d += (p_p->get_ith(i)->x + p_pXuh->get_ith(i)->x);
  }

  count_ops(6*c);

  double min_col = long(4*(N+2));
  for (int j=0; j<c; ++j){
    double tmp = 0;
    count_ops(7*MT[j].size());
    for (line_element::iterator iter = MT[j].begin();
         iter != MT[j].end(); 
	 ++iter) {
      tmp += (*iter)->coeff * ((*iter)->sampler_pointer->x + (*iter)->u_sampler_pointer->x); //each sampler item stores part of var's value 
    }
    if (tmp < min_col)
      min_col = tmp;
    int p_var = p_d->get_ith(j)->x + p_dXu->get_ith(j)->x;
    sum_x_p += p_var;
  }

  ofstream out;
  char* sol_file_name = "fastpc_solution";
  out.open(sol_file_name);
  for(int index=0; index < c; ++index){
    out << (p_d->get_ith(index)->x + p_dXu->get_ith(index)->x)/max_row << endl;
  }
  out.close();
  cout << "Solution written in file '" << sol_file_name << "'." << endl;
  count_ops(2*c); //for totaling primal vars
  
  count_ops(2*r); //for totaling dual vars

  cout << "iterations = " << iteration;
  if (max_row == 0)
    cout << " primal = infinity ";
  else
    cout << " primal = " << double(sum_x_p)/max_row;
  if (min_col == 0)
    cout << " dual = infinity " << endl;
  else
    cout << " dual = " << double(sum_x_d)/min_col;
  if (max_row != 0)
    cout << " ratio = " << (double(sum_x_p)*min_col)/(max_row * sum_x_d);
  cout << endl;

  unsigned long elapsed_time = get_time() - start_time;
  double ops_per_increment = double(basic_ops) / (n_increments_p + n_increments_d);
  double ops_per_usec = basic_ops/elapsed_time;

  cout << " eps = " << eps << endl;
  cout << " epsilon = " << epsilon << endl;
  cout << " sort ratio = " << sort_ratio << endl;
  cout << " n_samples = " << n_samples << endl;
  cout << " n_increments_d = " << n_increments_d << endl;
  cout << " n_increments_p = " << n_increments_p << endl;
  cout << " n_increments = " << n_increments_p+n_increments_d << endl;
  cout << " entries_deleted = " << n_deletes << endl;
  cout << " constraints_deleted = " << c-J_size << endl;
  cout << " n_rebuilds_p = " << p_p->n_rebuilds() << endl;
  cout << " n_rebuilds_d = " << p_d->n_rebuilds() << endl;
  cout << " n_rebuild_ops = " << p_d->n_rebuild_ops() << endl;
  cout << " basic_ops = " << basic_ops << endl;
  cout << " ops_per_increment = " << ops_per_increment << endl;
  cout << " main_loop_time = " << main_loop_time/1000000.0 << "s" << endl;
  cout << " time = " << elapsed_time/1000000.0 << "s" << endl;
  cout << " ops_per_usec = " << ops_per_usec << endl;
  cout << " alloc_time = " << alloc_time/1000000.0 << "s" << endl;
  cout << " alloc_space = " << alloc_space << endl;
  cout << " allocs_per_usec = " << alloc_space/alloc_time << endl;
  cout << endl;
}

void solve_instance::random_pair(sampler_item_t** wi,sampler_item_t** wj, dual_sampler_t* p_p, dual_sampler_t* p_d, dual_sampler_t* p_pXuh, dual_sampler_t* p_dXu) {
  weight_t p_p_wt = p_p->get_update_total_weight();
  weight_t p_d_wt = p_d->get_update_total_weight();
  weight_t p_pXuh_wt = p_pXuh->get_update_total_weight();
  weight_t p_dXu_wt = p_dXu->get_update_total_weight();

  //store partial quantities of desired ratio
  double temp_1;
  double temp_2;
  double temp_3;

  //shift in primal is tricky since it moves in a way opposite the dual
  if (p_p->exp_shift_updated || p_pXuh->exp_shift_updated) {
    p_shift_ratio = p_p->shift_exp_weight(p_pXuh->get_exponent_shift() - p_p->get_exponent_shift());
    p_p->exp_shift_updated = false;
    p_pXuh->exp_shift_updated = false;
  }
  if (p_d->exp_shift_updated || p_dXu->exp_shift_updated) {
    d_shift_ratio = p_d->shift_exp_weight(p_d->get_exponent_shift() - p_dXu->get_exponent_shift());
    p_d->exp_shift_updated = false;
    p_dXu->exp_shift_updated = false;
  }

  count_ops(60); //20 per float division
  temp_1 = (double)p_p_wt/p_pXuh_wt;
  temp_2 = (double)p_dXu_wt/p_d_wt;
  temp_3 = (double)p_shift_ratio/d_shift_ratio;  //must keep ratios separate since they use different epsilons

  count_ops(23);
  double prob = 1.0 / (1 + (temp_1*temp_2*temp_3));  //overall prob used to choose samplers
  
  while (1) {  //go until neither chosen sampler fails
    count_ops(60); //30 for z, 30 to acct for multiple sampling
    float z = (rand()%1000)/999.0;
  
    if (prob > z) {
      *wi = p_pXuh->sample();
      if ((*wi) == NULL)
	continue;
      *wj = p_d->sample();
      if ((*wj) == NULL)
	continue;
      break;
    }
    else {
      *wi = p_p->sample();
      if ((*wi) == NULL)
	continue;
      *wj = p_dXu->sample();
      if ((*wj) == NULL)
	continue;
      break;
    }
  }
}

nonzero_entry_t* solve_instance::get_largest_active(line_element* row) {
  for (my_vector<nonzero_entry_t*>::iterator	 y = (*row).begin(); y != (*row).end(); ++y) {
    count_ops(3);
    if (!(*y)->sampler_pointer->removed) {
      return *y;
    }
  }
  return NULL;  //gets here when all row items have been marked for deletion
}

void solve_instance::get_two_largest_active(line_element* row, nonzero_entry_t** first, nonzero_entry_t** second) {
  bool got_first = false;
  for (my_vector<nonzero_entry_t*>::iterator y = (*row).begin(); y != (*row).end(); ++y) {
    count_ops(3);
    if (!(*y)->sampler_pointer->removed) {
      if (!got_first){
	*first = *y;
	got_first = true;
      }
      else {
	*second = *y;
	break;
      }
    }
  }
}

void solve_instance::compress_forward(my_vector<nonzero_entry_t*> *array, int end){
  int swap_level = 0;
  count_ops(4*(end-array->start_index));
  for(int i = end; i >= array->start_index; i--){ //go from the last element deleted to the start_index
    if( (*array)[i]->sampler_pointer->removed){ //when removed element seen
      swap_level = swap_level + 1;
    }else {//shift if the element if not deleted (no shift if swap_level = 0)
      (*array)[i+swap_level] = (*array)[i];
    }
  }
  array->start_index = array->start_index + swap_level;
}

void
solve_instance::freeze_and_sample(my_vector<line_element>& M, my_vector<line_element>& MT, int rows, int cols, dual_sampler_t* p_d, primal_sampler_t* p_p, dual_u_sampler_t* p_dXu, primal_u_sampler_t* p_pXuh, double epsilon, int prob) {
  //store current state of variables, prob distributions, expected values after frozen increments
  my_vector<double> x_p;
  my_vector<double> x_d;
  my_vector<double> x_p_prob;
  my_vector<double> x_d_prob;
  my_vector<double> x_p_expect;
  my_vector<double> x_d_expect;


  //store min prob to determine how many iterations must be done
  double min_prob = 1; 

  //debug-- make sure probs sum to 1
  double total_x_d_prob = 0;
  double total_x_p_prob = 0;

  // find total weight in samplers
  // unlike in main alg, weight is computed exactly 
  //in order to get correct distributions
  weight_t p_p_total = 0;
  weight_t p_d_total = 0;
  weight_t p_pXuh_total = 0;
  weight_t p_dXu_total = 0;

  //get total dual weights
  for (int i = 0; i < cols; i++) {
    weight_t wt = p_d->get_exponent_weight(p_d->get_ith(i)->exponent_entry);
    p_d_total += wt;
    //cout << "Exponent: " << p_d->get_ith(i)->exponent_entry->exponent;
    //cout << " x_" << i << " Item Weight (in dual sampler): " << wt << endl;
    weight_t u_wt = p_dXu->get_exponent_weight(p_dXu->get_ith(i)->exponent_entry);
    p_dXu_total += u_wt;
  }

  //get total primal weights
  for (int i = 0; i < rows; i++) {
    weight_t wt = p_p->get_exponent_weight(p_p->get_ith(i)->exponent_entry);
    p_p_total += wt;
    //cout << "Exponent: " << p_p->get_ith(i)->exponent_entry->exponent;
    //cout << "xhat_" << i << " Item Weight (in primal sampler): " << wt << endl;
    weight_t u_wt = p_pXuh->get_exponent_weight(p_pXuh->get_ith(i)->exponent_entry);
    p_pXuh_total += u_wt;
  }

  //store partial quantities of desired ratio
  double temp_1;
  double temp_2;
  double temp_3;

  //shift in primal is tricky since it moves in a way opposite the dual
  if (p_p->exp_shift_updated || p_pXuh->exp_shift_updated) {
    p_shift_ratio = p_p->shift_exp_weight(p_pXuh->get_exponent_shift() - p_p->get_exponent_shift());
    p_p->exp_shift_updated = false;
    p_pXuh->exp_shift_updated = false;
  }
  if (p_d->exp_shift_updated || p_dXu->exp_shift_updated) {
    d_shift_ratio = p_d->shift_exp_weight(p_d->get_exponent_shift() - p_dXu->get_exponent_shift());
    p_d->exp_shift_updated = false;
    p_dXu->exp_shift_updated = false;
  }

  temp_1 = p_p_total/p_pXuh_total;
  temp_2 = p_dXu_total/p_d_total;
  temp_3 = (double)p_shift_ratio/d_shift_ratio;  //must keep ratios separate since they use different epsilons

  double sampler_prob = 1.0 / (1 + (temp_1*temp_2*temp_3));
  
  //primal vars  
  for (int i = 0; i < cols; i++) {
    sampler_item_t* current_var = p_d->get_ith(i);
    sampler_item_t* current_u_var = p_dXu->get_ith(i);
    x_p.push_back(current_var->x + current_u_var->x);
    //cout << "x_" << i << ": " << x_p[i] << endl; //debug--print new vectors
    weight_t current_weight = p_d->get_exponent_weight(current_var->exponent_entry);
    double current_prob = (double)current_weight/p_d_total;
    weight_t current_u_weight = p_dXu->get_exponent_weight(current_u_var->exponent_entry);
    double current_u_prob = (double)current_u_weight/p_dXu_total;
    double combined_prob = sampler_prob*current_prob + (1-sampler_prob)*current_u_prob; //weighted avg of current_prob and current_u_prob
    x_p_prob.push_back(combined_prob);  
    total_x_p_prob += combined_prob; 
    if (combined_prob < min_prob && combined_prob > 0)
      min_prob = combined_prob;
  }

  //dual vars
  for (int i = 0; i < rows; i++) {
    sampler_item_t* current_var = p_p->get_ith(i);
    sampler_item_t* current_u_var = p_pXuh->get_ith(i);
    x_d.push_back(p_p->get_ith(i)->x + current_u_var->x);
    //cout << "xhat_" << i << ": " << x_d[i] << endl; //debug--print new vectors
    weight_t current_weight = p_p->get_exponent_weight(current_var->exponent_entry);
    double current_prob = (double)current_weight/p_p_total;
    weight_t current_u_weight = p_pXuh->get_exponent_weight(current_u_var->exponent_entry);
    double current_u_prob = (double)current_u_weight/p_pXuh_total;
    double combined_prob = sampler_prob*current_u_prob + (1-sampler_prob)*current_prob; //weighted avg of current_prob and current_u_prob
    x_d_prob.push_back(combined_prob);
    total_x_d_prob += combined_prob; 
    if (combined_prob < min_prob && combined_prob > 0)
      min_prob = combined_prob;
  }

  //debug
  cout << "Min prob: " << min_prob << endl;
  cout << "Sum of x_d probs: " << total_x_d_prob << endl;
  cout << "Sum of x_p probs: " << total_x_p_prob << endl;

  //compute how many iterations must be done--hard-code min prob so #iterations doesn't get huge
    if (min_prob < 0.01)
      min_prob = 0.01;  //adjust min prob so # of iterations doesn't get huge
    weight_t samples = (weight_t)ceil((3*log(prob)/(epsilon*epsilon))/min_prob);
  cout << "3*log(prob)/epsilon^2: " << (3*log(prob)/(epsilon*epsilon)) << " Samples: " << samples << endl;

  //store expected val of each variable after sampling
  for (int i=0; i < cols; i++) {
    x_p_expect.push_back(x_p[i]+(x_p_prob[i]*samples));
    //cout << "x_" << i << ": " << x_p[i] << " Prob: " << x_p_prob[i] << " Expected val: " << x_p_expect[i] << endl;
  }
  for (int i=0; i < rows; i++) {
    x_d_expect.push_back(x_d[i]+x_d_prob[i]*samples);
    //cout << "xhat_" << i << ": " << x_d[i] << " Prob: " << x_d_prob[i] << " Expected val: " << x_d_expect[i] << endl;
  }
  
  //main sampling loop-- note that vars are always incremented by 1, so non-uniform increments are NOT being tested!
  for (int t=0; t < samples; t++) {

    sampler_item_t* wi;
    sampler_item_t* wj;
    random_pair(&wi, &wj, p_p, p_d, p_pXuh, p_dXu);
    
    int i = wi->i;
    int j = wj->i;
   
    // line 6
    nonzero_entry_t* front_active = get_largest_active(&M[i]);
    double uh_i;
    if (front_active == NULL) {//if all columns in the selected row are marked for deletion
      uh_i = 0;
      continue;
    }
    else
      uh_i = front_active->coeff;
    double u_j = MT[j].front()->coeff;
    double delta = 1/(uh_i + u_j);
    wj->x += 1; //delta;
    wi->x += 1; //delta;
  }

  //print final status
  for (int i=0; i < cols; i++) {
    double expt_x = x_p_expect[i];
    double actual_x = p_d->get_ith(i)->x + p_dXu->get_ith(i)->x;
    cout << "x_" << i << ": " << left << setw(10) << actual_x << "Expected val: " << expt_x 
	 << "\tProb: " << x_p_prob[i];
    if (actual_x > (1+epsilon)*expt_x || actual_x < (1-epsilon)*expt_x)
      cout << "\tOut of range!\n";
    else
      cout << "\tWithin range.\n"; 
  }

  for (int i=0; i < rows; i++) {
    double expt_x = x_d_expect[i];
    double actual_x = p_p->get_ith(i)->x + p_pXuh->get_ith(i)->x;
    cout << "xhat_" << i << ": " << left << setw(10) << actual_x << "Expected val: " << expt_x
	 << "\tProb: " << x_p_prob[i];
    if (actual_x > (1+epsilon)*expt_x || actual_x < (1-epsilon)*expt_x)
      cout << "\tOut of range!\n";
    else
      cout << "\tWithin range.\n"; 
  }

  //restore vars to original state-- put entire val of var in main samplers
  for (int i=0; i < cols; i++) {
    p_d->get_ith(i)->x = x_p[i];
    p_dXu->get_ith(i)->x = 0;
  }

  for (int i=0; i < rows; i++) {
    p_p->get_ith(i)->x = x_d[i];
    p_pXuh->get_ith(i)->x = 0;
  }
}

int main(int argc, char *argv[])
{
  double epsilon = 0.1;
  string input_file="";
  int sort_ratio = 1;
  string usage = "Usage: fastpc <epsilon-factor> <filename> [sort-factor]";

  if (argc >= 3) {
    epsilon = atof(argv[1]);   
    input_file = argv[2];
  }
  else {
    cout << usage << endl;
    return 1;
  }
  if (argc >= 4) {
    sort_ratio = atoi(argv[3]);
    if (sort_ratio < 1) {
      cout << "Sort ratio can not be less than 1. Default sort ratio [1] will be used." << endl;
      sort_ratio = 1;
    }
  } 
		
#ifdef NDEBUG
  srand(time(NULL));
#endif

  solve_instance I(epsilon, input_file, sort_ratio);
  I.solve();
  return 0;
}
