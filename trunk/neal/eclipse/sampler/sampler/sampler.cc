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
	typedef int 				Exponent;
	typedef int 				Id;
	class						Element;
	class						Interval;

	typedef std::list<Interval>			List;
	typedef List::iterator				Iterator;
	typedef List::reverse_iterator		Reverse_iterator;

	unsigned	_size;
	Bigint		_total_weight;				// Sum of element weights; see _weight_of_exponent().
	Element*	_elements;					// _elements[id] always holds element with given id.
	Element**	_storage;					// _storage[0.._size-1] holds unique pointers to non-removed elements,
											// sorted by increasing element exponent (and some NULLs between intervals).
	List		_intervals;					// In _storage, for each maximal contiguous sequence of pointers
											// to elements with equal exponents, there is an interval structure.
											// This is a list of them ordered by increasing exponent.

	Exponent	_base_exponent;				// See _weight_of_exponent().
	int			_base_rank;					// Number of non-removed elements with exponent < _base_exponent.
	Iterator	_base_exponent_interval;	// Leftmost interval with exponent >= _base_exponent.

	Bigint	total_weight_mantissa() const { return _total_weight; }
	int		total_weight_exponent() const { return _base_exponent; }

	// The weight of item with exponent e is defined to be max(1, 2^(e - _base_exponent)).
	// _base_exponent is adjusted to keep _total_weight from overflowing.
	inline Bigint weight_of_exponent(Exponent e) const {
		e -= _base_exponent;
		if (e <= 0) return 1;
		return two_to_the(e);
	}

	// Holds info related to the element with a particular id.
	struct Element {
		Interval*	_interval;				// Points to the _storage interval I am in (NULL if removed).
		Element**	_back_ptr;				// Points to the pointer to me in _storage.

		Element():
			_interval(NULL),
			_back_ptr(NULL) { }
	};

	// The id associated with an element is only needed once the element is sampled.
	// The id is implicit in the location in the _elements array.
	int element_id(const Element* e) const { return e - _elements; }

	struct Interval {
		const Exponent	_exponent;			// Elements pointed to by pointers *_lo ... *_hi
		Element**		_lo;				// are the ones with this particular _exponent.
		Element**		_hi;
		Iterator		_iterator;			// The iterator for this interval in _Sampler::_intervals.
											// (STL linked list iterators are stable unless node is deleted.)

		// constructor
		Interval(Exponent exponent, Element** lo) : _exponent(exponent), _lo(lo), _hi(lo-1) {}

		// Relocate an element* in _storage to a new spot, updating the element's back pointer.
		inline static void move(Element* e, Element** where) {
			*where = e;
			e->_back_ptr = where;
		}
		// Exchange the location of two Element* 's in _storage, updating their back pointers.
		inline static void swap(Element** w1, Element **w2) {
			Element& e1(**w1);
			Element& e2(**w2);
			move(&e1, w2);
			move(&e2, w1);
		}
		// Move an Element* into this interval (must separately update _lo or _hi).
		inline void insert(Element* e, Element** where) {
			assert(*where == NULL);
			e->_interval = this;
			move(e, where);
		}
		// Remove an Element* (must separately update _lo or _hi).
		inline void remove(Element** where) {
			Element& e(**where);
			assert(e._interval == this  &&  e._back_ptr == where);
			*where = NULL;
			e._interval = NULL;
			e._back_ptr = NULL;
		}
		// Shift this _interval's pointers in _storage.
		// (For compacting after many items are removed.)
		// See _Sampler::compactify_intervals_to_left
		void shift_storage_right(Element **new_hi) {
			int			delta	= new_hi - _hi;
			Element**	new_lo	= _lo + delta;
			Element**	from	= _lo;
			Element**	to		= new_hi;
			assert(delta >= 0);

			// shift the pointers to the right by delta
			while (from < new_lo  &&  to >= new_lo) {
				assert(*to == NULL);		// overwrite only empty pointers
				move(*from, to);
				*from = NULL;
				++from;
				--to;
			}
			_lo = new_lo;
			_hi = new_hi;
		}

		inline int	size()	const	{ return _hi - _lo + 1; }
		inline bool	empty()	const	{ return size() == 0; }
	};

	Iterator create_first_interval(Exponent exponent) {
		assert(_intervals.empty());
		Interval i(exponent, _storage);
		Iterator it = _intervals.insert(_intervals.end(), i);
		it->_iterator = it;
		return it;
	}
	Iterator get_or_make_successor(Iterator it) {
		Exponent e = it->_exponent + 1;
		Iterator next = it;  ++next;
		if (next != _intervals.end()  &&  e == it->_exponent)		return next;
		Interval i2(e, it->_hi + 1);
		next = _intervals.insert(next, i2);  // insert i2 before it
		next->_iterator = next;
		return next;
	}
	// Insert element ptr into an interval.  Expand the interval
	// and update _Sampler fields accordingly.
	// Insert at lo end:
	void insert_lo(Iterator it, Element* e) {
		it->insert(e, --it->_lo);
		_total_weight += weight_of_exponent(it->_exponent);
		if (it->_exponent < _base_exponent)  _base_rank += 1;

		maintain_base_exponent();
	}
	// Insert at hi end:
	void insert_hi(Iterator it, Element* e) {
		it->insert(e, ++it->_hi);
		_total_weight += weight_of_exponent(it->_exponent);
		if (it->_exponent < _base_exponent)  _base_rank += 1;

		maintain_base_exponent();
	}
	// Remove an element ptr from an interval,
	// freeing the array location at the high end.
	void remove_hi(Iterator it, Element* elt) {
		// swap the element ptr with the high end one
		if (elt->_back_ptr != it->_hi)
			it->swap(elt->_back_ptr, it->_hi);

		// remove the element and adjust _hi
		it->remove(elt->_back_ptr);
		it->_hi -= 1;

		// maintain _Sampler invariants
		_total_weight -= weight_of_exponent(it->_exponent);
		if (it->_exponent < _base_exponent)  _base_rank -= 1;

		// free up interval if empty
		if (it->empty())  {
			if (it == _base_exponent_interval) ++ _base_exponent_interval;
			_intervals.erase(it);
		}
	}

	// return a uniformly random Bigint.
	// (assumes rand() returns sizeof(RAND_MAX) random bits.)
	static Bigint big_random() {
		const static int RAND_BITS = (8*sizeof(RAND_MAX) - 1);
		assert(RAND_MAX == ls_bits(RAND_BITS));

		Bigint r = 0;
		for (unsigned int b = 0;  b < 8*sizeof(Bigint);  b += RAND_BITS)
			r = (r << RAND_BITS) + rand();
		return r;
	}

	inline static Bigint two_to_the(int e)	{  return Bigint(1) << e;  }
	inline static Bigint ls_bits(int e)		{  return two_to_the(e) - Bigint(1);  }

	inline void maintain_base_exponent() {
		// _total_weight >= half max value?  most-significant bit set?
		while (_total_weight & ls_bits(8*sizeof(Bigint)))
			shift_base_exponent_right();
	}

	// Increase the _base_exponent,
	// maintaining the _total_weight and _base_rank invariants.
	// Increase it to the minimum existing element exponent
	// that is greater than the current _base_exponent.
	// This might not increase _base_rank.
	void shift_base_exponent_right() {
		Iterator	new_base_exponent_interval	= _base_exponent_interval;
		int			new_base_rank				= _base_rank;

		if (_base_exponent >= _base_exponent_interval->_exponent)  {
			// jump _base_exponent to next interval
			++ new_base_exponent_interval;
			new_base_rank += _base_exponent_interval->size();
			_total_weight -= weight_of_exponent(_base_exponent_interval->_exponent);
			assert(new_base_exponent_interval != _intervals.end());
		}
		// else interval stays same but _base_exponent
		// increases to the interval's exponent

		Exponent new_base_exponent = new_base_exponent_interval->_exponent;

		// Adjust _total_weight in O(1) time.
		// Subtract off weight of elements less than _base_exponent.
		// Divide remaining weight by 2^(increase in _base_exponent).
		// (The new total weight of the remaining elements.)
		// Add back in weight for elements less than _base_exponent.
		_total_weight -= _base_rank;
		assert((_total_weight & ls_bits(new_base_exponent - _base_exponent)) == 0);
		_total_weight >>= new_base_exponent - _base_exponent;
		_total_weight += new_base_rank;

		_base_rank = new_base_rank;
		_base_exponent = new_base_exponent;
		_base_exponent_interval = new_base_exponent_interval;
	}

	// Remove NULLS between all intervals to the left of this one
	// by shifting these intervals maximally to the right.
	void compactify_intervals_to_left(Iterator i)  {
		++i;
		Element** new_hi;
		if (i == _intervals.end())	new_hi = & _storage[_size-1];
		else 						new_hi = i->_lo - 1;
		do {
			--i;
			i->shift_storage_right(new_hi);
		} while (i != _intervals.begin());
	}

