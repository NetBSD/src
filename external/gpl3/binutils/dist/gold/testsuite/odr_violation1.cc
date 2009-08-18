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
