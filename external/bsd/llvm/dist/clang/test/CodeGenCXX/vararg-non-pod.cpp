// RUN: %clang_cc1 -Wno-error=non-pod-varargs -cxx-abi itanium -emit-llvm -o - %s | FileCheck %s

struct X {
  X();
  X(const X&);
  ~X();
};

void vararg(...);

// CHECK-LABEL: define void @_Z4test1X
void test(X x) {
  // CHECK: call void @llvm.trap()
  vararg(x);
  // CHECK: ret void
}
