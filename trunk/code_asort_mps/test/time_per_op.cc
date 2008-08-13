#include <cstdlib>
#include <iostream>
#include <math.h>

#include "main_include.h"

#define N 5000
#define T 10000

int f(int j) {
  return (j % 10);
}

typedef long long unsigned weight_t ;

weight_t random_weight_t(weight_t upper) {
  int bits = int(ceil(log2(upper)));
  weight_t mask = (weight_t(1)<<bits)-1;

  while (1) {
    // random returns random long in range 0 to 2**31 - 1
    weight_t r = random();

    for (int i = 1;  i*31 < bits;  ++i) {
      r = (r << 31) + random();
    }
    r &= mask;
    if (r < upper) return r;
  }
}

main() {
  int m[N];

  unsigned long time = get_time();

  for(int t = 0;  t < T;  ++t)
    for(int i = 0;  i < N;  ++i)
      m[i] = (i+t) % 10;

  unsigned long time1 = get_time(); 

  for(int t = 0;  t < T;  ++t)
    for(int i = 0;  i < N;  ++i)
      m[i] = random_weight_t((weight_t(1)<<(8*sizeof(weight_t)-3))*3);

  unsigned long time2 = get_time();

  std::cout << time << std::endl;
  std::cout << time1 << std::endl;
  std::cout << time2 << std::endl;

  double usecs_per_op = (time1-time) / (double(N)*T);
  double usecs_per_opfn = (time2-time1) / (double(N)*T);
  double usecs_per_fn = usecs_per_opfn - usecs_per_op;

  std::cout <<  usecs_per_op << " usec per operation" << std::endl;
  std::cout <<  usecs_per_opfn << " usec per function call" << std::endl;
  std::cout <<  usecs_per_fn << " usec extra per function call" << std::endl;
  std::cout <<  usecs_per_opfn/usecs_per_op << " ops per function call" << std::endl;

  std::cout << 1/usecs_per_op << " ops per usec" << std::endl;
}
