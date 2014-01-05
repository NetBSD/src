// RUN: %clang_cc1 %s -fvisibility hidden -cxx-abi itanium -emit-llvm -o - | FileCheck %s

namespace std __attribute__ ((__visibility__ ("default"))) {}
#pragma GCC visibility push(default)
void foo() {
}
#pragma GCC visibility pop
// CHECK-LABEL: define void @_Z3foov()
