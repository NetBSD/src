// RUN: %clang_cc1 -analyze -analyzer-checker=debug.DumpCFG -triple x86_64-apple-darwin12 -std=c++11 %s > %t 2>&1
// RUN: FileCheck --input-file=%t %s

// CHECK: ENTRY
// CHECK-NEXT: Succs (1): B1
// CHECK: [B1]
// CHECK: Succs (21): B2 B3 B4 B5 B6 B7 B8 B9
// CHECK: B10 B11 B12 B13 B14 B15 B16 B17 B18 B19
// CHECK: B20 B21 B0
// CHECK: [B0 (EXIT)]
// CHECK-NEXT: Preds (21): B2 B3 B4 B5 B6 B7 B8 B9
// CHECK-NEXT: B10 B11 B12 B13 B14 B15 B16 B17 B18 B19
// CHECK-NEXT: B20 B21 B1
void checkWrap(int i) {
  switch(i) {
    case 0: break;
    case 1: break;
    case 2: break;
    case 3: break;
    case 4: break;
    case 5: break;
    case 6: break;
    case 7: break;
    case 8: break;
    case 9: break;
    case 10: break;
    case 11: break;
    case 12: break;
    case 13: break;
    case 14: break;
    case 15: break;
    case 16: break;
    case 17: break;
    case 18: break;
    case 19: break;
  }
}

// CHECK: ENTRY
// CHECK-NEXT: Succs (1): B1
// CHECK: [B1]
// CHECK-NEXT:   1: int i;
// CHECK-NEXT:   2: int j;
// CHECK-NEXT:   3: 1
// CHECK-NEXT:   4: int k = 1;
// CHECK-NEXT:   5: int l;
// CHECK-NEXT:   6: 2
// CHECK-NEXT:   7: int m = 2;
// CHECK-NEXT: CXXConstructExpr
// CHECK-NEXT:   9: struct standalone myStandalone;
// CHECK-NEXT: CXXConstructExpr
// CHECK-NEXT:  11: struct <anonymous struct at {{.*}}> myAnon;
// CHECK-NEXT: CXXConstructExpr
// CHECK-NEXT:  13: struct named myNamed;
// CHECK-NEXT:   Preds (1): B2
// CHECK-NEXT:   Succs (1): B0
void checkDeclStmts() {
  int i, j;
  int k = 1, l, m = 2;

  struct standalone { int x, y; };
  struct standalone myStandalone;

  struct { int x, y; } myAnon;

  struct named { int x, y; } myNamed;

  static_assert(1, "abc");
}

// CHECK: ENTRY
// CHECK-NEXT: Succs (1): B1
// CHECK: [B1]
// CHECK-NEXT:   1: e
// CHECK-NEXT:   2: [B1.1] (ImplicitCastExpr, LValueToRValue, enum EmptyE)
// CHECK-NEXT:   3: [B1.2] (ImplicitCastExpr, IntegralCast, int)
// CHECK-NEXT:   T: switch [B1.3]
// CHECK-NEXT:   Preds (1): B2
// CHECK-NEXT:   Succs (1): B0
// CHECK: [B0 (EXIT)]
// CHECK-NEXT:   Preds (1): B1
enum EmptyE {};
void F(EmptyE e) {
  switch (e) {}
}

// CHECK: ENTRY
// CHECK-NEXT: Succs (1): B1
// CHECK: [B1]
// CHECK-NEXT:   1: __builtin_object_size
// CHECK-NEXT:   2: [B1.1] (ImplicitCastExpr, BuiltinFnToFnPtr, unsigned long (*)(const void *, int))
// CHECK-NEXT:   3: [B1.2](dummy(), 0)
// CHECK-NEXT:   4: (void)[B1.3] (CStyleCastExpr, ToVoid, void)
// CHECK-NEXT:   Preds (1): B2
// CHECK-NEXT:   Succs (1): B0
// CHECK: [B0 (EXIT)]
// CHECK-NEXT:   Preds (1): B1
void testBuiltinSize() {
  extern int *dummy();
  (void)__builtin_object_size(dummy(), 0);
}


class A {
public:
  A() {}
  ~A() {}
};

