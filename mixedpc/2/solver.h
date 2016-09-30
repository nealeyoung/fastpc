//  -*- mode: c++; flycheck-clang-language-standard: "c++11"; -*-

#ifndef SOLVER_H
#define SOLVER_H

#include "sparse_matrix.h"

void solve(sparse_matrix_t<double> & P,
           sparse_matrix_t<double> & C,
           double epsilon,
           // outputs:
           bool & feasible,
           std::vector<double> & solution);

#endif
