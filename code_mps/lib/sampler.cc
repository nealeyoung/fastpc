#include <math.h>
#include <iostream.h>
#include <iomanip>
#include <typeinfo>
using namespace std;

#include "sampler.h"

//for debugging-- define which samplers to trace by setting to 1
const bool DUAL_TRACE = 0;
const bool DUAL_U_TRACE = 0; 
const bool PRIMAL_TRACE = 0;
const bool PRIMAL_U_TRACE = 0;


dual_sampler_t::dual_sampler_t(int n, double epsilon, int min_expt, int max_expt, int prec)
    : permanent_min_exponent(min_expt), permanent_max_exponent(max_expt),
      eps(epsilon), exponents_per_bucket(ceil(1/eps)), NORMALIZE_SHIFT(1)
{
  assert(epsilon > 0 && epsilon <= 0.5);
  assert(min_expt < max_expt);

  total_weight = 0;
  current_min_bucket = NULL;
  current_max_bucket = NULL;
  rebuilds = 0;
  rebuild_ops = 0;
  recalculate_weight = true;  //causes calculation of initial total weight

  //no normalization has happened yet
  exp_shift = 0;
  exp_shift_updated = true; //used in fastpc
  
  int n_expts = 1 + max_expt - min_expt;

  items.resize(n);
  exponents.resize(n_expts);
  buckets.resize(2+n_expts/exponents_per_bucket);

  count_ops(10+n+n_expts);

  for (int i = 0;  i < items.size();  ++i) {
    count_ops(6);

    items[i].i = i;
    items[i].x = 0;
    items[i].removed = true;
    items[i].exponent_entry = NULL;
    items[i].bucket = NULL;
    items[i].index_in_bucket = -1;
    items[i].exponent_overflow = 0;
  }

  for (int i = 0;  i < buckets.size();  ++i) {
    count_ops(1);

    buckets[i].min_exponent = permanent_max_exponent + 1;
  }

  //possible optimization-- only compute new bucket once per bucket (at edge)
  for (int i = permanent_max_exponent;  i >= permanent_min_exponent;  --i) {
    count_ops(3);

    expt(i).exponent = i;
    expt(i).bucket = &buckets[(i-permanent_min_exponent)/exponents_per_bucket];
    expt(i).bucket->min_exponent = i;
  }

  // get rid of buckets with no exponents
  while (buckets.size() > 0
	 && buckets[buckets.size()-1].min_exponent == permanent_max_exponent + 1)
    buckets.pop_back();
}

void
dual_sampler_t::init() {
  count_ops(buckets.size() + items.size()*2);

  // dual exponents increase with time
  for (int i = 1;  i < buckets.size();   ++i)
    expt(buckets[i].min_exponent-1).at_boundary = true;
  expt(permanent_max_exponent).at_boundary = true;

  // insert items in buckets
  current_min_bucket = expt(permanent_min_exponent).bucket;
  current_max_bucket = current_min_bucket;

  for (int i = 0;  i < items.size();  ++i) {
    items[i].exponent_entry = &expt(permanent_min_exponent);
    insert_in_bucket(&items[i],  items[i].exponent_entry->bucket);
  }
}

void
primal_sampler_t::init() {
  count_ops(buckets.size() + items.size()*2);

  // primal exponents decrease
  for (int i = 0;  i < buckets.size();   ++i)
    expt(buckets[i].min_exponent).at_boundary = true;

  // insert items in buckets
  current_min_bucket = expt(permanent_max_exponent).bucket;
  current_max_bucket = current_min_bucket;

  for (int i = 0;  i < items.size();  ++i) {
    items[i].exponent_entry = &expt(permanent_max_exponent);
    insert_in_bucket(&items[i],  items[i].exponent_entry->bucket);
  }
}

int
dual_sampler_t::n_rebuilds() { return rebuilds; }

int
dual_sampler_t::n_rebuild_ops() { return rebuild_ops; }  

