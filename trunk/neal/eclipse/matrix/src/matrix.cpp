/*
 * matrix.cpp
 *
 *  Created on: Jun 10, 2009
 *      Author: neal
 */

#include <iostream>
#include <algorithm>

#include "matrix.h"

Matrix::Matrix(Entry* entries, int n_entries) {
	_storage = entries;
	_n_rows = _n_cols = 0;

	// count n_rows and n_cols
	for (int i = 0;  i < n_entries;  ++i)  {
		_n_rows = std::max(_n_rows, entries[i]._row + 1);
		_n_cols = std::max(_n_cols, entries[i]._col + 1);
	}

	// allocate rows and cols
	_rows = new EntryVector*[_n_rows];
	_cols = new EntryVector*[_n_cols];
	assert(_rows && _cols);

	// count #  non-zero entries in each col and in each row
	int n_in_col[_n_cols];
	int n_in_row[_n_rows];
	for (int i = 0;  i < _n_rows;  ++i)		n_in_row[i] = 0;
	for (int i = 0;  i < _n_cols;  ++i)		n_in_col[i] = 0;
	for (int i = 0;  i < n_entries;  ++i)  {
		assert(entries[i]._value >= 0);
		if (entries[i]._value == 0) continue;
		int row = entries[i]._row;
		int col = entries[i]._col;
		n_in_col[col] += 1;
		n_in_row[row] += 1;
	}

	// allocate each row and each col
	for (int i = 0;  i < _n_rows;  ++i)  _rows[i] = new EntryVector(n_in_row[i]);
	for (int i = 0;  i < _n_cols;  ++i)  _cols[i] = new EntryVector(n_in_col[i]);

	// initialize each row and each col to point to its entries,
	for (int i = 0;  i < n_entries;  ++i)  {
		if (entries[i]._value == 0) continue;
		_rows[entries[i]._row]->add(&entries[i]);
		_cols[entries[i]._col]->add(&entries[i]);
	}
	// sort rows and cols
	for (int i = 0;  i < _n_rows;  ++i)  _rows[i]->sort();
	for (int i = 0;  i < _n_cols;  ++i)  _cols[i]->sort();
}
void Matrix::dump() {
	for (int i = 0;  i < _n_rows;  ++i) {
		std::cout << "row " << i << ": ";
		for (Entry** entry = first_entry_in_row(i);  entry;  entry = next_entry_in_row(i,entry))
			std::cout << (*entry)->_col << ":" << (*entry)->_value << " ";
		std::cout << std::endl;
	}
}

