//  -*- mode: c++; flycheck-clang-language-standard: "c++11"; -*-

#include <cmath>
#include <limits>

#include "sparse_matrix.h"
#include "approx_sum.h"

struct rc_data_t {
  double _value;
  double top;
  double value() { return _value; }

  // for maintaining approx_sum for m_hat_M
  double last_xj;
  size_t index_in_approx_sum;
  int    exponent;

  rc_data_t(double v) : _value(v), last_xj(0) {
    top = std::pow(2, std::ceil(std::log2(v)));
  }
};

struct solver_matrix_t {

  solver_matrix_t(std::string matrix_name,
                  size_t m,
                  size_t n,
                  double epsilon,
                  std::vector<double> & x,
                  sparse_matrix_t<double> & M0, 
                  std::vector<double> & x_normalizer,
                  double delta,
                  double upper_threshold);

  std::string                matrix_name;
  size_t                     m;
  size_t                     n;
  double                     epsilon;
  std::vector<double> &      x;
  sparse_matrix_t<rc_data_t> M;
  std::vector<double>        M_x;
  approx_sum_t               y;
  std::vector<approx_sum_t>  y_M;

  void update_y_M_term(size_t j, base_row_col_t<rc_data_t> * e);

  size_t update_M_x(size_t j, 
                    bool final = false,
                    double U = std::numeric_limits<double>::infinity());

  void reset_M_x();

  void sanity_check();
};

