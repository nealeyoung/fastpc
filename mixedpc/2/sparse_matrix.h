//  -*- mode: c++; flycheck-clang-language-standard: "c++11"; -*-

#ifndef SPARSE_MATRIX_H
#define SPARSE_MATRIX_H

#include <vector>
#include <iostream>

#include "ordered_vector.h"

// T data1, data2;
//
// data1.value() * data2.value() + 0
// data1.value() < data2.value()
//

template<typename T>
struct row_col_t {
  size_t row;
  size_t col;
  bool _removed;
  T data;
  
  row_col_t(size_t r, size_t c, T d) 
    : row(r), col(c), _removed(false), data(d) {}

  typedef decltype(data.value()) value_t;

  // so ordered_vector_t<row_col_t *> makes sense:
  inline value_t value() { return data.value(); }
  inline bool removed() { return _removed; }
};

template<>
struct row_col_t<double> {
  size_t row;
  size_t col;
  bool _removed;
  double data;
  
  row_col_t(size_t r, size_t c, double d) 
    : row(r), col(c), _removed(false), data(d) {}

  typedef double value_t;

  // so ordered_vector_t<row_col_t *> makes sense:
  inline value_t value() { return data; }
  inline bool removed() { return _removed; }
};

template<typename T>
struct sparse_matrix_t {
  typedef typename row_col_t<T>::value_t   value_t;
  typedef std::vector<value_t>             vector_t;
  typedef ordered_vector_t<row_col_t<T> *> rc_vector_t;
  typedef typename ordered_vector_t<row_col_t<T> *>::iterator_t iterator_t;

  rc_vector_t entries;
  std::vector<rc_vector_t> rows;
  std::vector<rc_vector_t> cols;

  inline rc_vector_t & row(size_t i) { return rows.at(i); }
  inline rc_vector_t & col(size_t i) { return cols.at(i); }

  inline size_t n_rows() { return rows.size(); }
  inline size_t n_cols() { return cols.size(); }
  
  size_t n_non_empty_rows;
  bool _done_adding;

  void push_back(row_col_t<T> * rc) { 
    assert (! _done_adding);
    assert (rc);
    if (rc->row >= n_rows()) rows.resize(rc->row+1);
    if (rc->col >= n_cols()) cols.resize(rc->col+1);
    
    entries.push_back(rc); 
    row(rc->row).push_back(rc); 
    col(rc->col).push_back(rc); 
  }

  void push_back(size_t row, size_t col, T data) {
    push_back(new row_col_t<T>(row, col, data));
  }

  void done_adding() {
    n_non_empty_rows = 0;

    assert (! _done_adding);
    _done_adding = true;
    entries.done_adding();
    for (auto& r: rows) {
      r.done_adding();
      if (! r.empty()) ++n_non_empty_rows;
    }
    for (auto& c: cols) c.done_adding();
  }

  sparse_matrix_t() : _done_adding(false) {}

  void remove_entries_in_row(rc_vector_t & r) {
    if (r.empty()) return;
    for (auto e: r)
      e->_removed = true;
    --n_non_empty_rows;
  };
  inline void remove_entries_in_row(size_t i) { 
    remove_entries_in_row(row(i)); 
  }

  value_t
  col_dot_vector(rc_vector_t & c, vector_t & y) {
    value_t sum = 0;
    for (auto e: c) sum += e->value() * y.at(e->row);
    return sum;
  }

  inline value_t col_dot_vector(size_t j, vector_t & y) 
  { return col_dot_vector(col(j), y); }

  value_t
  row_dot_vector(rc_vector_t & r, vector_t & x) {
    value_t sum = 0;
    for (auto e: r) sum += e->value() * x.at(e->col);
    return sum;
  }

  inline value_t row_dot_vector(size_t i, vector_t & x) 
  { return row_dot_vector(row(i), x); }

  vector_t 
  times_vector(vector_t & x) {
    vector_t y;
    for (auto & r: rows)
      y.push_back(row_dot_vector(r, x));
    return y;
  }

  inline vector_t operator*(vector_t & x) 
  { return times_vector(x); }

  void dump() {
    for (auto& r : rows)
      for (auto e : r)
        std::cout 
          << "(" << e->row << ", " << e->col << ", " << e->value() << ")" 
          << std::endl;
  }

  iterator_t begin() { return entries.begin(); }
  iterator_t end()   { return entries.end(); }
};

// uncomment for flycheck
struct data_t { double value() { return 0; } };  
template struct sparse_matrix_t<data_t>;

#endif
