#include <stdlib.h>
#include <iostream>

#include "main_include.h"

using namespace std;

main() {
  int n = 0;
  unsigned long now, last = 0;
  for (int i = 0;  i < 10;  ++i) {
    while ((now = get_time()) == last) {
      ++n;
    }
    last = now;
    cout << n << " " << now << endl;
  }
}
