//  -*- mode: c++; flycheck-clang-language-standard: "c++11"; -*-

#include <cstdint>
#include <cmath>
#include <assert.h>
#include <iostream>
#include <limits>

#include "utilities.h"

#include "approx_sum.h"

//typedef approx_sum_t::exponent_t exponent_t;
//typedef approx_sum_t::rounded_t rounded_t;
typedef approx_sum_t::scalar_t scalar_t;

static const exponent_t exponent_t_max   = std::numeric_limits<exponent_t>::max();
static const rounded_t  rounded_t_max    = std::numeric_limits<rounded_t>::max();

static_assert ((rounded_t) ((long double) rounded_t_max) == rounded_t_max,
               "rounded_t_max doesn't fit in long double");

void
approx_sum_t::initialize(std::vector<scalar_t>& values, double _epsilon) {

  epsilon = _epsilon;

  assert (std::abs(epsilon) < 1.0  &&  epsilon != 0.0);
  std::vector<exponent_t> exponents(values.size()); // hides this->exponents
  for (size_t i = 0;  i < values.size();  ++i)
    exponents.at(i) = _scalar_to_exponent(values.at(i));

  initialize(exponents, _epsilon);
}

void
approx_sum_t::initialize(std::vector<exponent_t>& _exponents, double _epsilon) {

  epsilon = _epsilon;
  exponents = _exponents;

  assert (std::abs(epsilon) < 1.0  &&  epsilon != 0.0);

  if (exponents.size() == 0) {
    sum = 0;
    shift = 0;
    n_remaining = 0;
    return;
  }

  removed.resize(exponents.size(), false);
  n_remaining = removed.size();

  long double tmp;

  if (epsilon > 0)
    // set shifted_exponent_limit 
    // to maximum exponent_t e such that (1+eps)**e fits in a rounded_t
    //
    // (1+eps)**(e-shift) <= rounded_t_max
    // e - shift <= log(rounded_t_max) / log(1+eps)
    tmp = std::floor(std::log((long double) rounded_t_max)
                     / std::log1p((long double) epsilon));
  else
    // set shifted_exponent_limit 
    // to minimum exponent_t e such that (1-eps)**e fits in a rounded_t
    //
    // (1-eps)**(e-shift) <= rounded_t_max
    // i.e. log(1-eps) (e - shift) <= log(rounded_t_max)
    // e - shift >= - log(rounded_t_max) / -log(1-eps)
    tmp = std::ceil(std::log((long double) rounded_t_max)
                    / std::log1p((long double) epsilon));

  // check that it fits in exponent_t shifted_exponent_limit without loss
  shifted_exponent_limit = tmp;
  assert (tmp == (long double) shifted_exponent_limit);

  // check that in _exponent_to_rounded(shifted_exponent_limit)
  // casting does not overflow or underflow rounded_t
  shift = 0;
  tmp = std::floor(std::pow((long double) (1+epsilon), tmp));
  assert ((long double) _exponent_to_rounded(shifted_exponent_limit)
          == tmp);

  // set the shift
  _reset_shift();
}

void 
approx_sum_t::raise_exponent(size_t k, exponent_t exponent) {
  ASSERT(epsilon != 0, "approx_sum_t::raise_exponent called on uninitialized approx_sum");
  ASSERT(! removed.at(k), "approx_sum_t::raise_exponent called on removed exponent");

  auto & e = exponents.at(k);
  ASSERT(exponent >= e,
         "attempt to lower exponent"
         _(k)
         _(e)
         _(exponent));

  if (exponent == e) return;

  if (epsilon > 0) {

    // this case only increases sum, so maintains error bound
    // only danger is overflowing sum

    if (e >= shift) 
      assert (safe_subtract(sum, _exponent_to_rounded(e), sum));

    e = exponent;

    if (e >= shift) {
      exponent_t diff;
      if (! safe_subtract(e, shift, diff)) {
        _reset_shift();
        assert (safe_subtract(e, shift, diff));
      } else if (diff > shifted_exponent_limit) { 
        _reset_shift();
      } else {
        if(! safe_add(sum, _exponent_to_rounded(e), sum))
          _reset_shift();
      }
    }
  } else { // epsilon < 0
    // this case decreases sum, so may violate error bound
    // should not underflow sum...

    if (e <= shift)
      assert (safe_subtract(sum, _exponent_to_rounded(e), sum));

    e = exponent;

    if (e <= shift)
      assert (safe_add(sum, _exponent_to_rounded(e), sum));

    // want upper_bound <= (1+eps) lower_bound
    // suffices if sum >= m / eps

    if (sum < exponents.size() / std::abs(epsilon))
      _reset_shift();
  }

  // assert error bound
  assert (sum >= exponents.size() / std::abs(epsilon));

  // for debugging
  ASSERT(upper_bound() <= (1+std::abs(epsilon))*lower_bound(),
         _(sum)
         _(epsilon)
         _(exponents.size())
         _(lower_bound())
         _(upper_bound()));
}

