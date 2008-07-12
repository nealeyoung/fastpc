#ifndef SAMPLER_H
#define SAMPLER_H

#include <assert.h>
#include <stdlib.h>

//#define inline

#include "my_vector.h"

typedef long long unsigned weight_t;

//#define MAX_WEIGHT_T (weight_t((1<< (8*sizeof(weight_t)))-1))
#define MAX_WEIGHT_T (weight_t(-1))

class sampler_weight_t {
 public:
  int i;
  int x;
  bool removed;

protected:
  struct bucket_t* bucket;
  int index_in_bucket;
  struct exponent_entry_t* exponent_entry;

  friend class dual_sampler_t;
  friend class primal_sampler_t;
};

struct bucket_t :  my_vector<sampler_weight_t *> {
  int min_exponent;

  bucket_t() : min_exponent(-1) {};
};

struct exponent_entry_t {
  int exponent;
  bucket_t* bucket;
  bool at_boundary;
  weight_t cached_weight;
  bucket_t* cached_weight_min_bucket;

  exponent_entry_t() :
    exponent(-1), bucket(NULL), at_boundary(false),
    cached_weight(0), cached_weight_min_bucket(NULL)
  {
  };
};

class dual_sampler_t {
 public:
  dual_sampler_t(int n, double epsilon, int min_expt, int max_expt, int prec = 0);
  void init();
  sampler_weight_t* sample();
  void remove(sampler_weight_t*);

  sampler_weight_t* get_ith(int i) { return &weights[i]; }
  int get_ith_exponent(int i) { return weights[i].exponent_entry->exponent; }

  int n_rebuilds();
  int n_rebuild_ops();

protected:
  double eps;
  weight_t total_weight; 	// depends on current_min_bucket
  bucket_t* current_min_bucket;	// min bucket with positive size 
  bucket_t* current_max_bucket;	// max bucket with positive size 

  int permanent_min_exponent;
  int permanent_max_exponent;

  int exponents_per_bucket;
  bucket_t* total_weight_min_bucket;

  int rebuilds;
  int rebuild_ops;

  my_vector<sampler_weight_t> weights;
  my_vector<exponent_entry_t> exponents;
  my_vector<bucket_t> buckets;

  void insert_in_bucket(sampler_weight_t*, bucket_t*);
  weight_t max_bucket_weight(bucket_t*);
  weight_t exponent_weight(exponent_entry_t*);
  weight_t random_weight_t(weight_t);
  
  inline 
  exponent_entry_t& expt(int e) { 
    return exponents[e-permanent_min_exponent]; 
  }

public:
  inline int
  dual_sampler_t::increment_exponent(sampler_weight_t *w) {
    exponent_entry_t* &e = w->exponent_entry;

    if (! e->at_boundary) {
      e += 1;
    } else{
      count_ops(3);

      remove(w);
      e += 1;
      insert_in_bucket(w, e->bucket);
    }
    return e->exponent;
  }
};

// to implement the primal sampler,
// where the weight is supposed to be (1+eps)^(yp_i)
// we use the dual sampler, where the weight is (1-eps')^(yd_i)

// but we take eps' such that 1-eps' = 1/(1+eps), and yd_i = -yp_i
//
// then (1-eps')^(yd_i) = (1+eps)^(yp_i)
//
// also the exponents in the dual_sampler are negative and decreasing,
// which is okay, but we need to adjust the shift a little to the right

class primal_sampler_t : public dual_sampler_t {
public:
  primal_sampler_t(int n, double epsilon, int min_expt, int max_expt, int prec = 0)
    : dual_sampler_t(n, epsilon/(1+epsilon), -max_expt,-min_expt, prec) 
  {
  }
  void init();

  int get_ith_exponent(int i) { return -dual_sampler_t::get_ith_exponent(i); }

  inline int
  increment_exponent(sampler_weight_t *w) {
    exponent_entry_t* &e = w->exponent_entry;

    if (! e->at_boundary) {
      e -= 1;
    } else{
      count_ops(3);

      remove(w);
      e -= 1;
      insert_in_bucket(w, e->bucket);
    }
    return -e->exponent;
  }
};


#endif
