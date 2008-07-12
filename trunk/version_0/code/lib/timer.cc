#include <stdio.h>
#include <sys/time.h>
#include <cstdlib>
#include <iostream>

#include "timer.h"

#define TIMER_WRAP 1000000
#define TIMER_TIMER ITIMER_REAL
//#define TIMER_TIMER ITIMER_PROF
//
// REAL - real time
// PROF - process and subprocess time (careful, may have less precision)

unsigned long
get_time() {
  static int initialized = 0;
  static struct timeval start;
  struct itimerval itimer;
  struct timeval diff;

  if (! initialized) {
    itimer.it_interval.tv_sec = TIMER_WRAP;
    itimer.it_interval.tv_usec = 0;

    itimer.it_value.tv_sec = TIMER_WRAP;
    itimer.it_value.tv_usec = 0;
    if (setitimer( TIMER_TIMER, &itimer, NULL )) {
      fprintf(stderr, "setitimer error\n");
      exit(-1);
    }

    if (getitimer( TIMER_TIMER, &itimer )) {
      fprintf(stderr, "getitimer error\n");
      exit(-1);
    }
    start = itimer.it_value;
    initialized = 1;
  }
  
  if (getitimer( TIMER_TIMER, &itimer )) {
      fprintf(stderr, "getitimer error\n");
      exit(-1);
  }
  timersub(&start, &itimer.it_value, &diff);
  return  diff.tv_sec*1000000 + diff.tv_usec;
}