approx_sum_t::exponent_t
approx_sum_t::_scalar_to_exponent(scalar_t scalar) {
  // (1+eps)**e <= scalar
  // e*log(1+eps) <= log(scalar)
  // e <= log(scalar)/log(1+eps) or

  // (1-eps)**e <= scalar
  // e*log(1-eps) <= log(scalar)
  // e >= log(scalar)/log(1-eps)
  
  long double tmp = std::log((long double) scalar) / std::log1p(epsilon);

  if (epsilon > 0)
    tmp = std::floor(tmp);
  else
    tmp = std::ceil(tmp);

  exponent_t e = tmp;
  assert ((long double) e == tmp);

  return e;
}

approx_sum_t::rounded_t
approx_sum_t::_exponent_to_rounded(exponent_t exponent) {
  exponent_t shifted;
  assert (safe_subtract(exponent, shift, shifted));
  if (epsilon > 0) {
    assert (shifted >= 0);
    assert (shifted <= shifted_exponent_limit);
  } else {
    assert (shifted <= 0);
    assert (shifted >= shifted_exponent_limit);
  }
  // assert for debugging
  long double tmp = std::floor(std::pow((long double) (1+epsilon), (long double)(shifted)));
  assert ((long double)((rounded_t) tmp) == tmp);
  return (rounded_t) tmp;
}

void 
approx_sum_t::remove_exponent(size_t k) {
  assert (epsilon < 0);

  if (removed.at(k)) return;
  removed.at(k) = true;
  --n_remaining;

  exponent_t & e = exponents.at(k);
  if (e <= shift)
    assert (safe_subtract(sum, _exponent_to_rounded(e), sum));

  if (n_remaining == 0) {
    assert (sum == 0);
    shift = 0;
  }

  // Set it so that it will always exceed shift
  // so cannot be further increased 
  // and will never enter sum.
  // Using here epsilon < 0.
  // no longer necessary (?)
  e = exponent_t_max;
}

void
approx_sum_t::_reset_shift() {
  // Initialize shift as large as possible subject to error bound and overflow constraints.
  // Then reset sum.

  // std::cout << "_reset_shift" << std::endl;

  if (epsilon > 0) {

    // Error is at most (1+eps)**shift * m
    // need it to be at most eps * sum_e (1+eps)** e.
    // i.e. (1+eps)**shift * m <= eps * sum_e (1+eps)**e.
    // i.e. m/eps <= sum_e (1+eps)**(e - shift).
    //
    // Ask for stronger condition
    //   m/eps <= max_e floor((1+eps)**(e - shift)).
    // i.e. ceil(m/eps) <= (1+eps)**(max e - shift).
    // i.e. shift <= (max e) - ceil(log(ceil(m/eps))/log(1+eps)).

    // This should also guarantee
    // m/eps <= sum = sum_e floor((1+eps)**(e - shift)). (if eps > 0)

    // For no sum overflow when eps > 0,
    // suffices if m*(1+eps)**(max e - shift) <= rounded_t_max.
    // i.e. max e - shift <= log(rounded_t_max / m)/log(1+eps)
    // i.e. shift >= max e - log(rounded_t_max / m)/log(1+eps)

    // For no too-large shifted exponent overflow when eps > 0,
    // suffices if max e - shift <= shifted_exponent_limit
    // i.e. shift >= max e - log(rounded_t_max) /log(1+eps);

    exponent_t max_e = max(exponents);
    long double tmp = std::ceil(std::log(std::ceil((long double) exponents.size()/epsilon))
                                / std::log1p((long double) epsilon));

    exponent_t tmp_e = tmp;
    assert ( tmp = (long double) tmp_e );
    
    assert ( safe_subtract(max_e, tmp_e, shift) );

    sum = 0;
    for (auto e : exponents)
      if (e >= shift)
        assert (safe_add(sum, _exponent_to_rounded(e), sum));

    assert (sum >= exponents.size()/epsilon);
    sanity_check();

  } else {

    if (n_remaining == 0) {
      sum = 0;
      shift = 0;
      n_remaining = 0;
      return;
    }

    // For error when eps < 0, 
    // suffices if (1-eps)**shift * m <= eps * floor((1-eps)** (min e)).
    // i.e. shift + log(m/eps)/log(1-eps) >= min e.
    // i.e. shift >= (min e) + log(m/eps)/-log(1-eps).

    // For no sum overflow when eps < 0,
    // suffices if m*(1-eps)**(min e - shift) <= rounded_t_max.
    // i.e. (1-eps)**(min e - shift) <= rounded_t_max/m.
    // i.e. log(1-eps)*(min e - shift) <= log(rounded_t_max/m).
    // i.e. -log(1-eps)*(shift - min e) <= log(rounded_t_max/m).
    // i.e. shift - min e <= log(rounded_t_max/m)/-log(1-eps).
    // i.e. shift <= min e + floor(log(rounded_t_max/m)/-log(1-eps)).

    // For no too-small shifted exponent overflow when eps < 0,
    // suffices if min e - shift >= shifted_exponent_limit.
    // i.e. min e - shift >= -log(rounded_t_max) /-log(1-eps).
    // i.e. shift <= min e + log(rounded_t_max) /-log(1-eps).

    exponent_t min_e = min(exponents);
    long double tmp = std::floor(std::log((long double) rounded_t_max/exponents.size())
                                 / -std::log1p((long double) epsilon));
    exponent_t tmp_e = tmp;
    assert ( tmp = (long double) tmp_e);

    assert ( safe_add(min_e, tmp_e, shift) );

    sum = 0;
    for (auto e : exponents)
      if (e <= shift)
        assert (safe_add(sum, _exponent_to_rounded(e), sum));

    assert (sum >= exponents.size()/-epsilon);
    sanity_check();
  }
}

