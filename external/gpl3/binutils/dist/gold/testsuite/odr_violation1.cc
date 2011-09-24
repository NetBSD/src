#include <algorithm>

class Ordering {
 public:
  bool operator()(int a, int b) {
    return a < b;
  }
};

void SortAscending(int array[], int size) {
  std::sort(array, array + size, Ordering());
}

extern "C" int OverriddenCFunction(int i) __attribute__ ((weak));
extern "C" int OverriddenCFunction(int i) {
  return i;
}
