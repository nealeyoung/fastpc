#ifndef _LIB_INCLUDE_H
#define _LIB_INCLUDE_H

unsigned long get_time();

extern long long unsigned basic_ops;
extern long long unsigned alloc_time;
extern long long unsigned alloc_space;

#define count_ops(i) do { basic_ops += (i); } while(0)

#endif
