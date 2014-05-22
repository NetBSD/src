// RUN: %clang_cc1 -fno-rtti -emit-llvm -triple=i386-pc-win32 %s -o - | FileCheck %s --check-prefix=CHECK32
// RUN: %clang_cc1 -fno-rtti -emit-llvm -triple=x86_64-pc-win32 %s -o - | FileCheck %s --check-prefix=CHECK64

struct S {
  int x, y, z;
};

struct C {
  virtual void foo();
  virtual int bar(int, double);
  virtual S baz(int);
};

namespace {
struct D {
  virtual void foo();
};
}

void f() {
  void (C::*ptr)();
  ptr = &C::foo;
  ptr = &C::foo; // Don't crash trying to define the thunk twice :)

  int (C::*ptr2)(int, double);
  ptr2 = &C::bar;

  S (C::*ptr3)(int);
  ptr3 = &C::baz;

  void (D::*ptr4)();
  ptr4 = &D::foo;

// CHECK32-LABEL: define void @"\01?f@@YAXXZ"()
// CHECK32: store i8* bitcast (void (%struct.C*)* @"\01??_9C@@$BA@AE" to i8*), i8** %ptr
// CHECK32: store i8* bitcast (i32 (%struct.C*, i32, double)* @"\01??_9C@@$B3AE" to i8*), i8** %ptr2
// CHECK32: store i8* bitcast (void (%struct.S*, %struct.C*, i32)* @"\01??_9C@@$B7AE" to i8*), i8** %ptr3
// CHECK32: store i8* bitcast (void (%"struct.<anonymous namespace>::D"*)* @"\01??_9D@?A@@$BA@AE" to i8*), i8** %ptr4
// CHECK32: }
//
// CHECK64-LABEL: define void @"\01?f@@YAXXZ"()
// CHECK64: store i8* bitcast (void (%struct.C*)* @"\01??_9C@@$BA@AA" to i8*), i8** %ptr
// CHECK64: store i8* bitcast (i32 (%struct.C*, i32, double)* @"\01??_9C@@$B7AA" to i8*), i8** %ptr2
// CHECK64: store i8* bitcast (void (%struct.S*, %struct.C*, i32)* @"\01??_9C@@$BBA@AA" to i8*), i8** %ptr3
// CHECK64: store i8* bitcast (void (%"struct.<anonymous namespace>::D"*)* @"\01??_9D@?A@@$BA@AA" to i8*), i8** %ptr
// CHECK64: }
}


// Thunk for calling the 1st virtual function in C with no parameters.
// CHECK32-LABEL: define linkonce_odr x86_thiscallcc void @"\01??_9C@@$BA@AE"(%struct.C* %this) unnamed_addr
// CHECK32: [[VPTR:%.*]] = getelementptr inbounds void (%struct.C*)** %{{.*}}, i64 0
// CHECK32: [[CALLEE:%.*]] = load void (%struct.C*)** [[VPTR]]
// CHECK32: call x86_thiscallcc void [[CALLEE]](%struct.C* %{{.*}})
// CHECK32: ret void
// CHECK32: }
//
// CHECK64-LABEL: define linkonce_odr void @"\01??_9C@@$BA@AA"(%struct.C* %this) unnamed_addr
// CHECK64: [[VPTR:%.*]] = getelementptr inbounds void (%struct.C*)** %{{.*}}, i64 0
// CHECK64: [[CALLEE:%.*]] = load void (%struct.C*)** [[VPTR]]
// CHECK64: call void [[CALLEE]](%struct.C* %{{.*}})
// CHECK64: ret void
// CHECK64: }

