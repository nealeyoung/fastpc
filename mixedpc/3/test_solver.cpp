//  -*- mode: c++; flycheck-clang-language-standard: "c++11"; -*-

#include <tuple>
#include <algorithm> 
#include <cstdio>
#include <iostream>
#include <cassert>
#include <random>

#include "solver.h"
#include "utilities.h"

typedef double scalar_t;

void
random_P_C_x(int _n, 
             int _m_p,
             int _m_c,
             // outputs:
             sparse_matrix_t<double> & P, 
             sparse_matrix_t<double> & C, 
             std::vector<double> & x) {
  assert(_n > 0 && _m_p >= 0 && _m_c > 0);
  size_t n = _n;
  size_t m_p = _m_p;
  size_t m_c = _m_c;

  x.resize(n);

  std::default_random_engine generator;
  std::uniform_real_distribution<scalar_t> distribution(0.0, 1.0);

  auto random_01 = [&]() {
    return distribution(generator);
  };

  scalar_t sum = 0;
  while (sum == 0)
    for (size_t j = 0;  j < n;  ++j) {
      x[j] = random_01() < 0.2 ? random_01() : 0;
      sum += x[j];
    }

  auto push_row = [&](size_t row, 
                      sparse_matrix_t<double> & M,
                      scalar_t scale) {
    std::vector<double> v(n, 0);
    scalar_t row_sum = 0;
    while (row_sum == 0) {
      for (size_t j = 0;  j < n;  ++j)
        if (random_01() < 0.5) {
          v[j] = random_01();
          row_sum += v[j]*x[j];
        }
    };
    for (size_t j = 0;  j < n;  ++j)
      if (v[j] > 0)
        M.push_back(row, j, scale*v[j]/row_sum);
  };

  for (size_t i = 0;  i < m_p;  ++i)
    push_row(i, P, 0.999);

  push_row(0, C, 1001);
  for (size_t i = 1;  i < m_c;  ++i)
    push_row(i, C, 1.001);

  C.done_adding();
  P.done_adding();

  assert(min(C*x) >= 1.0);
  assert(max(P*x) <= 1.0);
}

int main(int argc, char* argv[]) {
  size_t n = 15;
  scalar_t epsilon = 0.03;

  if (argc > 1)
    epsilon = atof(argv[1]);

  if (argc > 2)
    n = atoi(argv[2]);
  
  sparse_matrix_t<double> P;
  sparse_matrix_t<double> C;
  std::vector<double> x0;

  random_P_C_x(n, 2*n, 2*n, P, C, x0);

  bool feasible;
  std::vector<double> x;

  solve(P, C, epsilon, feasible, x);
  if (feasible) {
    std::printf("eff eps: %.3f\n", double(max(P*x)/min(C*x) - 1));
    // std::printf("y: %s\n", str_vector(y));
    // std::printf("x: %s\n", str_vector(x));
  } else {
    std::cout << "solve returned infeasible" << std::endl;

    if (max(P*x0) <= 1.0 && min(C*x0) >= 1.0) {
      std::cout << "error: solve returned infeasible for feasible instance" << std::endl;
    } else {
      std::cout << "warning: generated instance not feasible" << std::endl;
    }
  }
}
