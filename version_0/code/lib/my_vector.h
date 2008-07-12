#ifndef MY_VECTOR_H
#define MY_VECTOR_H

#include <assert.h>
#include <string.h>

unsigned long get_time();

extern long long unsigned basic_ops;
extern long long unsigned alloc_time;
extern long long unsigned alloc_space;

#define count_ops(i) do { basic_ops += (i); } while(0)

template <class V>
class my_vector {
 public:
  typedef V* iterator;

 private:
  V* table;
  int table_size, n_elts;

  void init(int n) {

    unsigned long start = get_time();

    table = new V[n];
    assert(table);
    table_size = n_elts = n;

    alloc_space += n*sizeof(V);
    alloc_time += get_time() - start;
    count_ops(n);

  }
 public:
  my_vector() { init(16);  n_elts = 0; }
  my_vector(int n) { init(n); }
  ~my_vector() { delete[] table; }

  void resize(int n) {
    count_ops(1);
    if (n <= table_size) {
      n_elts = n;
    } else {
      count_ops(n);
      unsigned long start = get_time();

      V* new_table = new V[n];
      assert(new_table);
      memcpy(new_table, table, n_elts*sizeof(V));
      delete[] table;
      table = new_table;
      n_elts = table_size = n;

      alloc_space += n*sizeof(V);
      alloc_time += get_time() - start;
    }
  }

  void push_back(V x) {
    count_ops(2);
    if (n_elts == table_size) {
      int old_n = n_elts;
      resize(2*table_size);
      n_elts = old_n;
    }
    assert(n_elts < table_size);
    table[n_elts++] = x;
  }
  void pop_back() {  count_ops(1); assert(n_elts > 0);  --n_elts; }

  void shrink() {  
    int n = 2*n_elts+5;
    if (table_size <= n) return;

    count_ops(n);
    unsigned long start = get_time();

    V* new_table = new V[n];
    assert(new_table);
    memcpy(new_table, table, n_elts*sizeof(V));
    delete[] table;
    table = new_table;
    table_size = n;

    alloc_space += n*sizeof(V);
    alloc_time += get_time() - start;
  }

  void remove(int i) {
    int last = n_elts-1;
    if (i != last) {
      table[i] = table[last];
    }
    --n_elts;
  }

  void clear() { n_elts = 0; }
  inline V& operator[](int i) { 
    assert(i >= 0);
    assert(i < n_elts);
    return table[i]; 
  }
  bool empty() { return n_elts == 0; }
  int size() { return n_elts; }

  iterator begin() { return &table[0]; }
  iterator end() { return &table[n_elts]; }
};

#endif