// Thunk for calling the 2nd virtual function in C, taking int and double as parameters, returning int.
// CHECK32-LABEL: define linkonce_odr x86_thiscallcc i32 @"\01??_9C@@$B3AE"(%struct.C* %this, i32, double) unnamed_addr
// CHECK32: [[VPTR:%.*]] = getelementptr inbounds i32 (%struct.C*, i32, double)** %{{.*}}, i64 1
// CHECK32: [[CALLEE:%.*]] = load i32 (%struct.C*, i32, double)** [[VPTR]]
// CHECK32: [[CALL:%.*]] = call x86_thiscallcc i32 [[CALLEE]](%struct.C* %{{.*}}, i32 %{{.*}}, double %{{.*}})
// CHECK32: ret i32 [[CALL]]
// CHECK32: }
//
// CHECK64-LABEL: define linkonce_odr i32 @"\01??_9C@@$B7AA"(%struct.C* %this, i32, double) unnamed_addr
// CHECK64: [[VPTR:%.*]] = getelementptr inbounds i32 (%struct.C*, i32, double)** %{{.*}}, i64 1
// CHECK64: [[CALLEE:%.*]] = load i32 (%struct.C*, i32, double)** [[VPTR]]
// CHECK64: [[CALL:%.*]] = call i32 [[CALLEE]](%struct.C* %{{.*}}, i32 %{{.*}}, double %{{.*}})
// CHECK64: ret i32 [[CALL]]
// CHECK64: }

// Thunk for calling the 3rd virtual function in C, taking an int parameter, returning a struct.
// CHECK32-LABEL: define linkonce_odr x86_thiscallcc void @"\01??_9C@@$B7AE"(%struct.S* noalias sret %agg.result, %struct.C* %this, i32) unnamed_addr
// CHECK32: [[VPTR:%.*]] = getelementptr inbounds void (%struct.S*, %struct.C*, i32)** %{{.*}}, i64 2
// CHECK32: [[CALLEE:%.*]] = load void (%struct.S*, %struct.C*, i32)** [[VPTR]]
// CHECK32: call x86_thiscallcc void [[CALLEE]](%struct.S* sret %agg.result, %struct.C* %{{.*}}, i32 %{{.*}})
// CHECK32: ret void
// CHECK32: }
//
// CHECK64-LABEL: define linkonce_odr void @"\01??_9C@@$BBA@AA"(%struct.S* noalias sret %agg.result, %struct.C* %this, i32) unnamed_addr
// CHECK64: [[VPTR:%.*]] = getelementptr inbounds void (%struct.S*, %struct.C*, i32)** %{{.*}}, i64 2
// CHECK64: [[CALLEE:%.*]] = load void (%struct.S*, %struct.C*, i32)** [[VPTR]]
// CHECK64: call void [[CALLEE]](%struct.S* sret %agg.result, %struct.C* %{{.*}}, i32 %{{.*}})
// CHECK64: ret void
// CHECK64: }

// Thunk for calling the virtual function in internal class D.
// CHECK32-LABEL: define internal x86_thiscallcc void @"\01??_9D@?A@@$BA@AE"(%"struct.<anonymous namespace>::D"* %this) unnamed_addr
// CHECK32: [[VPTR:%.*]] = getelementptr inbounds void (%"struct.<anonymous namespace>::D"*)** %{{.*}}, i64 0
// CHECK32: [[CALLEE:%.*]] = load void (%"struct.<anonymous namespace>::D"*)** [[VPTR]]
// CHECK32: call x86_thiscallcc void [[CALLEE]](%"struct.<anonymous namespace>::D"* %{{.*}})
// CHECK32: ret void
// CHECK32: }
//
// CHECK64-LABEL: define internal void @"\01??_9D@?A@@$BA@AA"(%"struct.<anonymous namespace>::D"* %this) unnamed_addr
// CHECK64: [[VPTR:%.*]] = getelementptr inbounds void (%"struct.<anonymous namespace>::D"*)** %{{.*}}, i64 0
// CHECK64: [[CALLEE:%.*]] = load void (%"struct.<anonymous namespace>::D"*)** [[VPTR]]
// CHECK64: call void [[CALLEE]](%"struct.<anonymous namespace>::D"* %{{.*}})
// CHECK64: ret void
// CHECK64: }
