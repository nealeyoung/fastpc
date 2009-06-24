/*
 * sampler.h
 *
 *  Created on: May 23, 2009
 *      Author: neal
 */

#ifndef SAMPLER_H_
#define SAMPLER_H_

#define inline


/*** made class non-virtual for performance
 *  Interface described below is correct.
 *  See later in the file for actual declaration.
 *
class Sampler {
public:
	// Create a Sampler containing the n integers in initial_exponents[0..n-1].
	// The integer initial_exponents[i] is associated with id i.
	// (The array initial_exponents is used only for initialization, not for storage.)
	// right now k is assumed > 0.
	static Sampler* create				(int k, int n, int *initial_exponents = NULL);

	// Increment the integer (thought of as an exponent) associated with id i.
	virtual void 	increment_exponent	(unsigned int id)				= 0;

	// Decrement the integer (thought of as an exponent) associated with id i.
	virtual void 	decrement_exponent	(unsigned int id)				= 0;

	// Decrease the integer (thought of as an exponent) associated with id i.
	// Current implementation may be slow (?).
	// Needed when covering constraint deleted (row maxes can decrease).
	virtual void 	decrease_exponent	(unsigned int id, int new_exponent)	= 0;

	// virtual int		get_exponent		(unsigned int id)				= 0;

	// Return a random id, or -1, where the probability of selecting any id
	// is proportional to the id weight:  2^(integer associated with that id / k).
	virtual int 	sample				()								= 0;

	// Return an upper bound mantissa * 2^exponent on the sum of the weights.
	// In sample(), above, the probability of returning an id exactly equals
	// 2^(integer associated with that id / k) / (mantissa * 2^exponent)
	// The probability of returning -1 is the remaining probability.
	virtual void	total_weight		(int* mantissa, int* exponent)	= 0; // const

	// Remove an integer from the set.
	virtual void	remove				(unsigned int id)				= 0;

	virtual int		n_remaining			()						const	= 0;
	virtual bool	empty				()						const	= 0;

	// For debugging...
	virtual void	dump				()						const	= 0;

	virtual 		~Sampler			() 								{};
};

**/

