//  -*- mode: c++; flycheck-clang-language-standard: "c++11"; -*-

#ifndef ORDERED_VECTOR_H
#define ORDERED_VECTOR_H

#include <vector>

// assumptions about T
// T t;
// t->value() returns a type that can be compared with < (e.g. double)
// t->removed() returns true if element has been removed

// ordered_vector<T> v;
//
// 1. Add all elements using v.push_back(T t).
// 2. Call v.done_adding().
// 3. Access (non-deleted) elements in order of decreasing value
//    via iterator, e.g.: for(auto t: v) { ... t->value(); }
// 4. To cause deletion of element t, set t->removed() to true

template<typename T>
class ordered_vector_t {

private:

  typedef std::template vector<T>                   vector_t;
  typedef typename vector_t::reverse_iterator       vector_iterator_t;

  vector_t          vector;
  bool              _done_adding;
  bool              needs_compaction;
  vector_iterator_t compaction_end;

  typedef decltype(vector[0]->value()) value_t;

  void _compact() {
    if (! needs_compaction) return;
    needs_compaction = false;

    vector.erase(std::remove_if(compaction_end.base()-1,
                                vector.end(),
                                [](T & a){return a->removed();}),
                 vector.end());
  }
  // void _compact_const() const {
  //   const_cast<ordered_vector_t*>(this)->_compact();
  // }

  // Support iterating over non-removed entries in order of decreasing size.
  // Compact vector by deleting references to entries marked removed.

  struct _iterator_t : vector_iterator_t {
    const ordered_vector_t * const container;

    _iterator_t(ordered_vector_t * c, vector_iterator_t b) :
      vector_iterator_t(b),
      container(c)
    { }

    const _iterator_t& _advance();
    const _iterator_t& operator++() {
      ((vector_iterator_t*) this)->operator++();
      return _advance();
    }
  };

public:

  typedef _iterator_t iterator_t;

  ordered_vector_t() 
    : _done_adding(false), needs_compaction(false) {};

  inline void push_back(T t)
  { if (! t->removed()) vector.push_back(t); }

  void done_adding() { 
    if (_done_adding) return;
    _done_adding = true;
    std::sort(vector.begin(), vector.end(),
              [](T a, T b) -> bool {
                return a->value() < b->value();
              });
  }

  T    max()   { return *begin(); }
  bool empty() { return begin() == end(); }

  size_t _size() const 
  { return vector.size(); } // including some removed entries

  const iterator_t end() { return iterator_t(this, vector.rend()); }
  const iterator_t begin() { 
    // if (needs_compaction) _compact_const();
    if (needs_compaction) _compact();
    return iterator_t(this, vector.rbegin())._advance(); 
  }
};

////////////////////////////////////////////////////////////////
//
// member function in .h file for instantiation
//
////////////////////////////////////////////////////////////////

template<typename T>
const typename ordered_vector_t<T>::iterator_t &
ordered_vector_t<T>::iterator_t::_advance() {
  while (true) {
    if (*((vector_iterator_t*) this) == container->vector.rend())
      return *this;
    if (! (**this)->removed())
      return *this;
    const_cast<ordered_vector_t*>(container)->needs_compaction = true;
    const_cast<ordered_vector_t*>(container)->compaction_end = *this;
    ((vector_iterator_t*) this)->operator++();
  }
}

#endif
