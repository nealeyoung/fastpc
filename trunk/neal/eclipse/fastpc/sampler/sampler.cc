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
	static const int EMPTY;

	typedef long long unsigned int Bigint;
	typedef int Exponent;

	struct Rank;
	struct Id;
	class Element;
	class Interval;

	friend class Interval;

	// Id and Rank types are ints, but with their own distinct type. This is
	// just so the compiler type-checking can catch improper mixing of the types,
	// and can do type-dependent function resolution. Not sure it's worth it though.
	struct Id {
		int value;

		Id(const int& i)	: value(i) {};
		Id()	 			: value(EMPTY) {};
		inline 				operator int() 			const	{ return value; }
		inline bool 	 	operator ==(Id i) 		const	{ return value == i.value; }
		inline bool 	 	operator <(Id i) 		const	{ return value <  i.value; }
		inline Id			operator ++() 					{ ++value; return *this; }

		inline Rank&			_rank_in   (_Sampler& s)		const	{ return s._rank(*this);    }
		inline Rank				_rank_in   (const _Sampler& s)	const	{ return s._rank(*this);    }
		inline Element&			_element_in(_Sampler& s)		const	{ return s._element(*this); }
		inline const Element& 	_element_in(const _Sampler& s)	const	{ return s._element(*this); }
	};
	struct Rank {
		int value;

		Rank(const int& i)	: value(i) {};
		Rank()	 			: value(EMPTY) {};
		inline 				operator int() 			const { return value; }
		inline Rank		 	operator + (int  i)		const { return value + i; }
		inline Rank		 	operator * (Rank i)		const { return value * i.value; }
		inline Rank		 	operator - (int  i) 	const { return value - i; }
		inline int			operator - (Rank i) 	const { return value - i.value; }
		inline bool 	 	operator < (Rank i) 	const { return value < i.value; }
		inline bool 	 	operator <=(Rank i) 	const { return value <= i.value; }
		inline bool 	 	operator > (Rank i) 	const { return value >  i.value; }
		inline bool 	 	operator ==(Rank i) 	const { return value == i.value; }
		inline Rank			operator +=(int i) 		{ value += i; 					return *this; }
		inline Rank			operator -=(int i)  	{ value -= i; 					return *this; }
		inline Rank			operator ++() 			{ ++value; 						return *this; }
		inline Rank			operator --() 			{ --value; 						return *this; }
		inline Rank			operator ++(int)		{ Rank copy = *this; ++value;	return copy; }
		inline Rank			operator --(int) 		{ Rank copy = *this; --value;	return copy; }

		inline Id&				_id_in     (_Sampler& s)		const	{ return s._id(*this); }
		inline Id				_id_in     (const _Sampler& s)	const	{ return s._id(*this); }
		inline Element& 		_element_in(_Sampler& s)		const	{ return s._element(*this); }
		inline const Element& 	_element_in(const _Sampler& s)	const	{ return s._element(*this); }
	};

	// used as member functions in contexts where sampler() is defined

	#define ID			_id_in(sampler())
	#define ELEMENT		_element_in(sampler())
	#define RANK		_rank_in(sampler())

	// utility functions

	Bigint big_random() const {
		const static int RAND_BITS = (8*sizeof(RAND_MAX) - 1);
		assert(RAND_MAX == (((unsigned int) 1) << RAND_BITS) - 1);

		Bigint r = 0;
		for (unsigned int b = 0;  b < 8*sizeof(Bigint);  b += RAND_BITS)
			r = (r << RAND_BITS) + rand();
		return r;
	}
	inline Bigint weight_of_exponent(Exponent e) const {
		e -= _base_exponent;
		if (e <= 0) return 1;
		return Bigint(1) << int(e);
	}

	// classes

	class Element {
	private:
		Exponent  _exponent;
		Interval* _interval;
		Id        _id;
		Rank      _rank;

	public:
		Element():
			_exponent(-1),
			_interval(NULL),
			_id(EMPTY),
			_rank(EMPTY) { }

		inline bool			removed()		const	{ return _interval == NULL; }
		inline Exponent& 	exponent() 				{ return _exponent; }
		inline Exponent 	exponent()		const	{ return _exponent; }
		inline Interval*& 	interval()				{ return _interval; }
		inline const Interval* interval()	const	{ return _interval; }
		inline _Sampler& 	sampler()  				{ return _interval->sampler(); }
		inline const _Sampler& 	sampler()  	const	{ return _interval->sampler(); }
		inline Id& 			id()       				{ return _id; }
		inline Id 			id()			const 	{ return _id; }
		inline Rank& 		rank()     				{ return _rank; }
		inline Rank 		rank()     		const	{ return _rank; }
		inline Bigint 		weight() 		const	{ return sampler().weight_of_exponent(exponent()); }
	};

	class Interval {
	public:
		typedef std::list<Interval>	List;
		typedef List::iterator		Iterator;

	private:
		_Sampler&		_sampler;
		const Exponent	_exponent;
		Rank			_lo;
		Rank			_hi;
		Iterator		_iterator;

		inline void set_rank(Element& e, Rank rank) {
			assert(e.removed());
			e.interval() = this;
			e.rank() = rank;
			rank.ID = e.id();
			total_weight() += e.weight();
			if (e.exponent() < sampler()._base_exponent)  sampler()._base_rank += 1;
		}
		inline void unset_rank(Element& e) {
			if (e.exponent() < sampler()._base_exponent)  sampler()._base_rank -= 1;
			total_weight() -= e.weight();
			e.rank().ID = EMPTY;
			e.rank() = EMPTY;
			e.interval() = NULL;
			if (empty()) remove();  // delete this interval if empty
		}
		inline void swap_ranks(Element& e1, Element& e2) {
			assert(e1.interval() == this);
			assert(e2.interval() == this);

			if (e1.id() == e2.id()) return;
			Rank r1 = e1.rank();
			Rank r2 = e2.rank();
			r1.ID = e2.id();		e2.rank() = r1;
			r2.ID = e1.id();		e1.rank() = r2;
		}
		void swap_ranks(Id id1, Id id2)			{ swap_ranks(id1.ELEMENT, id2.ELEMENT); }

		inline 	List&		intervals()				{ return sampler().intervals(); }
		inline 	Bigint&		total_weight()			{ return sampler().total_weight(); }
		inline 	Bigint		total_weight()	const	{ return sampler().total_weight(); }

		inline void remove() {
			assert(empty());
			if (this == sampler()._base_exponent_interval)
				sampler()._base_exponent_interval = interval_to_right(); // what if NULL?
			intervals().erase(iterator());  // deletes *this
		}

	public:
		Interval(_Sampler& sampler, Exponent exponent, Rank lo, Rank hi = EMPTY)
			: _sampler(sampler), _exponent(exponent), _lo(lo), _hi(hi)
		{ if (_hi == Rank(EMPTY)) _hi = _lo - 1; }

		inline 			_Sampler& sampler()  		{ return _sampler; }
		inline const	_Sampler& sampler() const	{ return _sampler; }
		inline Rank&	lo()      					{ return _lo; }
		inline Rank&	hi()      					{ return _hi; }
		inline Rank		lo() const 					{ return _lo; }
		inline Rank		hi() const 					{ return _hi; }
		inline Interval::Iterator& iterator()		{ return _iterator; }

		inline Exponent  exponent() const	{ return _exponent; }
		inline int       size()     const	{ return hi() - lo() + 1; }
		inline bool      empty()    const	{ return size() == 0; }
		inline Bigint    weight()   const	{ return Bigint(size()) * sampler().weight_of_exponent(exponent()); }

		inline Interval* interval_to_right() {
			Interval::Iterator it = iterator();
			++it;
			if (it == intervals().end())	return NULL;
			return &(*it);
		}

		inline Interval* interval_to_left()  {
			Interval::Iterator it = iterator();
			if (it == intervals().begin())	return NULL;
			--it;
			return &(*it);
		}

		void insert_element_right(Element& elt) { set_rank(elt, ++hi()); }
		void insert_element_right(Id id)		{ insert_element_right(id.ELEMENT); }

		void insert_element_left(Element& elt)	{ set_rank(elt, --lo()); }
		void insert_element_left(Id id)			{ insert_element_left(id.ELEMENT); }

		void remove_element_right(Element& elt) {
			swap_ranks(elt, hi().ELEMENT);
			hi() -= 1;
			unset_rank(elt);
		}
		void remove_element_right(Id id)
		{ remove_element_right(id.ELEMENT);	}

		void shift_interval_right() {
			Rank new_hi;
			{
				Interval* i2   = interval_to_right();
				if (i2) new_hi = i2->lo() - 1;
				else    new_hi = size() - 1;
			}

			Rank delta     = new_hi - hi();
			Rank new_lo    = lo() + delta;
			Rank from_rank = lo();
			Rank to_rank   = new_hi;

			while (from_rank < new_lo  &&  to_rank >= new_lo) {
				assert(to_rank.ID == Id(EMPTY));
				Id from_id     = from_rank.ID;
				from_id.RANK = to_rank;
				to_rank.ID   = from_id;
				from_rank.ID = EMPTY;
				++from_rank;
				--to_rank;
			}
			lo() = new_lo;
			hi() = new_hi;
		}

		void compactify_intervals_to_left()
		{ for(Interval* i = this;  i;  i = i->interval_to_left())  i->shift_interval_right(); }

		Interval* add_interval_to_right(Exponent exponent) {
			Interval i2(sampler(), exponent, hi()+1);
			Interval::Iterator it = iterator(); ++it;
			it = intervals().insert(it, i2);
			it->iterator() = it;
			return &(*it);
		}
	};

	int _size;
	Bigint _total_weight;
	Element* _elements;
	Id* _id_of_rank;
	Interval::List _intervals;
	Exponent _base_exponent;
	Rank _base_rank;			       // number of non-removed elements with exponent < _base_exponent
	Interval* _base_exponent_interval; // leftmost interval with exponent >= _base_exponent

	inline Id& 			_id(Rank rank)		{ return _id_of_rank[rank]; }
	inline Element&		_element(Id id)		{ return _elements[id]; }
	inline Element&		_element(Rank rank)	{ return _element(_id(rank)); }
	inline _Sampler& 	sampler()			{ return *this; }
	inline Rank&		_rank(Id id)		{ return _element(id).rank(); }
	inline Exponent&	_exponent(Id id)	{ return _element(id).exponent(); }
	inline Interval::List&	intervals()		{ return _intervals; }
	inline Bigint&			total_weight()	{ return _total_weight; }

	inline Id 				_id(Rank rank)		const	{ return _id_of_rank[rank]; }
	inline const Element&	_element(Id id) 	const	{ return _elements[id]; }
	inline const Element&	_element(Rank rank)	const  	{ return _element(_id(rank)); }
	inline const _Sampler&	sampler()			const	{ return *this; }
	inline Rank				_rank(Id id)		const	{ return _element(id).rank(); }
	inline Exponent			_exponent(Id id)	const	{ return _element(id).exponent(); }
	inline Bigint			total_weight()		const	{ return _total_weight; }

	// not used
	// bool&      _removed(Index id)      { return _removed(_element(id)); }
	// Interval*& _interval(Index id)     { return _interval(_element(id)); }


	inline Interval* leftmost_interval()  { return &(* intervals().begin()); }
	inline Interval* rightmost_interval()
	{ Interval::Iterator it = intervals().end();  --it;  return &(*it); }

	Interval* create_first_interval(Exponent exponent) {
		assert(intervals().empty());
		Interval i(*this, exponent, 0);
		Interval::Iterator it = intervals().insert(intervals().end(), i);
		it->iterator() = it;
		return &(*it);
	}

	void shift_base_exponent_right() {
		// update:
		// _base_rank
		// _base_exponent
		// _base_exponent_interval
		// _total_weight
		assert(_base_exponent_interval);

		dump();

		Interval* new_base_exponent_interval;
		Rank new_base_rank;

		if (_base_exponent < _base_exponent_interval->exponent())  {
			// move _base_exponent to left of current _base_exponent_interval
			new_base_exponent_interval = _base_exponent_interval;
			new_base_rank = _base_rank;
		} else {
			// move _base_exponent to left of interval_to_right(_base_exponent_interval)
			new_base_exponent_interval = _base_exponent_interval->interval_to_right();
			new_base_rank = _base_rank + _base_exponent_interval->size();
			_total_weight -= _base_exponent_interval->weight();
			assert(new_base_exponent_interval);
		}
		Exponent new_base_exponent = new_base_exponent_interval->exponent();

		_total_weight -= Bigint(int(_base_rank));
		assert((_total_weight & ((Bigint(1) << (new_base_exponent - _base_exponent)) - 1)) == 0);
		_total_weight >>= new_base_exponent - _base_exponent;
		_total_weight += Bigint(int(new_base_rank));

		_base_rank = new_base_rank;
		_base_exponent = new_base_exponent;
		_base_exponent_interval = new_base_exponent_interval;
	}

