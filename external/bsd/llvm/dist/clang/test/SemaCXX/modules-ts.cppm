// RUN:     %clang_cc1 -std=c++1z -fmodules-ts -emit-module-interface %s -o %t.pcm -verify -DTEST=0
// RUN:     %clang_cc1 -std=c++1z -fmodules-ts -emit-module-interface %s -o %t.pcm -verify -Dmodule=int -DTEST=1
// RUN: not %clang_cc1 -std=c++1z -fmodules-ts -emit-module-interface %s -fmodule-file=%t.pcm -o %t.pcm -DTEST=2 2>&1 | FileCheck %s --check-prefix=CHECK-2
// RUN:     %clang_cc1 -std=c++1z -fmodules-ts -emit-module-interface %s -fmodule-file=%t.pcm -o %t.pcm -verify -Dfoo=bar -DTEST=3

#if TEST == 0
// expected-no-diagnostics
#endif

module foo;
#if TEST == 1
// expected-error@-2 {{expected module declaration at start of module interface}}
#elif TEST == 2
// CHECK-2: error: redefinition of module 'foo'
#endif

static int m; // ok, internal linkage, so no redefinition error
int n;
#if TEST == 3
// expected-error@-2 {{redefinition of '}}
// expected-note@-3 {{previous}}
#endif

#if TEST == 0
export {
  int a;
  int b;
  constexpr int *p = &n;
}
export int c;

namespace N {
  export void f() {}
}

export struct T {} t;
#elif TEST == 3
int use_a = a; // expected-error {{declaration of 'a' must be imported from module 'foo' before it is required}}
// expected-note@-13 {{previous}}

#undef foo
import foo;

export {} // expected-error {{export declaration cannot be empty}}
export { ; }
export { static_assert(true); }

// FIXME: These diagnostics are not very good.
export import foo; // expected-error {{expected unqualified-id}}
export { import foo; } // expected-error {{expected unqualified-id}}

int use_b = b;
int use_n = n; // FIXME: this should not be visible, because it is not exported

extern int n;
static_assert(&n == p); // FIXME: these are not the same entity
#endif


#if TEST == 1
struct S {
  export int n; // expected-error {{expected member name or ';'}}
  export static int n; // expected-error {{expected member name or ';'}}
};
#endif

// FIXME: Exports of declarations without external linkage are disallowed.
// Exports of declarations with non-external-linkage types are disallowed.

// Cannot export within another export. This isn't precisely covered by the
// language rules right now, but (per personal correspondence between zygoloid
// and gdr) is the intent.
#if TEST == 1
export {
  extern "C++" {
    namespace NestedExport {
      export { // expected-error {{appears within another export}}
        int q;
      }
    }
  }
}
#endif
