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
	Bigint		_total_weight;
	Element*	_elements;
	List		_intervals;
	Element**	_interval_storage;

	Exponent	_base_exponent;
	int			_base_rank;			     // number of non-removed elements with exponent < _base_exponent
	Iterator	_base_exponent_interval; // leftmost interval with exponent >= _base_exponent

	Bigint	total_weight_mantissa() const { return _total_weight; }
	int		total_weight_exponent() const { return _base_exponent; }

	struct Element {
		Interval*	_interval;
		Element**	_where_in_interval_storage;

		Element():
			_interval(NULL),
			_where_in_interval_storage(NULL) { }
	};

	int element_id(const Element* e) const { return e - _elements; }

	struct Interval {
		const Exponent	_exponent;
		Element**		_lo;
		Element**		_hi;
		Iterator		_iterator;

		Interval(Exponent exponent, Element** lo) : _exponent(exponent), _lo(lo), _hi(lo-1) {}

		inline void move(Element* e, Element** where) {
			*where = e;
			e->_where_in_interval_storage = where;
		}
		inline void swap(Element** w1, Element **w2) {
			Element& e1(**w1);
			Element& e2(**w2);
			move(&e1, w2);
			move(&e2, w1);
		}
		inline void insert(Element* e, Element** where) {
			assert(*where == NULL);
			e->_interval = this;
			move(e, where);
		}
		inline void remove(Element** where) {
			Element& e(**where);
			assert(e._interval == this  &&  e._where_in_interval_storage == where);
			*where = NULL;
			e._interval = NULL;
			e._where_in_interval_storage = NULL;
		}

		inline int	size()	const	{ return _hi - _lo + 1; }
		inline bool	empty()	const	{ return size() == 0; }
	};

	Iterator create_first_interval(Exponent exponent) {
		assert(_intervals.empty());
		Interval i(exponent, _interval_storage);
		Iterator it = _intervals.insert(_intervals.end(), i);
		it->_iterator = it;
		return it;
	}

	Iterator add_interval_to_right(Iterator i) {
		Interval i2(i->_exponent+1, i->_hi+1);
		Iterator next = i;  ++next;
		next = _intervals.insert(next, i2);
		next->_iterator = next;
		return next;
	}
	void insert_lo(Iterator it, Element* e) {
		it->insert(e, --it->_lo);
		_total_weight += weight_of_exponent(it->_exponent);
		if (it->_exponent < _base_exponent)  _base_rank += 1;
	}
	void insert_hi(Iterator it, Element* e) {
		it->insert(e, ++it->_hi);
		_total_weight += weight_of_exponent(it->_exponent);
		if (it->_exponent < _base_exponent)  _base_rank += 1;
	}
	void remove_hi(Iterator it, Element* elt) {
		if (elt->_where_in_interval_storage != it->_hi)
			it->swap(elt->_where_in_interval_storage, it->_hi);
		it->remove(elt->_where_in_interval_storage);
		it->_hi -= 1;
		_total_weight -= weight_of_exponent(it->_exponent);
		if (it->_exponent < _base_exponent)  _base_rank -= 1;
		if (it->empty())  {
			if (it == _base_exponent_interval) ++ _base_exponent_interval;
			_intervals.erase(it);
		}
	}

	static Bigint big_random() {
		const static int RAND_BITS = (8*sizeof(RAND_MAX) - 1);
		assert(RAND_MAX == (((unsigned int) 1) << RAND_BITS) - 1);

		Bigint r = 0;
		for (unsigned int b = 0;  b < 8*sizeof(Bigint);  b += RAND_BITS)
			r = (r << RAND_BITS) + rand();
		return r;
	}

	inline static Bigint two_to_the(int e)	{  return Bigint(1) << e;  }
	inline static Bigint ls_bits(int e)		{  return two_to_the(e) - Bigint(1);  }

	inline Bigint weight_of_exponent(Exponent e) const {
		e -= _base_exponent;
		if (e <= 0) return 1;
		return two_to_the(e);
	}

	void shift_base_exponent_right() {
		Iterator	new_base_exponent_interval	= _base_exponent_interval;
		int			new_base_rank				= _base_rank;

		if (_base_exponent >= _base_exponent_interval->_exponent)  {
			// move _base_exponent to left of interval to right of _base_exponent_interval
			++ new_base_exponent_interval;
			new_base_rank += _base_exponent_interval->size();
			_total_weight -= weight_of_exponent(_base_exponent_interval->_exponent);
			assert(new_base_exponent_interval != _intervals.end());
		}
		Exponent new_base_exponent = new_base_exponent_interval->_exponent;

		_total_weight -= _base_rank;
		assert((_total_weight & ls_bits(new_base_exponent - _base_exponent)) == 0);
		_total_weight >>= new_base_exponent - _base_exponent;
		_total_weight += new_base_rank;

		_base_rank = new_base_rank;
		_base_exponent = new_base_exponent;
		_base_exponent_interval = new_base_exponent_interval;
	}

	void shift_interval_right(Interval* interval) {
		Iterator i(interval->_iterator);
		Element** new_hi;
		{
			Iterator i2   = i;  ++i2;
			if (i2 != _intervals.end())		new_hi = i2->_lo - 1;
			else    						new_hi = &_interval_storage[_size-1];
		}

		int			delta	= new_hi - i->_hi;
		Element**	new_lo	= i->_lo + delta;
		Element**	from	= i->_lo;
		Element**	to		= new_hi;

		while (from < new_lo  &&  to >= new_lo) {
			assert(*to == NULL);
			i->move(*from, to);
			*from = NULL;
			++from;
			--to;
		}
		i->_lo = new_lo;
		i->_hi = new_hi;
	}

	void compactify_intervals_to_left(Interval* interval)  {
		Iterator i(interval->_iterator);
		do {
			shift_interval_right(&(*i));
			if (i == _intervals.begin())  break;
			--i;
		} while (1);
	}

