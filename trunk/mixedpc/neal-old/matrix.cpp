/*
 * matrix.cpp
 */

#include <iostream>
#include <algorithm>
#include <list>
#include <cassert>

#include "matrix.h"

typedef Matrix::Entry               Entry;
typedef Matrix::Vector              Vector;
typedef std::list<Entry*>::iterator Iterator;

Matrix::Matrix() : 
  _done_adding_entries(false),
  _rows(NULL), _cols(NULL), 
  _n_rows(0), _n_cols(0)
{}

void Matrix::add_entry(int row, int col, double value) {
  assert(value >= 0 && row >= 0 && col >= 0);
  if (value == 0) return;
  _entries.push_back(new Entry(row, col, value)); 

  // count n_rows and n_cols
  _n_rows = std::max(_n_rows, row + 1);
  _n_cols = std::max(_n_cols, col + 1);
}

void Matrix::done_adding_entries() {
  Iterator it;

  assert(! _done_adding_entries);
  _done_adding_entries = true;
  
  _n_non_empty_rows = _n_rows;
  _n_non_empty_cols = _n_cols;

  // count # entries in each col and in each row
  int n_in_col[_n_cols];
  int n_in_row[_n_rows];
  for (int row = 0;  row < _n_rows;  ++row)   n_in_row[row] = 0;
  for (int col = 0;  col < _n_cols;  ++col)   n_in_col[col] = 0;

  for (it = _entries.begin();  it != _entries.end();  ++it)  {
    n_in_col[(*it)->_col] += 1;
    n_in_row[(*it)->_row] += 1;
  }

  // allocate _rows and _cols
  _rows = new Vector*[_n_rows];
  _cols = new Vector*[_n_cols];
  assert(_rows && _cols);

  // allocate each row and each col
  for (int i = 0;  i < _n_rows;  ++i)  _rows[i] = new Vector(n_in_row[i]);
  for (int i = 0;  i < _n_cols;  ++i)  _cols[i] = new Vector(n_in_col[i]);

  restore();
}

void Matrix::restore() {
  Iterator it;
  int row, col;

  for (row = 0;  row < _n_rows;  ++row)  _rows[row]->clear();
  for (col = 0;  col < _n_cols;  ++col)  _cols[col]->clear();

  // initialize each row and each col to point to its entries,
  for (it = _entries.begin();  it != _entries.end();  ++it)  {
    if ((*it)->_value == 0) continue;
    (*it)->_removed = false;
    _rows[(*it)->_row]->add(*it);
    _cols[(*it)->_col]->add(*it);
  }
  // sort rows and cols
  for (row = 0;  row < _n_rows;  ++row)  _rows[row]->sort();
  for (col = 0;  col < _n_cols;  ++col)  _cols[col]->sort();
}

Matrix::~Matrix() {
  if (_done_adding_entries) {
    for (int i = 0;  i < _n_rows;  ++i)  delete _rows[i];
    delete[] _rows;

    for (int i = 0;  i < _n_cols;  ++i)  delete _cols[i];
    delete[] _cols;
  }
}

Vector* Matrix::row(int i) { assert (0 <= i && i < _n_rows); return _rows[i]; }
Vector* Matrix::col(int i) { assert (0 <= i && i < _n_cols); return _cols[i]; }

void Matrix::remove_entry (Entry** entry) {
  if ((*entry)->_removed) return;
  (*entry)->_removed = true;

  Vector* row = _rows[(*entry)->_row];
  if (-- row->_n_not_removed == 0)  -- _n_non_empty_rows;

  assert( row->_n_not_removed >= 0 && _n_non_empty_rows >= 0);

  Vector* col = _cols[(*entry)->_col];
  if (-- col->_n_not_removed == 0)  -- _n_non_empty_cols;

  assert( col->_n_not_removed >= 0 && _n_non_empty_cols >= 0);
}

void Matrix::remove_col (int i) {
  Vector* v = _cols[i];
  for (Entry** e = v->first_entry();  e;  e = v->next_entry(e))
    remove_entry(e);
}
void Matrix::remove_row (int i) {
  Vector* v = _rows[i];
  for (Entry** e = v->first_entry();  e;  e = v->next_entry(e))
    remove_entry(e);
}

void Matrix::compact () {
  for (int i = 0;  i < _n_rows;  ++i)  _rows[i]->compact();
  for (int i = 0;  i < _n_cols;  ++i)  _cols[i]->compact();
}

int     Matrix::n_rows             ()        { return _n_rows; }
int     Matrix::n_cols             ()        { return _n_cols; }

int     Matrix::n_non_empty_rows   ()        { return _n_non_empty_rows; }
int     Matrix::n_non_empty_cols   ()        { return _n_non_empty_cols; }

void Matrix::dump() {
  std::cout << std::endl;
  std::cout << "-------------begin matrix-------------" << std::endl;
  for (int i = 0;  i < _n_rows;  ++i) {
    std::cout << "row " << i << ": ";
    for (Entry** entry = row(i)->first_entry();  entry;  entry = row(i)->next_entry(entry))
      std::cout << (*entry)->_col << ":" << (*entry)->_value << " ";
    std::cout << std::endl;
  }
  std::cout << "-------------end matrix-------------" << std::endl;
  std::cout << std::endl;
}

// Matrix::Vector::

Vector::~Vector     ()        { delete[] _storage; }
int  Vector::size   ()        { return _n_not_removed; }
void Vector::clear  ()        { _n_high = _n_not_removed = 0; }

Entry** Vector::max_entry   ()        { return first_entry(); }

void Vector::add (Entry* entry) { 
  assert (_n_high < _storage_size);
  _storage[_n_high ++] = entry;
}

Entry** Vector::next_entry(Entry** entry, double threshold) {
  Entry** _hi = & _storage[_n_high - 1];
  while (++entry <= _hi)  {
    if (! (*entry)->_removed && (*entry)->_value >= threshold) 
      return entry;
  }
  return NULL;
}

Entry** Vector::first_entry(double threshold) 
{ return next_entry(& _storage[0] - 1, threshold); }

struct Vector::Comp {
  bool operator()(Entry* const& a, Entry* const& b) {
    return a->_value > b->_value;  // decreasing order
  }
};

void Vector::sort() {
  Comp comp;
  std::sort(&_storage[0], &_storage[_storage_size-1]+1, comp);
}

void Vector::compact() {
  if (_n_not_removed < 0.8*_n_high) {
    int next = 0;
    for (int i = 0;  i < _n_high;  ++i) {
      if (! _storage[i]->_removed)
        _storage[next ++] = _storage[i];
    }
    _n_high = next;
    assert (_n_not_removed == _n_high);
  }
}

