#include "fastpc.h"

#include <iomanip>
#include<fstream>
#include <ctime>
#include <cstdlib>

using namespace std;


nonzero_entry_t::nonzero_entry_t(double value, double eps, sampler_item_t* sampler, sampler_item_t* u_sampler):
  coeff(value),  sampler_pointer(sampler), u_sampler_pointer(u_sampler) 
{
  //always round to make approximation an upper bd on value of coefficient
  if (eps > 0) //dual
    exponent = floor(log(value)/log(1-eps));   // =log base 1-eps (value)
  else //primal
    exponent = ceil(log(value)/log(1-eps));    // =log base 1-eps (value)

}

solve_instance::solve_instance(double EPSILON, string infile) :
  eps(0.9*EPSILON),
  epsilon(EPSILON),
  file_name(infile),
  p_shift_ratio(-1),
  d_shift_ratio(-1),
  p_exp_shift(0),
  d_exp_shift(0)
{
  // constructor reads the input and creates matrices M and MT (transpose)
  int row, col, total;
  double val;
  string s;
  {
    //open input file
    ifstream in_file;
    in_file.open(file_name.c_str());

    if (in_file.fail()) {
      cout << "Error opening " << file_name << endl;
      exit(1);
    }

    //read and parse 1st line of input (parameters)
    string s;
    in_file >> r >> c >> total >> s;
    cout << r << " " << c << " " << total << " " << s << endl;

    M.resize(r);
    MT.resize(c);
    M_copy.resize(r);

    N = int(ceil(2*log(r*c)/(eps*eps)));

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

   int non_zero_entry_count = 0;
    while(true) {
    	if (non_zero_entry_count > total) break; //stop scanning input if all nonzeros have been scanned
      in_file >> row  >> col >> val;  //took out string s 
      if (in_file.eof()) break;

      M[row].push_back(new nonzero_entry_t(val,eps*(-1.0), p_d->get_ith(col), p_dXu->get_ith(col)));
      M_copy[row].push_back(new nonzero_entry_t(val, eps*(-1.0), p_d->get_ith(col), p_dXu->get_ith(col)));
      MT[col].push_back(new nonzero_entry_t(val, eps, p_p->get_ith(row), p_pXuh->get_ith(row)));
      non_zero_entry_count++;
    }
    
    //close file
    in_file.close();

    //SORT ROWS OF M
    line_element* first = &M[0];
    line_element* last = &M[r-1];
    for (line_element* p = first; p <= last; ++p) {
      p->sort(list_sort_criteria()); //sort row linked list
      //print sorted matrix
      //for(list<nonzero_entry_t*>::iterator x = p->begin(); x != p->end(); ++x){
      // 	cout << (*x)->coeff << " ";   
      //}
      //cout << "\n";
    }
    
    //sort MT
    line_element* first_t = &MT[0];
    line_element* last_t = &MT[c-1];
    for (line_element* p = first_t; p <= last_t; ++p) {
      p->sort(list_sort_criteria()); //sort row of MT linked list
      //print sorted matrix
      //for(list<nonzero_entry_t*>::iterator x = p->begin(); x != p->end(); ++x){
	//cout << (*x)->coeff << " ";   
      //}
      //cout << "\n";
    }
   	
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
    	p_diff = max_uh_exp - min_uh_exp - (N+10-ceil(1/eps));
    }
   
    //re-initialize u_sampler_items with normalized exponents if necessary
    if (min_u_exp != 0) {
	    for (int i = 0; i < c; i++) {
	      sampler_item_t* item = p_dXu->get_ith(i);
	      int exponent = MT[i].front()->exponent - min_u_exp;
	      if (exponent > max_exp) {
	      	item->exponent_overflow = exponent - max_exp; //initialize overflow if necessary
		exponent = max_exp;
	      }
	      p_dXu->update_item_exponent(item,exponent);
	    }
	    p_dXu->exp_shift -= min_u_exp;  //initial shift of sampler-- can be negative
	    p_dXu->exp_shift_updated = true;
    }

    int p_exp_shift_init = max_uh_exp - p_diff; //total shift amt needed
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

    //print normalized exponents
    //print M
    //cout << "PRINTING M: SUBTRACTED " << max_uh_exp-p_diff << " TO NORMALIZE.\n";
    // for (int i = 0; i < r; ++i) {
//       for(list<nonzero_entry_t*>::iterator x = M[i].begin(); x != M[i].end(); ++x){
// 	cout << "Coeff:" << (*x)->coeff << " Exponent:" << p_pXuh->get_ith(i)->exponent_entry->exponent 
// 	       << " Overflow:" << (*x)->u_sampler_pointer->exponent_overflow << endl;   //debug
//       }
//       cout << "\n";
//     }


    //print MT
    //cout << "PRINTING MT: SUBTRACTED " << min_u_exp << " TO NORMALIZE.\n";
    //first_t = &MT[0];
    //last_t = &MT[c-1];
    // for (int j=0; j < c; ++j) {
//         for(list<nonzero_entry_t*>::iterator x = MT[j].begin(); x != MT[j].end(); ++x){
// 	  cout << "Coeff:" << (*x)->coeff << " Exponent:" << p_dXu->get_ith(j)->exponent_entry->exponent 
// 	       << " Overflow:" << (*x)->u_sampler_pointer->exponent_overflow << endl;   //debug
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

  double assumed_ops_per_usec = 1e2; // ckou's machine
  double assumed_time_per_op = 1/assumed_ops_per_usec;

  cout << "predict at most " << double(N)*(r+c) << " increments" << endl;
  cout << "predict at most " << double(N)*eps/2 << " rebuilds of each sampler" << endl;
  cout << "predict " << 6.0*N*(r+c)*assumed_time_per_op/1000000 << " seconds ";
  cout << "assuming " << assumed_ops_per_usec << "ops per usec" << endl;

  //srand(time(0));

  while (!done){
    iteration++;
    //cout <<"iteration "<<iteration<<endl;

    //call function to freeze samplers and test them
    // if (iteration == N){ //arbitrary freeze point in middle of alg 
//       int prob_reciprocal = 6;   //prob is less than 1/6 of x deviating from E[x] by eps-factor
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
    else
      uh_i = front_active->coeff;
    double u_j = MT[j].front()->coeff;
    double delta = 1/(uh_i + u_j);
    wj->x += delta;
    wi->x += delta;
 
    // line 7
    double z = (rand()%1000)/999.0;
 
    //line 8
    {
      for (list<nonzero_entry_t*>::iterator x = MT[j].begin(); x != MT[j].end(); ++x) {
	double increment = ((*x)->coeff)*delta;

	//if the row is not active any more
	if ((*x)->u_sampler_pointer->removed){
	  continue;
	}	

	if (increment >= z) {
	  p_pXuh->increment_exponent((*x)->u_sampler_pointer);
	  // stop when a packing constraint becomes tight
	  if (p_p->increment_exponent((*x)->sampler_pointer) >= N)
	    done = true;  
	} else {
	  break;
	}
      }
      int size = MT[j].size();
      count_ops(12); 
      n_increments_p += size;
      count_ops(5*size);
    }

    // line 9 
    {
      for (list<nonzero_entry_t*>::iterator x = M[i].begin(); x != M[i].end(); ++x) {
	double increment = ((*x)->coeff)*delta;
	
	//if the column is not active any more
	if ((*x)->sampler_pointer->removed){
	  x = M[i].erase(x); // delete it
	  --x;
	  ++n_deletes;
	  continue;
	}	

	if (increment >= z) {
	  p_dXu->increment_exponent((*x)->u_sampler_pointer);
	  // remove covering constraint when it's met
	  if (p_d->increment_exponent((*x)->sampler_pointer) >= N) {	    
	    //update p_pXuh if uh_i changed for that row
	    //to do this, iterate through dropped column in MT and update uh_i for each row if necessary
	    int colIndex = (*x)->sampler_pointer->i;
	    for (list<nonzero_entry_t*>::iterator y = MT[colIndex].begin(); y != MT[colIndex].end(); ++y) {
	      int rowIndex = (*y)->sampler_pointer->i; //locate which row this entry is in
	      nonzero_entry_t* row_active_first = NULL;
	      nonzero_entry_t* row_active_second = NULL;
	      get_two_largest_active(&M[rowIndex], &row_active_first, &row_active_second);
 
	      //update exponents for new uh_i if y is first active element in row, 
	      //i.e. current uh_i-value came from y and y is not the only remaining item
	      if(row_active_first == *y && row_active_second != NULL) {    
		int exp_diff = row_active_first->exponent - row_active_second->exponent; //dif b/t exponents of first and second active elements of row
		if (exp_diff != 0) { //if y and next element don't have same exponent
		  p_pXuh->update_item_exponent(row_active_first->u_sampler_pointer, row_active_first->u_sampler_pointer->exponent_entry->exponent - exp_diff);
		}
	      }
	    }

	    p_d->remove((*x)->sampler_pointer);
	    p_dXu->remove((*x)->u_sampler_pointer);
	    	    
	    --J_size;
	  }
	} else {
	  break; 
	}
      }
    }
    if (J_size == 0) 
      done = true; 
    // stop when there is no active covering constraint (all are met)
  }

  // end of iterations. compute final x_p and x_d values as in line 10 of the algorithm
  // cout<<"end of iterations"<<endl;

  count_ops(3*r);

  double max_row = 0;
  for (int i=0; i<r; ++i){
    double tmp = 0;
    count_ops(3*M_copy[i].size());
    for (line_element::iterator iter = M_copy[i].begin(); 
	 iter != M_copy[i].end(); 
	 ++iter) {
      tmp += (*iter)->coeff * ((*iter)->sampler_pointer->x + (*iter)->u_sampler_pointer->x); //each sampler item stores part of var's value 
    }
    if (tmp > max_row)
      max_row = tmp;
  }

  count_ops(3*c);

  cout << "DUAL:\n";
  double min_col = long(4*(N+2));
  for (int j=0; j<c; ++j){
    double tmp = 0;
    count_ops(3*MT[j].size());
    for (line_element::iterator iter = MT[j].begin();
         iter != MT[j].end(); 
	 ++iter) {
      tmp += (*iter)->coeff * ((*iter)->sampler_pointer->x + (*iter)->u_sampler_pointer->x); //each sampler item stores part of var's value 
    }
    //cout <<" x_hat_" << j << ": " << tmp << endl; //debug-- print each MT_jxh  before normalization
    if (tmp < min_col)
      min_col = tmp;
  }

  double sum_x_p=0, sum_x_d=0;

  count_ops(2*c);

  //compute this in previous for loops to save time
  for (int j=0; j<c; ++j) sum_x_p += (p_d->get_ith(j)->x + p_dXu->get_ith(j)->x);

  count_ops(2*r);

  for (int i=0; i<r; ++i) sum_x_d += (p_p->get_ith(i)->x + p_pXuh->get_ith(i)->x);

  //cout << "iterations = " << iteration;
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
  cout << " time = " << elapsed_time/1000000.0 << "s" << endl;
  cout << " ops_per_usec = " << ops_per_usec << endl;
  cout << " alloc_time = " << alloc_time/1000000.0 << "s" << endl;
  cout << " alloc_space = " << alloc_space << endl;
  cout << " allocs_per_usec = " << alloc_space/alloc_time << endl;
}