/*
 * pasted in from sampler.cc
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
#include <cmath>  // for ldexp

namespace _neal_sampler_ {

// #include "sampler.h"

class _Sampler1 {  // sampler for k = 1 (i.e., base 2)
private:
	typedef unsigned int		Weight;
	typedef int 				Exponent;
	typedef int 				Id;
	class						Element;
	class						Interval;

	inline static int			count_leading_zeros(Weight w)   {  return __builtin_clz(w);  }
	// see http://gcc.gnu.org/onlinedocs/gcc-4.3.3/gcc/Other-Builtins.html for clz
	// Ñ Built-in Function: int __builtin_clz (unsigned int x)
	// Returns the number of leading 0-bits in x, starting at the most significant bit position. If x is 0, the result is undefined.
	// Ñ Built-in Function: int __builtin_clzl (unsigned long)
	// Similar to __builtin_clz, except the argument type is unsigned long.

	inline static Weight 		two_to_the(int e)				{  return Weight(1) << e;  }
	inline static Weight 		ls_bits(int e)					{  return two_to_the(e) - Weight(1);  }

	typedef std::list<Interval>			List;
	typedef List::iterator				Iterator;
	typedef List::reverse_iterator		Reverse_iterator;

	int			_max_id;
	unsigned	_n_remaining;
	Element*	_elements;					// _elements[id] always holds element with given id.
	Element**	_storage;					// _storage[0.._size-1] holds unique pointers to non-removed elements,
											// sorted by increasing element exponent (and some NULLs between intervals).
	List		_intervals;					// In _storage, for each maximal contiguous sequence of pointers
											// to elements with equal exponents, there is an interval structure.
											// This is a list of them ordered by increasing exponent.
	Exponent	_base_exponent;				// See weight_of_exponent().
	Weight		_cached_weight_to_right;	// Cached sum of weights of elements with exponent > _base_exponent; see _weight_of_exponent().
	int			_cached_n_to_left;			// Number of elements with exponent <= _base_exponent.
	bool		_cached_weight_valid;		// Is the cached sum up to date?
	Iterator	_cached_interval;			// leftmost interval with exponent > _base_exponent

	// The weight of item with exponent e is defined to be max(1, 2^(e - _base_exponent)).
	inline Weight weight_of_exponent(Exponent e) const {
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
	// (store explicitly to save the integer division (!))
	//inline int element_id(const Element* e) const { return e->_id; }

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

		inline unsigned	size()	const	{ return _hi - _lo + 1; }
		inline bool		empty()	const	{ return size() == unsigned(0); }
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
		if (next != _intervals.end()  &&  e == next->_exponent)		return next;
		Interval i2(e, it->_hi + 1);
		next = _intervals.insert(next, i2);  // insert i2 before next
		next->_iterator = next;
		return next;
	}
	Iterator get_or_make_predecessor(Iterator it) {
		Exponent e = it->_exponent - 1;
		Iterator prev = it;
		if (it != _intervals.begin()  &&  e == (--prev)->_exponent)		return prev;
		Interval i2(e, it->_lo);
		prev = _intervals.insert(it, i2);  // insert i2 before it
		prev->_iterator = prev;
		return prev;
	}
	// Insert element ptr into an interval.  Expand the interval
	// and update _Sampler fields accordingly.
	// Insert at lo end:
	inline void insert_lo(Iterator it, Element* e) { it->insert(e, --it->_lo);  _cached_weight_valid = false; }
	// Insert at hi end:
	inline void insert_hi(Iterator it, Element* e) { it->insert(e, ++it->_hi);	_cached_weight_valid = false; }

	// Remove an element ptr from an interval,
	// freeing the array location at the high end.
	inline void remove_hi(Element* elt) {
		Iterator it(elt->_interval->_iterator);
		// swap the element ptr with the high end one
		if (elt->_back_ptr != it->_hi)			it->swap(elt->_back_ptr, it->_hi);

		// remove the element and adjust _hi, free up interval if empty
		it->remove(elt->_back_ptr);
		it->_hi -= 1;
		if (it->empty())	_intervals.erase(it);
		_cached_weight_valid = false;
	}
	// Remove an element ptr from an interval,
	// freeing the array location at the low end.
	inline void remove_lo(Element* elt) {
		Iterator it(elt->_interval->_iterator);
		// swap the element ptr with the lo end one
		if (elt->_back_ptr != it->_lo)			it->swap(elt->_back_ptr, it->_lo);

		// remove the element and adjust _lo, free up interval if empty
		it->remove(elt->_back_ptr);
		it->_lo += 1;
		if (it->empty())	_intervals.erase(it);
		_cached_weight_valid = false;
	}

	// return a uniformly random Weight in 0..max .
	// (assumes rand() returns sizeof(RAND_MAX) random bits.)
	static Weight random_weight(Weight max) {
		const static int RAND_BITS = 8*sizeof(RAND_MAX) - 1; // number of random bits returned by rand()
		assert(RAND_MAX == ls_bits(RAND_BITS));
		assert(Weight(0) <= max  &&  max < Weight(RAND_MAX));

		const Weight bucket_size = RAND_MAX / (max+1);  // RAND_MAX / bucket_size >= max+1
		Weight r;
		do {
			r = rand() / bucket_size;
		} while (r > max);
		return r;
	}

	// Remove NULLS between all intervals to the left of this one
	// by shifting these intervals maximally to the right.
	void compactify_intervals_to_left(Iterator i)  {
		++i;
		Element** new_hi;
		if (i == _intervals.end())	new_hi = & _storage[_max_id-1];
		else 						new_hi = i->_lo - 1;
		do {
			--i;
			i->shift_storage_right(new_hi);
			new_hi = i->_lo - 1;
		} while (i != _intervals.begin());
	}

	void update_cached_weight() {
		if (_cached_weight_valid)  return;

		_cached_weight_valid		= true;
		_cached_interval			= _intervals.end();

		if (_cached_interval == _intervals.begin())	{					// no intervals
			_cached_weight_to_right		= 0;
			_cached_n_to_left 			= _n_remaining;
			_base_exponent 				= 0;
			return;
		}
		-- _cached_interval;											// rightmost interval
		_base_exponent				=	_cached_interval->_exponent - 1;
		_cached_weight_to_right		=	_cached_interval->size();
		_cached_n_to_left			=	_n_remaining - _cached_interval->size();
		assert(_cached_n_to_left == 0 || _cached_interval != _intervals.begin());

		while (_cached_weight_to_right < Weight(_cached_n_to_left)) {
			-- _cached_interval;
			int to_shift			= _base_exponent - (_cached_interval->_exponent-1);
			assert(to_shift > 0);
			int zeros_left			= count_leading_zeros(_cached_weight_to_right | _cached_interval->size());

			if (to_shift <= zeros_left - 2) {
				_base_exponent			=	_cached_interval->_exponent - 1;
				_cached_weight_to_right	<<=	to_shift;
				_cached_weight_to_right	+=	_cached_interval->size() * 2; // 2 = weight_of_exponent(_cached_interval->_exponent);
				_cached_n_to_left 		-=	_cached_interval->size();     // beware overflow
				assert(_cached_weight_to_right < ((Weight(1) << (8*sizeof(Weight)-1))));
				assert(_cached_weight_to_right < RAND_MAX);
			} else {
				to_shift = zeros_left - 1;
				assert(to_shift > 0);
				++ _cached_interval;
				_base_exponent			-=	to_shift;
				_cached_weight_to_right	<<=	to_shift;
				assert(_cached_weight_to_right >= Weight(_cached_n_to_left));
				break;
			}
		}
	}
public:
	_Sampler1() : _max_id(-1), _n_remaining(0), _elements(NULL), _storage(NULL) {}

	void initialize(int n, int *initial_exponents) {
		assert(n > 0);
		assert(_max_id == -1);

		_max_id					= n-1;
		_n_remaining			= n;
		_elements				= new Element[n];
		_storage				= new Element*[n];
		_cached_weight_valid	= false;

		assert(_elements && _storage);

		// set up _elements and _storage
		{
			std::pair<int, Id> sorted_exponents[n];
			assert(sorted_exponents);

			for (int i = 0;  i < n;  ++i) _storage[i] = NULL;

			for (int id = 0;  id < n;  ++id) {
				//_elements[id]._id = id;

				sorted_exponents[id].first = initial_exponents[id];
				sorted_exponents[id].second = id;
			}
			sort(sorted_exponents, sorted_exponents+n);

			Exponent min_exponent = Exponent(sorted_exponents[0].first);
			Iterator it = create_first_interval(min_exponent);

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
					<= _storage[i+1]->_interval->_exponent);

		for (int id = 0;  id < n;  ++id) {
			const Element& elt(_elements[id]);
			assert(*elt._back_ptr == &elt);
			assert(elt._interval->_lo <= elt._back_ptr);
			assert(elt._interval->_hi >= elt._back_ptr);
		}

		assert(_intervals.begin()->_lo == _storage);
		assert((--_intervals.end())->_hi == &_storage[_max_id]);
		Iterator prev = _intervals.begin();
		Iterator next = prev;  ++next;
		for (;  next != _intervals.end();   ++next, ++prev) {
			assert(next->_lo == prev->_hi + 1);
			assert(!prev->empty());
		}
	}

	void dump() const {
		std::cout	<< "n_remaining = "					<< _n_remaining
					<< ", cached_weight_to_right = "	<< _cached_weight_to_right
					<< ", cached_n_to_left = "			<< _cached_n_to_left
					<< ", cached_weight_valid = "		<< _cached_weight_valid
					<< ", base_exponent = "				<< _base_exponent
					<< std::endl;

		for (int rank = 0;  rank <= _max_id;  ++rank) {
			const Element*	elt(_storage[rank]);
			if (! elt) continue;
			const Interval*	i = elt->_interval;
			std::cout 	<< "rank=" 			<< int(rank)
						<< ", id=" 			<< element_id(elt)
						<< ", interval=" 	<< &(*i);
			if (i) {
				std::cout
						<< " (lo="		<< i->_lo
						<< ", hi=" 		<< i->_hi
						<< ", exponent="<< i->_exponent
						<< ", weight="	<< weight_of_exponent(i->_exponent) << ")";
			}
			std::cout << std::endl;
		}
	}

	void remove(unsigned int id)	{		assert(int(id) <= _max_id);
		Element& elt(_elements[id]);		assert(elt._interval);
		remove_hi(&elt);
		--_n_remaining;
	}
	int  n_remaining	() const { return _n_remaining; }
	bool empty			() const { return _n_remaining == 0; }

	void increment_exponent(unsigned int id) {		assert(int(id) <= _max_id);
		Element &elt(_elements[id]);				assert(elt._interval);
		Iterator it = elt._interval->_iterator;
		Iterator next = get_or_make_successor(it);

		remove_hi(&elt);  // may delete it->_interval!
		insert_lo(next, &elt);
	}
	inline void decrement_exponent(unsigned int id) {		assert(int(id) <= _max_id);
		Element &elt(_elements[id]);				assert(elt._interval);
		Iterator it = elt._interval->_iterator;
		Iterator next = get_or_make_predecessor(it);

		remove_lo(&elt);  // may delete it->_interval!
		insert_hi(next, &elt);
	}

	// TODO: speed up decrease_exponent?
	void decrease_exponent(unsigned int id, int new_exponent) {
													assert(int(id) <= _max_id);
		Element &elt(_elements[id]);				assert(elt._interval);
		assert(new_exponent <= elt._interval->_exponent);
		while(new_exponent < elt._interval->_exponent)
			decrement_exponent(id);
	}

	/*
	int get_exponent(unsigned int id) {
		assert(id <= unsigned(_max_id));
		assert(_elements[id]._interval);
		return _elements[id]._interval->_exponent;
	}
	*/

	inline int sample() {
		static const Exponent RAND_BITS(8*sizeof(RAND_MAX) - 1);
		assert(RAND_MAX == ls_bits(RAND_BITS));

		if (! _cached_weight_valid) update_cached_weight();
		assert(_cached_weight_to_right > 0);

		Weight r = random_weight(_cached_weight_to_right + _cached_n_to_left - 1);

		if (r < _cached_weight_to_right) {
			for (Reverse_iterator i = _intervals.rbegin();  i != _intervals.rend();  ++i)  {
				assert(i->_exponent >= _base_exponent);
				Weight element_weight	= weight_of_exponent(i->_exponent);
				Weight interval_weight	= i->size() * element_weight;
				if (r < interval_weight)
					return element_id(i->_lo[r/element_weight]);
				else
					r -= interval_weight;
			}
			assert(false);
		} else {
			r						-= _cached_weight_to_right;

			assert(_cached_interval != _intervals.begin());

			Iterator	i			= _cached_interval;  -- i;
			Element**	leftmost_lo = _intervals.begin()->_lo;
			int			n_slots		= (i->_hi - leftmost_lo) + 1;

			// compactify if necessary
			if (_cached_n_to_left < n_slots / 2) {
				compactify_intervals_to_left(i->_iterator);
				leftmost_lo			= _intervals.begin()->_lo;
				n_slots 			= (i->_hi - leftmost_lo) + 1;
				assert(_cached_n_to_left == n_slots);
			}

			// choose Element* from random non-NULL pointer in [leftmost_lo.. i->_hi()]
			Element*	chosen;
			do
				chosen = leftmost_lo[random_weight(n_slots-1)];
			while (chosen == NULL);

			// accept it (don't reject) with probability...
			//   true weight / 1
			//     = 2^(_exponent - _base_exponent)
			Exponent	e			= _base_exponent - chosen->_interval->_exponent;
			assert(e >= 0);
			while (e > RAND_BITS) {
				if (rand() != 0) return -1;
				e -= RAND_BITS;
			}
			if (e > 0  &&  (rand() & ls_bits(e)) != 0) return -1;
			return element_id(chosen);
		}
		assert(false);
		return -1;
	}
	void total_weight(int* mantissa, int* exponent) {
		update_cached_weight();
		*mantissa = _cached_weight_to_right + _cached_n_to_left;
		*exponent = _base_exponent;
	}
	~_Sampler1() {
		delete[] _elements;
		delete[] _storage;
	}
	friend class _Sampler;
};

