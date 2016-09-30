//  -*- mode: c++; flycheck-clang-language-standard: "c++11"; -*-

#ifndef APPROX_SUM_H
#define APPROX_SUM_H

#include <vector>
#include <assert.h>

// Maintain a set of terms { a_k } where each term is a scalar.
// Allow updates to a_k's.
// Return scalar z such that z <= log(sum_k a_k) <= z + log(1+delta) <= z + delta,
// where delta is a pre-specified parameter.
//

typedef long double exponent_t;

class approx_sum_t {

public:

  typedef long double scalar_t;

  std::vector<scalar_t> terms;

private:
  scalar_t                delta;
  scalar_t                max_error;
  scalar_t                sum;
  size_t                  n_non_zero;

  std::vector<scalar_t> errors;

  void rebuild();

public:

  void clear(double _delta = 0) {
    if (_delta != 0) delta = _delta;
    sum = 0;
    n_non_zero = 0;
    max_error = 0;
    terms.clear();
    errors.clear();
  };
       
  approx_sum_t(double delta = 0) { clear(delta); }
  void    set_exponent (size_t k, scalar_t e);
  void    set_term     (size_t k, scalar_t v);
  void    zero_term    (size_t k);
  bool    term_zeroed  (size_t k);
  scalar_t  log_lb     ();
  scalar_t  log_ub     ();
};

#endif
