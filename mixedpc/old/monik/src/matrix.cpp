/*
 * matrix.cpp
 *
 *  Created on: Jun 10, 2009
 *      Author: neal
 */

#include <iostream>
#include <algorithm>
#include <list>

#include "matrix.h"

void Matrix::EntryVector::sort() {
	Comp comp;
	std::sort(_lo, _hi+1, comp);
}

typedef std::list<Matrix::Entry*>::iterator Iterator;

void Matrix::done_adding_entries() {
	Iterator it;
	int row, col;

	assert(! _done_adding_entries);
	_done_adding_entries = true;

	_n_rows = _n_cols = 0;

	// count n_rows and n_cols
	for (it = _entries.begin();  it != _entries.end();  ++it)  {
		_n_rows = std::max(_n_rows, (*it)->_row + 1);
		_n_cols = std::max(_n_cols, (*it)->_col + 1);
	}
	_n_rows_curr = _n_rows;
	_n_cols_curr = _n_cols;

	// allocate rows and cols
	_rows = new EntryVector*[_n_rows];
	_cols = new EntryVector*[_n_cols];
	assert(_rows && _cols);

	// count #  non-zero entries in each col and in each row
	int n_in_col[_n_cols];
	int n_in_row[_n_rows];
	for (row = 0;  row < _n_rows;  ++row)	n_in_row[row] = 0;
	for (col = 0;  col < _n_cols;  ++col)	n_in_col[col] = 0;
	for (it = _entries.begin();  it != _entries.end();  ++it)  {
		assert((*it)->_value >= 0);
		if ((*it)->_value == 0) continue;
		int row = (*it)->_row;
		int col = (*it)->_col;
		n_in_col[col] += 1;
		n_in_row[row] += 1;
	}

	// allocate each row and each col
	for (int i = 0;  i < _n_rows;  ++i)  _rows[i] = new EntryVector(n_in_row[i]);
	for (int i = 0;  i < _n_cols;  ++i)  _cols[i] = new EntryVector(n_in_col[i]);

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

void Matrix::dump() {
	std::cout << std::endl;
	std::cout << "-------------begin matrix-------------" << std::endl;
	for (int i = 0;  i < _n_rows;  ++i) {
		std::cout << "row " << i << ": ";
		for (Entry** entry = first_entry_in_row(i);  entry;  entry = next_entry_in_row(i,entry))
			std::cout << (*entry)->_col << ":" << (*entry)->_value << " ";
		std::cout << std::endl;
	}
	std::cout << "-------------end matrix-------------" << std::endl;
	std::cout << std::endl;
}
