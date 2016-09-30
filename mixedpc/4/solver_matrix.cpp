//  -*- mode: c++; flycheck-clang-language-standard: "c++11"; -*-

#include <cmath>

#include "utilities.h"

#include "solver_matrix.h"

solver_matrix_t::
solver_matrix_t(std::string matrix_name,
                size_t m,
                size_t n,
                double epsilon,
                std::vector<long double> & x,
                sparse_matrix_t<double> & M0, 
                std::vector<double> & x_normalizer,
                double delta,
                double upper_threshold,
                double U)
  : matrix_name(matrix_name), 
    m(m), n(n), 
    epsilon(epsilon), 
    x(x), 
    M0(M0), 
    x_normalizer(x_normalizer), 
    delta(delta),
    alpha(std::log1p(epsilon)),     // exp(alpha z) = (1+eps)^z
    upper_threshold(upper_threshold), 
    U(U), 
    M_x(m),
    y(delta/10),
    y_M(n) 
{
  rebuild();
}

void
solver_matrix_t::rebuild() {

  // M

  M.clear();

  M.extend(m-1, n-1);

  for (auto & m_ij: M0) {
    auto nf = x_normalizer.at(m_ij->col);
    assert (nf > 0);
    auto value = std::min(m_ij->value() / nf, upper_threshold);
    if (value >= delta) {
      exponent_t exponent = std::log(value);
      rc_data_t m_ij_data(value, exponent);
      assert (m_ij_data.last_xj == 0);
      // rc_data.index_in_approx_sum set below
      M.push_back(m_ij->row, m_ij->col, m_ij_data);
    } 
  }
  M.done_adding();

  // M_x = 0

  decltype(M_x)(m, 0).swap(M_x);

  // y

  y.clear(delta/10);
  for (size_t i = 0;  i < m;  ++i) {
    y.set_exponent(i, 0);
  }

  // y_M[j]

  for (size_t j = 0;  j < n;  ++j) {
    y_M.at(j).clear(delta/10);

    size_t k = 0;
    for (auto m_ij : M.col(j))
      m_ij->data.index_in_approx_sum = k++;

    update_for_x_j(j, true);
  }

  sanity_check();
}

size_t
solver_matrix_t::update_for_x_j(size_t j, bool final) {

  size_t steps = 0;

  long double x_j = x.at(j);

  for (auto m_ij: M.col(j)) {
    ++steps;

    double dxj         = x_j - m_ij->data.last_xj;

    if (m_ij->data.top * dxj < 0.5 and not final) break;

    size_t i           = m_ij->row;
    size_t k           = m_ij->data.index_in_approx_sum;
    double m_ij_value  = m_ij->data.value();
    auto & Mx_i        = M_x.at(i);

    assert (not M.row_removed(i));

    m_ij->data.last_xj = x_j;

#ifndef NDEBUG
    auto before = Mx_i;
#endif

    Mx_i += dxj * m_ij_value;

    if (dxj * m_ij_value > 2) {
      std::cout << "large dxj * m_ij_value"
        __
        _(i)
        _(j)
        _(dxj)
        _(m_ij_value)
        _(dxj * m_ij_value)
        __;
    }

    // rounding errors?
    assert (Mx_i - before >= dxj * m_ij_value * 0.99);

    if (Mx_i < U) {

      y.set_exponent(i, Mx_i * alpha);        // (1+eps)^(M_i x)

      y_M.at(j).set_exponent(k, Mx_i*alpha + m_ij->data.exponent); 
      // M_ij (1+eps)^(M_i x) 
      // = M_ij exp( alpha M_i x)
      // = exp( M_ij + alpha M_i x)

      //auto e = std::pow(1+epsilon, Mx_i);
      //y.set_term(i, e);
      //y_M.at(j).set_term(k, m_ij_value * e);
    } else {
#ifndef NDEBUG
      std::cout << "removing C row " << i << ". " << M.n_remaining_rows << " left\n";
#endif
      y.zero_term(i);
      assert (not M.row_removed(i));
      for (auto e : M.row(i)) 
        if (not e->removed()) {
          auto k = e->data.index_in_approx_sum;
          y_M.at(e->col).zero_term(k);
        }
      M.remove_entries_in_row(i);
      assert (M.row_removed(i));
    }
  }
  return steps;
}

void
solver_matrix_t::sanity_check(size_t j)  {

#ifndef NDEBUG
  auto near = [&](double a, double b) { 
    return std::abs(a-b) <= 0.01*std::max(a,b); 
  };
#endif

  std::vector<double> X(n);
  for (size_t j1 = 0;  j1 < n;  ++j1) X.at(j1) = x.at(j1);

  std::vector<double> M_x = M * X;

  std::vector<double> M_y(M_x.size());

  for (size_t i = 0;  i < M_x.size();  ++i) {
    if (M_x.at(i) >= U) {
      M_y.at(i) = 0;
      assert (M.row_removed(i));
      assert (y.term_zeroed(i));
    } else {
      M_y.at(i) = std::pow(1+epsilon, M_x.at(i));

      ASSERT (near(M_y.at(i), std::exp(y.exponents.at(i))),
              _(M_y.at(i))
              _(std::exp(y.exponents.at(i))));
    }
  }

  auto M_y_M = M.transpose_times_vector(M_y);

  if (j != std::numeric_limits<size_t>::max())
    ASSERT (near(M_y_M.at(j), std::exp(y_M.at(j).log_lb())),
            _(j)
            _(M_y_M.at(j))
            _(std::exp(y_M.at(j).log_lb())));
}
