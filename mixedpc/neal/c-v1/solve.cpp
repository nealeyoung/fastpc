//  -*- mode: c++; flycheck-clang-language-standard: "c++11"; -*-

#include <cmath>
#include <assert.h>
#include <iostream>
#include <cstdio> // printf

#include "solve.h"

typedef std::vector<double> vector;
typedef long double long_double;
typedef std::vector<long_double> long_vector;

void solve(Matrix & P, Matrix & C, double eps,
           bool & feasible, vector & solution) {
  P.done_adding_entries();
  C.done_adding_entries();

  auto n = std::max(P.n_cols, C.n_cols);
  auto m_p = P.n_rows;
  auto m_c = C.n_rows;
  auto m = m_p + m_c;
  auto U = 2*log(m)/(eps*eps);
  auto problem_size = P.entries.size() + C.entries.size();

  std::vector<double> x(n, 0.0);

  if (m_c == 0) {
    feasible = true;
    solution = x;
    return;
  }

  vector Px(m_p);
  vector Cx(m_c);

  double max_Px;
  double min_Cx;

  double alpha = -log1p(-eps)/log1p(eps);
  double shift = 0;

  long_vector p_hat(m_p);
  long_vector c_hat(m_c);

  long double p_hat_value;
  long double c_hat_value;

  auto reset = [&]() -> void {
    
    Px = P.times_vector(x);
    Cx = C.times_vector(x);

    min_Cx = *std::min_element(Cx.begin(), Cx.end()); 
    max_Px = *std::max_element(Px.begin(), Px.end()); 

    shift = min_Cx;

    p_hat_value = 0;
    for (size_t i = 0;  i < m_p;  ++i) {
      p_hat[i] = pow(1+eps, Px[i] - alpha*shift);
      p_hat_value += p_hat[i];
      assert (p_hat_value <= 0.001 * std::numeric_limits<long_double>::max());
    }

    c_hat_value = 0;
    for (size_t i = 0;  i < m_c;  ++i) {
      c_hat[i] = Cx[i] < U ? pow(1-eps, Cx[i] - shift) : 0;
      c_hat_value += c_hat[i];
    }
    assert (c_hat_value >= n*std::numeric_limits<long_double>::min());
    assert (c_hat_value <= 0.001 * std::numeric_limits<long_double>::max());
  };
  
  auto potential = [&]() -> double {
    return p_hat_value * c_hat_value / (m_p * m_c);
  };

  double factor = (1+eps)*(1+eps) / (1-eps);

  struct {
    size_t outer_iterations = 0;
    size_t inner_steps = 0;
    size_t wasted_steps = 0;
  } stats;

  auto report = [&]() -> void {
    std::cout
    << "stats.outer_iterations = " << stats.outer_iterations << std::endl
    << "stats.inner_steps = " << stats.inner_steps   << std::endl
    << "stats.wasted_steps = " << stats.wasted_steps << std::endl
    << "stats.wasted_steps/(1+stats.inner_steps) = "
    << stats.wasted_steps/(1.0+stats.inner_steps) <<  std::endl
    << "c_hat_value = " << c_hat_value  << std::endl
    << "p_hat_value = " << p_hat_value << std::endl
    << "potential() = " << potential() << std::endl;
  };

  auto check_done = [&]() -> bool {
    // return early if (1+eps)-feasible
    double eff_eps = min_Cx > 0 ? max_Px/min_Cx-1 : 1;
    
    std::printf("outer_iterations= %d", int(stats.outer_iterations));
    std::printf(", eff eps= %.3g", eff_eps);
    std::printf(", potential= %.3g", potential());
    std::printf(", min_Cx= %.3g", min_Cx);
    std::printf(", max_Px= %.3g", max_Px);
    std::printf(", max_Px-alpha*shift= %.3g\n", max_Px - alpha*shift);

    if (C.n_non_empty_rows == 0  ||  eff_eps <= eps) {
      report();
      for (size_t j = 0;  j < n;  ++j) x[j] /= min_Cx;
      feasible = true;
      solution = x;
      return true;
    }

    // check infeasibility
    for (size_t j = 0;  j < n;  ++j)  {
      if (P.col_dot_vector(j, p_hat) / p_hat_value
          <= C.col_dot_vector(j, c_hat) / c_hat_value)
        return false;
    }
    feasible = false;
    return true;
  };

  size_t inner_steps_last_check = 0;
  size_t inner_steps_last_outer = 0;
  double ave_potential = 2.0;

  int numeric_limits_reached = 0;

  auto time_to_check = [&]() -> bool {
    return C.n_non_empty_rows == 0  
    || stats.inner_steps-inner_steps_last_check > 5*problem_size
    || numeric_limits_reached;
    // || (min_Cx > 0  &&  potential() > ave_potential);
  };

  // TODO: update to skip increments as in paper
  auto update = [&](Matrix* M, vector & Mx, long_vector & m_hat, size_t j,
                    long double & m_hat_value, 
                    long double & Mj_dot_m_hat_value,
                    vector & Mjk_last_xj,
                    bool final = false) -> void {
    int min_removed_k = -1;
    for (int k = M->cols[j].size() - 1;  k >= 0 && ! numeric_limits_reached;  --k) {
      stats.inner_steps += 1;

      Entry* e = M->cols[j][k];
      size_t i = e->row;
      double Mij = e->value;
      if (e->removed) {
        min_removed_k = k;
        continue;
      }

      double dxj = x[j] - Mjk_last_xj[k];
      if (e->top * dxj < 0.5 && ! final) break;

      m_hat_value -= m_hat[i];
      Mj_dot_m_hat_value -= Mij * m_hat[i];
      Mx[i] += dxj * Mij;
      if (M == &C) {
        if (Mx[i] < U) {
          m_hat[i] = powl(1-eps, Mx[i] - shift);
          numeric_limits_reached += (m_hat[i] <= 1000*std::numeric_limits<long_double>::min());

          // m_hat[i] *= pow(1-eps, dxj * Mij);
          // m_hat[i] -= eps * dxj * Mij ;
        } else {
          M->remove_entries_in_row(i);
          m_hat[i] = 0;
        }
      } else {
        m_hat[i] = powl(1+eps, Mx[i] - alpha*shift);
        numeric_limits_reached += (m_hat[i] >= std::numeric_limits<long_double>::max() / n);
        // m_hat[i] *= pow(1+eps, * dxj * Mij);
        // m_hat[i] += eps * dxj * Mij;
      }
      m_hat_value += m_hat[i];
      numeric_limits_reached += (m_hat_value >= 0.001*std::numeric_limits<long_double>::max());
      Mj_dot_m_hat_value += Mij * m_hat[i];
      numeric_limits_reached += (Mj_dot_m_hat_value >= 0.001*std::numeric_limits<long_double>::max());
    }
    if (min_removed_k >= 0)
      M->col_compact(j, min_removed_k);
  };

  reset();

  while(true) {
    stats.outer_iterations += 1;
    inner_steps_last_outer = stats.inner_steps;
    
    if (stats.outer_iterations % 100 == 0)
      std::cout
        << "stats.outer_iterations, stats.inner_steps = "
        << stats.outer_iterations << ", " << stats.inner_steps << std::endl;

    for (size_t j = 0;  j < n && ! time_to_check();  ++j)  {

      long double Pj_dot_p_hat_value = 0;
      for (auto e: P.cols[j]) {
        Pj_dot_p_hat_value += e->value * p_hat[e->row];
        stats.wasted_steps += 1;
      }
      
      long double Cj_dot_c_hat_value = 0;
      for (auto e: C.cols[j]) {
        if (! e-> removed)
          Cj_dot_c_hat_value += e->value * c_hat[e->row];
        stats.wasted_steps += 1;
      }
      
      if (Pj_dot_p_hat_value * c_hat_value
          >= factor * Cj_dot_c_hat_value * p_hat_value)
        continue;

      ave_potential = 
        (std::min(int(problem_size), 1000)*ave_potential + potential())
        / (std::min(int(problem_size), 1000) + 1);

      vector Pjk_last_xj(P.cols[j].size(), x[j]);
      vector Cjk_last_xj(C.cols[j].size(), x[j]);

      size_t inner_loops;

      for (inner_loops = 0;  
           Pj_dot_p_hat_value * c_hat_value
             < factor * Cj_dot_c_hat_value * p_hat_value
             &&  ! time_to_check();
           ++inner_loops)  {

        double dxj = 1 / std::max(P.col_max(j), C.col_max(j));

        x[j] += dxj;

        update(&P, Px, p_hat, j, p_hat_value, Pj_dot_p_hat_value, Pjk_last_xj, true);
        update(&C, Cx, c_hat, j, c_hat_value, Cj_dot_c_hat_value, Cjk_last_xj, true);
      }
      if (inner_loops) {
        update(&P, Px, p_hat, j, p_hat_value, Pj_dot_p_hat_value, x, Pjk_last_xj, true);
        update(&C, Cx, c_hat, j, c_hat_value, Cj_dot_c_hat_value, x, Cjk_last_xj, true);
      }
    } // end for j

    if (time_to_check()
        or stats.inner_steps == inner_steps_last_outer) {
      inner_steps_last_check = stats.inner_steps;
      if (numeric_limits_reached) {
        numeric_limits_reached = 0;
        std::cout << "## WARNING: numeric limit reached" << std::endl;
      }
      reset();
      if (check_done())
        return;
    }
  } // end while true
}
