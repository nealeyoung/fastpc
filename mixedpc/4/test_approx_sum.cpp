//  -*- mode: c++; flycheck-clang-language-standard: "c++11"; -*-

#include <cmath>
#include <iostream>

#include "approx_sum.h"

int main() {
  size_t n = 1000;
  double delta = 0.001;

  std::cout << "epsilon = %" << 100*delta << std::endl;

  approx_sum_t p(delta);
  long double sum = 0;
  
  for (exponent_t i = 0;  i < n;  ++i) {
    p.set_exponent(i, i);
    sum += std::exp((long double) i);
  }
  std::cout << std::log(sum) << std::endl;
  std::cout << p.log_lb() << std::endl;
  std::cout << "additive err = " << std::log(sum) - p.log_lb() << std::endl;

  for (exponent_t i = 3;  i < n-1;  ++i) {
    p.zero_term(i);
  }
  p.zero_term(n-1);
  auto e = std::exp(1);
  auto ans = std::log(1 + e + e*e);
  std::cout << "additive err = " << ans - p.log_lb() << std::endl;
}
