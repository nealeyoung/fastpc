//  -*- mode: c++; flycheck-clang-language-standard: "c++11"; -*-

#include <cmath>
#include <forward_list>
#include <assert.h>
#include <vector>

#include <tuple>
#include <random>

#include <iostream>

struct Entry {
  size_t    row;
  size_t    col;
  size_t    index_in_row;

  double value;
  double top;
  bool   removed;
  
  Entry(size_t r, size_t c, double v) : row(r), col(c), value(v) {
    top = pow(2, ceil(log2(value)));
  }
};

struct Matrix {
  std::vector<Entry*> entries;
  bool done_adding = false;
  size_t n_rows = 0;
  size_t n_cols = 0;
  size_t n_non_empty_rows;
  std::vector<std::vector<Entry*> > cols;
  std::vector<std::vector<Entry*> > rows;

  void add_entry(int row, int col, double value) {
    assert (! done_adding);

    assert (row >= 0  &&  col >= 0  &&  value >= 0);
    if (value == 0) return;
    size_t r = row;
    size_t c = col;

    auto e = new Entry(size_t(r), size_t(c), value);
    assert (e);
    entries.push_back(e);

    n_rows = std::max(r+1, n_rows);
    n_cols = std::max(c+1, n_cols);
  }

  ~Matrix() {
    for (auto e: entries) delete e;
  }
  
  void done_adding_entries() {
    if (done_adding) return;

    entries.shrink_to_fit();
    cols.resize(n_cols);
    rows.resize(n_rows);

    sort(entries.begin(), entries.end(), [](Entry* a, Entry* b) {
        return a->value < b->value;
      });

    for (auto e: entries) {
      e->index_in_row = rows[e->row].size();
      rows[e->row].push_back(e);
    }

    for (auto & r: rows)
      r.shrink_to_fit();

    restore();
    done_adding = true;
  }

  void restore() {
    n_non_empty_rows = 0;
    for (auto & r: rows)
      if (! r.empty()) n_non_empty_rows += 1;

    for (auto& c: cols) c.clear();
    for (auto e: entries) {
      e->removed = false;
      cols[e->col].push_back(e);
    }

    if (! done_adding)
      for (auto & c: cols) c.shrink_to_fit();

    assert (rows.size() == n_rows);
    assert (cols.size() == n_cols);
  }
  
  double row_dot_vector(size_t i, std::vector<double> x) {
    double sum = 0;
    for (auto e: rows.at(i))
      sum += e->value * x.at(e->col);
    return sum;
  }

  double col_dot_vector(size_t i, std::vector<double> y) {
    double sum = 0;
    for (auto e: cols.at(i))
      if (! e->removed) 
        sum += e->value * y.at(e->row);
    return sum;
  }

  std::vector<double> times_vector(std::vector<double> x) {
    std::vector<double> y;
    for (size_t i = 0;  i < n_rows;  ++i)
      y.push_back(row_dot_vector(i, x));
    return y;
  }

  long double row_dot_vector(size_t i, std::vector<long double> x) {
    long double sum = 0;
    for (auto e: rows.at(i))
      sum += e->value * x.at(e->col);
    return sum;
  }

  long double col_dot_vector(size_t i, std::vector<long double> y) {
    long double sum = 0;
    for (auto e: cols.at(i))
      if (! e->removed) 
        sum += e->value * y.at(e->row);
    return sum;
  }

  std::vector<long double> times_vector(std::vector<long double> x) {
    std::vector<long double> y;
    for (size_t i = 0;  i < n_rows;  ++i)
      y.push_back(row_dot_vector(i, x));
    return y;
  }

  double col_max(int j) {
    assert(j >= 0);
    if (j < int(n_cols) && ! cols[j].empty())
      return cols[j].back()->value;
    return 0;
  }
        
  void col_compact(size_t j, size_t start_k) {
    auto & col = cols.at(j);
    assert (start_k <= col.size());

    for (auto k = start_k;  k < col.size();  ++k)
      if (! col[k]->removed)
        col[start_k++] = col[k];

    col.erase(col.begin() + start_k, col.end());
  }

  void remove_entries_in_row(size_t i) {
    auto & row = rows.at(i);
    if (row.empty()) return;
    for (auto e : row)
      if (! e->removed) {
        e->removed = true;
        auto & col = cols[e->col];
        while ((! col.empty()) && col.back()->removed)
          col.pop_back();
      }
    n_non_empty_rows -= 1;
  }

  void check() {
    assert (done_adding);

    for (auto e : entries)
        assert (e->row < 1000);

    for (auto r : rows)
      for (auto e : r)
        assert (e->row < 1000);
  }

  void dump() {
    assert (done_adding);
    check();

    for (auto & r : rows)
      for (auto e : r) {
        std::cout << "(" << e->row << ", " << e->col << ", " << e->value << ")" << std::endl;
        assert (e->row < 1000);
      }
  }
};

double random_01();

void random_P_C_x(int n, Matrix & P, Matrix & C, std::vector<double> & x);
