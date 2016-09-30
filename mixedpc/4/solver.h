//  -*- mode: c++; flycheck-clang-language-standard: "c++11"; -*-

#ifndef SOLVER_H
#define SOLVER_H

#include "sparse_matrix.h"

void solve(sparse_matrix_t<double> & P,
           sparse_matrix_t<double> & C,
           double epsilon,
           // outputs:
           bool & feasible,
           std::vector<double> & primal_solution,
           std::vector<double> & dual_solution_P,
           std::vector<double> & dual_solution_C);

// outputs: either
// 1. feasible = true and x = primal_solution satisfies C x >= 1, P x <= 1 + epsilon, or
// 2. feasible = false, and (y,z) = (dual_solution_P, dual_solution_C) 
//    satisfies y C_j / |y| < z P_j / |z| (for all columns j), or
// 3. feasible = false and C is all-zeros and P has an all-zero column

#endif
