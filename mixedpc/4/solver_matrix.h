//  -*- mode: c++; flycheck-clang-language-standard: "c++11"; -*-

#include <cmath>
#include <limits>

#include "sparse_matrix.h"
//#include "approx_sum_debug.h"

#define APPROX_SUM_EXP

#ifdef APPROX_SUM_EXP
#include "approx_sum_exp.h"
#endif
#ifdef APPROX_SUM_KAHAN
#include "approx_sum_kahan.h"
#endif
#ifdef APPROX_SUM_ROUNDED
#include "approx_sum_rounded.h"
#endif

struct rc_data_t {
  const double     _value;
  const double     top;

  double           value() const { return _value; }

  // for maintaining approx_sum for m_hat_M
  const exponent_t exponent;
  long double      last_xj;
  size_t           index_in_approx_sum;

  rc_data_t(double v, exponent_t e) 
    : _value   (v), 
      top      (std::pow(2, std::ceil(std::log2(v)))),
      exponent (e),
      last_xj  (0), 
      index_in_approx_sum(std::numeric_limits<size_t>::max())
  { }
};

struct solver_matrix_t {

  solver_matrix_t(std::string matrix_name,
                  size_t m,
                  size_t n,
                  double epsilon,
                  std::vector<long double> & x,
                  sparse_matrix_t<double> & M0, 
                  std::vector<double> & x_normalizer,
                  double delta,
                  double upper_threshold,
                  double U = std::numeric_limits<double>::infinity()
                  );

  const std::string                matrix_name;
  const size_t                     m;
  const size_t                     n;
  const double                     epsilon;
  const std::vector<long double> & x;
  const sparse_matrix_t<double> &  M0;
  const std::vector<double> &      x_normalizer;
  const double                     delta;
  const double                     alpha;
  const double                     upper_threshold;
  const double                     U;

  sparse_matrix_t<rc_data_t> M;
  std::vector<long double>   M_x;
  approx_sum_t               y;
  std::vector<approx_sum_t>  y_M;

  void rebuild();

  size_t update_for_x_j   (size_t j, bool final = false);

  void sanity_check(size_t j = std::numeric_limits<size_t>::max());
  void check_col_j_equality(size_t j);
};

