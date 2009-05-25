/*
 * sampler.cc
 *
 *  Created on: May 23, 2009
 *      Author: neal
 */

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <utility>
#include <algorithm>
#include <list>

#include "sampler.h"

class _Sampler : public Sampler {
private:

	typedef long long unsigned int Bigint;
	typedef int Index;
	typedef int Exponent;

	static const Index NOINDEX = -1;

	// utility functions

	Bigint big_random() {
		const static int RAND_BITS = (8*sizeof(RAND_MAX) - 1);
		assert(RAND_MAX == (((unsigned int) 1) << RAND_BITS) - 1);

		Bigint r = 0;
		for (unsigned int b = 0;  b < 8*sizeof(Bigint);  b += RAND_BITS)
			r = (r << RAND_BITS) + rand();
		return r;
	}

	// classes

	struct Element;
	struct Interval;

	struct Element {
		bool _removed;
		Exponent _exponent;
		Interval* _interval;
		Index _id;
		Index _rank;
		Element(): _removed(true), _exponent(-1), _interval(NULL), _id(NOINDEX), _rank(NOINDEX) { }
	};
	inline bool& _removed(Element& elt) { return elt._removed; }
	inline Exponent& _exponent(Element& elt) { return elt._exponent; }
	inline Interval*& _interval(Element& elt) { return elt._interval; }
	inline Index& _id(Element& elt) { return elt._id; }
	inline Index& _rank(Element& elt) { return elt._rank; }

	struct Interval {
		typedef std::list<Interval> List;
		typedef List::iterator Iterator;

		const Exponent _exponent;
		Index _lo;
		Index _hi;
		Iterator _iterator;

		Interval(Exponent exponent, Index lo, Index hi = NOINDEX)
			: _exponent(exponent), _lo(lo), _hi(hi) { if (_hi == NOINDEX) _hi = _lo - 1; }
	};
	inline Exponent _exponent(Interval* i) { return i->_exponent; }
	inline Index& _lo(Interval* i) { return i->_lo; }
	inline Index& _hi(Interval* i) { return i->_hi; }
	inline Interval::Iterator& _iterator(Interval* i) { return i->_iterator; }
	inline bool _empty(Interval* i) { return _hi(i) == _lo(i) - 1; }
	inline Index size(Interval* i) { return _hi(i) - _lo(i) + 1; }

	Index _size;
	Bigint _total_weight;
	Exponent _exponent_shift;
	Element* _elements;
	Index* _id_of_rank;
	Interval::List _intervals;
	// interval* _leftmost_heavy_interval;
	// Bigint _heavy_intervals_total_weight;

	inline Index& _id(Index rank) { return _id_of_rank[rank]; }
	inline Element& _element(Index id) { return _elements[id]; }
	inline Element& _element_of_rank(Index rank) { return _element(_id(rank)); }

	// not used
	// bool& _removed(Index id)  { return _removed(_elt(id)); }
	// Exponent& _exponent(Index id) { return _exponent(_elt(id)); }
	// Interval*& _interval(Index id) { return _interval(_elt(id)); }
	// Index& _rank(Index id) { return _rank(_elt(id)); }

	inline Interval* leftmost_interval()
	{ return &(*_intervals.begin()); }

	inline Interval* rightmost_interval()
	{ Interval::Iterator it = _intervals.end();  --it;  return &(*it); }

	inline Interval* interval_to_right(Interval* i)
	{ Interval::Iterator it = i->_iterator;  ++it;  if (it == _intervals.end())  return NULL;  return &(*it); }

	inline Interval* interval_to_left(Interval* i)
	{ Interval::Iterator it = i->_iterator;  if (it == _intervals.begin())  return NULL; --it;  return &(*it); }

	inline Bigint weight_of_element_in_interval(Interval* i) {
		Exponent e = i->_exponent + _exponent_shift;
		assert(e >= 0);
		return Bigint(1) << e;
	}
	inline Bigint total_weight_in_interval(Interval* i)
	{ return size(i) * weight_of_element_in_interval(i); }

	inline void remove_interval(Interval* i)
	{ assert(_empty(i));  _intervals.erase(_iterator(i)); }

	void set_rank(Element& e, Index rank, Interval* i) {
		assert (_rank(e) == NOINDEX);
		assert (_id(rank) == NOINDEX);
		assert (rank >= _lo(i)  &&  rank <= _hi(i));

		_total_weight += weight_of_element_in_interval(i);
		_removed(e) = false;
		_rank(e) = rank;
		_interval(e) = i;
		_id(rank) = _id(e);
	}
	void set_rank(Index id, Index rank, Interval* i)
	{ set_rank(_element(id), rank, i); }

	void unset_rank(Element& e) {
		assert (_rank(e) != NOINDEX);
		Interval* i = _interval(e);

		_total_weight -= weight_of_element_in_interval(i);
		if (_empty(i)) remove_interval(i);
		_removed(e) = true;
		_id(_rank(e)) = NOINDEX;
		_rank(e) = NOINDEX;
		_interval(e) = NULL;
	}
	void unset_rank(Index id)
	{ unset_rank(_element(id)); }

	void swap_ranks(Element& e1, Element& e2) {
		assert(_exponent(e1) == _exponent(e2));
		if (_id(e1) == _id(e2)) return;
		Index r1 = _rank(e1);
		Index r2 = _rank(e2);
		_id(r1) = _id(e2);		_rank(e2) = r1;
		_id(r2) = _id(e1);		_rank(e1) = r2;
	}
	void swap_ranks(Index id1, Index id2)
	{ swap_ranks(_element(id1), _element(id2)); }

