//  -*- mode: c++; flycheck-clang-language-standard: "c++11"; -*-

#include <cstdint>
#include <cmath>
#include <assert.h>
#include <iostream>
#include <limits>

#include "approx_sum.h"

typedef approx_sum_t::exponent_t exponent_t;
typedef approx_sum_t::rounded_t rounded_t;
typedef approx_sum_t::scalar_t scalar_t;

static const exponent_t exponent_t_max = std::numeric_limits<exponent_t>::max();
static const exponent_t exponent_t_min = std::numeric_limits<exponent_t>::min();
static const rounded_t  rounded_t_max  = std::numeric_limits<rounded_t>::max();
static const rounded_t  rounded_t_min  = std::numeric_limits<rounded_t>::min();

static_assert (-(exponent_t_min+1) <= exponent_t_max, "-exponent_t_min too large");
static_assert (-(exponent_t_max-1) >= exponent_t_min, "-exponent_t_max too small");

approx_sum_t::approx_sum_t(size_t size, scalar_t epsilon) 
  : epsilon(epsilon),
    exponents(size)
{
  assert (epsilon > -1.0 && epsilon != 0.0);
  assert (size > 0);

#ifdef DEBUG_APPROX_SUM
  debug_sum = 0;
#endif

  if (epsilon > 0) {
    sum = 0;
    for (size_t k = 0;  k < size;  ++k) exponents[k] = exponent_t_min;
    most_significant_exponent = exponent_t_min;
    shift = exponent_t_min + 1;

    // (1+eps)**shifted_exponent_limit <= rounded_t_max / size
    shifted_exponent_limit = floor(log(scalar_t(rounded_t_max) / scalar_t(size))/log1p(epsilon));

  } else {
    sum = 0;
    for (size_t k = 0;  k < size;  ++k) exponents[k] = exponent_t_max;
    most_significant_exponent = exponent_t_max;
    shift = exponent_t_max - 1;

    // (1+eps)**shifted_exponent_limit <= rounded_t_max / size  (eps < 0)
    shifted_exponent_limit = ceil(log(scalar_t(rounded_t_max) / scalar_t(size))/log1p(epsilon));
  }
#ifdef DEBUG_APPROX_SUM
  debug();
#endif
}

void approx_sum_t::raise_exponent(size_t k, exponent_t exponent) {
  assert (epsilon > 0);
  assert (k < exponents.size());
  assert (exponent >= exponents[k]);
  if (exponent == exponents[k]) return;

  if (exponents[k] >= shift)
    sum -= _exponent_to_rounded(exponents[k] - shift);
    
#ifdef DEBUG_APPROX_SUM
  debug_sum -= pow(1+epsilon, exponents[k]);
#endif
  exponents[k] = exponent;

#ifdef DEBUG_APPROX_SUM
  debug_sum += pow(1+epsilon, exponents[k]);
#endif

  if (exponents[k] >= shift)
    sum += _exponent_to_rounded(exponents[k] - shift);

  if (exponent > most_significant_exponent)
    most_significant_exponent = exponent;

  if (most_significant_exponent - shift > shifted_exponent_limit) {
    // m (1+epsilon)**shift <= epsilon (1+epsilon)**most_significant_exponent
    // (1+epsilon)**(most_significant_exponent-shift) >= m/epsilon
    // most_significant_exponent - shift >= log_{1+epsilon} (m/epsilon)
    // shift <= most_significant_exponent - log_{1+epsilon} (m/epsilon)
    shift = floor(most_significant_exponent - log(exponents.size()/epsilon)/log1p(epsilon));
    assert (most_significant_exponent - shift < shifted_exponent_limit);
      
    std::cout << "SHIFT INCREASE" << std::endl;

    sum = 0;
    for (auto exp : exponents) 
      if (exp >= shift)
        sum += _exponent_to_rounded(exp - shift);
  }
#ifdef DEBUG_APPROX_SUM
  debug();
#endif
}

