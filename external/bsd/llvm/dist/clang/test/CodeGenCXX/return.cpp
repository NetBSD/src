// RUN: %clang_cc1 -emit-llvm -cxx-abi itanium -o - %s | FileCheck %s
// RUN: %clang_cc1 -emit-llvm -cxx-abi itanium -O -o - %s | FileCheck %s --check-prefix=CHECK-OPT

// CHECK:     @_Z9no_return
// CHECK-OPT: @_Z9no_return
int no_return() {
  // CHECK:      call void @llvm.trap
  // CHECK-NEXT: unreachable

  // CHECK-OPT-NOT: call void @llvm.trap
  // CHECK-OPT:     unreachable
}
