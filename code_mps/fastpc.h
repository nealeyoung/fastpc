#ifndef FASTPC
#define FASTPC

#include <iostream>
#include <cstdlib>
#include <ctime>
#include <map>
#include <cmath>
#include <list>

#include <fstream>
#include <string>
#include <assert.h>
#include <stdio.h>

#include "my_vector.h"
#include "sampler.h"

#include "main_include.h"

using namespace std;


//objects for nonzero elements in the input matrix
//has a coefficient and pointers to sampler objects
class nonzero_entry_t {
 public:
  double coeff;
  sampler_item_t* sampler_pointer;    //element in p_p or p_d
  sampler_item_t* u_sampler_pointer;  //element in p_pXuh or p_dXu
  int exponent;  //coeff rounded to nearest power of (1-eps)
  nonzero_entry_t(double value, double eps, sampler_item_t* sampler, sampler_item_t* u_sampler);
};

//used for exact sorting of matrices M and MT
struct list_sort_criteria{
  //compare and return
  bool operator()(nonzero_entry_t* left, nonzero_entry_t* right) 
  {
    return (left->coeff > right->coeff);  //sort in decreasing order
  };
};

typedef list<nonzero_entry_t *> line_element;

class solve_instance{
private:
  int r, c, N;

  primal_sampler_t *p_p;
  dual_sampler_t *p_d;

  primal_u_sampler_t *p_pXuh;
  dual_u_sampler_t *p_dXu;

  my_vector<line_element> M, MT, M_copy;
  
  string file_name;
  double eps, epsilon;

  // To implement random_pair function in paper, we must compute |p_pXuh||p_d|/(|p_pXuh||p_d|+|p_p||p_dXu|)
  // Because of normalization, sampler weights at any given time during alg are internally consistent,
  // but not consistent relative to other samplers. So, we compute an offset for each sampler based on 
  // how often it's been normalized and factor this in to the calculation to correct the ratios of the 
  // sampler weights.
  // So finally, we compute 1/(1+ratios), where ratios = (|p_p|/|p_pXuh|)*(|p_dXu|/|p_d|)*(1-eps)^(total offset).
  // We also use the rejection method to ensure accurate distributions; if either chosen sampler rejects when sampling,
  // the entire sampling process is re-started, including the choice of samplers
  double p_shift_ratio;
  double d_shift_ratio;

  void random_pair(sampler_item_t** wi, sampler_item_t** wj, dual_sampler_t* p_p, dual_sampler_t* p_d, dual_sampler_t* p_pXuh, dual_sampler_t* p_dXu);
  
  //find largest active entry/entries of a row in M; used to re-calculate uh_i when constraint is dropped
  nonzero_entry_t* get_largest_active(line_element* row);
  void get_two_largest_active(line_element* row, nonzero_entry_t** first, nonzero_entry_t** second);

  //for debugging test of sampling process-- simulate sample and increment of x and xhat, but don't increment samplers
  void freeze_and_sample(my_vector<line_element>& M, my_vector<line_element>& MT, int rows, int cols, dual_sampler_t* p_d, primal_sampler_t* p_p, dual_u_sampler_t* p_dXu, primal_u_sampler_t* p_pXuh, double epsilon, int prob);

public:	
  solve_instance(double epsilon, string file_name);
  void solve();
  void pseudo_sort(my_vector<line_element> *matrix,int col );
};

 
#endif
