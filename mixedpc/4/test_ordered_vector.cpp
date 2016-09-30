//  -*- mode: c++; flycheck-clang-language-standard: "c++11"; -*-

#include <iostream>

#include "ordered_vector.h"

struct entry_t {
  int _value;
  bool _removed;

  int value() { return _value; }
  bool removed() { return _removed; }

  entry_t(int v) : _value(v), _removed(false) {}
};

int
main() {
  ordered_vector_t<entry_t *> v;
  // sorted_vector_t v;

  v.push_back(new entry_t(3));
  v.push_back(new entry_t(10));
  v.push_back(new entry_t(5));
  v.push_back(new entry_t(2));
  v.push_back(new entry_t(6));
  v.push_back(new entry_t(11));
  v.push_back(new entry_t(12));
  v.push_back(new entry_t(13));

  v.done_adding();

  auto dump = [&]() -> void {
    std::cout << "##"
    << " size = " << v._size() 
    << " max = " << v.max()->_value
    << " size = " << v._size()
    << std::endl;
  };
  dump();
  for (auto e: v) {
    std::cout << e->_value << std::endl;
    if (e->_value % 2) {
      e->_removed = true;
       std::cout << "removing " << e->_value << std::endl;
    }
  }
  dump();
  for (auto e: v)
    std::cout << e->_value << " " << e->_removed << std::endl;
  dump();
  for (auto e: v)
    std::cout << e->_value << " " << e->_removed << std::endl;
  dump();
}
