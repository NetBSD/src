// RUN: %clang_cc1 -emit-llvm -cxx-abi itanium -o - %s | FileCheck %s

namespace N {
  void free(void *i) {}
}

int main(void) {
  // CHECK: call void @_ZN1N4freeEPv
  void *fp __attribute__((cleanup(N::free)));
  return 0;
}
