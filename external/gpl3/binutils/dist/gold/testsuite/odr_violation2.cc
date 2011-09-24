#include <algorithm>

class Ordering {
 public:
  bool operator()(int a, int b) {
    // We need the "+ 1" here to force this operator() to be a
    // different size than the one in odr_violation1.cc.
    return a + 1 > b + 1;
  }
};

void SortDescending(int array[], int size) {
  std::sort(array, array + size, Ordering());
}

// This is weak in odr_violation1.cc.
extern "C" int OverriddenCFunction(int i) {
  return i * i;
}
// This is inline in debug_msg.cc, which makes it a weak symbol too.
int SometimesInlineFunction(int i) {
  return i * i;
}
