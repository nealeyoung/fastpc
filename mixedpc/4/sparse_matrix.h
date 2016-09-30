//  -*- mode: c++; flycheck-clang-language-standard: "c++11"; -*-

#ifndef SPARSE_MATRIX_H
#define SPARSE_MATRIX_H

#include <vector>
#include <iostream>
#include <assert.h>

// http://stackoverflow.com/questions/12073689/c11-template-function-specialization-for-integer-types
#include <type_traits>

#include "ordered_vector.h"

// T data1, data2;
//
// data1.value() * data2.value() + 0
// data1.value() < data2.value()
//

template <typename T>
struct base_row_col_t {

  const size_t row;
  const size_t col;
  bool         _removed;
  T            data;
  
  base_row_col_t(size_t r, size_t c, T d) 
    : row(r), col(c), _removed(false), data(d) {}

  // so ordered_vector_t<row_col_t *> makes sense:
  inline bool removed() const { return _removed; }
  // derived class should define:
  //  typedef value_t 
  //  value_t value()
};

template<typename T, bool = std::is_arithmetic<T>::value >
struct row_col_t : base_row_col_t<T> {
  typedef decltype(base_row_col_t<T>::data.value()) value_t;
  inline value_t value() const { return base_row_col_t<T>::data.value(); }
  using base_row_col_t<T>::base_row_col_t;
};

template<typename T>
struct row_col_t<T, true> : base_row_col_t<T> {
  typedef T value_t;
  inline value_t value() const { return base_row_col_t<T>::data; }
  using base_row_col_t<T>::base_row_col_t;
};

template<typename T>
struct sparse_matrix_t {
  typedef typename row_col_t<T>::value_t   value_t;
  typedef std::vector<value_t>             vector_t;
  typedef ordered_vector_t<row_col_t<T> *> col_vector_t;
  typedef std::vector<row_col_t<T> *>      row_vector_t;
  typedef typename ordered_vector_t<row_col_t<T> *>::iterator_t iterator_t;

  row_vector_t              entries;
  std::vector<row_vector_t> rows;
  std::vector<col_vector_t> cols;
  std::vector<bool>         rows_removed;
  size_t                    n_remaining_rows;
  bool                      _done_adding;
  
  inline row_vector_t & row(size_t i) { return rows.at(i); }
  inline col_vector_t & col(size_t i) { return cols.at(i); }

  inline const row_vector_t & row(size_t i) const { return rows.at(i); }
  inline const col_vector_t & col(size_t i) const { return cols.at(i); }

  inline size_t n_rows() const { return rows.size(); }
  inline size_t n_cols() const { return cols.size(); }
  
  inline bool row_removed(size_t i) const { return rows_removed.at(i); }

  void clear() {
    for (auto e : entries) delete e;
    entries.clear();
    rows.clear();
    rows_removed.clear();
    cols.clear();
    _done_adding = false;
  }

  sparse_matrix_t()  { clear(); }
  ~sparse_matrix_t() { clear(); }

  void extend(size_t row, size_t col) {
    if (row >= n_rows()) {
      rows.resize(row+1);
      rows_removed.resize(row+1, true);
    }
    if (col >= n_cols()) 
      cols.resize(col+1);
  }

  void push_back(row_col_t<T> * rc) { 
    assert (! _done_adding);
    assert (rc);

    extend(rc->row, rc->col);

    entries.push_back(rc); 
    col(rc->col).push_back(rc); 
    row(rc->row).push_back(rc); 
    rows_removed.at(rc->row) = false;
  }

  void push_back(size_t row, size_t col, T data) 
  { push_back(new row_col_t<T>(row, col, data)); }

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

    for (auto e: r) {
      e->_removed = true;
      assert (e->row == i);
    }
    --n_remaining_rows;
    rows_removed.at(i) = true;
  };
  inline void remove_entries_in_row(size_t i) 
  { remove_entries_in_row(row(i)); }

  // does not include removed entries

  value_t
  col_dot_vector(col_vector_t & c, vector_t & y) // should be const
  {
    value_t sum = 0;
    for (auto e: c) sum += e->value() * y.at(e->row);
    return sum;
  }
  inline value_t col_dot_vector(size_t j, vector_t & y) //should be const
  { return col_dot_vector(col(j), y); }

  // includes removed entries

  inline vector_t transpose_times_vector(vector_t & y) const
  { 
    vector_t x(n_cols(), 0);
    for (auto const e : entries)
      x.at(e->col) += y.at(e->row) * e->value();
    return x;
  }

  // includes removed entries

  value_t
  row_dot_vector(const row_vector_t & r, const vector_t & x) const {
    value_t sum = 0;
    for (auto e: r) sum += e->value() * x.at(e->col);
    return sum;
  }

  inline value_t row_dot_vector(size_t i, const vector_t & x) const
  { return row_dot_vector(row(i), x); }

  // includes removed entries
  vector_t 
  times_vector(const vector_t & x) const {
    vector_t y;
    for (auto & r: rows)
      y.push_back(row_dot_vector(r, x));
    return y;
  }

  inline vector_t operator*(const vector_t & x) const
  { return times_vector(x); }

  void dump() const {
    for (auto & r : rows)
      for (auto e : r)
        std::cout 
          << "(" << e->row << ", " << e->col << ", " << e->value() << ")" 
          << std::endl;
  }

  typename row_vector_t::iterator begin() { return entries.begin(); }
  typename row_vector_t::iterator end()   { return entries.end(); }

  typename row_vector_t::const_iterator begin() const { return entries.begin(); }
  typename row_vector_t::const_iterator end()   const { return entries.end(); }
};

// uncomment for flycheck
struct data_t { double value() const { return 0; } };  
template struct sparse_matrix_t<data_t>;
template struct sparse_matrix_t<double>;

#endif
