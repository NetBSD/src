// RUN: %clang_cc1 -triple %itanium_abi_triple -emit-llvm -g %s -o - | FileCheck %s

// CHECK: [[ENUMS:![0-9]*]], {{[^,]*}}, {{[^,]*}}, {{[^,]*}}, {{[^,]*}}, {{[^,]*}}} ; [ DW_TAG_compile_unit ]
// CHECK: [[ENUMS]] = metadata !{metadata [[E1:![0-9]*]], metadata [[E2:![0-9]*]], metadata [[E3:![0-9]*]]}

namespace test1 {
// CHECK: [[E1]] = metadata !{i32 {{[^,]*}}, {{[^,]*}}, metadata [[TEST1:![0-9]*]], {{.*}}, metadata [[TEST1_ENUMS:![0-9]*]], {{[^,]*}}, null, null, metadata !"_ZTSN5test11eE"} ; [ DW_TAG_enumeration_type ] [e]
// CHECK: [[TEST1]] = {{.*}} ; [ DW_TAG_namespace ] [test1]
// CHECK: [[TEST1_ENUMS]] = metadata !{metadata [[TEST1_E:![0-9]*]]}
// CHECK: [[TEST1_E]] = {{.*}}, metadata !"E", i64 0} ; [ DW_TAG_enumerator ] [E :: 0]
enum e { E };
void foo() {
  int v = E;
}
}

namespace test2 {
// rdar://8195980
// CHECK: [[E2]] = metadata !{i32 {{[^,]*}}, {{[^,]*}}, metadata [[TEST2:![0-9]*]], {{.*}}, metadata [[TEST1_ENUMS]], {{[^,]*}}, null, null, metadata !"_ZTSN5test21eE"} ; [ DW_TAG_enumeration_type ] [e]
// CHECK: [[TEST2]] = {{.*}} ; [ DW_TAG_namespace ] [test2]
enum e { E };
bool func(int i) {
  return i == E;
}
}

namespace test3 {
// CHECK: [[E3]] = metadata !{i32 {{[^,]*}}, {{[^,]*}}, metadata [[TEST3:![0-9]*]], {{.*}}, metadata [[TEST3_ENUMS:![0-9]*]], {{[^,]*}}, null, null, metadata !"_ZTSN5test31eE"} ; [ DW_TAG_enumeration_type ] [e]
// CHECK: [[TEST3]] = {{.*}} ; [ DW_TAG_namespace ] [test3]
// CHECK: [[TEST3_ENUMS]] = metadata !{metadata [[TEST3_E:![0-9]*]]}
// CHECK: [[TEST3_E]] = {{.*}}, metadata !"E", i64 -1} ; [ DW_TAG_enumerator ] [E :: -1]
enum e { E = -1 };
void func() {
  e x;
}
}