void solve_instance::random_pair(sampler_item_t** wi,sampler_item_t** wj, dual_sampler_t* p_p, dual_sampler_t* p_d, dual_sampler_t* p_pXuh, dual_sampler_t* p_dXu) {
  double p_p_wt = p_p->get_update_total_weight();
  double p_d_wt = p_d->get_update_total_weight();
  double p_pXuh_wt = p_pXuh->get_update_total_weight();
  double p_dXu_wt = p_dXu->get_update_total_weight();

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

  temp_1 = p_p_wt/p_pXuh_wt;
  temp_2 = p_dXu_wt/p_d_wt;
  temp_3 = (double)p_shift_ratio/d_shift_ratio;  //must keep ratios separate since they use different epsilons

  double prob = 1.0 / (1 + (temp_1*temp_2*temp_3));  //overall prob used to choose samplers
  
  while (1) {  //go until neither chosen sampler fails
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
  for (list<nonzero_entry_t*>::iterator	 y = (*row).begin(); y != (*row).end(); ++y) {
    if (!(*y)->sampler_pointer->removed) {
      return *y;
    }
  }
  return NULL;  //gets here when all row items have been marked for deletion
}

void solve_instance::get_two_largest_active(line_element* row, nonzero_entry_t** first, nonzero_entry_t** second) {
  bool got_first = false;
  for (list<nonzero_entry_t*>::iterator	 y = (*row).begin(); y != (*row).end(); ++y) {
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


//@TODO fix this function to incorporate portions of vars stored in p_pXuh and p_pXu 
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

  //find total weight in samplers
  weight_t p_p_total = 0;
  weight_t p_d_total = 0;
  weight_t p_pXuh_total = 0;
  weight_t p_dXu_total = 0;

  //get total dual weights
  for (int i = 0; i < cols; i++) {
    weight_t wt = p_d->get_exponent_weight(p_d->get_ith(i)->exponent_entry);
    p_d_total += wt;
    //cout << "Exponent: " << p_d->get_ith(i)->exponent_entry->exponent;
    cout << " x_" << i << " Item Weight (in dual sampler): " << wt << endl;
    weight_t u_wt = p_dXu->get_exponent_weight(p_dXu->get_ith(i)->exponent_entry);
    p_dXu_total += u_wt;
  }

  //get total primal weights
  for (int i = 0; i < rows; i++) {
    weight_t wt = p_p->get_exponent_weight(p_p->get_ith(i)->exponent_entry);
    p_p_total += wt;
    //cout << "Exponent: " << p_p->get_ith(i)->exponent_entry->exponent;
    cout << "xhat_" << i << " Item Weight (in primal sampler): " << wt << endl;
    weight_t u_wt = p_pXuh->get_exponent_weight(p_pXuh->get_ith(i)->exponent_entry);
    p_pXuh_total += u_wt;
  }

  //get prob of using each sampler, just as in main alg  
  double temp_1;
  double temp_2;
  double temp_3;

  int temp_p_exp_shift = 0;
  int temp_d_exp_shift = 0;

  if (p_p->exp_shift_updated || p_pXuh->exp_shift_updated) 
    temp_p_exp_shift = p_pXuh->get_exponent_shift() - p_p->get_exponent_shift();
  if (p_d->exp_shift_updated || p_dXu->exp_shift_updated) 
    temp_d_exp_shift = p_d->get_exponent_shift() - p_dXu->get_exponent_shift();

  temp_1 = p_p_total/p_pXuh_total;
  temp_2 = p_dXu_total/p_d_total;
  temp_3 = pow(1.0-eps, (temp_p_exp_shift - temp_d_exp_shift));  //combine primal and dual ratio since they're both powers of 1-eps

  double sampler_prob = 1.0 / (1 + (temp_1*temp_2*temp_3));
  
  //primal vars  
  for (int i = 0; i < cols; i++) {
    sampler_item_t* current_var = p_d->get_ith(i);
    sampler_item_t* current_u_var = p_dXu->get_ith(i);
    x_p.push_back(current_var->x + current_u_var->x);
    cout << "x_" << i << ": " << x_p[i] << endl; //debug--print new vectors
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
    cout << "xhat_" << i << ": " << x_d[i] << endl; //debug--print new vectors
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
  weight_t samples = ceil((3*log(prob)/(epsilon*epsilon))/min_prob);
  cout << "3*log(prob)/epsilon^2: " << (3*log(prob)/(epsilon*epsilon)) << " Samples: " << samples << endl;

  //store expected val of each variable after sampling
  for (int i=0; i < cols; i++) {
    x_p_expect.push_back(x_p[i]+(x_p_prob[i]*samples));
    cout << "x_ " << i << ": " << x_p[i] << " Prob: " << x_p_prob[i] << " Expected val: " << x_p_expect[i] << endl;
  }
  for (int i=0; i < rows; i++) {
    x_d_expect.push_back(x_d[i]+x_d_prob[i]*samples);
    cout << "xhat_ " << i << ": " << x_d[i] << " Prob: " << x_d_prob[i] << " Expected val: " << x_d_expect[i] << endl;
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
    cout << "x_" << i << ": " << actual_x << " Expected val: " << expt_x;
    if (actual_x > (1+epsilon)*expt_x || actual_x < (1-epsilon)*expt_x)
      cout << "\tOut of range!\n";
    else
      cout << "\tWithin range.\n"; 
  }

  for (int i=0; i < rows; i++) {
    double expt_x = x_d_expect[i];
    double actual_x = p_p->get_ith(i)->x + p_pXuh->get_ith(i)->x;
    cout << "xhat_" << i << ": " << actual_x << " Expected val: " << expt_x;
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
  string usage = "Usage: fastpc <epsilon-factor> <filename> ";
		
  if (argc >= 2)
    epsilon = atof(argv[1]);

  if (argc >= 3)
    input_file = argv[2];
  else {
    cout << usage << endl;
    return 1;
  }
		
#ifdef NDEBUG
  srand(time(NULL));
#endif

  solve_instance I(epsilon, input_file);
  I.solve();
  return 0;
}