void
dual_sampler_t::remove(sampler_item_t *w) {
  count_ops(10);

  assert(w);
  assert(! w->removed);

  //for debugging--print sampler snapshot
  
  //dual_sampler
  if (DUAL_TRACE && typeid(*this) == typeid(dual_sampler_t)) {
    cerr << "CALLED FROM DUAL SAMPLER" << endl;
    output_sampler_remove(w);
  }

  //dual_u sampler
  if (DUAL_U_TRACE && typeid(*this) == typeid(dual_u_sampler_t)) {
    cerr << "CALLED FROM DUAL_U SAMPLER" << endl;
    output_sampler_remove(w);
  }

  //primal sampler
  if (PRIMAL_TRACE && typeid(*this) == typeid(primal_sampler_t)) {
    cerr << "CALLED FROM PRIMAL SAMPLER" << endl;
    output_sampler_remove(w);
  }

  //primal_u sampler
  if (PRIMAL_U_TRACE && typeid(*this) == typeid(primal_u_sampler_t)) {
    cerr << "CALLED FROM PRIMAL_U SAMPLER" << endl;
    output_sampler_remove(w);
  }

  bucket_t* bucket = w->bucket;
  assert(bucket);
  int i = w->index_in_bucket;
  assert(i >= 0);
  int n = bucket->size()-1;
  if (i != n) {
    (*bucket)[i] = (*bucket)[n];
    (*bucket)[i]->index_in_bucket = i;
  }
  bucket->pop_back();
  w->index_in_bucket = -1;
  w->bucket = NULL;
  w->removed = true;

  total_weight -= max_bucket_weight(bucket);

  while (current_min_bucket->size() == 0) {
    count_ops(3);

    current_min_bucket->shrink();
    ++current_min_bucket;
    exp_shift_updated = true;
    recalculate_weight = true; //since exponents will carry different weights now
    if (current_min_bucket - &buckets[0] >= buckets.size()) {
      current_min_bucket == NULL;
      break;
    }
  }

  while (current_max_bucket->size() == 0) {
    count_ops(3);
    current_max_bucket->shrink();

    --current_max_bucket;
    if (current_max_bucket < &buckets[0]) {
      current_max_bucket == NULL;
      break;
    }
  }

    

}

void
dual_sampler_t::insert_in_bucket(sampler_item_t *w, bucket_t* b) {
  count_ops(6);

  assert(b);
  w->bucket = b;
  b->push_back(w);
  w->index_in_bucket = b->size()-1;
  w->removed = false;

  if (current_max_bucket == NULL || b > current_max_bucket)  {
    current_max_bucket = b;
  } 

  if (current_min_bucket == NULL || b < current_min_bucket)  {
    exp_shift_updated = true;
    recalculate_weight = true;
    current_min_bucket = b;
  } else {  
    total_weight += max_bucket_weight(b);
	}

  //for debugging--print sampler snapshot
  
  //dual_sampler
  if (DUAL_TRACE && typeid(*this) == typeid(dual_sampler_t)) {
    cerr << "CALLED FROM DUAL SAMPLER" << endl;
    output_sampler_insert(w);
  }

  //dual_u sampler
  if (DUAL_U_TRACE && typeid(*this) == typeid(dual_u_sampler_t)) {
    cerr << "CALLED FROM DUAL_U SAMPLER" << endl;
    output_sampler_insert(w);
         
  }  

  //primal sampler
  if (PRIMAL_TRACE && typeid(*this) == typeid(primal_sampler_t)) {
    cerr << "CALLED FROM PRIMAL SAMPLER" << endl;
    output_sampler_insert(w);
  }

  //primal_u sampler
  if (PRIMAL_U_TRACE && typeid(*this) == typeid(primal_u_sampler_t)) {
    cerr << "CALLED FROM PRIMAL_U SAMPLER" << endl;
    output_sampler_insert(w);
  }
  

}

weight_t
dual_sampler_t::max_bucket_weight(bucket_t* b) {
  assert(b);
  return exponent_weight(&expt(b->min_exponent));
}

weight_t
dual_sampler_t::exponent_weight(exponent_entry_t* expt) {
  count_ops(2);

  assert(expt);
  assert(expt->bucket);
  assert(current_min_bucket);
  
  if (expt->cached_weight_min_bucket != current_min_bucket) {
    count_ops(30);

    int exp_offset = expt->exponent - current_min_bucket->min_exponent;
    expt->cached_weight = exact_exp_weight(exp_offset);
    expt->cached_weight_min_bucket = current_min_bucket;
  }
  return  expt->cached_weight;
}

weight_t
dual_sampler_t::random_weight_t(weight_t upper) {
  count_ops(30);

  assert(upper > 0);
  int bits = int(ceil(log2(upper)));
  weight_t mask = (weight_t(1)<<bits)-1;

  while (1) {
    // random returns random long in range 0 to 2**31 - 1
    weight_t r = random();
    count_ops(30);

    for (int i = 1;  i*31 < bits;  ++i) {
      r = (r << 31) + random();
      count_ops(30);
    }
    r &= mask;
    if (r < upper) return r;
  }
  assert(false);
  return 0;
}

