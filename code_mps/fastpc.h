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


//these objects hold a coefficient and a pointer to a sampler object
//one of these objects is created for every nonzero element in the input matrix
class nonzero_entry_t {
 public:
  double coeff;
  sampler_item_t* sampler_pointer;
  sampler_item_t* u_sampler_pointer;
  int exponent;
  nonzero_entry_t(double value, double eps, sampler_item_t* sampler, sampler_item_t* u_sampler);

  //bool operator<(nonzero_entry_t* a);
};

struct list_sort_criteria{
  //compare and return
  bool operator()(nonzero_entry_t* left, nonzero_entry_t* right) 
  {
    return (left->coeff > right->coeff);
  };
};
typedef list <double> double_list;
typedef list<nonzero_entry_t *> line_element;

class solve_instance{
private:
  int r, c, N;

  primal_sampler_t *p_p;
  dual_sampler_t *p_d;

  primal_u_sampler_t *p_pXuh;
  dual_u_sampler_t *p_dXu;

  my_vector<line_element> M, MT, M_copy;
  
  //test samplers
  float *u;
  float *uh;
  unsigned long*   p;
  unsigned long* ph;
  unsigned long* pXuh;
  unsigned long* phXu;
  unsigned long p_total,ph_total,pXuh_total,phXu_total;


  string file_name;
  double eps, epsilon;

  double p_shift_ratio;
  double d_shift_ratio;

  //store relative shifts in p/pXuh and d/dXu
  int p_exp_shift;
  int d_exp_shift;

  void random_pair(sampler_item_t** wi, sampler_item_t** wj, dual_sampler_t* p_p, dual_sampler_t* p_d, dual_sampler_t* p_pXuh, dual_sampler_t* p_dXu);
  void random_pair(unsigned long* p,unsigned long* ph,unsigned long* pXuh,unsigned long* phXu,unsigned long p_total,unsigned long ph_total,unsigned long pXuh_total,unsigned long phXu_total);
  nonzero_entry_t* get_largest_active(line_element* row);
  void get_two_largest_active(line_element* row, nonzero_entry_t** first, nonzero_entry_t** second);

  //for debugging-- simulate sample and increment of x and xhat, but don't increment samplers
  void freeze_and_sample(my_vector<line_element>& M, my_vector<line_element>& MT, int rows, int cols, dual_sampler_t* p_d, primal_sampler_t* p_p, dual_u_sampler_t* p_dXu, primal_u_sampler_t* p_pXuh, double epsilon, int prob);

public:	
  solve_instance(double epsilon, string file_name);
  void solve();
  void sudo_sort(my_vector<line_element> *matrix,int col );
};

 
#endif
