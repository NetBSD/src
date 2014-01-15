// RUN: %clang_cc1 -triple %itanium_abi_triple -emit-llvm -o - -fsanitize=thread %s | FileCheck %s
// RUN: %clang_cc1 -triple %itanium_abi_triple -emit-llvm -o - -O1 %s | FileCheck %s
// RUN: %clang_cc1 -triple %itanium_abi_triple -emit-llvm -o - -O1  -relaxed-aliasing -fsanitize=thread %s | FileCheck %s
//
// RUN: %clang_cc1 -triple %itanium_abi_triple -emit-llvm -o - %s | FileCheck %s --check-prefix=NOTBAA
// RUN: %clang_cc1 -triple %itanium_abi_triple -emit-llvm -o - -O2  -relaxed-aliasing %s | FileCheck %s --check-prefix=NOTBAA
//
// Check that we generate TBAA for vtable pointer loads and stores.
// When -fthread-sanitizer is used TBAA should be generated at all opt levels
// even if -relaxed-aliasing is present.
struct A {
  virtual int foo() const ;
  virtual ~A();
};

void CreateA() {
  new A;
}

void CallFoo(A *a) {
  a->foo();
}

// CHECK: %{{.*}} = load {{.*}} !tbaa ![[NUM:[0-9]+]]
// CHECK: store {{.*}} !tbaa ![[NUM]]
// CHECK: [[NUM]] = metadata !{metadata [[TYPE:!.*]], metadata [[TYPE]], i64 0}
// CHECK: [[TYPE]] = metadata !{metadata !"vtable pointer", metadata !{{.*}}
// NOTBAA-NOT: = metadata !{metadata !"Simple C/C++ TBAA"}