sampler_item_t*
dual_sampler_t::sample() {

  get_update_total_weight();  //updates total weight if sampler has been normalized
                              // since last calculation of it
                              
  assert(total_weight > 0);
  while (1) {
    bool tried = false;
    weight_t total_weight_check = 0;

    weight_t r = random_weight_t(total_weight);
    for(bucket_t *b = current_min_bucket;  b <= current_max_bucket;  ++b) {
      weight_t w = max_bucket_weight(b);
      count_ops(3);

      int s = b->size();

      total_weight_check += w*s;

      if (r >= w*s)  {
	r -= w*s;
      } else {
	count_ops(4);

	tried = true;

	int i = r/w;
	r = r - i*w;

	if (r < exponent_weight((*b)[i]->exponent_entry))
	  return (*b)[i];
	else {
	  return NULL; //allows sampling to be restarted from beginning of RANDOM_PAIR
	}
      }
    }
    assert(tried || total_weight_check == total_weight);
    assert(tried);
  }
  return NULL;
}

void 
dual_u_sampler_t::update_item_exponent(sampler_item_t* item, int exp) {
  if(!item->removed) {
    exponent_entry_t* e = &expt(exp);
    item->exponent_entry = e;
    if (item->bucket != e->bucket) { //remove and re-insert only if the item goes into another bucket
      remove(item);
      insert_in_bucket(item, e->bucket);
    }
  }
}

void 
primal_u_sampler_t::update_item_exponent(sampler_item_t* item, int exp) {
  if(!item->removed) {
    exponent_entry_t* e = &expt(exp);
    item->exponent_entry = e;
    if (item->bucket != e->bucket) { //remove and re-insert only if the item goes into another bucket
      remove(item);
      insert_in_bucket(item, e->bucket);
    }
  }
}

weight_t
dual_sampler_t::get_update_total_weight() {
if (recalculate_weight) {
    ++rebuilds;

    assert(current_min_bucket);
    assert(current_max_bucket);

    total_weight = 0;
    recalculate_weight = false;

    for (bucket_t* b = current_min_bucket;  b < current_max_bucket;  ++b) { //handle last bucket separately
      rebuild_ops += 6;
      count_ops(2);

      weight_t w = max_bucket_weight(b);
      if (w == 0) break;
      total_weight += w*b->size();
    }
    // in last bucket, don't add weights for the items that have overflow 
    // since their (relative) weights are small enough to be ignored
    total_weight += max_bucket_weight(current_max_bucket)*non_overflow_size(current_max_bucket);
  }
  return total_weight;
}

int
dual_sampler_t::non_overflow_size(bucket_t* b) {
  return b->size();
}

int
primal_sampler_t::non_overflow_size(bucket_t* b) {
  return dual_sampler_t::non_overflow_size(b);
}

int
dual_u_sampler_t::non_overflow_size(bucket_t* b) {
  int num = 0;
  for (my_vector<sampler_item_t*>::iterator y = (*b).begin(); y != (*b).end(); ++y) {
    if ((*y)->exponent_overflow == 0)
      num++;
  }
  return num;
}

int
primal_u_sampler_t::non_overflow_size(bucket_t* b) {
  int num = 0;
  for (my_vector<sampler_item_t*>::iterator y = (*b).begin(); y != (*b).end(); ++y) {
    if ((*y)->exponent_overflow == 0){
      num++;
    }
  }
  return num;
}

int dual_sampler_t::increment_exponent(sampler_item_t *w) {
  exponent_entry_t* &e = w->exponent_entry;

  if (! e->at_boundary) {
    e += 1;
  } else {
    count_ops(3);

    remove(w);
    e += 1;
    insert_in_bucket(w, e->bucket);
  }
  return e->exponent;
}

int primal_sampler_t::increment_exponent(sampler_item_t *w) {
  exponent_entry_t* &e = w->exponent_entry;
  if (! e->at_boundary) {
    e -= 1;
  } else {
    count_ops(3);
      
    remove(w);
    e -= 1;
    insert_in_bucket(w, e->bucket);
  }
  return -e->exponent;
}

int dual_u_sampler_t::increment_exponent(sampler_item_t *w) {
  exponent_entry_t* &e = w->exponent_entry;


  if (! e->at_boundary) {
    e += 1;
  } else if (e->exponent < permanent_max_exponent) {//boundary of a bucket but not the last bucket
    count_ops(3);

    remove(w);
    e += 1;
    insert_in_bucket(w, e->bucket);
    //normalize when NORMALIZE_SHIFT buckets are empty at beginning of list
    if (current_min_bucket->min_exponent >= (permanent_min_exponent + NORMALIZE_SHIFT*exponents_per_bucket))
      normalize_exponents();
  } else { //its the boundary of the last bucket
    w->exponent_overflow++;
  }
  return e->exponent;
}

int primal_u_sampler_t::increment_exponent(sampler_item_t *w) {
  exponent_entry_t* &e = w->exponent_entry;
  if (! e->at_boundary) {
    //change the exponent_entry only when there is no overflow, otherwise change only the overflow
    if (w->exponent_overflow > 0)
      w->exponent_overflow--;
    else
      e -= 1;
  } else {
    count_ops(3);
    if (e->exponent < permanent_min_exponent + 2+exponents_per_bucket)
      normalize_exponents();
    remove(w);
    e -= 1;
    insert_in_bucket(w, e->bucket);
  }
  return -e->exponent;
}

