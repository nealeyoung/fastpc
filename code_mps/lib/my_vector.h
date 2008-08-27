#ifndef MY_VECTOR_H
#define MY_VECTOR_H

#include <assert.h>
#include <string.h>
#include <math.h>
#include "lib_include.h"
#include <iostream>

using namespace std;
template <class V>
class my_vector {
 public:
  typedef V* iterator;
  int start_index;
 private:
  V* table;
  int table_size, n_elts;

  void init(int n) {

    unsigned long start = get_time();
    start_index = 0;
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

  // shrink is untested
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

  void sort() {
    sort_entries(&table, size());
  }

  void sort_entries(V** tab, int t_size) {
    V* t = *tab;
    if (t_size <= 1)
      return;
    int middle = floor(t_size/2);
    V* left = new V[middle];
    V* right = new V[t_size - middle];

    for (int i = 0; i < t_size; i++) {
      if (i < middle) {
	left[i] = t[i];
      } else {
	right[i-middle] = t[i];
      }
    }
    sort_entries(&left, middle);
    sort_entries(&right, t_size - middle);
    V* result = merge(left, middle, right, t_size - middle);
    *tab = result;
  }

  V* merge(V* left, int l_size, V* right, int r_size) {
    V* result = new V[l_size + r_size];
    int left_index = 0;
    int right_index = 0;
    int result_index = 0;

    while (left_index < l_size && right_index < r_size) {
      if (left[left_index] <= right[right_index]) {
	result[result_index] = left[left_index];
	left_index++;
      } else {
	result[result_index] = right[right_index];
	right_index++;
      }
      result_index++;
    }
    while (left_index < l_size) {
      result[result_index] = left[left_index];
      left_index++;
      result_index++;
    }
    while (right_index < r_size) {
      result[result_index] = right[right_index];
      right_index++;
      result_index++;
    }
    return result;
  }

  void clear() { n_elts = 0; }

  inline V& operator[](int i) { 
    //cout<<"i " <<i<<"helppppppppppppppppppppppppp "<<n_elts<<endl<<flush;
    assert(i >= 0);
    assert(i < n_elts);
    return table[i]; 
  }

  bool empty() { return n_elts == 0; }
  int size() { return n_elts; }

  iterator begin(){return &table[start_index];}
  iterator end() { return &table[n_elts]; }
};

#endif
