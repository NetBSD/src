// RUN: %clang_cc1 -cxx-abi itanium -emit-llvm %s -o - | FileCheck %s
struct A {};
A& (A::*x)(const A&) = &A::operator=;
// CHECK-LABEL: define linkonce_odr {{.*}}%struct.A* @_ZN1AaSERKS_
