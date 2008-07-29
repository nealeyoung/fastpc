#include "fastpc.h"

#include <iomanip>
#include<fstream>
#include <ctime>
#include <cstdlib>

using namespace std;


nonzero_entry_t::nonzero_entry_t(double value, double eps, sampler_item_t* sampler, sampler_item_t* u_sampler):
  coeff(value),  sampler_pointer( sampler), u_sampler_pointer(u_sampler) 
{
  //Need to be careful about taking floor or ceiling
  exponent = ceil(log(value)/log(1-eps));//   =log base 1-eps (value)
}

solve_instance::solve_instance(double EPSILON, string infile) :
  eps(0.9*EPSILON),
  epsilon(EPSILON),
  file_name(infile)
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
    /*string s;
    cin >> r >> c >> total >> s;
    cout<< r << ", " << c << endl;
    */
    M.resize(r);
    MT.resize(c);
    M_copy.resize(r);

    N = int(ceil(2*log(r*c)/(eps*eps)));

    cout << "N = " << N << endl;
 
    p_p = new primal_sampler_t(r, eps, 0, N+10);
    p_d = new dual_sampler_t(c, eps, 0, N+10);
    p_pXuh = new primal_sampler_t(r, eps, 0, N+10);
    p_dXu = new dual_sampler_t(c, eps, 0, N+10);

    assert(p_p);
    assert(p_d);
    assert(p_pXuh);
    assert(p_dXu);

    p_p->init();
    p_d->init();
    p_pXuh->init();
    p_dXu->init();

    //cout << fixed << setprecision(1);

    while(true) {
      in_file >> row  >> col >> val;  //took out string s 
      // find b
      if (in_file.eof()) break;
      M[row].push_back(new nonzero_entry_t(val,eps/-1.0, p_d->get_ith(col), p_dXu->get_ith(col)));//remove u_sampler_pointer from the constructor
      M_copy[row].push_back(new nonzero_entry_t(val, eps/-1.0, p_d->get_ith(col), p_dXu->get_ith(col)));
      MT[col].push_back(new nonzero_entry_t(val, eps, p_p->get_ith(row), p_pXuh->get_ith(row)));
    }

    //SORT ROWS OF M
    line_element* first = &M[0];
    line_element* last = &M[r-1];
    for (line_element* p = first; p <= last; ++p) {
      cout << "Inside loop "; //debug
      p->sort(list_sort_criteria()); //sort row linked list
      for(list<nonzero_entry_t*>::iterator x = p->begin(); x != p->end(); ++x){
	cout << (*x)->exponent << " ";   
      }
      cout << "\n";
    }
    
    //sort MT
    line_element* first_t = &MT[0];
    line_element* last_t = &MT[c-1];
    for (line_element* p = first_t; p <= last_t; ++p) {
      cout << "Inside loop MT "; //debug
      p->sort(list_sort_criteria()); //sort row of MT linked list
      for(list<nonzero_entry_t*>::iterator x = p->begin(); x != p->end(); ++x){
	cout << (*x)->exponent << " ";
      }
      cout << "\n";
    }

    //find the minimum exponent for MT and maximum exponent for M
    //this is to normalize the exponents
    //these will be used to normalize the exponents in the matrices
    //and that is required to ensure that all exponents in u are non-negative
    //and all exponents in u_hat are non-positive
    int min_u_exp = 0;
    int max_u_exp = MT[0].front()->exponent;
    int max_uh_exp = 0;
    int min_uh_exp = M[0].front()->exponent;
    for (int j = 0; j < c; j++) {
      int temp = MT[j].front()->exponent;
      if (temp < min_u_exp)
				min_u_exp = temp;
      if (temp > max_u_exp)
				max_u_exp;
    }
    for (int i = 0; i < r; i++) {
      int temp = M[i].front()->exponent;
      if (temp > max_uh_exp)
				max_uh_exp = temp;
      if (temp < min_uh_exp)
				min_uh_exp = temp;
    }

    cout << "min_u_exp " << min_u_exp << endl;
    cout << "max_uh_exp " << max_uh_exp << endl;
    
    int d_diff = 0;
    if (max_u_exp - min_u_exp > N+10-ceil(1/eps)) {
    	d_diff = max_u_exp - min_u_exp - (N+10-ceil(1/eps));
    }

    int p_diff = 0;
    if (max_uh_exp - min_uh_exp > N+10-ceil(1/eps)) {
    	p_diff = max_uh_exp - min_uh_exp - (N+10-ceil(1/eps));
    }

    //initialize the new samplers based on the max/min exponents--add new
    //buckets to accommodate more exponents
//    p_pXuh = new primal_sampler_t(r, eps, 0, N+10+max_uh_exp - min_uh_exp);
//    p_dXu = new dual_sampler_t(c, eps, 0, N+10+max_u_exp - min_u_exp);


    //START HERE 
    //WE NOW NEED TO PUT POINTERS TO NEW SAMPLER ITEMS IN THE NONZERO_ENTRIES FOR M AND MT

    //set u_sampler pointers in M to elements of p_pXuh with indices corresponding
    // to elements of p_p indicated by sampler_pointers
