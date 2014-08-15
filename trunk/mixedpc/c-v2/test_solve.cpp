//  -*- mode: c++; flycheck-clang-language-standard: "c++11"; -*-

#include <tuple>
#include <algorithm> 
#include <cstdio>
#include <iostream>
#include <cassert>

#include "solve.h"

int main(int argc, char* argv[]) {
  size_t n = 15;
  double epsilon = 0.03;

  if (argc > 1)
    epsilon = atof(argv[1]);

  if (argc > 2)
    n = atoi(argv[2]);
  
  Matrix P;
  Matrix C;
  std::vector<double> y;
  random_P_C_x(n, P, C, y);
  bool feasible;
  std::vector<double> x;
  solve(P, C, epsilon, feasible, x);
  if (feasible) {
    auto Cx = C.times_vector(x);
    auto Px = P.times_vector(x);

    double minCx = *std::min_element(Cx.begin(), Cx.end());
    double maxPx = *std::max_element(Px.begin(), Px.end());

    std::printf("eff eps: %.3f\n", (maxPx/minCx - 1));
    // std::printf("y: %s\n", str_vector(y));
    // std::printf("x: %s\n", str_vector(x));
  } else {
    std::cout << "solve returned infeasible" << std::endl;

    auto Cy = C.times_vector(y);
    auto Py = P.times_vector(y);
    double minCy = *std::min_element(Cy.begin(), Cy.end());
    double maxPy = *std::max_element(Py.begin(), Py.end());

    if (0 < maxPy  &&  maxPy <= minCy) {
      std::cout << "error: solve returned infeasible for feasible instance" << std::endl;
    } else {
      std::cout << "warning: generator may have generated infeasible instance" << std::endl;
    }
  }
}
