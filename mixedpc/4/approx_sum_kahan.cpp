//  -*- mode: c++; flycheck-clang-language-standard: "c++11"; -*-

#include <cmath>
#include <assert.h>
#include <limits>

#include "utilities.h"

#include "approx_sum_debug.h"

typedef approx_sum_t::scalar_t scalar_t;

void
approx_sum_t::set_exponent(size_t k, scalar_t new_e) {
  set_term(k, std::exp(new_e));
}

void
approx_sum_t::set_term(size_t k, scalar_t new_v) {
  if (k >= terms.size()) {
    terms.resize(k+1, 0);
    errors.resize(k+1, 0);
  }

  auto & term = terms.at(k);

  if (term == new_v) return;

  if (term == 0)
    ++n_non_zero;
  else if (new_v == 0)
    --n_non_zero;


  auto & error = errors.at(k);

  auto delta = new_v - term + error;
  auto old_sum = sum;
  sum += delta; // sum - old_sum - delta = 0
  error = (sum - old_sum) - delta; // kahan summation
  term = new_v;

  if (error > max_error)  max_error = error;

  if (max_error*n_non_zero >= delta * sum)  rebuild();
}

void 
approx_sum_t::rebuild() {
  sum = 0;
  max_error = 0;
  for (size_t k = 0;  k < terms.size();  ++k) {
    auto & term = terms.at(k);
    auto & error = errors.at(k);
    auto old_sum = sum;
    sum += term; // sum - old_sum - delta = 0
    error = (sum - old_sum) - term; // kahan summation
    if (error > max_error)  max_error = error;
  }
  ASSERT (max_error*n_non_zero <= delta * sum,
          _(max_error)
          _(n_non_zero)
          _(delta)
          _(sum)
          );
}

bool    
approx_sum_t::term_zeroed(size_t k) {
  return (k >= terms.size()
          || terms.at(k) == 0);
}

void
approx_sum_t::zero_term(size_t k) {
  set_term(k, 0);
}

scalar_t  
approx_sum_t::log_lb() {
  // log(exp(-shift) * sum))
  return std::log(sum);
}

scalar_t
approx_sum_t::log_ub() {
  // log(exp(-shift) * sum))
  return std::log(sum + max_error*n_non_zero);
}