public:
	int size() const { return _size; }

	_Sampler(int *initial_exponents, int n) {
		assert(n > 0);

		_size = n;
		_total_weight = 0;
		_elements = new Element[n];
		_id_of_rank = new Id[n];
		// _base_exponent set later
		// _base_rank set later

		assert(_elements && _id_of_rank);

		std::pair<int, unsigned int> sorted_exponents[n];

		// set up _elements (_ids and _exponents)
		// and _ids_sorted_by_exponent
		for (Id id = 0;  id < Id(n);  ++id) {
			_element(id).id()       = id;
			_element(id).exponent() = initial_exponents[id];

			sorted_exponents[id].first = initial_exponents[id];
			sorted_exponents[id].second = id;
		}
		sort(sorted_exponents, sorted_exponents+n);

		Exponent min_exponent = Exponent(sorted_exponents[0].first);

		Interval* i = create_first_interval(min_exponent);
		_base_exponent = min_exponent;
		_base_rank = 0;
		_base_exponent_interval = i;

		for (Rank rank = 0;  rank < Rank(n);  ++rank) {
			Exponent e = sorted_exponents[rank].first;
			Id id = sorted_exponents[rank].second;
			if (e != i->exponent()) i = i->add_interval_to_right(e);
			i->insert_element_right(id);
		}

		// check work
		for (Rank rank = 0;  rank < Rank(n-1);  ++rank)
			assert(rank.ELEMENT.exponent()
					<= Rank(rank+1).ELEMENT.exponent());

		for (Id id = 0;  id < Id(n);  ++id) {
			Element& elt(_element(id));
			assert(_id(elt.rank()) == id);
			assert(elt.exponent() == elt.interval()->exponent());
			assert(elt.interval()->lo() <= elt.rank());
			assert(elt.rank() <= elt.interval()->hi());
			assert(! elt.removed());
		}

		Interval* prev = NULL;
		assert(leftmost_interval()->lo() == Rank(0));
		assert(rightmost_interval()->hi() == Rank(_size)-1);
		for (Interval* i = leftmost_interval();  i;  i = i->interval_to_right()) {
			assert((!prev) || i->lo() == prev->hi() + 1);
			assert(!i->empty());
			prev = i;
		}
	}

	void dump() const {
		std::cout << "size = " << _size
			<< ", base_exponent = " << _base_exponent
			<< ", base_rank = " << int(_base_rank)
			<< ", total_weight = " << _total_weight
			<< std::endl;
		for (Rank rank = 0;  rank < Rank(_size);  ++rank) {
			const Element& elt(rank.ELEMENT);
			const Interval* i = elt.interval();
			std::cout << "rank=" << int(rank) << ", id=" << int(elt.id())
				<< ", exponent=" << elt.exponent()
				<< ", weight=" << elt.weight()
				<< ", interval=" << i
				<< ", lo=" << i->lo()
				<< ", hi=" << i->hi()
				<< ", exponent=" << i->exponent()
				<< ", weight=" << i->weight()
				<< std::endl;
		}
	}

	void remove(unsigned int id)	{
		Element& elt(_element(Id(id)));
		assert(! elt.removed());
		elt.interval()->remove_element_right(elt);
	}

	void increment_exponent(unsigned int id) {
		assert(Id(id) < Id(_size));

		Element &elt(_element(Id(id)));
		Interval* i = elt.interval();
		Exponent new_exponent = elt.exponent() + 1;
		Interval* next = i->interval_to_right();
		if (!next  ||  next->exponent() != new_exponent)
			next = i->add_interval_to_right(new_exponent);
		elt.interval()->remove_element_right(elt);
		elt.exponent() = new_exponent;
		next->insert_element_left(elt);

		// _total_weight >= half max value?  most-significant bit set?
		while (_total_weight & (Bigint(1) << (8*sizeof(_total_weight)-1)))
			shift_base_exponent_right();
	}

	int sample() {
		static const Exponent RAND_BITS(8*sizeof(RAND_MAX) - 1);
		assert(RAND_MAX == (((unsigned int) 1) << RAND_BITS) - 1);

		assert(_total_weight > 0);

		Bigint r = big_random() % _total_weight;

		for (Interval* i = rightmost_interval();  i;  i = i->interval_to_left())  {
			if (i->exponent() >= _base_exponent) {
				Bigint w = i->weight();
				if (r < w)
					return Rank(i->lo() + int(r/weight_of_exponent(i->exponent()))).ID;
				else
					r -= w;
			} else {	// stop at first interval < _base_exponent
				assert(r < (unsigned int) _base_rank); // total weight in remaining intervals = # elements

				Rank leftmost_lo = leftmost_interval()->lo();
				int n_slots = i->hi() - leftmost_lo + 1;

				// compactify if necessary
				if (_base_rank < Rank(n_slots / 2)) {
					i->compactify_intervals_to_left();
					leftmost_lo = leftmost_interval()->lo();
					n_slots = i->hi() - leftmost_lo + 1;
				}
				// choose id from random non-empty rank in [leftmost_lo.. _hi(i)]
				assert(_base_rank > Rank(0)  &&  _base_rank >= Rank(n_slots / 2));
				Id id_chosen = EMPTY;
				do
					id_chosen = Rank(leftmost_lo + int(random() % n_slots)).ID;
				while (id_chosen == Id(EMPTY));

				// rejection method
				// return element with probability 1/2^e.  e can be large
				Exponent e = _base_exponent - _exponent(id_chosen);
				assert(e >= 0);
				if (e == 0) return id_chosen;
				while (e > RAND_BITS) {
					if (rand() != 0) return -1;
					e -= RAND_BITS;
				}
				if (rand() & ((Bigint(1) << e) - 1) != 0) return -1;
				return id_chosen;
			}
		}
		assert(false);
		return -1;
	}
	~_Sampler() {
	}
};
const int _Sampler::EMPTY = -1;

Sampler* Sampler::create(int *initial_exponents, int n) {
	return new _Sampler(initial_exponents, n);
}

