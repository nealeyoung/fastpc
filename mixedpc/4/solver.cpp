//  -*- mode: c++; flycheck-clang-language-standard: "c++11"; -*-

#include <cmath>
#include <assert.h>
#include <iostream>
#include <cstdio> // printf

#include "utilities.h"
#include "solver_matrix.h"
#include "solver.h"

void solve(sparse_matrix_t<double> & P0, // const
           sparse_matrix_t<double> & C0, // const
           double epsilon_given,
           // outputs:
           bool & feasible,
           std::vector<double> & primal_solution,
           std::vector<double> & dual_solution_P,
           std::vector<double> & dual_solution_C) {

#ifndef NDEBUG
  for (auto e: P0) assert(e->value() >= 0);
  for (auto e: C0) assert(e->value() >= 0);
#endif

  primal_solution.clear();
  dual_solution_P.clear();
  dual_solution_C.clear();

  auto n_p = P0.n_cols();
  auto m_p = P0.n_rows();

  auto n_c = C0.n_cols();
  auto m_c = C0.n_rows();

  auto n   = std::max(n_p, n_c);
  auto m   = m_p + m_c;
  
  // check trivial feasibility or infeasibility
  {
    if (m_c == 0) {
      feasible = true;
      std::vector<double>(n, 0).swap(primal_solution);
      return;
    }

    std::vector<double> y(n, 1.0);
    auto Cy = C0 * y;
    auto min_Cy = min(Cy);

    if (min_Cy == 0.0) {
      // C0 is all zeros
      feasible = false;

      std::vector<double> z(m_p, 1);

      if (min(P0.transpose_times_vector(z)) == 0) return;  // P has an all-zero column

      std::vector<double>(m_c, 1).swap(dual_solution_C);
      std::vector<double>(m_p, 1).swap(dual_solution_P);
      
      return;
    }

    auto Py = P0 * y;
    auto max_Py = max(Py);

    if (m_p == 0 or max_Py <= min_Cy) {
      feasible = true;
      std::vector<double>(n, 1.0/min_Cy).swap(primal_solution);
      return;
    }
  }
  assert (n >= 1);
  assert (m_p >= 1);
  assert (m_c >= 1);

  double delta = epsilon_given/20;
  // (1+eps)(1+delta)/(1-delta)^2 = 1+epsilon
  double eps; 
  {
    double r = (1-delta)/(1+delta);
    eps = (1+epsilon_given)*(1-delta)*r - 1;
  }

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
    
  auto denormalize_primal = [&](std::vector<long double>& x) {
    std::vector<double> y(n);
    for (size_t j = 0;  j < n;  ++j) {
      y.at(j) = x.at(j)/(x_normalizer.at(j) == 0 ? 1.0 : x_normalizer.at(j));
      y.at(j) /= 1-delta;
    }
    return y;
  };
  
  // dual solution is (y,z) such that
  // yP_j/|y| >  zC_j/|z|    (for all j) (@)
  // Then, Px <= 1 <= Cx 
  // would imply yP/|y| . x <= (zC/|z|) . x, which contradicts (@)
  
  // x_normalizer transforms given P0 into P where P_ij = P0_ij / x_normalizer_j
  // x_normalizer transforms given C0 into C where C_ij = C0_ij / x_normalizer_j
  
  // given dual solution (y,z) for P, C
  // is also dual solution for P0, C0:
  // y P0_j /|y| = y P_j x_normalizer_j / |y|
  // < y C_j x_normalizer_j / |y|
  // = y C0_j / |y|

  std::vector<long double> x(n, 0); 

  solver_matrix_t  P("P", m_p, n,  eps, x, P0, x_normalizer, delta, 
                     std::min(m, n));

  assert (P.M.n_rows() <= m_p);

  solver_matrix_t  C("C", m_c, n, -eps, x, C0, x_normalizer, delta, 
                     std::min(m, n)*std::min(m, n)/delta, U);

  assert (C.M.n_rows() <= m_c);

  size_t           problem_size = P.M.entries.size() + C.M.entries.size();

  auto lmin_lb = [&]() { return C.y.log_lb()/std::log1p(-eps); };
  auto lmax_lb = [&]() { return P.y.log_lb()/std::log1p( eps); };
#ifndef NDEBUG
  auto lmax_ub = [&]() { return P.y.log_ub()/std::log1p( eps); };
#endif
  // auto lmin_ub = [&]() { return C.y.log_ub()/std::log1p(-eps); };

  // double factor = (1+eps)*(1+eps) / (1-eps);
  double factor = (1+eps) / (1-eps);
  //double log_factor = std::log1p(eps); // - std::log1p(-eps);

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
    
#ifndef NDEBUG
    // occasional report
    std::cout << "## "
    _(stats.outer_iterations)
    _(stats.inner_steps)
    _(eff_eps)
    _(lmin_lb())
    _(min_Cx)
    _(max_Px)
    _(lmax_ub())
    _(lmin_lb()/lmax_ub())
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
#endif

    // std::cout << "RESET" << std::endl;

    // P.rebuild();
    // C.rebuild();

    // check infeasibility

    bool done = false;
    bool infeasible = true;

    for (size_t j = 0;  j < n;  ++j)
      if (P.y_M.at(j).log_lb() - P.y.log_ub() <= C.y_M.at(j).log_ub() - C.y.log_lb()) {
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

#ifndef NDEBUG
    if (done) {
      // final report
      std::cout <<
      "### final report "
      __
      _(stats.outer_iterations)
      _(stats.inner_steps)
      _(stats.wasted_steps)
      _(stats.wasted_steps/(1.0+stats.inner_steps))__;
    }
#endif
    return done;
  };

  size_t inner_steps_last_check = 0;
  size_t inner_steps_last_outer = 0;

  auto time_to_check = [&]() {
    return C.M.n_remaining_rows == 0  
    or stats.inner_steps-inner_steps_last_check > 2*problem_size;
  };

  while(true) {
    stats.outer_iterations += 1;
    inner_steps_last_outer = stats.inner_steps;
    
#ifndef NDEBUG
    if (stats.outer_iterations % 100 == 0)
      std::cout
        _(stats.outer_iterations)
        _(stats.inner_steps)
        __;
#endif

    for (size_t j = 0;  j < n and not time_to_check();  ++j)  {

      auto & x_j = x.at(j);
      auto & P_j = P.M.col(j);
      auto & C_j = C.M.col(j);

#ifndef NDEBUG
      for (auto e : P_j) assert ( e->data.last_xj == x_j );
      for (auto e : C_j) assert ( e->data.last_xj == x_j );
#endif

      stats.wasted_steps += P.update_for_x_j(j, true);
      stats.wasted_steps += C.update_for_x_j(j, true);

      // debug
      // P.check_col_j_equality(j);
      // C.check_col_j_equality(j);

      size_t inner_loops = 0;

      double P_ratio = std::exp(P.y_M.at(j).log_lb() - P.y.log_lb());
      double C_ratio = std::exp(C.y_M.at(j).log_lb() - C.y.log_lb());

      auto lmin_lb0 = lmin_lb();
      auto lmax_lb0 = lmax_lb();

      double d_lmax_UB = 0;
      double d_lmin_LB = 0;

      for (inner_loops = 0;  

           P_ratio <= C_ratio * factor
             and not time_to_check();

           ++inner_loops)  {

        assert (P_ratio > 0);
        assert (C_ratio > 0);

        double M = P_j.max()->value();

        if (not C_j.empty())   M = std::max(M, C_j.max()->value());
        
        double dxj = 1 / M;

        assert (dxj > 0);

#ifndef NDEBUG
        auto before = x_j;
#endif

        x_j += dxj;

        // rounding errors?
        assert (x_j - before >= dxj*0.99);

        // debug

        assert (d_lmax_UB >= 0);

        d_lmax_UB +=  P_ratio*dxj*(eps/std::log1p( eps));
        d_lmin_LB += -C_ratio*dxj*(eps/std::log1p(-eps));

        
        ASSERT (d_lmax_UB > 0,
                _(d_lmax_UB)
                _(P_ratio)
                _(dxj)
                _(eps/std::log1p(eps))
                _(P_ratio * dxj * (eps/std::log1p(eps)))
                );

        stats.inner_steps += P.update_for_x_j(j);
        stats.inner_steps += C.update_for_x_j(j);

        // debug
        // stats.inner_steps += P.update_for_x_j(j, true);
        // stats.inner_steps += C.update_for_x_j(j, true);

        // debug
        // P.reset();
        // P.sanity_check();
        // C.sanity_check();
        // C.reset();
        
        P_ratio = std::exp(P.y_M.at(j).log_lb() - P.y.log_lb());
        C_ratio = std::exp(C.y_M.at(j).log_lb() - C.y.log_lb());
      }
      if (inner_loops) {
        stats.inner_steps += P.update_for_x_j(j, true);
        stats.inner_steps += C.update_for_x_j(j, true);

        // debug
        auto d_lmin = lmin_lb() - lmin_lb0;
        auto d_lmax = lmax_lb() - lmax_lb0;

        if (d_lmin < d_lmin_LB or d_lmax > d_lmax_UB) {
          std::cout
            _(j)
            _(d_lmax_UB)
            _(d_lmax)
            _(d_lmax/d_lmax_UB)
            _(d_lmin_LB)
            _(d_lmin)
            _(d_lmin/d_lmin_LB)
            __
            ;
        }
      }
      // debug
      // P.sanity_check();
      // C.sanity_check();
      // P.check_col_j_equality(j);
      // C.check_col_j_equality(j);
    } // end for j

    if (time_to_check()
        or stats.inner_steps == inner_steps_last_outer) {
      inner_steps_last_check = stats.inner_steps;

      if (check_done()) {
        if (feasible) {

          // scale_vector_by(x, 1/(min(C.M*x)));
          primal_solution = denormalize_primal(x);
          std::vector<double> y(n);
          for (size_t j = 0;  j < n;  ++j) y[j] = x[j];

#ifndef NDEBUG
          std::cout
            _(max(P.M_x))
            _(max(P.M*y))
            _(min(C.M_x))
            _(min(C.M*y))
            _(max(P.M*y)/min(C.M*y) - 1.0)
            _(max(P0*primal_solution))
            _(min(C0*primal_solution))
            _(max(P0*primal_solution)/min(C0*primal_solution) - 1.0)
            __;
#endif
        } else { // not feasible
          dual_solution_P.resize(m_p);
          for (size_t i = 0;  i < m_p;  ++i)
            dual_solution_P.at(i) = std::exp(P.y.exponents.at(i) - P.y.log_lb());
          
          dual_solution_C.resize(m_c);
          for (size_t i = 0;  i < m_c;  ++i)
            dual_solution_C.at(i) = std::exp(C.y.exponents.at(i) - C.y.log_lb());
        }
        return;
      }
    }
  } // end while true
}
