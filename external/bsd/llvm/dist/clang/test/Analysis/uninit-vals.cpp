// RUN: %clang_cc1 -analyze -analyzer-checker=core.builtin -verify -DCHECK_FOR_CRASH %s
// RUN: %clang_cc1 -analyze -analyzer-checker=core -verify %s

#ifdef CHECK_FOR_CRASH
// expected-no-diagnostics
#endif

namespace PerformTrivialCopyForUndefs {
struct A {
  int x;
};

struct B {
  A a;
};

struct C {
  B b;
};

void foo() {
  C c1;
  C *c2;
#ifdef CHECK_FOR_CRASH
  // If the value of variable is not defined and checkers that check undefined
  // values are not enabled, performTrivialCopy should be able to handle the
  // case with undefined values, too.
  c1.b.a = c2->b.a;
#else
  c1.b.a = c2->b.a; // expected-warning{{Function call argument is an uninitialized value}}
#endif
}
}

