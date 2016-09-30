//  -*- mode: c++; flycheck-clang-language-standard: "c++11"; -*-

#include <cmath>
#include <assert.h>
#include <limits>

#include "utilities.h"

#include "approx_sum_exp.h"

// Method:
//
// maintain approximation A to B = sum_k exp(a_k) of the form
//
//     A = exp(-shift) * C, where C = sum_k floor( exp(a_k + shift) ).
//
// Then A <= B <= A + exp(-shift) * #terms, so
//
//   B/A <= 1 + exp(-shift) * #terms / A
//       <= 1 + #terms / C
//
// Maintain shift large enough so that C >= #terms / delta,
// so log(B) - log(A) = log(B/A) <= log(1 + delta) <= delta.
//
// To do so, it suffices to maintain max_k exp(a_k + shift) >= #terms / delta.
// That is, max_k a_k + shift >= log(#terms / delta),
// that is, shift >= log(#terms / delta) - max_k a_k
// That is, shift >= log(#terms) + log(1/delta) - max_k a_k
//
// Store C in an exact (unsigned integer) type, so that
// arithmetic errors do not accumulate as terms are updated.
//
// Specifically, store C in an unsigned 64-bit type.
// It fits as long as (#terms)*exp(max_k a_k + shift) <= 2**64.
// That is, shift <= log(2**64/#terms) - max_k a_k.
// That is, shift <= 64*log(2) - log(#terms) - max_k a_k.
// 
// So, roughly, we have plenty of room as long as 
//
//   2*log(#terms) + log(1/delta) <= 64 * log(2) ~ 10^20 
// 
// e.g. #terms/delta <= 10^20

typedef approx_sum_t::exact_t exact_t;

static_assert(std::numeric_limits<exponent_t>::is_iec559, "IEEE 754 required");

static const double     negative_infinity    = -std::numeric_limits<exponent_t>::infinity();
static const exponent_t shifted_exponent_max =  std::log((double) std::numeric_limits<exact_t>::max());

void 
approx_sum_t::reset_shift() { 
  if (n_non_zero == 0) {
    assert (sum == 0);
    shift = 0;
    return;
  }

  exponent_t max_e = max(exponents);

  double top    = 8*sizeof(exact_t)*std::log(2.0) - std::log(n_non_zero);
  double bottom = std::log2(n_non_zero) + std::log2(1.0/delta);

  assert (top - bottom >= 5);

  shift = (top + bottom)/2 - max_e;

  sum = 0;
  n_non_zero = 0;

  for (size_t k = 0;  k < exponents.size();  ++k)  {
    auto & e     = exponents.at(k);
    auto & exact = exacts.at(k);
    
    if (e != negative_infinity) ++ n_non_zero;

    assert (e + shift <= shifted_exponent_max);
    if (e + shift < 1) {
      exact = 0;
      continue;
    }
    double exact_double = std::floor(std::exp(e + shift));
    exact = exact_double;
    assert (exact_double = (double) exact);
    if (not SafeAdd(sum, exact, sum)) assert (false);
  }
  assert (sum*delta >= n_non_zero);
}

void
approx_sum_t::set_exponent(size_t k, exponent_t new_e) {
  if (k >= exponents.size()) {
    exponents.resize(k+1, negative_infinity);
    exacts.resize(k+1, 0);
  }
  exponent_t & old_e = exponents.at(k);
  if (old_e == new_e) return;

  if (old_e == negative_infinity)
    ++ n_non_zero;

  old_e = new_e;

  if (new_e + shift > shifted_exponent_max) {
    reset_shift();
    return;
  } 
  exact_t  & exact = exacts.at(k);
  sum -= exact;

  if (new_e + shift >= 1) {
    double exact_double = std::floor(std::exp(new_e + shift));
    exact = exact_double;
    assert (exact_double = (double) exact);
    if (not SafeAdd(sum, exact, sum)) {
      reset_shift();
      return;
    }
  } else {
    if (new_e == negative_infinity) -- n_non_zero;
    exact = 0;
  }

  maybe_reset_shift();
}

bool    
approx_sum_t::term_zeroed(size_t k) {
  return (k >= exponents.size()
          || exponents.at(k) == negative_infinity);
}

void
approx_sum_t::zero_term(size_t k) {
  if (k < exponents.size())
    set_exponent(k, negative_infinity);
}

base_t  
approx_sum_t::log_lb() {
  // log(exp(-shift) * sum))
  return std::log((base_t) sum) - (base_t) shift;
}

base_t  
approx_sum_t::log_ub() {
  // log(exp(-shift) * sum))
  return std::log((base_t) sum + n_non_zero) - (base_t) shift;
}