//    for (int j = 0; j < r; ++j) {
//      for (list<nonzero_entry_t*>::iterator x = M[j].begin(); x != M[j].end(); 
//	   ++x) {
//	(*x)->u_sampler_pointer = p_pXuh->get_ith((*x)->sampler_pointer->i);
//      }
//    }

    //same for MT and pointers to p_dXu
//    for (int i = 0; i < c; ++i) {
//      for (list<nonzero_entry_t*>::iterator x = MT[i].begin(); x != MT[i].end();
//	   ++x) {
//	(*x)->u_sampler_pointer = p_dXu->get_ith((*x)->sampler_pointer->i);
//      }
//    }

    //re-initialize u_sampler_items with normalized exponents
    for (int i = 0; i < c; i++) {
      sampler_item_t* item = p_dXu->get_ith(i);
      int exponent = MT[i].front()->exponent - min_u_exp - d_diff;
      if (exponent < 0)
      	exponent = 0;
      //cout << "exponent: " << exponent << endl;
      p_dXu->update_item_exponent(item,exponent);
    }
    for (int j = 0; j < r; j++) {
      sampler_item_t* item = p_pXuh->get_ith(j);
      int exponent = M[j].front()->exponent - max_uh_exp + p_diff;
      if (exponent > 0)
      	exponent = 0;
      //cout << "exponent: " << exponent << endl;
      p_pXuh->update_item_exponent(item,exponent);
    }

    // //for p_pXuh
//     for (int i = 0; i < r; i++) {
//       p_pXuh->update_item_exponent(p_pXuh->get_ith(i), M[i].front()->exponent);
//     }
//     //for p_dXu
//     for (int i = 0; i < c; i++) {
//       p_dXu->update_item_exponent(p_dXu->get_ith(i), MT[i].front()->exponent);
//     }
  }
}

