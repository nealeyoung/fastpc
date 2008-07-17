#include "fastpc.h"
#include <iomanip>
#include<fstream>

using namespace std;

//typedef my_vector<sampler_item_t *> line_element;

//typedef my_vector<nonzero_entry_t *> line_element;


nonzero_entry_t::nonzero_entry_t(double value, sampler_item_t* sampler):
  coeff(value),  sampler_pointer( sampler)
{}


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
    assert(p_p);
    assert(p_d);

    p_p->init();
    p_d->init();

   cout << fixed << setprecision(1);

    while(true) {
      in_file >> row  >> col >> val;  //took out string s 
      if (in_file.eof()) break;
      //if(cin.eof()) break;
      //cout<< row << ", " << col << endl;
      M[row].push_back(new nonzero_entry_t(val,p_d->get_ith(col)));
      M_copy[row].push_back(new nonzero_entry_t(val,p_d->get_ith(col)));
      MT[col].push_back(new nonzero_entry_t(val,p_p->get_ith(row)));

    }
    //sort rows and cols of M (eventually we need to pseudo-sort)
    line_element* last = &M[r-1];
    for (line_element* p = &M[0]; p <= last; ++p) {
      cout << "Inside loop\n"; //debug
      //sort((*p)[0], (*p)[(p->size())-1], nonzero_entry_t_comparator());
      sort(p->begin(), p->end(), nonzero_entry_t_comparator());

      // cout << "loop " << x << endl;
      
      for (int x = 0; x < (p->size()); ++x) 
	cout << (*p)[x]->coeff << " ";   
      
    }
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
    wj->x++;
    wi->x++;

    // line 7
    //cout << j << ": ";
    {
      
      register line_element& column = MT[j];
      nonzero_entry_t** first = &column[0];

      int size = column.size();
      register nonzero_entry_t** last = &column[size-1];

      count_ops(12);

      for (register nonzero_entry_t** w = first;
	   w <= last; 
	   ++w) {
   
	// stop when a packing constraint becomes tight
	if (p_p->increment_exponent((*w)->sampler_pointer) >= N)
	  done = true;

	
      }
      n_increments_p += size;
      count_ops(5*size);
    }

 

    // line 8 
    // cout << i << ": ";
    {
      register line_element& row = M[i];
      nonzero_entry_t ** first = &row[0];
      int size = row.size();
      register nonzero_entry_t** last = &row[size-1];

      n_increments_d += size;
      count_ops(6*(size+1));

      for (register nonzero_entry_t** w = first;
	   w <= last;
	   ++w) {
	//if the column is not active any more
	if ((*w)->sampler_pointer->removed){ 
	  row.remove(w - first); // delete it //@steve should we still use w??
	  --w;
	  --last;
	  ++n_deletes;
	  continue;
	}
	if (p_d->increment_exponent((*w)->sampler_pointer) >= N) {
	  // if covering constraint is met, make it inactive
	  p_d->remove((*w)->sampler_pointer); //@steve remove the nonzero_entry_t too??
	  --J_size;
	}
      }
    }
    // cout << endl;

    //	cout << "J size "<<J_size<<endl;
    //	cout << N_d<<" "<<endl;

    if (J_size == 0) done = true; 
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