public:
	int size() const { return _size; }

	_Sampler(int *initial_exponents, int n) {
		assert(n > 0);

		_size = n;
		_total_weight = 0;
		_elements = new Element[n];
		_storage = new Element*[n];
		// _base_exponent set later
		// _base_rank set later

		assert(_elements && _storage);

		// set up _elements and _storage
		{
			std::pair<int, Id> sorted_exponents[n];

			for (int i = 0;  i < n;  ++i) {
				sorted_exponents[i].first = initial_exponents[i];
				sorted_exponents[i].second = i;
			}
			sort(sorted_exponents, sorted_exponents+n);

			Exponent min_exponent = Exponent(sorted_exponents[0].first);

			Iterator it = create_first_interval(min_exponent);

			// start with _base_exponent as small as possible
			_base_exponent = min_exponent;
			_base_rank = 0;
			_base_exponent_interval = it;

			for (int i = 0;  i < n;  ++i) {
				Exponent	e	= sorted_exponents[i].first;
				Id			id	= sorted_exponents[i].second;

				if (e != it->_exponent)  it = get_or_make_successor(it);
				insert_hi(it, &_elements[id]);
			}
		}

		// check work
		for (int i = 0;  i < n-1;  ++i)
			assert(_storage[i]->_interval->_exponent
					< _storage[i+1]->_interval->_exponent);

		for (int id = 0;  id < n;  ++id) {
			const Element& elt(_elements[id]);
			assert(*elt._back_ptr == &elt);
			assert(elt._interval->_lo <= elt._back_ptr);
			assert(elt._interval->_hi >= elt._back_ptr);
		}

		assert(_intervals.begin()->_lo == _storage);
		assert((--_intervals.end())->_hi == &_storage[_size-1]);
		Iterator prev = _intervals.begin();
		Iterator next = prev;  ++next;
		for (;  next != _intervals.end();   ++next, ++prev) {
			assert(next->_lo == prev->_hi + 1);
			assert(!prev->empty());
		}
	}

	void dump() const {
		std::cout << "size = " << _size
			<< ", base_exponent = " << _base_exponent
			<< ", base_rank = " << int(_base_rank)
			<< ", total_weight = " << _total_weight
			<< std::endl;
		for (unsigned rank = 0;  rank < _size;  ++rank) {
			const Element& elt(*_storage[rank]);
			const Interval* i = elt._interval;
			std::cout << "rank=" << int(rank)
				<< ", id=" << element_id(&elt)
				<< ", interval=" << &(*i);
			if (i) {
				std::cout << " (lo=" << i->_lo
				<< ", hi=" << i->_hi
				<< ", exponent=" << i->_exponent
				<< ", weight=" << weight_of_exponent(i->_exponent) << ")";
			}
			std::cout << std::endl;
		}
	}

	void remove(unsigned int id)	{
		assert(id <= _size);
		Element& elt(_elements[id]);
		assert(elt._interval);
		Interval* i(elt._interval);
		remove_hi(i->_iterator, &elt);
	}

	void increment_exponent(unsigned int id) {
		assert(id <= _size);
		Element &elt(_elements[id]);
		assert(elt._interval);
		Iterator it = elt._interval->_iterator;
		Iterator next = get_or_make_successor(it);

		remove_hi(it, &elt);  // may delete it->_interval!
		insert_lo(next, &elt);
	}

	int sample() {
		static const Exponent RAND_BITS(8*sizeof(RAND_MAX) - 1);
		assert(RAND_MAX == (((unsigned int) 1) << RAND_BITS) - 1);

		assert(_total_weight > 0);

		Bigint r = big_random() % _total_weight;

		for (Reverse_iterator i = _intervals.rbegin();  i != _intervals.rend();  ++i)  {
			if (i->_exponent >= _base_exponent) {
				Bigint element_weight = weight_of_exponent(i->_exponent);
				Bigint interval_weight = i->size() * element_weight;
				if (r < interval_weight)
					return element_id(i->_lo[r/element_weight]);
				else
					r -= interval_weight;
			} else {	// stop at first interval < _base_exponent
				assert(r < (unsigned int) _base_rank); // total weight in remaining intervals = # elements

				Element** leftmost_lo = _intervals.begin()->_lo;
				int n_slots = (i->_hi - leftmost_lo) + 1;

				// compactify if necessary
				if (_base_rank < n_slots / 2) {
					compactify_intervals_to_left(i->_iterator);
					leftmost_lo = _intervals.begin()->_lo;
					n_slots = (i->_hi - leftmost_lo) + 1;
					assert(_base_rank == n_slots);
				}
				assert(_base_rank > 0);

				// choose id from random non-empty rank in [leftmost_lo.. _hi(i)]
				Element* chosen;
				do
					chosen = leftmost_lo[random() % n_slots];
				while (chosen == NULL);

				// rejection method
				Exponent e = _base_exponent - chosen->_interval->_exponent;
				// return element with probability 1/2^e.  e can be large
				assert(e >= 0);
				if (e == 0)  return element_id(chosen);
				while (e > RAND_BITS) {
					if (rand() != 0) return -1;
					e -= RAND_BITS;
				}
				if (rand() & ls_bits(e) == 0) return element_id(chosen);
				return -1;
			}
		}
		assert(false);
		return -1;
	}
	~_Sampler() {
	}
};

Sampler* Sampler::create(int *initial_exponents, int n) {
	return new _Sampler(initial_exponents, n);
}

