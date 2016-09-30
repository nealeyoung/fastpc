//  -*- mode: c++; flycheck-clang-language-standard: "c++11"; -*-

#ifndef APPROX_SUM_H
#define APPROX_SUM_H

#include <vector>
#include <cmath>

// Maintain a set of exponents { (1+delta)^{a_k} : k = 0,1,..,size }
// Each exponent is an integer.  delta is in (-1,1) - {0}.
// Allow updates to sizes.
// Maintain close lower bound to sum_k (1+delta)^{a_k}
// in the form
//
//     (1+delta)^shift  *  sum_k floor( (1+delta)^{a_k - shift} ),
//
// where shift is maintained so the error bound (1+delta)^shift sum_k 1
// is at most delta times the sum on the right,
// and the term [ sum_k floor( (1+delta)^{a_k - shift} )  ] 
// in the lower bound fits in a 64-bit unsigned integer.

// assumptions:
//  delta > -1 and delta != 0
//  if delta > 0, each a_k never decreases
//  if delta < 0, each a_k never increases
//

// increase x[j] by delta
//
// -> increase P_i x by delta_i P_ij for many i
// -> increase p[i] = (1+eps)**(P_i x) by (1+eps)**delta_i factor for these i
// -> increase term for p[i]  in  P'_{j} p  by (1+eps)**delta_i
//
//  & increase C_i x by delta_i C_ij for many i
// -> decrease c[i] = (1-eps)**(C_i x) by (1-eps)**delta_i factor for these i
// -> decrease term for c[i]  in  C'_{j} c  by (1-eps)**delta_i
//
// maintain approx_sum_t's for |p| and |c|
// and, within inner loop for j, for C'_j c and P'_j p

// support removal of terms from approx_sum's for |c| and C'_j c
//

typedef int           exponent_t;
typedef uint_fast64_t rounded_t;

struct approx_sum_t {
  typedef exponent_t exponent_t;
  typedef rounded_t rounded_t;
  typedef double scalar_t;

  double delta;

  // Lower bound to sum is pow(1+delta, shift) * sum.
  // Upper bound to sum is pow(1+delta, shift) * (sum + # exponents).
  // Maintained so upper bound <= upper bound*(1+delta).

  rounded_t      sum;
  exponent_t     shift;
  std::vector<exponent_t> exponents;
  std::vector<bool> removed;
  size_t n_remaining;

  void  raise_exponent(size_t k, exponent_t exponent);
  void remove_exponent(size_t k);                       // precondition: delta < 0

  void initialize(std::vector<exponent_t>& _exponents, double _delta);
  void initialize(std::vector<scalar_t>& values, double _delta);

  // delta in (-1,1) - {0}
  approx_sum_t(std::vector<exponent_t>& _exponents, double _delta) {
    initialize(_exponents, _delta);
  }

  approx_sum_t(size_t size, int exponent, double delta) {
    std::vector<exponent_t> _exponents(size, exponent);
    initialize(_exponents, delta);
  }

  approx_sum_t() : delta(0) {
    // make sure to call initialize() before using
  }

  scalar_t   times(approx_sum_t & other);
  scalar_t   divided_by(approx_sum_t & other);

  inline scalar_t operator*(approx_sum_t & other) { return times(other); }
  inline scalar_t operator/(approx_sum_t & other) { return divided_by(other); }

  // private stuff

  void       sanity_check();
  void       _reset_shift();
  exponent_t shifted_exponent_limit;
  rounded_t  _exponent_to_rounded(exponent_t exponent);
  exponent_t _scalar_to_exponent(scalar_t scalar);

  // only use this for testing small values
  inline long double lower_bound() {
    return std::pow((long double)(1.0+delta), (long double) shift) 
      * (long double) sum; 
  }

  inline long double upper_bound() {
    return std::pow((long double)(1.0+delta), (long double) shift)
      * (long double) (sum + exponents.size()); 
  }
};

#endif
