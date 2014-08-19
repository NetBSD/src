// RUN: %clang_cc1 -emit-llvm -g %s -o - | FileCheck %s
// rdar://problem/14985269.
//
// Verify that the global init helper function does not get associated
// with any source location.
//
// CHECK: define internal void @_GLOBAL__sub_I_globalinit_loc.cpp
// CHECK: !dbg ![[DBG:.*]]
// CHECK: "_GLOBAL__sub_I_globalinit_loc.cpp", i32 0, {{.*}}, i32 0} ; [ DW_TAG_subprogram ] [line 0] [local] [def]
// CHECK: ![[DBG]] = metadata !{i32 0, i32 0,
# 99 "someheader.h"
class A {
public:
  A();
  int foo() { return 0; }
};
# 5 "main.cpp"
A a;

int f() {
  return a.foo();
}

