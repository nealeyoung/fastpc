#include <cstdlib>
#include <iostream>
#include <math.h>

#include "main_include.h"

#include "../fast_bit_vector.cc"

#define N 2000
#define T 5000

int f(int j) {
  return (j % 10);
}

typedef long long unsigned weight_t ;

int I = 0;

int function_call(int i, int t) {
  return (I++ + t)%10;
}

main() {
  int m[N];

  fast_bit_vector_t bit_vector;

  unsigned long time = get_time();

  for(int t = 0;  t < T;  ++t)
    for(int i = 0;  i < N;  ++i)
      m[i] = (i+t) % 10;

  unsigned long time1 = get_time(); 

  for(int t = 0;  t < T;  ++t)
    for(int i = 0;  i < N;  ++i)
      m[i] = function_call(i,t);

  unsigned long time2 = get_time();

  for(int t = 0;  t < T;  ++t)
    for(int i = 0;  i < N;  ++i) {
      bit_vector.insert(i & 63);
      bit_vector.min();
    }

  unsigned long time3 = get_time();
  
  std::cout << time << std::endl;
  std::cout << time1 << std::endl;
  std::cout << time2 << std::endl;
  std::cout << time3 << std::endl;

  double usecs_per_op = (time1-time) / (double(N)*T);
  double usecs_per_opfn = (time2-time1) / (double(N)*T);
  double usecs_per_fn = usecs_per_opfn - usecs_per_op;
  double usecs_per_bv = (time3-time2) / (double(N)*T);

  std::cout <<  usecs_per_op << " usec per operation" << std::endl;
  std::cout <<  usecs_per_opfn << " usec per function call" << std::endl;
  std::cout <<  usecs_per_fn << " usec extra per function call" << std::endl;
  std::cout <<  usecs_per_opfn/usecs_per_op << " ops per function call" << std::endl;
  std::cout <<  usecs_per_bv << " usec per bit vector op" << std::endl;

  std::cout << 1/usecs_per_op << " ops per usec" << std::endl;
}
