//  -*- mode: c++; flycheck-clang-language-standard: "c++11"; -*-

#include <vector>
#include "matrix.h"

// set feasible and, if feasible, set solution
void solve(Matrix & P, Matrix & C, double eps,
           bool & feasible, std::vector<double> & solution);
