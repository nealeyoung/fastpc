#include <stdlib.h>
#include <iostream>

#include "sampler.h"
#include "main_include.h"

using namespace std;

main() {
  dual_sampler_t s(3, 0.1, 0, 100);
  s.init();

  for (int i = 0;  i < 40;  ++i)
    cout << s.sample()->i << " ";
  cout << endl;
  cout << endl;

  s.remove(s.get_ith(0));
  for (int i = 0;  i < 40;  ++i)
    cout << s.sample()->i << " ";
  cout << endl;

  for (int t = 0; t < 30;  ++t)  {
    s.increment_exponent(s.get_ith(1), false);

    for (int i = 1;  i < 3;  ++i)
      cout << s.get_exponent(s.get_ith(i)) << " ";
    cout << endl;

    for (int i = 0;  i < 80;  ++i)
      cout << s.sample()->i << " ";
    cout << endl;
  }
  cout << endl;

  for (int t = 0; t < 60;  ++t)  {
    s.increment_exponent(s.get_ith(2), false);

    for (int i = 1;  i < 3;  ++i)
      cout << s.get_exponent(s.get_ith(i)) << " ";
    cout << endl;

    for (int i = 0;  i < 80;  ++i)
      cout << s.sample()->i << " ";
    cout << endl;
  }
  cout << endl;

  for (int i = 1;  i < 3;  ++i)
    cout << s.get_exponent(s.get_ith(i)) << " ";
  cout << endl;
}
