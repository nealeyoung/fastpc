#ifndef SAMPLER_H
#define SAMPLER_H

#include <assert.h>
#include <stdlib.h>
#include <iostream>

using namespace std;

//#define inline

#include "my_vector.h"

typedef long long unsigned weight_t;
#define MAX_WEIGHT_T (weight_t(-1))

class dual_sampler_t;		// container supporting random sampling
class primal_sampler_t;		// container supporting random sampling
class dual_u_sampler_t;         //ph X u sampler in alg
class primal_u_sampler_t;       //p X uh sampler in alg

// sampler_item_t:
//  a single sampleable item
class sampler_item_t {
 public:
  int i;			// index in collection
  double x;			// value (to get or set, not used internally)
  bool removed;			// if removed from collection

  //when an item's weight is so small that it does not fit in the sampler,
  //we keep it in the smallest-weight sampler bucket with exponent of permanent_max_exponent and 
  //consider its weight to be 0, but we keep track of its exact exponent via an overflow variable so that
  //if its weight increases again (due to normalization) it can re-join the sampler as normal.
  //specifically, such an item's exact exponent will be sum of the
  //exponent in its exponent_entry (permanent_max_exponent) and the exponent_overflow.
  unsigned int exponent_overflow;
  
  struct exponent_entry_t* exponent_entry; 

protected:
  struct bucket_t* bucket;
  int index_in_bucket;

  friend class dual_sampler_t;
  friend class primal_sampler_t;
  friend class dual_u_sampler_t;
  friend class primal_u_sampler_t;
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

  void init();			// call after construction
  
  //return total_weight, updating if necessary
  weight_t get_update_total_weight();

  //count number of non-overflowing items in a bucket-- only called on smallest-weight bucket
  virtual int non_overflow_size(bucket_t* b);  
  sampler_item_t* get_ith(int i) { return &items[i]; } 
				// access item with index i
  sampler_item_t* sample();	// randomly sample an item
  void remove(sampler_item_t*);
				// remove item from collection

  int get_exponent(sampler_item_t* s) { return s->exponent_entry->exponent; }
  virtual int get_exponent_shift(); //how much items have been normalized
  weight_t exact_exp_weight(int exp); //calculate exact weight for given exponent
  weight_t shift_exp_weight(int exp); //calculate weight without scaling by MAX_WEIGHT_T
  int n_rebuilds();
  int n_rebuild_ops();

  //total shift in exponents between the ones currently stored and actual; incremented on normalization 
  int exp_shift; 
  bool exp_shift_updated; 


  protected:
  const double eps;

  my_vector<exponent_entry_t> exponents; // bookkeeping info per exponent
  my_vector<bucket_t> buckets;	// groups of ptrs to "close" items,
				// see bucket_t above

  weight_t total_weight; 	// upper bound on total of all weights
  bool recalculate_weight;      // signals recalculation of weight when current_min_bucket changes
  
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
  const int NORMALIZE_SHIFT; //how many buckets to shift items by during normalization

  int rebuilds;			// just for profiling
  int rebuild_ops;
  
  void insert_in_bucket(sampler_item_t*, bucket_t*);
  weight_t max_bucket_weight(bucket_t*);
  weight_t exponent_weight(exponent_entry_t*);
  weight_t random_weight_t(weight_t);

  //for debugging-- outputs current sampler snapshot to cerr; w is sampler item being removed or inserted
  void output_sampler_insert(sampler_item_t* w);
  void output_sampler_remove(sampler_item_t* w);
   
  inline 
  exponent_entry_t& expt(int e) { 
    return exponents[e-permanent_min_exponent]; 
  }

public:
  virtual int increment_exponent(sampler_item_t *w);

  //allows access to exponent_weight in fastpc
  inline
    weight_t get_exponent_weight(exponent_entry_t* e) {
    return exponent_weight(e);
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
	
  //count number of non-overflowing items in a bucket
  virtual int non_overflow_size(bucket_t* b);

  virtual int get_exponent_shift();

  virtual int increment_exponent(sampler_item_t *w);
};

 // sampler p_dXu 
 class dual_u_sampler_t : public dual_sampler_t { 
  public: 
   dual_u_sampler_t(int n, double epsilon, int min_expt, int max_expt, int prec = 0)
     : dual_sampler_t(n, epsilon, min_expt, max_expt, prec)
     { 
     } 
   
     //allows for insertion of element with arbitrary exponent, i.e.
     //arbitrary probability
     void update_item_exponent(sampler_item_t* item, int exp);

     //when buckets at front of sampler vacate, we shift all items' exponents downward (towards the front)
     //so as to keep those buckets occupied; this ensures that items with overflow
     //can be re-inserted into normal sampler activity when all items' exponents "catch up"
     void normalize_exponents();
     
     //count number of non-overflowing items in a bucket
     virtual int non_overflow_size(bucket_t* b);

     virtual int get_exponent_shift(); //how much items have been normalized

     virtual int increment_exponent(sampler_item_t *w);
 };

 // sampler p_pXuh
 class primal_u_sampler_t : public primal_sampler_t { 
  public: 
   primal_u_sampler_t(int n, double epsilon, int min_expt, int max_expt, int prec = 0) 
     : primal_sampler_t(n, epsilon, min_expt, max_expt, prec)
     { 
     } 

     // see comment for dual_u_sampler_t
     void update_item_exponent(sampler_item_t* item, int exp);
  
     // when items approach the permanent_min_exponent, we shift all exponents upward
     // i.e. backwards in sampler, to prevent going off front of sampler
     void normalize_exponents();

     // count number of non-overflowing items in a bucket
     virtual int non_overflow_size(bucket_t* b);

     virtual int get_exponent_shift(); //how much items have been normalized

     virtual int increment_exponent(sampler_item_t *w);
}; 

#endif
