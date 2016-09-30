//  -*- mode: c++; flycheck-clang-language-standard: "c++11"; -*-

#ifndef APPROX_SUM_H
#define APPROX_SUM_H

#include <vector>
#include <assert.h>

// Maintain a set of terms { exp(a_k) } where each exponent a_k is a scalar.
// Allow updates to a_k's.
// Return scalar z such that z <= log(sum_k exp(a_k)) <= z + log(1+delta) <= z + delta,
// where delta is a pre-specified parameter.
//

typedef double          exponent_t;
typedef double          base_t;

class approx_sum_t {

public:

  typedef uint_fast64_t   exact_t;
  typedef exponent_t      exponent_t;
  typedef base_t          base_t;

private:

  double                  delta;
  exact_t                 sum;
  exponent_t              shift;
  size_t                  n_non_zero;

  std::vector<exact_t>    exacts;

  void reset_shift();
  inline void maybe_reset_shift() { if (sum*delta < n_non_zero) reset_shift(); }

public:

  std::vector<exponent_t> exponents;

  void clear(double _delta = 0) {
    if (_delta != 0) delta = _delta;
    assert (delta > 0);
    sum = 0;
    shift = 0;
    n_non_zero = 0;
    exponents.clear();
    exacts.clear();
  };
       
  approx_sum_t(double delta = 0) : delta (delta) 
  { 
    assert (delta >= 0);
    if (delta > 0) clear(); 
  }
  void    set_exponent (size_t k, exponent_t e);
  void    zero_term    (size_t k);
  bool    term_zeroed  (size_t k);
  base_t  log_lb          ();
  base_t  log_ub          ();
};

#endif
