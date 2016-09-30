#define NDEBUG

#include <assert.h>
#include "utilities.h"

int
main() {
  assert(false);
  ASSERT(false, "hi");
}
