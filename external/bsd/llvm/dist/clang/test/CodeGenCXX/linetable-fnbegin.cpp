// RUN: %clang_cc1 -emit-llvm -g %s -o - | FileCheck %s
// Test that the line table info for Foo<T>::bar() is pointing to the
// right header file.
// CHECK: define{{.*}}bar
// CHECK-NOT: define
// CHECK: ret {{.*}}, !dbg [[DBG:.*]]
// CHECK: [[HPP:.*]] = metadata !{metadata !"./template.hpp",
// CHECK: [[SP:.*]] = metadata !{i32 786478, metadata [[HPP]],{{.*}}[ DW_TAG_subprogram ] [line 22] [def] [bar]
// We shouldn't need a lexical block for this function.
// CHECK: [[DBG]] = metadata !{i32 23, i32 0, metadata [[SP]], null}


# 1 "./template.h" 1
template <typename T>
class Foo {
public:
  int bar();
};
# 21 "./template.hpp"
template <typename T>
int Foo<T>::bar() {
  return 23;
}
int main (int argc, const char * argv[])
{
  Foo<int> f;
  return f.bar();
}
