//  -*- mode: c++; flycheck-clang-language-standard: "c++11"; -*-

#ifndef UTILITIES_H
#define UTILITIES_H

#include <iostream>
#include <assert.h>

//// VECTOR UTILITIES

#include <vector>
#include <numeric>

template<typename T>
T min(const std::vector<T>& v) { return *std::min_element(v.begin(), v.end()); }

template<typename T>
T max(const std::vector<T>& v) { return *std::max_element(v.begin(), v.end()); }

template<typename T>
T sum(const std::vector<T>& v) { return std::accumulate(v.begin(), v.end(), T(0.0)); }

template<typename T, typename S>
void scale_vector_by(std::vector<T>& v, S s) { for (auto & e: v) e *= s; }

template<typename T>
void dump(const std::vector<T> & v) {
  std::cout << "[";
  for (auto & e : v) std::cout << e << " ";
  std::cout << "]" << std::endl;
}

//// OVERFLOW UTILITIES

#include "SafeInt3.hpp"

// increase a by b and return true,
// unless overflow would occur, in which case return false

// from SafeInt3.hpp
// * inline bool SafeCast( const T From, U& To ) throw()
// * inline bool SafeEquals( const T t, const U u ) throw()
// * inline bool SafeNotEquals( const T t, const U u ) throw()
// * inline bool SafeGreaterThan( const T t, const U u ) throw()
// * inline bool SafeGreaterThanEquals( const T t, const U u ) throw()
// * inline bool SafeLessThan( const T t, const U u ) throw()
// * inline bool SafeLessThanEquals( const T t, const U u ) throw()
// * inline bool SafeModulus( const T& t, const U& u, T& result ) throw()
// * inline bool SafeMultiply( T t, U u, T& result ) throw()
// * inline bool SafeDivide( T t, U u, T& result ) throw()
// * inline bool SafeAdd( T t, U u, T& result ) throw()
// * inline bool SafeSubtract( T t, U u, T& result ) throw()

#define safe_add SafeAdd
#define safe_subtract SafeSubtract


//// ASSERT, OUTPUT UTILITIES

// http://stackoverflow.com/questions/3767869/adding-message-to-assert
#ifndef NDEBUG
# define ASSERT(condition, to_show)                                     \
  do {                                                                  \
    static const auto ASSERT_SEP = "\n";                                \
    ASSERT_SEP; /* avoid unused variable complaints from compiler */    \
    if (! (condition)) {                                                \
      std::cerr << "Assertion `" #condition "` failed in " << __FILE__  \
                << " line " << __LINE__ << ": " << std::endl << "" to_show << std::endl; \
        assert(condition);                                              \
    }                                                                   \
  } while (false)

#else
#   define ASSERT(condition, message) do { } while (false)
#endif

static const auto ASSERT_SEP = ", ";

#define __      << std::endl
#define _(expr) << #expr "= " << expr << ASSERT_SEP

#endif

