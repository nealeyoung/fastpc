//  -*- mode: c++; flycheck-clang-language-standard: "c++11"; -*-

#include <cmath>

#include "utilities.h"

#include "solver_matrix.h"

solver_matrix_t::
solver_matrix_t(std::string matrix_name,
                size_t m,
                size_t n,
                double epsilon,
                std::vector<double> & x,
                sparse_matrix_t<double> & M0, 
                std::vector<double> & x_normalizer,
                double delta,
                double upper_threshold)
  : matrix_name(matrix_name), m(m), n(n), epsilon(epsilon), x(x), 
    M_x(m, 0), y(m, 0, epsilon), y_M(n) {

  // M

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

  M_x = M*x;
  
  // y was set up in constructor above (y_i = (1+eps)**M_i x = 1)

  // y_M

  for (size_t j = 0;  j < n;  ++j) {
    approx_sum_t & approx_sum = y_M.at(j);
    auto &         col        = M.col(j);
    size_t         size       = col.size();

    std::vector<double> values(size);

    size_t k = 0;
    for (auto & e : col) {
      e->data.index_in_approx_sum = k;
      values.at(k++) = e->value();
    }
    approx_sum.initialize(values, epsilon);

    assert(approx_sum.exponents.size() == col.size());

    for (auto & e : col)
      e->data.exponent = approx_sum.exponents.at(e->data.index_in_approx_sum);
  }
}

void
solver_matrix_t::update_y_M_term(size_t j, base_row_col_t<rc_data_t> * e) {

    // update term for M.col(j) entry e in approx_sum for y_M[j]
    // y_M[j] = sum_i M_ij y[i]
    // y[i] = (1+eps)**(round(M_i x))

    // assume y.exponents[i] = round(M_i x)

    size_t     i = e->row;
    size_t     k = e->data.index_in_approx_sum;
    exponent_t Mij_exponent = e->data.exponent;
    exponent_t y_i_exponent = y.exponents.at(i);
    if (! M.row_removed(i)) {
      assert(! y.removed.at(i));
      exponent_t tmp;
      assert(safe_add(Mij_exponent, y_i_exponent, tmp));
      y_M.at(j).raise_exponent(k, tmp);
    } else 
      y_M.at(j).remove_exponent(k);
}

size_t
solver_matrix_t::update_M_x(size_t j, bool final, double U) {

  size_t steps = 0;

  double x_j = x.at(j);

  for (auto e: M.col(j)) {
    ++steps;
    size_t i = e->row;
    double Mij = e->value();
    double dxj = x_j - e->data.last_xj;

    assert (! M.row_removed(i));

    if (e->data.top * dxj < 0.5 && ! final) break;

    e->data.last_xj = x_j;

    auto & Mx_i = M_x.at(i);

    Mx_i += dxj * Mij;

    if (Mx_i < 100*U) {
      exponent_t e = std::round(Mx_i);
      if (e > y.exponents.at(i)) y.raise_exponent(i, e);
    } else {
      y.remove_exponent(i);
      std::cout << "removing C row " << i << ". " << M.n_remaining_rows << " left\n";
      M.remove_entries_in_row(i);
      assert (M.row_removed(i));
    }
    update_y_M_term(j, e);
  }
  return steps;
}

void
solver_matrix_t::reset_M_x() {
  M_x = M*x;

  for (size_t i = 0;  i < m;  ++i) {
    if (y.removed.at(i))  continue;
    exponent_t e = std::round(M_x.at(i));
    if (e > y.exponents.at(i))
      y.raise_exponent(i, e);
  }
    
  for (size_t j = 0;  j < n;  ++j)
    for (auto & e : M.col(j))
      update_y_M_term(j, e);
}

void
solver_matrix_t::sanity_check() {

  auto Mx_check = M*x;
  long double y_check = 0;

  for (size_t i = 0;  i < M_x.size();  ++i) {
    if (M.row_removed(i)) continue;

    ASSERT (std::abs(Mx_check.at(i) - M_x.at(i)) < 1,
            "inaccurate Mx[i]\n"
            _(matrix_name)
            _(i)
            _(Mx_check.at(i))
            _(M_x.at(i))
            _(y.exponents.at(i)));

    assert (! y.removed.at(i));

    ASSERT (std::abs(y.exponents.at(i) -  M_x.at(i)) < 1,
            "incorrect y exponent\n"
            _(matrix_name)
            _(i)
            _(M_x.at(i))
            _(Mx_check.at(i))
            _(y.exponents.at(i)));

    y_check += std::pow((long double)(1+epsilon),
                                (long double)(Mx_check.at(i)));
  }

  y.sanity_check();

  auto y_lb = y.lower_bound();
  auto y_ub = y.upper_bound();

  if (y.n_remaining == 0) {
    assert (y_lb == 0);
  } else {
    auto err1 = std::abs(y_check       / y_lb - 1.0);
    auto err2 = std::abs(y_ub / y_check       - 1.0);
    auto err3 = std::abs(y_ub / y_lb - 1.0);

    ASSERT (err1 >= 0  && err2 >= 0 && err3 <= std::abs(epsilon),
            "INACCURATE Y ESTIMATES "
            _(matrix_name)
            _(y_lb)
            _(y_check)
            _(epsilon)
            _(err1)
            _(err2)
            _(err3));
  }
};