void
solve_instance::solve() {
  int J_size = c;	  // we have c active columns at the beginning

  bool done = false;
//  unsigned iteration = 0;

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

  srand(time(0));

  while (!done){
//    iteration++;
//    cout <<"iteration "<<iteration<<endl;

    sampler_item_t* wi;
    sampler_item_t* wj;
    random_pair(&wi, &wj, p_p, p_d, p_pXuh, p_dXu);
    //sampler_item_t* wi = p_p->sample();
    int i = wi->i;
    //cout<<"sampled1 "<< i <<endl << flush;

    //sampler_item_t* wj = p_d->sample();
    int j = wj->i;
    //cout<<"sampled2 "<< j <<endl << flush;

    ++n_samples;

    // line 6
    
    double uh_i = M[i].front()->coeff;
    double u_j = MT[j].front()->coeff;
    double delta = 1/(uh_i + u_j);
    wj->x += delta;
    wi->x += delta;

    // line 7
    //    double random_num = rand()%1000;
    double z = (rand()%1000)/999.0;//@steve how much precision do we need here?
    //cout << "z: " << z << endl; //debug

    //line 8
    //cout << j << ": ";
    {
      for (list<nonzero_entry_t*>::iterator x = MT[j].begin(); x != MT[j].end(); ++x) {
	double increment = ((*x)->coeff)*delta;
	if (increment >= z) {
	  // stop when a packing constraint becomes tight
	  p_pXuh->increment_exponent((*x)->u_sampler_pointer, true);
	  if (p_p->increment_exponent((*x)->sampler_pointer, false) >= N)
	    done = true;  
	} else {
	  break;
	}
      }
      int size = MT[j].size();
      count_ops(12);  //not actual value-- how many??
      // register line_element& column = MT[j];
//       nonzero_entry_t** first = &column[0];

//       int size = column.size();
//       register nonzero_entry_t** last = &column[size-1];

//       count_ops(12);

//       for (register nonzero_entry_t** w = first;
// 	   w <= last; 
// 	   ++w) {
   
// 	// stop when a packing constraint becomes tight
// 	if (p_p->increment_exponent((*w)->sampler_pointer) >= N)
// 	  done = true;

	
//       }
      n_increments_p += size;
      count_ops(5*size);
    }

    // line 9 
    // cout << i << ": ";
    {

      for (list<nonzero_entry_t*>::iterator x = M[i].begin(); x != M[i].end(); ++x) {
	double increment = ((*x)->coeff)*delta;
	
	//if the column is not active any more
	if ((*x)->sampler_pointer->removed){
	  x = M[i].erase(x); // delete it //@steve should we still use x??
	  --x;//test
	  //read comments in the next block below
	  uh_i = 2*M[i].front()->coeff;
	  delta = 1/(u_j + uh_i); //this probably is not needed
	  ++n_deletes;
	  continue;
	}	

	if (increment >= z) {
	  // remove covering constraint when it's met
	  //cout << (*x)->u_sampler_pointer->exponent_entry->exponent << " --- EXPONENT \n";
	  p_dXu->increment_exponent((*x)->u_sampler_pointer, true);
	  if (p_d->increment_exponent((*x)->sampler_pointer, false) >= N) {
	    //@steve - why don't we delete x from M[i] here?
	    //we can recalculate delta every time x is deleted
	    //and that could be a minor performance enhancement
	    //even though we evaluate u_j and delta again, which is O(c) overall
	    p_d->remove((*x)->sampler_pointer);
	    
	    //remove the corresponding sampler pointer from p_dXu
	    p_dXu->remove((*x)->u_sampler_pointer);
	    
	    //update p_pXuh if u_hat changed for that row
	    //iterate through this column in MT
	    //and update if u for a row has changed
	    int colIndex = (*x)->sampler_pointer->i;
	    for (list<nonzero_entry_t*>::iterator y = MT[colIndex].begin(); y != MT[colIndex].end(); ++y) {
	      int rowIndex = (*y)->sampler_pointer->i;
	      nonzero_entry_t* row_front = M[rowIndex].front();
	      if(row_front == *y) {    //if y is first element in row, i.e. current u-value came from y
		int exp_diff = row_front->exponent - (*(++M[rowIndex].begin()))->exponent; //dif b/t exponents of first and second elements of row
		if (exp_diff != 0) {
			//@monik the new exponent may be close to/below the permanent_min_exponent because of normalization
		  p_pXuh->update_item_exponent(row_front->u_sampler_pointer, row_front->u_sampler_pointer->exponent_entry->exponent - exp_diff);
		}
	      }
	    }
	    --J_size;
	  }
	} else {
	  break;
	}


// 	//if the column is not active any more
// 	if ((*x)->sampler_pointer->removed){ 
// 	  x = M[i].erase(x); // delete it //@steve should we still use x??
// 	  --x;//test
// 	  ++n_deletes;
// 	  continue;
// 	}
	// if (p_d->increment_exponent((*x)->sampler_pointer) >= N) {
// 	  // if covering constraint is met, make it inactive
// 	  p_d->remove((*x)->sampler_pointer); //@steve remove the nonzero_entry_t too??
// 	  --J_size;
// 	}
      }  // -----------------------DON"T DELETE ME

      // register line_element& row = M[i];
//       nonzero_entry_t ** first = &row[0];
//       int size = row.size();
//       register nonzero_entry_t** last = &row[size-1];

//       n_increments_d += size;
//       count_ops(6*(size+1));

//       for (register nonzero_entry_t** w = first;
// 	   w <= last;
// 	   ++w) {
// 	//if the column is not active any more
// 	if ((*w)->sampler_pointer->removed){ 
// 	  row.remove(w - first); // delete it //@steve should we still use w??
// 	  --w;
// 	  --last;
// 	  ++n_deletes;
// 	  continue;
// 	}
// 	if (p_d->increment_exponent((*w)->sampler_pointer) >= N) {
// 	  // if covering constraint is met, make it inactive
// 	  p_d->remove((*w)->sampler_pointer); //@steve remove the nonzero_entry_t too??
// 	  --J_size;
// 	}
//       }


    }  //----------------DON"T DELETE ME


    // cout << endl;

    //	cout << "J size "<<J_size<<endl;
    //	cout << N_d<<" "<<endl;

    if (J_size == 0) 
      done = true; 
    // stop when there is no active covering constraint (all are met)
  }

  // end of iterations. compute final x_p and x_d values as in line 10 of the algorithm
  // cout<<"end of iterations"<<endl;

  count_ops(3*r);

  long max_row = 0;
  for (int i=0; i<r; ++i){
    long tmp = 0;
    count_ops(3*M_copy[i].size());
    for (line_element::iterator iter = M_copy[i].begin(); 
	 iter != M_copy[i].end(); 
	 ++iter)
      tmp += (*iter)->sampler_pointer->x;
    if (tmp > max_row)
      max_row = tmp;
  }

  count_ops(3*c);

  long min_col = long(4*(N+2));
  for (int j=0; j<c; ++j){
    long tmp = 0;
    count_ops(3*MT[j].size());
    for (line_element::iterator iter = MT[j].begin();
         iter != MT[j].end(); 
	 ++iter)
      tmp += (*iter)->sampler_pointer->x;
    if (tmp < min_col)
      min_col = tmp;
  }

  double sum_x_p=0, sum_x_d=0;

  count_ops(2*c);

  for (int j=0; j<c; ++j) sum_x_p += p_d->get_ith(j)->x;

  count_ops(2*r);

  for (int i=0; i<r; ++i) sum_x_d += p_p->get_ith(i)->x;

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

  float z = (rand()%1000)/999.0;
  double prob = (p_pXuh_wt*p_d_wt)/(p_pXuh_wt*p_d_wt + p_p_wt*p_dXu_wt);

  if (prob > z) {
    *wi = p_pXuh->sample();
    *wj = p_d->sample();
  }
  else {
    *wi = p_p->sample();
    *wj = p_dXu->sample();
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
