//  -*- mode: c++; flycheck-clang-language-standard: "c++11"; -*-

#include <cmath>
#include <assert.h>
#include <iostream>
#include <cstdio> // printf

#include "utilities.h"
#include "approx_sum.h"
#include "solver_matrix.h"
#include "solver.h"

typedef double scalar_t;

void solve(sparse_matrix_t<double> & P0, // const
           sparse_matrix_t<double> & C0, // const
           double epsilon_given,
           // outputs:
           bool & feasible,
           std::vector<double> & solution) {

  for (auto e: P0) assert(e->value() >= 0);
  for (auto e: C0) assert(e->value() >= 0);

  auto n_p = P0.n_cols();
  auto n_c = C0.n_cols();
  auto n = std::max(n_p, n_c);

  auto m_p = P0.n_rows();
  auto m_c = C0.n_rows();
  auto m = m_p + m_c;
  
  // check trivial feasibility or infeasibility
  {
    if (m_c == 0) {
      feasible = true;

      std::vector<double> x(n, 0.0);
      solution = x;
      return;
    }

    std::vector<double> y(n, 1.0);
    auto Cy = C0 * y;
    auto min_Cy = min(Cy);
    if (min_Cy == 0.0) {
      feasible = false;
      return;
    }
    auto Py = P0 * y;
    if (m_p == 0 or max(Py) <= min_Cy) {
      feasible = true;

      scale_vector_by(y, 1.0/min_Cy);
      solution = y;
      return;
    }
  }
  assert (n >= 1);
  assert (m_p >= 1);
  assert (m_c >= 1);

  double delta = epsilon_given/100;
  // (1+eps)(1+delta)/(1-delta)^2 = 1+epsilon
  // eps = (1+epsilon)*(1-delta)^2/(1+delta) - 1
  // scalar_t eps = (1+epsilon)*(1-delta)*(1-delta)/(1+delta) - 1;
  double eps = (1+epsilon_given)*(1-delta)/(1+delta) - 1;

  double U = 2.0*std::log(double(m))/(eps*eps);

  // create normalized P, C

  std::vector<double> x_normalizer(n, 0.0);
  {
    auto fn = m <= n
      ? [](double a, double b) { return a + b; }
      : [](double a, double b) { return std::max(a, b); };

    for (auto e: P0)
      x_normalizer.at(e->col)
        = fn (x_normalizer.at(e->col), e->value());

    for (size_t j = 0;  j < n;  ++j)
      if (x_normalizer.at(j) == 0.0)
        x_normalizer.at(j) = 1.0;
  }
    
  auto denormalize = [&](std::vector<double>& x) {
    std::vector<double> y(x);
    for (size_t j = 0;  j < n;  ++j) {
      y.at(j) /= x_normalizer.at(j) == 0 ? 1.0 : x_normalizer.at(j);
      y.at(j) /= 1-delta;
    }
    return y;
  };

  std::vector<double>   x(n, 0); 

  solver_matrix_t P("P", m_p, n,  eps, x, P0, 
                    x_normalizer, delta, std::min(m, n));

  solver_matrix_t C("C", m_c, n, -eps, x, C0, 
                    x_normalizer, delta, std::min(m, n)*std::min(m, n)/delta);

  auto problem_size = P.M.entries.size() + C.M.entries.size();

  auto potential = [&]() { 
    return double (P.y.lower_bound() * C.y.lower_bound() / (m_p * m_c)); 
  };

  // double factor = (1+eps)*(1+eps) / (1-eps);
  double factor = (1+eps) / (1-eps);
  // double factor = 1 / (1-eps);

  struct {
    size_t outer_iterations = 0;
    size_t inner_steps = 0;
    size_t wasted_steps = 0;
  } stats;

  auto check_done = [&]() {
    // return early if (1+eps)-feasible

    auto min_Cx = min(C.M_x);
    auto max_Px = max(P.M_x);
    
    double eff_eps = min_Cx > 0 ? max_Px/min_Cx-1 : 1;
    
    // occasional report
    std::cout << "## "
    _(stats.outer_iterations)
    _(eff_eps)
    _(potential())
    _(min_Cx)
    _(max_Px)
    // __ << "## "
    // _(c_hat_sum.lower_bound())
    // // _(c_hat_value)
    // _(c_hat_sum.upper_bound())
    // _(p_hat_sum.lower_bound())
    // // _(p_hat_value)
    // _(p_hat_sum.upper_bound())
    __;
    
    P.sanity_check();
    C.sanity_check();

    P.reset_M_x();
    C.reset_M_x();

    // check infeasibility

    bool done = false;
    bool infeasible = true;

    for (size_t j = 0;  j < n;  ++j)
      if (P.y_M.at(j) / P.y <= C.y_M.at(j) / C.y) {
        infeasible = false;
        break;
      }
    
    if (infeasible) {
      done = true;
      feasible = false;
    } else if (C.M.n_remaining_rows == 0  ||  eff_eps <= eps) {
      done = true;
      feasible = true;
    } 

    if (done) {
      // final report
      std::cout << "## final report "
        _(stats.outer_iterations)__
        _(stats.inner_steps)__
        _(stats.wasted_steps)__
        _(stats.wasted_steps/(1+stats.inner_steps))__
        _(C.y.lower_bound())__
        _(P.y.lower_bound())__
        _(potential())
        __;
    }
    return done;
  };

  size_t inner_steps_last_check = 0;
  size_t inner_steps_last_outer = 0;

  auto time_to_check = [&]() {
    return C.M.n_remaining_rows == 0  
    || stats.inner_steps-inner_steps_last_check > problem_size;
  };

  while(true) {
    stats.outer_iterations += 1;
    inner_steps_last_outer = stats.inner_steps;
    
    if (stats.outer_iterations % 100 == 0)
      std::cout
        _(stats.outer_iterations)
        _(stats.inner_steps)
        __;

    for (size_t j = 0;  j < n && ! time_to_check();  ++j)  {

      auto & P_j = P.M.col(j);
      auto & C_j = C.M.col(j);

      for (auto e : P_j)  P.update_y_M_term(j, e);
      for (auto e : C_j)  C.update_y_M_term(j, e);

      stats.wasted_steps += P_j.size() + C_j.size();
      
      auto & x_j = x.at(j);

      for (auto e: P_j) e->data.last_xj = x_j;
      for (auto e: C_j) e->data.last_xj = x_j;

      size_t inner_loops = 0;

      for (inner_loops = 0;  
           P.y_M.at(j) / P.y < factor * (C.y_M.at(j) / C.y)
             && ! time_to_check();
           ++inner_loops)  {

        double M = P_j.max()->value();
        if (! C_j.empty())   M = std::max(M, C_j.max()->value());
        
        double dxj = 1 / M;

        x_j += dxj;

        stats.inner_steps += P.update_M_x(j);
        stats.inner_steps += C.update_M_x(j, true, U);

        // debug
        // P.reset_M_x();
        // C.sanity_check();
        C.reset_M_x();
      }
      if (inner_loops) {
        stats.inner_steps += P.update_M_x(j, true);
        stats.inner_steps += C.update_M_x(j, true, U);
      }
    } // end for j

    if (time_to_check()
        or stats.inner_steps == inner_steps_last_outer) {
      inner_steps_last_check = stats.inner_steps;

      if (check_done()) {
        if (feasible)
          solution = denormalize(x);
        return;
      }
    }
  } // end while true
}