	Interval* create_first_interval(Exponent exponent) {
		assert(_intervals.empty());
		Interval i(exponent, 0);
		Interval::Iterator it = _intervals.insert(_intervals.end(), i);
		_iterator(&(*it)) = it;
		return &(*it);
	}
	Interval* add_interval_to_right(Interval* i, Exponent exponent) {
		Interval i2(exponent, _hi(i)+1);
		Interval::Iterator it = _iterator(i); ++it;
		it = _intervals.insert(it, i2);
		_iterator(&(*it)) = it;
		return &(*it);
	}
	void remove_element_shift_left(Element& elt) {
		Interval* i = _interval(elt);
		swap_ranks(elt, _element_of_rank(_hi(i)));
		_hi(i) -= 1;
		unset_rank(elt);
	}
	void remove_element_shift_left(Index id) { remove_element_shift_left(_element(id));	}

	void add_element_to_right(Interval* i, Element& elt) {
		// precondition: rank to right of interval is not in any interval
		set_rank(elt, ++_hi(i), i);
	}
	void add_element_to_right(Interval* interval, Index id) { add_element_to_right(interval, _element(id)); }

	void add_element_to_left(Interval* i, Element& elt) {
		// precondition: rank to left of interval is not in any interval
		set_rank(elt, --_lo(i), i);
	}
	void add_element_to_left(Interval* interval, Index id) { add_element_to_left(interval, _element(id)); }

public:
	_Sampler(int *initial_exponents, int n_in) {
		Index n(n_in);
		assert(n > 0);

		_size = n;
		_total_weight = 0;
		_exponent_shift = 0;
		_elements = new Element[n];
		_id_of_rank = new Index[n];

		assert(_elements && _id_of_rank);

		for(Index rank = 0;  rank < n;  ++rank)
			_id(rank) = NOINDEX;

		std::pair<int, Index> sorted_exponents[n];

		// set up _elements (_ids and _exponents)
		// and _ids_sorted_by_exponent
		for (Index id = 0;  id < n;  ++id) {
			_id(_element(id)) = id;
			_exponent(_element(id)) = initial_exponents[id];

			sorted_exponents[id].first = initial_exponents[id];
			sorted_exponents[id].second = id;
		}
		sort(sorted_exponents, sorted_exponents+n);

		Interval* i = create_first_interval(sorted_exponents[0].first);

		for (Index rank = 0;  rank < n;  ++rank) {
			Exponent e = sorted_exponents[rank].first;
			Index id = sorted_exponents[rank].second;
			if (e != _exponent(i)) i = add_interval_to_right(i, e);
			add_element_to_right(i, id);
		}

		// check work
		for (Index rank = 0;  rank < n-1;  ++rank)
			assert(_exponent(_element_of_rank(rank))
					<= _exponent(_element_of_rank(rank+1)));

		for (Index id = 0;  id < n;  ++id) {
			Element& elt(_element(id));
			assert(_id(_rank(elt)) == id);
			assert(_exponent(elt) == _exponent(_interval(elt)));
			assert(_lo(_interval(elt)) <= _rank(elt));
			assert(_rank(elt) <= _hi(_interval(elt)));
			assert(! _removed(elt));
		}

		Interval* prev = NULL;
		assert(_lo(leftmost_interval()) == 0);
		assert(_hi(rightmost_interval()) == _size-1);
		for (Interval* i = leftmost_interval();  i;  i = interval_to_right(i)) {
			assert((!prev) || _lo(i) == _hi(prev) + 1);
			assert(!_empty(i));
			prev = i;
		}
	}

	void dump() {
		std::cout << _size << std::endl;
		for (Index rank = 0;  rank < _size;  ++rank) {
			Element& elt(_element_of_rank(rank));
			std::cout << "rank=" << rank << ", id=" << _id(elt)
				<< ", exponent=" << _exponent(elt)
				<< ", interval=" << _interval(elt)
				<< ", exponent=" << _exponent(_interval(elt))
				<< std::endl;
		}
	}

	void remove(unsigned int id_in) {
		Index id(id_in);
		remove_element_shift_left(id);
	}

	void increment_exponent(unsigned int id_in) {
		Index id(id_in);

		Element &elt(_element(id));
		Interval* i = _interval(elt);
		Exponent new_exponent = _exponent(elt) + 1;
		if (new_exponent > _exponent(i)) {
			Interval* next = interval_to_right(i);
			if (!next  ||  _exponent(next) != new_exponent)
				next = add_interval_to_right(i, new_exponent);
			remove_element_shift_left(elt);
			_exponent(elt) = new_exponent;
			add_element_to_left(next, elt);
		} else
			_exponent(elt) = new_exponent;
	}

	int sample() {
		const Exponent RAND_BITS(8*sizeof(RAND_MAX) - 1);
		assert(RAND_MAX == (((unsigned int) 1) << RAND_BITS) - 1);

		assert(_total_weight > 0);

		Bigint r = big_random() % _total_weight;
		Bigint passed_weight = 0;

		for (Interval *i = rightmost_interval();  i;  i = interval_to_left(i))  {
			Bigint w = total_weight_in_interval(i);
			if (r < w) {
				Element& elt(_element_of_rank(_lo(i) + r/weight_of_element_in_interval(i)));
				assert(! _removed(elt));
				Exponent e = _exponent(i) - _exponent(elt);
				assert(e >= 0);
				// return element with probability 1/2^e.  e can be large
				if (e == 0) return _id(elt);
				while (e > RAND_BITS) {
					if (rand() != 0) return -1;
					e -= RAND_BITS;
				}
				if (rand() & ((Bigint(1) << e) - 1) == 0) return _id(elt);
				return -1;
			}
			r -= w;
			passed_weight += w;
		}
		assert(false);
		return -1;
	}
	~_Sampler() {}
};

Sampler* create_Sampler(int *initial_exponents, int n) {
	return new _Sampler(initial_exponents, n);
}
