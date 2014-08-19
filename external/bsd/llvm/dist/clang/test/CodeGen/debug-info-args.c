// RUN: %clang_cc1 -triple x86_64-unk-unk -o - -emit-llvm -g %s | FileCheck %s

int somefunc(char *x, int y, double z) {
  
  // CHECK: metadata ![[NUM:[^,]*]], i32 0, null, null, null} ; [ DW_TAG_subroutine_type
  // CHECK: ![[NUM]] = {{metadata !{metadata ![^,]*, metadata ![^,]*, metadata ![^,]*, metadata ![^,]*}}}
  
  return y;
}
