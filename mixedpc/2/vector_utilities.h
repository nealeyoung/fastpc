//  -*- mode: c++; flycheck-clang-language-standard: "c++11"; -*-

#ifndef VECTOR_UTILITIES_H
#define VECTOR_UTILITIES_H

#include <vector>
#include <numeric>
#include <iostream>

template<typename T>
T min(const std::vector<T>& v) { return *std::min_element(v.begin(), v.end()); }

template<typename T>
T max(const std::vector<T>& v) { return *std::max_element(v.begin(), v.end()); }

template<typename T>
T sum(const std::vector<T>& v) { return std::accumulate(v.begin(), v.end(), T(0.0)); }

template<typename T>
void dump(std::vector<T> & v) {
  std::cout << "[";
  for (auto & e : v) std::cout << e << " ";
  std::cout << "]" << std::endl;
}

#endif

