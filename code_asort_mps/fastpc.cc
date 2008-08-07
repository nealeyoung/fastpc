#include "fastpc.h"
#include <iomanip>
#include<fstream>
#include <ctime>
#include <cstdlib>

using namespace std;


nonzero_entry_t::nonzero_entry_t(double value, sampler_item_t* sampler):
  coeff(value),  sampler_pointer( sampler)
{}
// bool nonzero_entry_t::operator<(nonzero_entry_t* a) {
//   cout << "testttttttttt ----- tttttt";
//     return (this->coeff < a->coeff);
//   }


//should get both bs at once
//need to fix r
void solve_instance::sudo_sort(my_vector<line_element> *matrix,int col ){

  //find b
  cout<<"start sudo sort \n";
  line_element *last = (line_element*)&((*matrix)[r-1]);
  line_element *first = (line_element*)&((*matrix)[0]);
  double b = 0;
  for (line_element *p =first; p <= last; ++p) {

    double min_temp = (*(p->begin()))->coeff;
     for(list<nonzero_entry_t*>::iterator x = p->begin(); x != p->end(); ++x){
      
       if ((*x)->coeff < min_temp){
	 min_temp = (*x)->coeff;   
       }
     }
     
     if (min_temp >b){
       b = min_temp;
     }
  }

  //scale all  elements
  double bound = b*eps/col;
  double replace = b*col/eps;
  for (line_element* p = first; p <= last; ++p) {

    for(list<nonzero_entry_t*>::iterator x = p->begin(); x != p->end(); ++x){
      
      if ((*x)->coeff < bound ){
	(*x)->coeff = 0;
      }else{
	(*x)->coeff = min((*x)->coeff,replace);
      }
    }
  }
  
  //create buckets for the  bucket sort
  int num_buckets = (int)ceil(log2(col/eps))*2; //not sure about base 2
  double_list** buckets = (double_list**)malloc(sizeof(double_list*)*num_buckets);    
  for(int i = 0; i<num_buckets; i++){
    buckets[i] =  new double_list();
  }

  // bucket sort each row
  for (line_element* p = first; p <= last; ++p) {

    cout <<"new row \n";

    //bucket sort
    for(list<nonzero_entry_t*>::iterator x = p->begin(); x != p->end(); ++x){
    
      //not sure about base 2 and the round might be floor
      int index = (int)floor(log2((*x)->coeff)-log2(b)+log2(c/eps));
      buckets[index]->push_back((*x)->coeff);
    }

    //print out the buckets need to add them to new list
    for(int i = 0; i<num_buckets; i++){
      for (list<double>::iterator x = buckets[i]->begin(); x != buckets[i]->end(); ++x){
	cout << (*x)<< " item ";   
      }
     cout <<"bucket \n";
    }

    //erase elements in buckets so they can be used for next row
    for(int i = 0; i<num_buckets; i++){
      buckets[i]->clear();
    }

  }

  
  cout <<"done with sudo sort \n";
  cout <<flush;
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
 
    p_p = new primal_sampler_t(r, eps, 0, N+10);
    p_d = new dual_sampler_t(c, eps, 0, N+10);
    
    //no clue about the next two lines... need to correct this
    p_pXuh = new primal_sampler_t(r, eps, 0, N+10);
    p_phXu = new dual_sampler_t(c, eps, 0, N+10);
    
    assert(p_p);
    assert(p_d);
    //no clue about the next two lines... need to correct this
    assert(p_pXuh);
    assert(p_phXu);

    p_p->init();
    p_d->init();

    //cout << fixed << setprecision(1);

    while(true) {
      in_file >> row  >> col >> val;  //took out string s 

      // find b
      if (in_file.eof()) break;
      M[row].push_back(new nonzero_entry_t(val,p_d->get_ith(col)));
      M_copy[row].push_back(new nonzero_entry_t(val,p_d->get_ith(col)));
      MT[col].push_back(new nonzero_entry_t(val,p_p->get_ith(row)));
    }

    //sudo sorts
    //sudo_sort(&M,c);
    //sudo_sort(MT,r);

    //sort rows of M
    line_element* last = &M[r-1];
    for (line_element* p = &M[0]; p <= last; ++p) {
      cout << "Inside loop "; //debug
      p->sort(list_sort_criteria()); //sort row linked list
      for(list<nonzero_entry_t*>::iterator x = p->begin(); x != p->end(); ++x){
	cout << (*x)->coeff << " ";   
      }
      cout << "\n";
    }
    
    //sort MT
    line_element* last_t = &MT[c-1];
    for (line_element* p = &MT[0]; p <= last_t; ++p) {
      cout << "Inside loop MT "; //debug
      p->sort(list_sort_criteria()); //sort row of MT linked list
      for(list<nonzero_entry_t*>::iterator x = p->begin(); x != p->end(); ++x){
	cout << (*x)->coeff << " ";   
      }
      cout << "\n";
    }

    //possibly need to sort M_copy

    //no clue about the next two lines... need to correct this
    p_pXuh->init();
    p_phXu->init();

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

  srand(time(0));

  while (!done){
    iteration++;
    //cout <<"iteration "<<iteration<<endl;

    sampler_item_t* wi = p_p->sample();
    int i = wi->i;
    //cout<<"sampled1 "<< i <<endl;

    sampler_item_t* wj = p_d->sample();
    int j = wj->i;
    //cout<<"sampled2 "<< j <<endl;

    ++n_samples;

    // line 6
    
    double uh_i = 2*M[i].front()->coeff;
    double u_j = 2*MT[j].front()->coeff;
    double delta = 1/(uh_i + u_j);
    wj->x += delta;
    wi->x += delta;

    // line 7
    //    double random_num = rand()%1000;
    double z = (rand()%1000)/999;//@steve how much precision do we need here?

    //line 8
    //cout << j << ": ";
    {
      for (list<nonzero_entry_t*>::iterator x = MT[j].begin(); x != MT[j].end(); ++x) {
	double increment = ((*x)->coeff)*delta;
	if (increment >= z) {
	  // stop when a packing constraint becomes tight
	  if (p_p->increment_exponent((*x)->sampler_pointer) >= N)
	    done = true;  
	} else if(increment < z/2) {
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
	  delta = 1/(u_j + uh_i);
	  ++n_deletes;
	  continue;
	}	

	if (increment >= z) {
	  // remove covering constraint when it's met
	  if (p_d->increment_exponent((*x)->sampler_pointer) >= N) {
	    //@steve - why don't we delete x from M[i] here?
	    //we can recalculate delta every time x is deleted
	    //and that could be a minor performance enhancement
	    //even though we evaluate u_j and delta again, which is O(c) overall
	    p_d->remove((*x)->sampler_pointer);
	    --J_size;
	  }
	} else if(increment < z/2) {
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

int main(int argc, char *argv[])
{
  double epsilon = 0.1;
  string input_file="";
		
  if (argc >= 2)
    epsilon = atof(argv[1]);

  if (argc >= 3)
    input_file = argv[2];
  else {
    cout << "Error!  Too few arguments.\n";
    return 1;
  }
		
#ifdef NDEBUG
  srand(time(NULL));
#endif

  solve_instance I(epsilon, input_file);
  I.solve();

  return 0;
}