// CHECK: [B2 (ENTRY)]
// CHECK-NEXT:   Succs (1): B1
// CHECK: [B1]
// CHECK-NEXT:   1:  (CXXConstructExpr, class A)
// CHECK-NEXT:   2: new A([B1.1])
// CHECK-NEXT:   3: A *a = new A();
// CHECK-NEXT:   4: a
// CHECK-NEXT:   5: [B1.4] (ImplicitCastExpr, LValueToRValue, class A *)
// CHECK-NEXT:   6: [B1.5]->~A() (Implicit destructor)
// CHECK-NEXT:   7: delete [B1.5]
// CHECK-NEXT:   Preds (1): B2
// CHECK-NEXT:   Succs (1): B0
// CHECK: [B0 (EXIT)]
// CHECK-NEXT:   Preds (1): B1
void test_deletedtor() {
  A *a = new A();
  delete a;
}

// CHECK: [B2 (ENTRY)]
// CHECK-NEXT:   Succs (1): B1
// CHECK: [B1]
// CHECK-NEXT:   1: 5
// CHECK-NEXT:   2:  (CXXConstructExpr, class A)
// CHECK-NEXT:   3: new A {{\[\[}}B1.1]]
// CHECK-NEXT:   4: A *a = new A [5];
// CHECK-NEXT:   5: a
// CHECK-NEXT:   6: [B1.5] (ImplicitCastExpr, LValueToRValue, class A *)
// CHECK-NEXT:   7: [B1.6]->~A() (Implicit destructor)
// CHECK-NEXT:   8: delete [] [B1.6]
// CHECK-NEXT:   Preds (1): B2
// CHECK-NEXT:   Succs (1): B0
// CHECK: [B0 (EXIT)]
// CHECK-NEXT:   Preds (1): B1
void test_deleteArraydtor() {
  A *a = new A[5];
  delete[] a;
}


namespace NoReturnSingleSuccessor {
  struct A {
    A();
    ~A();
  };

  struct B : public A {
    B();
    ~B() __attribute__((noreturn));
  };

// CHECK: ENTRY
// CHECK: 1: 1
// CHECK-NEXT: 2: return
// CHECK-NEXT: ~B() (Implicit destructor)
// CHECK-NEXT: Preds (1)
// CHECK-NEXT: Succs (1): B0
  int test1(int *x) {
    B b;
    if (x)
      return 1;
  }

// CHECK: ENTRY
// CHECK: 1: 1
// CHECK-NEXT: 2: return
// CHECK-NEXT: destructor
// CHECK-NEXT: Preds (1)
// CHECK-NEXT: Succs (1): B0
  int test2(int *x) {
    const A& a = B();
    if (x)
      return 1;
  }
}

// Test CFG support for "extending" an enum.
// CHECK:  [B7 (ENTRY)]
// CHECK-NEXT:    Succs (1): B2
// CHECK:  [B1]
// CHECK-NEXT:    1: x
// CHECK-NEXT:    2: [B1.1] (ImplicitCastExpr, LValueToRValue, int)
// CHECK-NEXT:    3: return [B1.2];
// CHECK-NEXT:    Preds (4): B3 B4 B5 B6
// CHECK-NEXT:    Succs (1): B0
// CHECK:  [B2]
// CHECK-NEXT:    1: 0
// CHECK-NEXT:    2: int x = 0;
// CHECK-NEXT:    3: value
// CHECK-NEXT:    4: [B2.3] (ImplicitCastExpr, LValueToRValue, enum MyEnum)
// CHECK-NEXT:    5: [B2.4] (ImplicitCastExpr, IntegralCast, int)
// CHECK-NEXT:    T: switch [B2.5]
// CHECK-NEXT:    Preds (1): B7
// CHECK-NEXT:    Succs (5): B3 B4 B5 B6 NULL
// CHECK:  [B3]
// CHECK-NEXT:   case D:
// CHECK-NEXT:    1: 4
// CHECK-NEXT:    2: x
// CHECK-NEXT:    3: [B3.2] = [B3.1]
// CHECK-NEXT:    T: break;
// CHECK-NEXT:    Preds (1): B2
// CHECK-NEXT:    Succs (1): B1
// CHECK:  [B4]
// CHECK-NEXT:   case C:
// CHECK-NEXT:    1: 3
// CHECK-NEXT:    2: x
// CHECK-NEXT:    3: [B4.2] = [B4.1]
// CHECK-NEXT:    T: break;
// CHECK-NEXT:    Preds (1): B2
// CHECK-NEXT:    Succs (1): B1
// CHECK:  [B5]
// CHECK-NEXT:   case B:
// CHECK-NEXT:    1: 2
// CHECK-NEXT:    2: x
// CHECK-NEXT:    3: [B5.2] = [B5.1]
// CHECK-NEXT:    T: break;
// CHECK-NEXT:    Preds (1): B2
// CHECK-NEXT:    Succs (1): B1
// CHECK:  [B6]
// CHECK-NEXT:   case A:
// CHECK-NEXT:    1: 1
// CHECK-NEXT:    2: x
// CHECK-NEXT:    3: [B6.2] = [B6.1]
// CHECK-NEXT:    T: break;
// CHECK-NEXT:    Preds (1): B2
// CHECK-NEXT:    Succs (1): B1
// CHECK:  [B0 (EXIT)]
// CHECK-NEXT:    Preds (1): B1
enum MyEnum { A, B, C };
static const enum MyEnum D = (enum MyEnum) 32;

