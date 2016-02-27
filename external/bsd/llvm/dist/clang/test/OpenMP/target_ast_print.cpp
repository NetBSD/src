// RUN: %clang_cc1 -verify -fopenmp -ast-print %s | FileCheck %s
// RUN: %clang_cc1 -fopenmp -x c++ -std=c++11 -emit-pch -o %t %s
// RUN: %clang_cc1 -fopenmp -std=c++11 -include-pch %t -fsyntax-only -verify %s -ast-print | FileCheck %s
// expected-no-diagnostics

#ifndef HEADER
#define HEADER

void foo() {}

template <typename T, int C>
T tmain(T argc, T *argv) {
  T i, j, a[20];
#pragma omp target
  foo();
#pragma omp target if (target:argc > 0)
  foo();
#pragma omp target if (C)
  foo();
#pragma omp target map(i)
  foo();
#pragma omp target map(a[0:10], i)
  foo();
#pragma omp target map(to: i) map(from: j)
  foo();
#pragma omp target map(always,alloc: i)
  foo();
  return 0;
}

// CHECK: template <typename T = int, int C = 5> int tmain(int argc, int *argv) {
// CHECK-NEXT: int i, j, a[20]
// CHECK-NEXT: #pragma omp target
// CHECK-NEXT: foo();
// CHECK-NEXT: #pragma omp target if(target: argc > 0)
// CHECK-NEXT: foo()
// CHECK-NEXT: #pragma omp target if(5)
// CHECK-NEXT: foo()
// CHECK-NEXT: #pragma omp target map(tofrom: i)
// CHECK-NEXT: foo()
// CHECK-NEXT: #pragma omp target map(tofrom: a[0:10],i)
// CHECK-NEXT: foo()
// CHECK-NEXT: #pragma omp target map(to: i) map(from: j)
// CHECK-NEXT: foo()
// CHECK-NEXT: #pragma omp target map(always,alloc: i)
// CHECK-NEXT: foo()
// CHECK: template <typename T = char, int C = 1> char tmain(char argc, char *argv) {
// CHECK-NEXT: char i, j, a[20]
// CHECK-NEXT: #pragma omp target
// CHECK-NEXT: foo();
// CHECK-NEXT: #pragma omp target if(target: argc > 0)
// CHECK-NEXT: foo()
// CHECK-NEXT: #pragma omp target if(1)
// CHECK-NEXT: foo()
// CHECK-NEXT: #pragma omp target map(tofrom: i)
// CHECK-NEXT: foo()
// CHECK-NEXT: #pragma omp target map(tofrom: a[0:10],i)
// CHECK-NEXT: foo()
// CHECK-NEXT: #pragma omp target map(to: i) map(from: j)
// CHECK-NEXT: foo()
// CHECK-NEXT: #pragma omp target map(always,alloc: i)
// CHECK-NEXT: foo()
// CHECK: template <typename T, int C> T tmain(T argc, T *argv) {
// CHECK-NEXT: T i, j, a[20]
// CHECK-NEXT: #pragma omp target
// CHECK-NEXT: foo();
// CHECK-NEXT: #pragma omp target if(target: argc > 0)
// CHECK-NEXT: foo()
// CHECK-NEXT: #pragma omp target if(C)
// CHECK-NEXT: foo()
// CHECK-NEXT: #pragma omp target map(tofrom: i)
// CHECK-NEXT: foo()
// CHECK-NEXT: #pragma omp target map(tofrom: a[0:10],i)
// CHECK-NEXT: foo()
// CHECK-NEXT: #pragma omp target map(to: i) map(from: j)
// CHECK-NEXT: foo()
// CHECK-NEXT: #pragma omp target map(always,alloc: i)
// CHECK-NEXT: foo()

// CHECK-LABEL: int main(int argc, char **argv) {
int main (int argc, char **argv) {
  int i, j, a[20];
// CHECK-NEXT: int i, j, a[20]
#pragma omp target
// CHECK-NEXT: #pragma omp target
  foo();
// CHECK-NEXT: foo();
#pragma omp target if (argc > 0)
// CHECK-NEXT: #pragma omp target if(argc > 0)
  foo();
// CHECK-NEXT: foo();

#pragma omp target map(i) if(argc>0)
// CHECK-NEXT: #pragma omp target map(tofrom: i) if(argc > 0)
  foo();
// CHECK-NEXT: foo();

#pragma omp target map(i)
// CHECK-NEXT: #pragma omp target map(tofrom: i)
  foo();
// CHECK-NEXT: foo();

#pragma omp target map(a[0:10], i)
// CHECK-NEXT: #pragma omp target map(tofrom: a[0:10],i)
  foo();
// CHECK-NEXT: foo();

#pragma omp target map(to: i) map(from: j)
// CHECK-NEXT: #pragma omp target map(to: i) map(from: j)
  foo();
// CHECK-NEXT: foo();

#pragma omp target map(always,alloc: i)
// CHECK-NEXT: #pragma omp target map(always,alloc: i)
  foo();
// CHECK-NEXT: foo();

  return tmain<int, 5>(argc, &argc) + tmain<char, 1>(argv[0][0], argv[0]);
}

#endif
