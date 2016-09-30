//  -*- mode: c++; flycheck-clang-language-standard: "c++11"; -*-

#ifndef APPROX_SUM_H
#define APPROX_SUM_H

#include <vector>

// Maintain a set of exponents { (1+epsilon)^{a_k} : k = 0,1,..,size }
// Each exponent is an integer.  epsilon is in (-1,1) - {0}.
// Allow updates to sizes.
// Maintain close lower bound to sum_k (1+epsilon)^{a_k}
// in the form
//
//     (1+epsilon)^shift  *  sum_k floor( (1+epsilon)^{a_k - shift} ),
//
// where shift is maintained so the error bound (1+epsilon)^shift sum_k 1
// is at most epsilon times the sum on the right,
// and the term [ sum_k floor( (1+epsilon)^{a_k - shift} )  ] 
// in the lower bound fits in a 64-bit unsigned integer.

// assumptions:
//  epsilon > -1 and epsilon != 0
//  if epsilon > 0, each a_k never decreases
//  if epsilon < 0, each a_k never increases
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

// uncomment to debug
// #define DEBUG_APPROX_SUM

struct approx_sum_t {
  approx_sum_t(size_t size, double epsilon); // epsilon in (-1,1) - {0}

  typedef double        scalar_t;
  typedef int           exponent_t;
  typedef uint_fast64_t rounded_t;

  // approximation to sum is pow(1+epsilon, shift) * sum
  const double epsilon;
  rounded_t      sum;
  exponent_t     shift;
  std::vector<exponent_t> exponents;

  void  raise_exponent(size_t k, exponent_t exponent);  // precondition: epsilon > 0
  void  lower_exponent(size_t k, exponent_t exponent);  // precondition: epsilon < 0
  void remove_exponent(size_t k);                       // precondition: epsilon < 0

  // private stuff
  exponent_t shifted_exponent_limit;
  exponent_t most_significant_exponent;

#ifdef DEBUG_APPROX_SUM
  scalar_t debug_sum;
  void debug();
#endif

  static exponent_t _scalar_to_exponent(double epsilon, scalar_t scalar);
  static rounded_t _exponent_to_rounded(double epsilon, exponent_t exponent);

  inline exponent_t _scalar_to_exponent(scalar_t scalar) {
    return _scalar_to_exponent(epsilon, scalar);
  }
  inline rounded_t _exponent_to_rounded(exponent_t exponent) {
    return _exponent_to_rounded(epsilon, exponent);
  }

  scalar_t times(approx_sum_t & other);
  scalar_t divided_by(approx_sum_t & other);

  inline scalar_t operator*(approx_sum_t & other) { return times(other); }
  inline scalar_t operator/(approx_sum_t & other) { return divided_by(other); }
};

#endif
