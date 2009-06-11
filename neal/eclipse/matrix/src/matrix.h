/*
 * matrix.h
 *
 *  Created on: Jun 10, 2009
 *      Author: neal
 */

#ifndef MATRIX_H_
#define MATRIX_H_

// sparse matrix class

#include <list>

class Matrix {
public:
	struct Entry;

private:
	struct EntryVector;

	int 			_n_rows, _n_cols;
	Entry*			_storage;
	EntryVector**	_rows;
	EntryVector**	_cols;
	std::list<Entry*>	_entries;
	bool			_done_adding_entries;

public:
	typedef double Scalar;

	struct Entry {
		int _row, _col;
		Scalar _value;
		Entry(int row, int col, Scalar value) {
			_row = row; _col = col; _value = value; _removed = false;
		}
		bool _removed;
	};

	Matrix() :
		_storage(NULL),
		_rows(NULL),
		_cols(NULL),
		_done_adding_entries(false) {}

	// use the following routines to initialize matrix
	void add_entry(int row, int col, Scalar value) { _entries.push_back(new Entry(row, col, value)); }
	void done_adding_entries();

	~Matrix() {
		for (int i = 0;  i < _n_rows;  ++i)  delete _rows[i];
		for (int i = 0;  i < _n_cols;  ++i)  delete _cols[i];
		delete[] _rows;
		delete[] _cols;
	}
	inline Entry**	first_entry_in_row	(int row, Scalar threshold = -1)  				{ return _rows[row]->first(threshold); }
	inline Entry**	next_entry_in_row	(int row, Entry** entry, Scalar threshold = -1)	{ return _rows[row]->next(entry, threshold); }
	inline Entry**	first_entry_in_col	(int col, Scalar threshold = -1)				{ return _cols[col]->first(threshold); }
	inline Entry**	next_entry_in_col	(int col, Entry** entry, Scalar threshold = -1)	{ return _cols[col]->next(entry, threshold); }
	inline Entry**	max_entry_in_row	(int row)										{ return _rows[row]->first(); }
	inline Entry**	max_entry_in_col	(int col)										{ return _cols[col]->first(); }
	inline void		remove_col			(int col)										{ _cols[col]->remove_all(); }
	inline void		remove_row			(int row)										{ _rows[row]->remove_all(); }
	inline int		n_rows				()												{ return _n_rows; }
	inline int		n_cols				()												{ return _n_cols; }
	void			dump				();

private:
	// for rows and columns
	struct EntryVector {
		Entry**		_storage;
		Entry**		_lo;
		Entry**		_hi;
		Entry**		_last_scanned_removed;

		EntryVector(int n) :
			_storage(new Entry*[n]),
			_lo(&_storage[n]),
			_hi(&_storage[n-1]),
			_last_scanned_removed(NULL)
		{}
		~EntryVector() 						{ delete[] _storage; }
		inline bool empty()					{ return _lo > _hi; }
		inline void add(Entry* entry)		{ *(--_lo) = entry;  assert(_lo >= _storage); }
		inline bool removed(Entry** entry)	{ return entry[0]->_removed; }
		inline void remove(Entry** entry)	{
			entry[0]->_removed = true;
			while (removed(_lo) && ! empty()) ++_lo;
			while (removed(_hi) && ! empty()) --_hi;
		}
		inline void remove_all()			{
			for (Entry** entry = _lo;  entry <= _hi;  ++entry) entry[0]->_removed = true;
			_lo = _hi + 1;
		}
		inline Entry** first(Scalar threshold = -1) { return next(_lo-1, threshold); }
		inline Entry** next(Entry** entry, Scalar threshold = -1) {
			while (++entry <= _hi)  {
				if (removed(entry)) {
					_last_scanned_removed = entry;
					continue;
				}
				if (entry[0]->_value >= threshold) return entry;
				break;
			}
			if (_last_scanned_removed) {
				entry = _last_scanned_removed;
				while(entry >= _lo)
					if (removed(entry)) --entry;
					else *(_last_scanned_removed--) = *(entry--);
				_lo = _last_scanned_removed + 1;
				_last_scanned_removed = NULL;
			}
			return NULL;
		}

		struct Comp {
			bool operator()(Entry* const& a, Entry* const& b) {
			    return a->_value > b->_value;  // decreasing order
			}
		};
		void sort() {
			Comp comp;
			std::sort(_lo, _hi+1, comp);
		}
	};
};


#endif /* MATRIX_H_ */
