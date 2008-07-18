#ifndef SAMPLER_H
#define SAMPLER_H

#include <assert.h>
#include <stdlib.h>

//#define inline

#include "my_vector.h"

typedef long long unsigned weight_t;
#define MAX_WEIGHT_T (weight_t(-1))

class dual_sampler_t;		// container supporting random sampling
class primal_sampler_t;		// container supporting random sampling

// sampler_item_t:
//  a single sampleable item
class sampler_item_t {
 public:
  int i;			// index in collection
  double x;			// value (to get or set, not used internally)
  bool removed;			// if removed from collection
  //int y;                        //approximately maintains M_ix

protected:
  struct bucket_t* bucket;
  int index_in_bucket;
  struct exponent_entry_t* exponent_entry;

  friend class dual_sampler_t;
  friend class primal_sampler_t;
};

// abstractly, a dual_sampler_t S is a collection of items
// and supports the following operations:
//   . access the ith item S[i]
//   . decrease weight of item S[i] by a 1-eps factor
//   . return a random index i with prob prop to weight of S[i]
//   . store a value S[i].x (x is not used internally)

// structures internal to dual_sampler_t:

// bookkeeping info for items with a given exponent
struct exponent_entry_t {
  int exponent;                 //now stores y instead of Mi_x
  bucket_t* bucket;		// which bucket (below) holds these items?
  bool at_boundary;		// is this exponent the max in the bucket?
  weight_t cached_weight;	// total weight of thse items
  bucket_t* cached_weight_min_bucket; // (recalculate if global min_bucket changes)

  exponent_entry_t() :
    exponent(-1), bucket(NULL), at_boundary(false),
    cached_weight(0), cached_weight_min_bucket(NULL)
  {
  };
};

// Group of ptrs to items with "close" exponents,
// ceil(1/eps) exponents per group.
struct bucket_t :  my_vector<sampler_item_t *> {
  int min_exponent;
  // items in this group should have exponents
  // between min_exponent and
  // min_exponent + (global) exponents_per_bucket

  bucket_t() : min_exponent(-1) {};
};


//
// the primary data structure
//
class dual_sampler_t {
protected:
  my_vector<sampler_item_t> items; // holds the items

public:
  dual_sampler_t(int n, double epsilon, int min_expt, int max_expt, int prec = 0);
				// constructor

  void init();			// call after construction (why?)

  sampler_item_t* get_ith(int i) { return &items[i]; } 
				// access item with index i
  sampler_item_t* sample();	// randomly sample an item
  void remove(sampler_item_t*);
				// remove item from collection

  int get_exponent(sampler_item_t* s) { return s->exponent_entry->exponent; }
  int n_rebuilds();
  int n_rebuild_ops();

protected:
  const double eps;

  my_vector<exponent_entry_t> exponents; // bookkeeping info per exponent
  my_vector<bucket_t> buckets;	// groups of ptrs to "close" items,
				// see bucket_t above

  weight_t total_weight; 	// upper bound on total of all weights
  bucket_t* total_weight_min_bucket; // ...recalc if min_bucket changes

  // Instead of keeping track of total weight exactly,
  // we count the weight of each item in a given bucket
  // as the max possible weight of any item in that bucket.
  // This is within a constant factor of the actual weight.
  // Then, when sampling, we take this into account as follows:
  // once we sample an item, we only take it
  // with probability = (actual weight)/(upper bound on weight).
  // If we don't take it, we start over...
  // This slows the sampling step but speeds up the weight-update step
  // and simplifies numerical precision issues.

  bucket_t* current_min_bucket;	// min non-empty bucket
  bucket_t* current_max_bucket;	// max non-empty bucket

  const int permanent_min_exponent;
  const int permanent_max_exponent;
  const int exponents_per_bucket;

  int rebuilds;			// just for profiling
  int rebuild_ops;

  void insert_in_bucket(sampler_item_t*, bucket_t*);
  weight_t max_bucket_weight(bucket_t*);
  weight_t exponent_weight(exponent_entry_t*);
  weight_t random_weight_t(weight_t);
  
  inline 
  exponent_entry_t& expt(int e) { 
    return exponents[e-permanent_min_exponent]; 
  }

public:
  inline int
  increment_exponent(sampler_item_t *w) {
    exponent_entry_t* &e = w->exponent_entry;

    if (! e->at_boundary) {
      e += 1;
    } else{
      count_ops(3);

      remove(w);
      e += 1;
      insert_in_bucket(w, e->bucket);
      // removing and inserting above may be less efficient
      // than direct implementation...?
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

  inline int
  increment_exponent(sampler_item_t *w) {
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