int test_enum_with_extension(enum MyEnum value) {
  int x = 0;
  switch (value) {
    case A: x = 1; break;
    case B: x = 2; break;
    case C: x = 3; break;
    case D: x = 4; break;
  }
  return x;
}

// CHECK:  [B7 (ENTRY)]
// CHECK-NEXT:    Succs (1): B2
// CHECK:  [B1]
// CHECK-NEXT:    1: x
// CHECK-NEXT:    2: [B1.1] (ImplicitCastExpr, LValueToRValue, int)
// CHECK-NEXT:    3: return [B1.2];
// CHECK-NEXT:    Preds (4): B3 B4 B5 B6
// CHECK-NEXT:    Succs (1): B0
// CHECK:  [B2]
// CHECK-NEXT:    1: 0
// CHECK-NEXT:    2: int x = 0;
// CHECK-NEXT:    3: value
// CHECK-NEXT:    4: [B2.3] (ImplicitCastExpr, LValueToRValue, enum MyEnum)
// CHECK-NEXT:    5: [B2.4] (ImplicitCastExpr, IntegralCast, int)
// CHECK-NEXT:    T: switch [B2.5]
// CHECK-NEXT:    Preds (1): B7
// CHECK-NEXT:    Succs (4): B4 B5 B6 NULL
// CHECK:  [B3]
// CHECK-NEXT:   default:
// CHECK-NEXT:    1: 4
// CHECK-NEXT:    2: x
// CHECK-NEXT:    3: [B3.2] = [B3.1]
// CHECK-NEXT:    T: break;
// CHECK-NEXT:    Succs (1): B1
// CHECK:  [B4]
// CHECK-NEXT:   case C:
// CHECK-NEXT:    1: 3
// CHECK-NEXT:    2: x
// CHECK-NEXT:    3: [B4.2] = [B4.1]
// CHECK-NEXT:    T: break;
// CHECK-NEXT:    Preds (1): B2
// CHECK-NEXT:    Succs (1): B1
// CHECK:  [B5]
// CHECK-NEXT:   case B:
// CHECK-NEXT:    1: 2
// CHECK-NEXT:    2: x
// CHECK-NEXT:    3: [B5.2] = [B5.1]
// CHECK-NEXT:    T: break;
// CHECK-NEXT:    Preds (1): B2
// CHECK-NEXT:    Succs (1): B1
// CHECK:  [B6]
// CHECK-NEXT:   case A:
// CHECK-NEXT:    1: 1
// CHECK-NEXT:    2: x
// CHECK-NEXT:    3: [B6.2] = [B6.1]
// CHECK-NEXT:    T: break;
// CHECK-NEXT:    Preds (1): B2
// CHECK-NEXT:    Succs (1): B1
// CHECK:  [B0 (EXIT)]
// CHECK-NEXT:    Preds (1): B1
int test_enum_with_extension_default(enum MyEnum value) {
  int x = 0;
  switch (value) {
    case A: x = 1; break;
    case B: x = 2; break;
    case C: x = 3; break;
    default: x = 4; break;
  }
  return x;
}