void approx_sum_t::lower_exponent(size_t k, exponent_t exponent) {
  assert (epsilon < 0);
  assert (k < exponents.size());
  assert (exponent <= exponents[k]);
  if (exponent == exponents[k]) return;

  if (exponents[k] <= shift)
    sum -= _exponent_to_rounded(exponents[k] - shift);

#ifdef DEBUG_APPROX_SUM
  debug_sum -= pow(1+epsilon, exponents[k]);
#endif
  exponents[k] = exponent;

#ifdef DEBUG_APPROX_SUM
  debug_sum += pow(1+epsilon, exponents[k]);
#endif

  if (exponents[k] <= shift)
    sum += _exponent_to_rounded(exponents[k] - shift);

  if (exponent < most_significant_exponent)
    most_significant_exponent = exponent;

  if (most_significant_exponent - shift < shifted_exponent_limit) {
    std::cout << "SHIFT DECREASE" << std::endl;

    // m (1-epsilon)**shift <= epsilon (1-epsilon)**most_significant_exponent
    // (1-epsilon)**(most_significant_exponent-shift) >= m/epsilon
    // most_significant_exponent - shift <= log_{1-epsilon} (m/epsilon)
    // shift >= most_significant_exponent - log_{1-epsilon} (m/epsilon)
    shift = ceil(most_significant_exponent - log(exponents.size()/-epsilon)/log1p(epsilon));
    assert (most_significant_exponent - shift > shifted_exponent_limit);
      
    sum = 0;
    for (auto exp: exponents) 
      if (exp <= shift)
        sum += _exponent_to_rounded(exp - shift);
  }
#ifdef DEBUG_APPROX_SUM
  debug();
#endif
}

void approx_sum_t::remove_exponent(size_t k) {
  assert (epsilon < 0);
  if (exponents[k] <= shift)
    sum -= _exponent_to_rounded(exponents[k] - shift);
  exponents[k] = exponent_t_max;
}

approx_sum_t::exponent_t
approx_sum_t::_scalar_to_exponent(scalar_t epsilon, scalar_t scalar) {
  return round(log(scalar)/log1p(epsilon));
}

approx_sum_t::rounded_t
approx_sum_t::_exponent_to_rounded(scalar_t epsilon, exponent_t exponent) {
  assert (epsilon*exponent >= 0);
  return floor(pow(1+epsilon, exponent));
}

scalar_t approx_sum_t::times(approx_sum_t & other) {
  assert (other.epsilon * epsilon < 0);
  // (1+other.epsilon)**other.shift * other.sum
  //   * (1+epsilon)**shift * sum
  return
    ((pow((1+other.epsilon)*(1+epsilon), shift)
      * pow(1+other.epsilon, other.shift-shift))
     * scalar_t(sum))
    * scalar_t(other.sum);
}
scalar_t approx_sum_t::divided_by(approx_sum_t & other) {
  // assert (other.epsilon * epsilon > 0);
  // (1+epsilon)**shift * sum
  //   / (1+other.epsilon)**other.shift * other.sum
  return
    (pow((1+epsilon)/(1+other.epsilon), shift)
     / pow(1+other.epsilon, other.shift - shift))
    * (scalar_t(sum) / scalar_t(other.sum));
}

#ifdef DEBUG_APPROX_SUM
void 
approx_sum_t::debug() {
  scalar_t sum1 = sum * pow(1+epsilon, shift);
  if ((sum1 == 0 && debug_sum == 0) || abs(sum1/debug_sum - 1.0) <= abs(epsilon)) return;

  std::cout << "DEBUG: sum = " << sum
            << ", sum1 = " << sum1 
            << ", debug_sum = " << debug_sum 
            << ", err = " << abs(sum1/debug_sum - 1.0)
            << ", shift = " << shift 
            << ", (1+epsilon)**shift = " << pow(1+epsilon, shift)
            << ", (1+epsilon)**(-shift) = " << pow(1+epsilon, -shift)
            << std::endl;
  for (size_t k = 0;  k < exponents.size();  ++k) {
    std::cout << k << ": " 
              << exponents[k] << ", "
              << pow(1+epsilon, exponents[k] - shift) << ", "
              << pow(1+epsilon, exponents[k])
              << std::endl;
  }
  assert(! "debug");
}
#endif

