//  -*- mode: c++; flycheck-clang-language-standard: "c++11"; -*-

#include "matrix.h"

std::default_random_engine generator;
std::uniform_real_distribution<double> distribution(0.0, 1.0);

extern double random_01() {
  return distribution(generator);
}

extern void
random_P_C_x(int _n, Matrix & P, Matrix & C, std::vector<double> & x) {
  assert(_n > 0);
  size_t n = _n;

  x.resize(n);

  for (size_t j = 0;  j < n;  ++j)
    x[j] = random_01() < 0.2 ? random_01() : 0;

  typedef std::vector<double> _vector;
  typedef std::vector<_vector> _matrix;

  _matrix _P;
  _matrix _C;

  auto maybe_push_row = [&](_matrix & M) -> bool {
    _vector v(n, 0);
    double sum = 0;
    for (size_t i = 0;  i < n;  ++i)
      if (random_01() < 0.5) {
        v[i] = random_01();
        sum += v[i]*x[i];
      }
    if (sum == 0) return false;
    for (size_t i = 0;  i < n;  ++i)
      v[i] /= sum;
    M.push_back(v);
    return true ;
  };

  while (_P.size() < n) { maybe_push_row(_P); }
  while (_C.size() < n) { maybe_push_row(_C); }

  auto fill_matrix = [](_matrix & _M, Matrix & M, double scale=1.0) -> void {
    for (size_t i = 0;  i < _M.size();  ++i)
      for (size_t j = 0;  j < _M[i].size();  ++j)
        if (_M[i][j] != 0)
          M.add_entry(i, j, scale * _M[i][j]);
    M.done_adding_entries();

    M.check(); // DEBUG
  };

  fill_matrix(_P, P, 0.999);
  fill_matrix(_C, C, 1.001);
}
