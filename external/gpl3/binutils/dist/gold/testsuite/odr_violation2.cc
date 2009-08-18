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
