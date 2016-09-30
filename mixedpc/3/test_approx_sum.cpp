//  -*- mode: c++; flycheck-clang-language-standard: "c++11"; -*-

#include <cmath>
#include <iostream>

#include "approx_sum.h"

typedef approx_sum_t::exponent_t exponent_t;
typedef approx_sum_t::scalar_t scalar_t;

void output(const std::string & msg, scalar_t a, scalar_t b) {
  std::cout << msg << ": "
            << a << " - " << b << " = " << a-b 
            << " = %" << 100*std::abs(a/b - 1) << std::endl;
}

int main() {
  exponent_t n = 50;
  double epsilon = 0.01;

  std::cout << "epsilon = %" << 100*epsilon << std::endl;

  approx_sum_t p(n, 0, epsilon);

  scalar_t sum1 = 0;
  for (exponent_t i = 0;  i < n;  ++i) {
    p.raise_exponent(i, i);
    sum1 += pow(1+epsilon, i);
  }
  for (exponent_t i = 0;  i < n/2;  ++i) {
    p.raise_exponent(i, n-i);
    sum1 += pow(1+epsilon, n-i) - pow(1+epsilon, i);
  }
  scalar_t sum1a = p.sum * pow(1+epsilon, p.shift);
  output("sum1", sum1, sum1a);
  
  approx_sum_t m(n, 0, -epsilon);
  scalar_t sum2 = 0;
  for (exponent_t i = 0;  i < n;  ++i) {
    m.raise_exponent(i, i);
    sum2 += pow(1-epsilon, i);
  }
  for (exponent_t i = 0;  i < n/2;  ++i) {
    m.raise_exponent(i, n-i);
    sum2 += pow(1-epsilon, n-i) - pow(1-epsilon, i);
  }
  scalar_t sum2a = m.sum * pow(1-epsilon, m.shift);
  output("sum2", sum2, sum2a);

  scalar_t prod = p * m;
  scalar_t proda = sum1 * sum2;
  output("prod", prod, proda);

  scalar_t ratio = p / m;
  scalar_t ratioa = sum1 / sum2;
  output("ratio", ratio, ratioa);
}
