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
struct base_row_col_t {
  size_t row;
  size_t col;
  bool _removed;
  T data;
  
  base_row_col_t(size_t r, size_t c, T d) 
    : row(r), col(c), _removed(false), data(d) {}

  // so ordered_vector_t<row_col_t *> makes sense:
  inline bool removed() { return _removed; }
  // derived class should define:
  //  typedef value_t 
  //  value_t value()
};

template<typename T>
struct row_col_t : base_row_col_t<T> {

  typedef decltype(base_row_col_t<T>::data.value()) value_t;
  inline value_t value() { return base_row_col_t<T>::data.value(); }
  
  using base_row_col_t<T>::base_row_col_t;
};

template<>
struct row_col_t<double> : base_row_col_t<double> {
  typedef double value_t;
  inline value_t value() { return base_row_col_t<double>::data; }

  using base_row_col_t<double>::base_row_col_t;
};

template<typename T>
struct sparse_matrix_t {
  typedef typename row_col_t<T>::value_t   value_t;
  typedef std::vector<value_t>             vector_t;
  typedef ordered_vector_t<row_col_t<T> *> col_vector_t;
  typedef std::vector<row_col_t<T> *>      row_vector_t;
  typedef typename ordered_vector_t<row_col_t<T> *>::iterator_t iterator_t;

  row_vector_t entries;
  std::vector<row_vector_t> rows;
  std::vector<col_vector_t> cols;
  std::vector<bool> rows_removed;

  inline row_vector_t & row(size_t i) { return rows.at(i); }
  inline col_vector_t & col(size_t i) { return cols.at(i); }

  inline size_t n_rows() { return rows.size(); }
  inline size_t n_cols() { return cols.size(); }
  
  size_t n_remaining_rows;
  bool _done_adding;

  sparse_matrix_t() : _done_adding(false) {}

  ~sparse_matrix_t() { for (auto e : entries) delete e; }

  void push_back(row_col_t<T> * rc) { 
    assert (! _done_adding);
    assert (rc);
    if (rc->row >= n_rows()) {
      rows.resize(rc->row+1);
      rows_removed.resize(rc->row+1, true);
    }
    if (rc->col >= n_cols()) cols.resize(rc->col+1);
    
    entries.push_back(rc); 
    row(rc->row).push_back(rc); 
    rows_removed.at(rc->row) = false;
    col(rc->col).push_back(rc); 
  }

  inline bool row_removed(size_t i) 
  { return i >= rows_removed.size() || rows_removed[i]; }

  void push_back(size_t row, size_t col, T data) {
    push_back(new row_col_t<T>(row, col, data));
  }

  void done_adding() {
    n_remaining_rows = 0;

    assert (! _done_adding);
    _done_adding = true;
    // entries.done_adding();
    for (auto r: rows_removed) {
      // r.done_adding();
      if (! r) ++n_remaining_rows;
    }
    for (auto & c: cols) c.done_adding();
  }

  void remove_entries_in_row(row_vector_t & r) {
    size_t i = &r - &row(0);
    assert (&row(i) == &r);
    
    if (rows_removed.at(i)) return;

    for (auto e: r) e->_removed = true;
    --n_remaining_rows;
    rows_removed.at(i) = true;
  };
  inline void remove_entries_in_row(size_t i) { remove_entries_in_row(row(i)); }

  value_t
  col_dot_vector(col_vector_t & c, vector_t & y) {
    value_t sum = 0;
    for (auto e: c) sum += e->value() * y.at(e->row);
    return sum;
  }

  // does not include removed entries
  inline value_t col_dot_vector(size_t j, vector_t & y) 
  { return col_dot_vector(col(j), y); }

  // includes removed entries
  inline vector_t transpose_times_vector(vector_t & y) 
  { 
    vector_t x(n_cols(), 0);
    for (auto e : entries)
      x.at(e->col) += y.at(e->row) * e->value();
    return x;
  }

  // includes removed entries
  value_t
  row_dot_vector(row_vector_t & r, vector_t & x) {
    value_t sum = 0;
    for (auto e: r) sum += e->value() * x.at(e->col);
    return sum;
  }

  inline value_t row_dot_vector(size_t i, vector_t & x) 
  { return row_dot_vector(row(i), x); }

  // includes removed entries
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
    for (auto & r : rows)
      for (auto e : r)
        std::cout 
          << "(" << e->row << ", " << e->col << ", " << e->value() << ")" 
          << std::endl;
  }

  decltype(entries.begin()) begin() { return entries.begin(); }
  decltype(entries.end())   end()   { return entries.end(); }
};

// uncomment for flycheck
struct data_t { double value() { return 0; } };  
template struct sparse_matrix_t<data_t>;
template struct sparse_matrix_t<double>;

#endif
