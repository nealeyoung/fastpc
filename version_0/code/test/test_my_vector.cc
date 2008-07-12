#include <stdlib.h>
#include <iostream>

#include <vector>

#include "my_vector.h"
#include "main_include.h"

using namespace std;

template <class V1, class V2>
bool compare(V1 &v1, V2 &v2) {
  typename V1::iterator i1;
  typename V2::iterator i2;
  if (v1.size() != v2.size()) return false;
  for (i1 = v1.begin(),
	 i2 = v2.begin();
       i1 != v1.end()  &&
	 i2 != v2.end();
       ++i1, ++i2) {
    if (*i1 != *i2)
      return false;
  }
  return true;
}

main() {
  my_vector<int> v1;
  vector<int> v2;

  int errs = 0;

  errs += !compare(v1, v2);
  v1.push_back(3);
  v2.push_back(3);
  errs += !compare(v1, v2);
  v1.push_back(1);
  v2.push_back(1);
  errs += !compare(v1, v2);
  v1.push_back(4);
  v2.push_back(4);
  errs += !compare(v1, v2);
  v1.pop_back();
  v2.pop_back();
  errs += !compare(v1, v2);
  v1.clear();
  v2.clear();
  errs += !compare(v1, v2);

  for (int i = 0;  i < 20;  ++i) {
    v1.push_back(i);
    v2.push_back(i);
  }
  errs += !compare(v1, v2);
  cout << errs << " errors" << endl;
  if (errs)
    cout << "FAILED" << endl;
}