void
dual_u_sampler_t::normalize_exponents() {
  int total_shift = NORMALIZE_SHIFT*exponents_per_bucket;
  exp_shift += total_shift;
  exp_shift_updated = true;

  int updated_exponent;
  for (int i = 0;  i < items.size();  ++i) {
    updated_exponent = items[i].exponent_entry->exponent + items[i].exponent_overflow - total_shift;
    if (updated_exponent > permanent_max_exponent) {
      items[i].exponent_overflow = updated_exponent - permanent_max_exponent;
      updated_exponent = permanent_max_exponent;
    } else {
      items[i].exponent_overflow = 0;
      update_item_exponent(&items[i], updated_exponent);
    }
  }
}

void
primal_u_sampler_t::normalize_exponents() {
  int total_shift = NORMALIZE_SHIFT*exponents_per_bucket;
  exp_shift += total_shift;
  exp_shift_updated = true;

  int updated_exponent;
  for (int i = 0;  i < items.size();  ++i) {
    updated_exponent = items[i].exponent_entry->exponent + items[i].exponent_overflow + total_shift;
    if (updated_exponent > permanent_max_exponent) {
      items[i].exponent_overflow = updated_exponent - permanent_max_exponent;
      updated_exponent = permanent_max_exponent;
    }
    update_item_exponent(&items[i], updated_exponent);
  }
}

int
dual_sampler_t::get_exponent_shift() {
  return current_min_bucket->min_exponent - permanent_min_exponent;
}

int
primal_sampler_t::get_exponent_shift() {
  //shift is diff b/t min exponent in bucket of permanent_max_exponent and min exponent in current_min_bucket
  return (permanent_max_exponent - exponents_per_bucket + 1) - current_min_bucket->min_exponent; 
}

int
dual_u_sampler_t::get_exponent_shift() {
  //dual_u_sampler also has shift from coeffs and normalization
  return exp_shift + current_min_bucket->min_exponent - permanent_min_exponent;
}

int
primal_u_sampler_t::get_exponent_shift() {
  return exp_shift + (permanent_max_exponent - exponents_per_bucket + 1) - current_min_bucket->min_exponent; 
}

weight_t
dual_sampler_t::exact_exp_weight(int exp) {
  return weight_t(pow(1.0-eps, exp)*(MAX_WEIGHT_T/(2*items.size())));  
}

weight_t
dual_sampler_t::shift_exp_weight(int exp) {
  return weight_t(pow(1.0-eps, exp));
}


void 
dual_sampler_t::output_sampler_insert(sampler_item_t* w) {
  cerr << "EXPONENT SHIFT: " << get_exponent_shift() << endl;
  //print one vertical line for each bucket
  cerr << "B ";
  for (int i=0; i < buckets.size(); i++)  
    for (int i=0; i < 2+((permanent_max_exponent-permanent_min_exponent)/exponents_per_bucket); i++)  //num of iterations = num of buckets
    cerr << "|";
  cerr << endl;    
  //print vertical line for each sampler item corresponding to which bucket it's in
  for (int i=0; i < items.size(); i++) {
    if (items[i].bucket != NULL) {
      int current_exp = items[i].exponent_entry->exponent;
      if (!items[i].removed) {
	cerr << i << " " << setw(1+((current_exp - permanent_min_exponent)/exponents_per_bucket)) << "|";
	if (&items[i] == w) cerr << "I";
	cerr << current_exp;
	cerr << endl;
      }
      else
	cerr << i << " " << setw(1+((current_exp - permanent_min_exponent)/exponents_per_bucket)) << "|X" << current_exp << endl;
    }
    
  }	
}

void 
dual_sampler_t::output_sampler_remove(sampler_item_t* w) {
  cerr << "EXPONENT SHIFT: " << get_exponent_shift() << endl;
  //print one vertical line for each bucket
  cerr << "B ";
  for (int i=0; i < buckets.size(); i++)  //num of iterations = num of buckets
    cerr << "|";
    cerr << endl;    
  //print vertical line for each sampler item corresponding to which bucket it's in
  for (int i=0; i < items.size(); i++) {
    int current_exp = items[i].exponent_entry->exponent;
    if (!items[i].removed) {      
      cerr << i << " " << setw(1+((current_exp - permanent_min_exponent)/exponents_per_bucket)) << "|";
      if (&items[i] == w) cerr << "R";
      cerr << current_exp;
      cerr << endl;
    }
    else
      cerr << i << " " << setw(1+((current_exp-permanent_min_exponent)/exponents_per_bucket)) << "|X" << current_exp << endl;
    
  }	
}