public:
	int size() const { return _size; }

	_Sampler(int *initial_exponents, int n) {
		assert(n > 0);

		_size = n;
		_total_weight = 0;
		_elements = new Element[n];
		_interval_storage = new Element*[n];
		// _base_exponent set later
		// _base_rank set later

		assert(_elements && _interval_storage);

		std::pair<int, Id> sorted_exponents[n];

		for (int i = 0;  i < n;  ++i) {
			sorted_exponents[i].first = initial_exponents[i];
			sorted_exponents[i].second = i;
		}
		sort(sorted_exponents, sorted_exponents+n);

		Exponent min_exponent = Exponent(sorted_exponents[0].first);

		Iterator it = create_first_interval(min_exponent);
		_base_exponent = min_exponent;
		_base_rank = 0;
		_base_exponent_interval = it;

		for (int i = 0;  i < n;  ++i) {
			Exponent	e	= sorted_exponents[i].first;
			Id			id	= sorted_exponents[i].second;
			Element* 	elt	= & _elements[id];

			if (e != it->_exponent)  it = add_interval_to_right(it);
			insert_hi(it, elt);
		}

		// check work
		for (int i = 0;  i < n-1;  ++i)
			assert(_interval_storage[i]->_interval->_exponent
					< _interval_storage[i+1]->_interval->_exponent);

		for (int id = 0;  id < n;  ++id) {
			const Element& elt(_elements[id]);
			assert(*elt._where_in_interval_storage == &elt);
			assert(elt._interval->_lo <= elt._where_in_interval_storage);
			assert(elt._interval->_hi >= elt._where_in_interval_storage);
		}

		assert(_intervals.begin()->_lo == _interval_storage);
		assert((--_intervals.end())->_hi == &_interval_storage[_size-1]);
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
			const Element& elt(*_interval_storage[rank]);
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
		Iterator i = elt._interval->_iterator;
		Exponent e = i->_exponent + 1;
		Iterator next = i;  ++next;

		if (next == _intervals.end()  ||  next->_exponent != e)
			next = add_interval_to_right(i);

		remove_hi(i, &elt);
		insert_lo(next, &elt);

		// _total_weight >= half max value?  most-significant bit set?
		while (_total_weight & ls_bits(8*sizeof(Bigint)))
			shift_base_exponent_right();
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
					compactify_intervals_to_left(&(*i));
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

