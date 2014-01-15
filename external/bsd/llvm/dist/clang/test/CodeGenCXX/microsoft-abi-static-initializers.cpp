// RUN: %clang_cc1 -emit-llvm %s -o - -mconstructor-aliases -triple=i386-pc-win32 | FileCheck %s

// CHECK: @llvm.global_ctors = appending global [2 x { i32, void ()* }]
// CHECK: [{ i32, void ()* } { i32 65535, void ()* @"\01??__Efoo@?$B@H@@YAXXZ" },
// CHECK:  { i32, void ()* } { i32 65535, void ()* @_GLOBAL__I_a }]

struct S {
  S();
  ~S();
};

S s;

// CHECK: define internal void @"\01??__Es@@YAXXZ"() [[NUW:#[0-9]+]]
// CHECK: %{{[.0-9A-Z_a-z]+}} = call x86_thiscallcc %struct.S* @"\01??0S@@QAE@XZ"
// CHECK: call i32 @atexit(void ()* @"\01??__Fs@@YAXXZ")
// CHECK: ret void

// CHECK: define internal void @"\01??__Fs@@YAXXZ"() [[NUW]] {
// CHECK: call x86_thiscallcc void @"\01??1S@@QAE@XZ"
// CHECK: ret void

void StaticLocal() {
  static S TheS;
}
// CHECK-LABEL: define void @"\01?StaticLocal@@YAXXZ"()
// CHECK: load i32* @"\01?$S1@?1??StaticLocal@@YAXXZ@4IA"
// CHECK: store i32 {{.*}}, i32* @"\01?$S1@?1??StaticLocal@@YAXXZ@4IA"
// CHECK: ret

void MultipleStatics() {
  static S S1;
  static S S2;
  static S S3;
  static S S4;
  static S S5;
  static S S6;
  static S S7;
  static S S8;
  static S S9;
  static S S10;
  static S S11;
  static S S12;
  static S S13;
  static S S14;
  static S S15;
  static S S16;
  static S S17;
  static S S18;
  static S S19;
  static S S20;
  static S S21;
  static S S22;
  static S S23;
  static S S24;
  static S S25;
  static S S26;
  static S S27;
  static S S28;
  static S S29;
  static S S30;
  static S S31;
  static S S32;
  static S S33;
  static S S34;
  static S S35;
}
// CHECK-LABEL: define void @"\01?MultipleStatics@@YAXXZ"()
// CHECK: load i32* @"\01?$S1@?1??MultipleStatics@@YAXXZ@4IA"
// CHECK: and i32 {{.*}}, 1
// CHECK: and i32 {{.*}}, 2
// CHECK: and i32 {{.*}}, 4
// CHECK: and i32 {{.*}}, 8
// CHECK: and i32 {{.*}}, 16
//   ...
// CHECK: and i32 {{.*}}, -2147483648
// CHECK: load i32* @"\01?$S1@?1??MultipleStatics@@YAXXZ@4IA1"
// CHECK: and i32 {{.*}}, 1
// CHECK: and i32 {{.*}}, 2
// CHECK: and i32 {{.*}}, 4
// CHECK: ret

// Force WeakODRLinkage by using templates
class A {
 public:
  A() {}
  ~A() {}
};

template<typename T>
class B {
 public:
  static A foo;
};

template<typename T> A B<T>::foo;

inline S &UnreachableStatic() {
  if (0) {
    static S s; // bit 1
    return s;
  }
  static S s; // bit 2
  return s;
}

// CHECK-LABEL: define linkonce_odr %struct.S* @"\01?UnreachableStatic@@YAAAUS@@XZ"()
// CHECK: and i32 {{.*}}, 2
// CHECK: or i32 {{.*}}, 2
// CHECK: ret

inline S &getS() {
  static S TheS;
  return TheS;
}

// CHECK-LABEL: define linkonce_odr %struct.S* @"\01?getS@@YAAAUS@@XZ"
// CHECK: load i32* @"\01??_B?1??getS@@YAAAUS@@XZ@51"
// CHECK: and i32 {{.*}}, 1
// CHECK: icmp ne i32 {{.*}}, 0
// CHECK: br i1
//   init:
// CHECK: or i32 {{.*}}, 1
// CHECK: store i32 {{.*}}, i32* @"\01??_B?1??getS@@YAAAUS@@XZ@51"
// CHECK: call x86_thiscallcc %struct.S* @"\01??0S@@QAE@XZ"(%struct.S* @"\01?TheS@?1??getS@@YAAAUS@@XZ@4U2@A")
// CHECK: call i32 @atexit(void ()* @"\01??__FTheS@?1??getS@@YAAAUS@@XZ@YAXXZ")
// CHECK: br label
//   init.end:
// CHECK: ret %struct.S* @"\01?TheS@?1??getS@@YAAAUS@@XZ@4U2@A"

void force_usage() {
  UnreachableStatic();
  getS();
  (void)B<int>::foo;  // (void) - force usage
}

// CHECK: define internal void @"\01??__Efoo@?$B@H@@YAXXZ"() [[NUW]]
// CHECK: %{{[.0-9A-Z_a-z]+}} = call x86_thiscallcc %class.A* @"\01??0A@@QAE@XZ"
// CHECK: call i32 @atexit(void ()* @"\01??__Ffoo@?$B@H@@YAXXZ")
// CHECK: ret void

// CHECK: define linkonce_odr x86_thiscallcc %class.A* @"\01??0A@@QAE@XZ"

// CHECK: define linkonce_odr x86_thiscallcc void @"\01??1A@@QAE@XZ"

// CHECK: define internal void @"\01??__Ffoo@?$B@H@@YAXXZ"
// CHECK: call x86_thiscallcc void @"\01??1A@@QAE@XZ"{{.*}}foo
// CHECK: ret void

// CHECK: define internal void @_GLOBAL__I_a() [[NUW]] {
// CHECK: call void @"\01??__Es@@YAXXZ"()
// CHECK: ret void

// CHECK: attributes [[NUW]] = { nounwind }
