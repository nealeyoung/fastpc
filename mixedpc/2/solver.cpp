//  -*- mode: c++; flycheck-clang-language-standard: "c++11"; -*-

#include <cmath>
#include <assert.h>
#include <iostream>
#include <cstdio> // printf

#include "solver.h"
#include "vector_utilities.h"

typedef double scalar_t;

void solve(sparse_matrix_t<double> & P0, // const
           sparse_matrix_t<double> & C0, // const
           double epsilon,
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
    auto max_Py = max(Py);
    if (max_Py <= min_Cy) {
      feasible = true;

      std::vector<double> z(n, 1.0/min_Cy);
      solution = z;
      return;
    }
  }
  assert (n >= 1 && m >= 2);

  double delta = epsilon/100;
  // (1+eps)(1+delta)/(1-delta)^2 = 1+epsilon
  // eps = (1+epsilon)*(1-delta)^2/(1+delta) - 1
  // scalar_t eps = (1+epsilon)*(1-delta)*(1-delta)/(1+delta) - 1;
  double eps = (1+epsilon)*(1-delta)/(1+delta) - 1;

  double U = 2.0*log(double(m))/(eps*eps);

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

  struct rc_data_t {
    double _value;
    double top;
    double value() { return _value; }
    double last_xj;

    rc_data_t(double v) : _value(v), last_xj(0) {
      top = pow(2, ceil(log2(v)));
    }
  };

  auto normalized = [&](sparse_matrix_t<double> & M0, 
                        double delta,
                        double upper_threshold)
    -> sparse_matrix_t<rc_data_t> {
    sparse_matrix_t<rc_data_t> M;
    for (auto & e: M0) {
      auto nf = x_normalizer.at(e->col);
      assert (nf > 0);
      auto value = std::min(e->value() / nf, upper_threshold);
      if (value >= std::abs(delta)) {
        rc_data_t rc_data(value);
        M.push_back(e->row, e->col, rc_data);
      }
    }
    M.done_adding();
    return M;
  };
    
  auto P = normalized(P0, delta, std::min(m, n));
  auto C = normalized(C0, -delta, std::min(m, n)*std::min(m, n)/delta);

  auto denormalize = [&](std::vector<double>& x) -> std::vector<double> {
    std::vector<double> y(x);
    for (size_t j = 0;  j < n;  ++j) {
      y[j] /= x_normalizer[j] != 0 ? x_normalizer[j] : 1.0;
      y[j] /= 1-delta;
    }
    return y;
  };

  auto problem_size = P.entries.size() + C.entries.size();

  std::vector<double> x(n, 0); 

  std::vector<double>   Px(m_p, 0);
  std::vector<scalar_t> p_hat(m_p, 1);
  scalar_t              p_hat_value(m_p);

  std::vector<double>   Cx(m_c, 0);
  std::vector<scalar_t> c_hat(m_c, 1);
  scalar_t              c_hat_value(m_c);

  auto potential = [&]() -> double { return p_hat_value * c_hat_value / (m_p * m_c); };

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

    // Cx = C*x;  -- can't, deleted entries won't be included

    for (size_t i = 0;  i < m_c;  ++i)
       c_hat.at(i) = C.row(i).empty() ? 0 : pow(1-eps, Cx.at(i));

    c_hat_value = sum(c_hat);

    Px = P*x;

    for (size_t i = 0;  i < m_p;  ++i)
      p_hat.at(i) = pow(1+eps, Px.at(i));

    p_hat_value = sum(p_hat);

    // std::cout
    // << "p_hat_value= " << p_hat_value
    // << ", sum(p_hat)= " << sum(p_hat)
    // << std::endl;

    auto min_Cx = min(Cx);
    auto max_Px = max(Px);
    
    double eff_eps = min_Cx > 0 ? max_Px/min_Cx-1 : 1;
    
    std::printf("outer_iterations= %d", int(stats.outer_iterations));
    std::printf(", eff eps= %.3g", double(eff_eps));
    std::printf(", potential= %.3g", double(potential()));
    std::printf(", min_Cx= %.3g", double(min_Cx));
    std::printf(", max_Px= %.3g", double(max_Px));
    std::printf(", c_hat_value= %.3g", double(c_hat_value));
    std::printf(", p_hat_value= %.3g\n", double(p_hat_value));
    // std::printf(", max_Px-alpha*shift= %.3g\n", double(max_Px - alpha*shift));

    if (C.n_non_empty_rows == 0  ||  eff_eps <= eps) {
      report();
      for (size_t j = 0;  j < n;  ++j) x[j] /= min_Cx;
      feasible = true;
      solution = x;
      return true;
    }

    // check infeasibility
    // the dot product does not include deleted rows i of C,
    // this should be okay as c_hat[i] = 0 for those (?)
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

  auto time_to_check = [&]() -> bool {
    return C.n_non_empty_rows == 0  
    || stats.inner_steps-inner_steps_last_check > 10*problem_size;
    // || (min_Cx > 0  &&  potential() > ave_potential);
  };

  auto update = [&](sparse_matrix_t<rc_data_t>& M, 
                    std::vector<double> & Mx, 
                    std::vector<double> & m_hat, 
                    size_t j,
                    double & m_hat_value, 
                    double & Mj_dot_m_hat_value,
                    bool final = false) -> void {

    for (auto e: M.col(j)) {
      stats.inner_steps += 1;

      size_t i = e->row;

      assert (! M.row(i).empty());

      double Mij = e->value();

      double dxj = x.at(j) - e->data.last_xj;
      if (e->data.top * dxj < 0.5 && ! final) break;

      e->data.last_xj = x.at(j);

      auto & m_hat_i = m_hat.at(i);
      auto & Mx_i = Mx.at(i);

      m_hat_value -= m_hat_i;
      Mj_dot_m_hat_value -= Mij * m_hat_i;
      Mx_i += dxj * Mij;
      if (&M == &C) {
        if (Mx_i < U)
          m_hat_i = pow(1-eps, Mx_i);
        else {
          m_hat_i = 0;
          M.remove_entries_in_row(i);
          std::cout << "removing C row " << i << ". " << M.n_non_empty_rows << " left\n";
        }
      } else {
        m_hat_i = pow(1+eps, Mx_i);
      }
      m_hat_value += m_hat_i;
      Mj_dot_m_hat_value += Mij * m_hat_i;
    }
  };

  while(true) {
    stats.outer_iterations += 1;
    inner_steps_last_outer = stats.inner_steps;
    
    if (stats.outer_iterations % 100 == 0)
      std::cout
        << "stats.outer_iterations, stats.inner_steps = "
        << stats.outer_iterations << ", " 
        << stats.inner_steps << std::endl;

    for (size_t j = 0;  j < n && ! time_to_check();  ++j)  {

      double Pj_dot_p_hat_value = P.col_dot_vector(j, p_hat);
      double Cj_dot_c_hat_value = C.col_dot_vector(j, c_hat);

      stats.wasted_steps += P.col(j).size() + C.col(j).size();
      
      if (Pj_dot_p_hat_value * c_hat_value
          >= factor * Cj_dot_c_hat_value * p_hat_value)
        continue;

      auto & x_j = x.at(j);

      for (auto e: P.col(j)) e->data.last_xj = x_j;
      for (auto e: C.col(j)) e->data.last_xj = x_j;

      size_t inner_loops;

      for (inner_loops = 0;  
           Pj_dot_p_hat_value * c_hat_value
             < factor * Cj_dot_c_hat_value * p_hat_value
             &&  ! time_to_check();
           ++inner_loops)  {

        double dxj = 1 / std::max(P.col(j).max()->value(), C.col(j).max()->value());

        x_j += dxj;

        update(P, Px, p_hat, j, p_hat_value, Pj_dot_p_hat_value);
        update(C, Cx, c_hat, j, c_hat_value, Cj_dot_c_hat_value);
      }
      if (inner_loops) {
        update(P, Px, p_hat, j, p_hat_value, Pj_dot_p_hat_value, true);
        update(C, Cx, c_hat, j, c_hat_value, Cj_dot_c_hat_value, true);
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

