#ifndef FASTPC
#define FASTPC

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


//these objects hold a coefficient and a pointer to a sampler object
//one of these objects is created for every nonzero element in the input matrix
class nonzero_entry_t {
 public:
  double coeff;
  sampler_item_t* sampler_pointer;
  nonzero_entry_t(double value, sampler_item_t* sampler);
};

typedef my_vector<nonzero_entry_t *> line_element;

class solve_instance{
private:
  int r, c, N;

  primal_sampler_t *p_p;
  dual_sampler_t *p_d;

  my_vector<line_element> M, MT, M_copy;

  string file_name;
  double eps, epsilon;


public:	
  solve_instance(double epsilon, string file_name);
  void solve();
};

struct nonzero_entry_t_comparator {
  bool operator()(nonzero_entry_t* a, nonzero_entry_t* b) {
    return (a->coeff < b->coeff);
  }
};

#endif