// class _Sampler : public Sampler {
class _Sampler {
private:
	class _Sampler1	_s1;
	int				_k;
	int*			_exponent_gaps;  // distance to next multiple of k:  k*ceil(exponent/k) - exponent
	int*			_powers;		 // _powers[i] = RAND_MAX * 2^(-i/_k)

	inline void split_exponent(int exp, int* next_power_of_two, int* gap) {
		if (exp > 0)
			*next_power_of_two = 1 + (exp - 1) / _k;
			// e.g. if k = 10
			//      1 + (10 - 1) / 10 = 1 +  9/10 = 1
			// 		1 + (11 - 1) / 10 = 1 + 10/10 = 2
		else // int division with negative numerator can round up!
			*next_power_of_two = - ((- exp) / _k);
			// e.g. if k = 10
			//		- ( -  -9) / 10 = -  9/10 =  0
			//      - ( - -10) / 10 = - 10/10 = -1
		*gap = (*next_power_of_two)*_k - exp;
		assert(0 <= *gap  &&  *gap < _k);
	}
	inline int assemble_exponent(int next_power_of_two, int gap) {
		return next_power_of_two*_k - gap;
	}

public:
	_Sampler(int k, int n, int *initial_exponents = NULL) : _k(k) {
		assert(n > 0);
		assert(k > 0);

		int next_powers_of_two[n];
		assert(next_powers_of_two);

		_exponent_gaps = new int[n];
		assert(_exponent_gaps);

		if (initial_exponents)
			for (int i = 0;  i < n;  ++i)
				split_exponent(initial_exponents[i],
						&next_powers_of_two[i],
						&_exponent_gaps[i]);
		else
			for (int i = 0;  i < n;  ++i)
				split_exponent(0,
						&next_powers_of_two[i],
						&_exponent_gaps[i]);

		_s1.initialize(n, next_powers_of_two);

		_powers = new int[k];
		assert(_powers);
		for (int i = 0;  i < k;  ++i)
			_powers[i] = int(RAND_MAX * pow(2.0, -double(i)/double(k)));
	}
	inline void increment_exponent(unsigned int id) {
		// increasing exponent decreases the gap
		if (--_exponent_gaps[id] == -1) {
			_exponent_gaps[id] = _k-1;
			_s1.increment_exponent(id);
		}
	}
	inline void decrement_exponent(unsigned int id) {
		if (++_exponent_gaps[id] == _k) {
			_exponent_gaps[id] = 0;
			_s1.decrement_exponent(id);
		}
	}
	void decrease_exponent(unsigned int id, int new_exponent) {
		int new_exp2;
		split_exponent(new_exponent, &new_exp2, &_exponent_gaps[id]);
		_s1.decrease_exponent(id, new_exp2);
	}
	/*
	inline int get_exponent(unsigned int id) {
		return assemble_exponent(_s1.get_exponent(id), _exponent_gaps[id]);
	}
	*/
	int sample() {
		int id = _s1.sample();
		if (id == -1) return -1;
		// accept (don't reject) with probability 1/2^(_exponent_gap[id]/k)
		if (rand() <= _powers[_exponent_gaps[id]]) return id;
		return -1;
	}

	void total_weight (int* mantissa, int* exponent) {
		_s1.total_weight(mantissa, exponent);
	}

	void remove			(unsigned int id) 	{ _s1.remove(id); }
	int	 n_remaining	() const			{ return _s1.n_remaining(); }
	bool empty			() const			{ return _s1.empty(); }
	void dump 			() const 			{ _s1.dump(); }
	~_Sampler			() 					{ delete[] _exponent_gaps; }

	static _Sampler* create(int k, int n, int* initial_exponents = NULL) {
		return new _Sampler(k, n, initial_exponents);
	}
};

} // end namespace

typedef _neal_sampler_::_Sampler Sampler;



#endif /* SAMPLER_H_ */