void 
approx_sum_t::sanity_check() {

  if (n_remaining == 0) {
    assert (sum == 0);
    assert (shift == 0);
    return;
  }

  // error bound

  // assert err <= eps
  assert (sum >= exponents.size() / std::abs(epsilon));

  // for debugging
  assert (upper_bound() <= (1+std::abs(epsilon))*upper_bound());

  // check sum
  long double check = 0;
  rounded_t check_rounded = 0;
  
  for (auto e : exponents) {
    if (epsilon > 0) {
      if (e < shift) continue;
      exponent_t shifted;
      assert (safe_subtract(e, shift, shifted));
      assert (shifted <= shifted_exponent_limit);
    } else {
      if (e > shift) continue;
      exponent_t shifted;
      assert (safe_subtract(e, shift, shifted));
      assert (shifted >= shifted_exponent_limit);
    }

    auto power = std::pow((long double)(1.0+epsilon), (long double)(e - shift));
    assert (power <= (long double)(rounded_t_max));
    assert (power >= 0);

    rounded_t power_rounded = std::floor(power);
    assert (std::floor(power) == (long double)(power_rounded));
    assert (power_rounded == _exponent_to_rounded(e));

    check += power;
    assert (safe_add(check_rounded, power_rounded, check_rounded));
  }

  assert (sum == check_rounded);

  // check relative error
  long double lb = sum;
  long double ub = sum + exponents.size();

  assert (lb <= check);
  assert (check <= ub);

  auto err1 = std::abs(ub/lb - 1.0);
  // auto err2 = std::abs(upper_bound()/lower_bound() - 1.0);

  // err = ub/lb - 1
  //   = (sum + m)/sum - 1
  //   = m/sum
  // want <= |eps|
                   
  ASSERT(err1 <= std::abs(epsilon), // && err2 <= std::abs(epsilon),
         _(lb)
         _(ub - lb)
         _(epsilon)
         _(err1)
         _(lower_bound())
         _(upper_bound() - lower_bound())
         // _(err2)
         );
}

scalar_t approx_sum_t::times(approx_sum_t & other) {
  assert (other.epsilon * epsilon < 0);
  // (1+other.epsilon)**other.shift * other.sum
  //   * (1+epsilon)**shift * sum
  return
    ((std::pow((1+other.epsilon)*(1+epsilon), shift)
      * std::pow(1+other.epsilon, other.shift-shift))
     * scalar_t(sum))
    * scalar_t(other.sum);
}
scalar_t approx_sum_t::divided_by(approx_sum_t & other) {
  // assert (other.epsilon * epsilon > 0);
  // (1+epsilon)**shift * sum
  //   / (1+other.epsilon)**other.shift * other.sum
  return
    (std::pow((1+epsilon)/(1+other.epsilon), shift)
     / std::pow(1+other.epsilon, other.shift - shift))
    * (scalar_t(sum) / scalar_t(other.sum));
}
