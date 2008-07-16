#include <iostream>
#include <cstdlib>

using namespace std;

class fast_bit_vector_t {
  
private:
  typedef long long unsigned int base_t;

  base_t _bit_vector;

  static bool fast_bit_vector_t::class_initialized;
  static short unsigned int fast_bit_vector_t::size;
  static fast_bit_vector_t::base_t* fast_bit_vector_t::bits;
  static fast_bit_vector_t::base_t* fast_bit_vector_t::bits_bar;
  static fast_bit_vector_t::base_t* fast_bit_vector_t::bits_left;
  static bool fast_bit_vector_t::initialize_class();

public:
  fast_bit_vector_t() {
    _bit_vector = 0;
  }
  inline void insert(int i) {
    assert (i < size);
    _bit_vector |= bits[i];
  }
  inline void remove(int i) {
    assert (i < size);
    _bit_vector &= bits_bar[i];
  }
  inline int min() {
    // google builtin_ffsll for documentation
    return __builtin_ffsll(_bit_vector)-1;
  }
  inline int max() {
    assert (_bit_vector);
    // google builtin_clzll for documentation
    return (sizeof(base_t)*8-1)-__builtin_clzll(_bit_vector);
  }
  // return largest in set >= i, or -1 if none
  inline int successor(int i) {
    return __builtin_ffsll(_bit_vector&bits_left[i])-1;
  }
};

bool fast_bit_vector_t::class_initialized = fast_bit_vector_t::initialize_class();
short unsigned int fast_bit_vector_t::size = sizeof(fast_bit_vector_t::base_t)*8;
fast_bit_vector_t::base_t* fast_bit_vector_t::bits;
fast_bit_vector_t::base_t* fast_bit_vector_t::bits_bar;
fast_bit_vector_t::base_t* fast_bit_vector_t::bits_left;

bool fast_bit_vector_t::initialize_class() {
  bits = new base_t[size];
  bits_bar = new base_t[size];
  bits_left = new base_t[size];
  for (int i = 0;  i < size;  ++i) {
    bits[i] = base_t(1) << i;
    bits_bar[i] = ~ bits[i];
    bits_left[i] = ~ ((base_t(1) << i)-base_t(1));
  }
}

main() {
  fast_bit_vector_t b;

  b.insert(4);
  b.insert(10);
  cout << b.successor(4) << endl << b.successor(5) << endl << b.successor(11) << endl;
}
