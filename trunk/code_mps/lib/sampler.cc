#include <math.h>
#include <iostream.h>
using namespace std;

#include "sampler.h"

dual_sampler_t::dual_sampler_t(int n, double epsilon, int min_expt, int max_expt, int prec)
    : permanent_min_exponent(min_expt), permanent_max_exponent(max_expt),
      eps(epsilon), exponents_per_bucket(ceil(1/eps))//, max_exp_fraction(ceil(1/eps))
{
  assert(epsilon > 0 && epsilon <= 0.5);
  assert(min_expt < max_expt);

  total_weight = 0;
  current_min_bucket = NULL;
  current_max_bucket = NULL;
  total_weight_min_bucket = 0;
  rebuilds = 0;
  rebuild_ops = 0;
  
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
  }

  for (int i = 0;  i < buckets.size();  ++i) {
    count_ops(1);

    buckets[i].min_exponent = permanent_max_exponent + 1;
  }

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
  
  cout << "IN INIT() \n";

  for (int i = 0;  i < items.size();  ++i) {
    items[i].exponent_entry = &expt(permanent_min_exponent);
    cout << "EXPONENT: " << items[i].exponent_entry->exponent << "\n";
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

  bucket_t* bucket = w->bucket;
  assert(bucket);
  int i = w->index_in_bucket;
  assert(i >= 0);
  //cout << "bucket " << bucket << "removed " << !w->removed << endl << flush; //debug
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
    current_min_bucket = b;
  } else {
    total_weight += max_bucket_weight(b);
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
    expt->cached_weight = 
      weight_t(pow(1.0-eps, exp_offset)*(MAX_WEIGHT_T/(2*items.size())));
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

  get_update_total_weight();  //does nothing if called from inside random_pair 

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
	  break;
	}
      }
    }
    assert(tried || total_weight_check == total_weight);
    assert(tried);
  }
  return NULL;
}

//check if appropriate attributes get updated for items and buckets
void 
dual_u_sampler_t::update_item_exponent(sampler_item_t* item, int exp) {
  if(!item->removed) {
    item->exponent_entry = &expt(exp);
    remove(item);
    insert_in_bucket(item,  item->exponent_entry->bucket);
  }
}

void 
primal_u_sampler_t::update_item_exponent(sampler_item_t* item, int exp) {
 if(!item->removed) {
  item->exponent_entry = &expt(exp);
  remove(item);
  insert_in_bucket(item,  item->exponent_entry->bucket);
 }
}

//return the total_weight of the sampler, updating if necessary
weight_t
dual_sampler_t::get_update_total_weight() {
if (total_weight_min_bucket != current_min_bucket) {
    ++rebuilds;

    assert(current_min_bucket);
    assert(current_max_bucket);

    total_weight = 0;
    total_weight_min_bucket = current_min_bucket;

    for (bucket_t* b = current_min_bucket;  b <= current_max_bucket;  ++b) {
      rebuild_ops += 6;
      count_ops(2);

      weight_t w = max_bucket_weight(b);
      if (w == 0) break;
      total_weight += w*b->size();
    }
  }
 return total_weight;
}

void
primal_u_sampler_t::normalize_exponents(sampler_item_t* w) {
	int exp = w->exponent_entry->exponent;
	if (exp < permanent_min_exponent + exponents_per_bucket) {
		// we increase the max exponent to 90% of the initial value 
		//and increase the other exponents by the same amount
	        //what should this diff be? need to discuss.
		int diff = floor(0.2*exp);
  	cout << "PRIMAL EXPONENT: " << exp << endl;
  	cout << "PRIMAL DIFF: " << diff << endl;

		int updated_exponent;
	  for (int i = 0;  i < items.size();  ++i) {
	  	updated_exponent = items[i].exponent_entry->exponent - diff;
	  	if (updated_exponent > permanent_max_exponent)
		  updated_exponent = permanent_max_exponent;//these items have a low probability and probably will not be chosen any time
	  	//cout << "NEW EXPONENT: " << updated_exponent << endl;
	  	update_item_exponent(&items[i], updated_exponent);
	  }
	}
}
