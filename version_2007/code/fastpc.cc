#include <iostream>
#include <cstdlib>
#include <ctime>
#include <map>
#include <cmath>
#include <fstream>
#include <string>
#include <assert.h>
#include <stdio.h>

#include "my_vector.h"
#include "sampler.h"

#include "main_include.h"

using namespace std;

typedef my_vector<sampler_item_t *> line_element;

class solve_instance{
private:
  int r, c, N;

  primal_sampler_t *p_p;
  dual_sampler_t *p_d;

  my_vector<line_element> M, MT, M_copy;

  double eps, epsilon;

public:	
  solve_instance(double epsilon);
  void solve();
};

solve_instance::solve_instance(double EPSILON) :
  eps(0.9*EPSILON),
  epsilon(EPSILON)
{
  // constructor reads the input and creates matrices M and MT (transpose)
  int row, col, total;
  int val;
  string s;

  {
    string s;
    cin >> r >> c >> total >> s;
    // cout<< r << ", " << c << endl;
    M.resize(r);
    MT.resize(c);
    M_copy.resize(r);

    cout << "Read first line of input\n" ; //debug

    N = int(ceil(2*log(r*c)/(eps*eps)));

    p_p = new primal_sampler_t(r, eps, 0, N+10);
    p_d = new dual_sampler_t(c, eps, 0, N+10);
    assert(p_p);
    assert(p_d);

    p_p->init();
    p_d->init();

    while(true) {
      cin >> row  >> col >> val >> s;
      if(cin.eof()) break;
      //cout<< row << ", " << col << endl;
      M[row].push_back(p_d->get_ith(col));
      M_copy[row].push_back(p_d->get_ith(col));
      MT[col].push_back(p_p->get_ith(row));
    }
    cout << "Finished with input\n"; //debug
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
    // cout <<"iteration "<<iteration<<endl;

    sampler_item_t* wi = p_p->sample();
    int i = wi->i;
    // cout<<"sampled1 "<< i <<endl;

    sampler_item_t* wj = p_d->sample();
    int j = wj->i;
    // cout<<"sampled2 "<< j <<endl;

    ++n_samples;

    // line 6
    wj->x++;
    wi->x++;

    // line 7
    // cout << j << ": ";
    {
      register line_element& column = MT[j];
      sampler_item_t** first = &column[0];
      int size = column.size();
      register sampler_item_t** last = &column[size-1];

      count_ops(12);

      for (register sampler_item_t** w = first;
	   w <= last; 
	   ++w) {
	// stop when a packing constraint becomes tight
	if (p_p->increment_exponent(*w) >= N)
	  done = true;
      }
      n_increments_p += size;
      count_ops(5*size);
    }

    // cout << endl;

    // line 8 
    // cout << i << ": ";
    {
      register line_element& row = M[i];
      sampler_item_t ** first = &row[0];
      int size = row.size();
      register sampler_item_t** last = &row[size-1];

      n_increments_d += size;
      count_ops(6*(size+1));

      for (register sampler_item_t** w = first;
	   w <= last;
	   ++w) {
	if ((*w)->removed){		//if the column is not active any more
	  row.remove(w-first); // delete it
	  --w;
	  --last;
	  ++n_deletes;
	  continue;
	}
	if (p_d->increment_exponent(*w) >= N) {
	  // if covering constraint is met, make it inactive
	  p_d->remove(*w);
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
      tmp += (*iter)->x;
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
      tmp += (*iter)->x;
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
		
  if (argc >= 2)
    epsilon = atof(argv[1]);
		
#ifdef NDEBUG
  srand(time(NULL));
#endif

  solve_instance I(epsilon);
  I.solve();

  return 0;
}

