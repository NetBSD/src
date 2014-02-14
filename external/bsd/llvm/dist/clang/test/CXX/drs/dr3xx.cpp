// RUN: %clang_cc1 -std=c++98 %s -verify -fexceptions -fcxx-exceptions -pedantic-errors
// RUN: %clang_cc1 -std=c++11 %s -verify -fexceptions -fcxx-exceptions -pedantic-errors
// RUN: %clang_cc1 -std=c++1y %s -verify -fexceptions -fcxx-exceptions -pedantic-errors

namespace dr300 { // dr300: yes
  template<typename R, typename A> void f(R (&)(A)) {}
  int g(int);
  void h() { f(g); }
}

namespace dr301 { // dr301: yes
  // see also dr38
  struct S;
  template<typename T> void operator+(T, T);
  void operator-(S, S);

  void f() {
    bool a = (void(*)(S, S))operator+<S> <
             (void(*)(S, S))operator+<S>;
    bool b = (void(*)(S, S))operator- <
             (void(*)(S, S))operator-;
    bool c = (void(*)(S, S))operator+ <
             (void(*)(S, S))operator-; // expected-error {{expected '>'}}
  }

  template<typename T> void f() {
    typename T::template operator+<int> a; // expected-error {{typename specifier refers to a non-type template}} expected-error +{{}}
    // FIXME: This shouldn't say (null).
    class T::template operator+<int> b; // expected-error {{identifier followed by '<' indicates a class template specialization but (null) refers to a function template}}
    enum T::template operator+<int> c; // expected-error {{expected identifier}} expected-error {{does not declare anything}}
    enum T::template operator+<int>::E d; // expected-error {{qualified name refers into a specialization of function template 'T::template operator +'}} expected-error {{forward reference}}
    enum T::template X<int>::E e;
    T::template operator+<int>::foobar(); // expected-error {{qualified name refers into a specialization of function template 'T::template operator +'}}
    T::template operator+<int>(0); // ok
  }

  template<typename T> class operator&<T*> {}; // expected-error +{{}}
  template<typename T> class T::operator& {}; // expected-error +{{}}
  template<typename T> class S::operator&<T*> {}; // expected-error +{{}}
}

namespace dr302 { // dr302: yes
  struct A { A(); ~A(); };
#if __cplusplus < 201103L
  struct B { // expected-error {{implicit default constructor for 'dr302::B' must explicitly initialize the const member 'n'}}
    const int n; // expected-note {{declared here}}
    A a;
  } b = B(); // expected-note {{first required here}}
  // Trivial default constructor C::C() is not called here.
  struct C {
    const int n;
  } c = C();
#else
  struct B {
    const int n; // expected-note {{deleted because field 'n' of const-qualified type 'const int' would not be initialized}}
    A a;
  } b = B(); // expected-error {{call to implicitly-deleted default constructor}}
  // C::C() is called here, because even though it's trivial, it's deleted.
  struct C {
    const int n; // expected-note {{deleted because field 'n' of const-qualified type 'const int' would not be initialized}}
  } c = C(); // expected-error {{call to implicitly-deleted default constructor}}
  struct D {
    const int n = 0;
  } d = D();
#endif
}

// dr303: na

namespace dr304 { // dr304: yes
  typedef int &a;
  int n = a(); // expected-error {{requires an initializer}}

  struct S { int &b; };
  int m = S().b;
#if __cplusplus < 201103L
  // expected-error@-3 {{requires an initializer}}
  // expected-note@-3 {{in value-initialization}}
#else
  // expected-error@-5 {{deleted}}
  // expected-note@-7 {{reference}}
#endif
}

namespace dr305 { // dr305: yes
  struct A {
    typedef A C;
  };
  void f(A *a) {
    struct A {};
    a->~A();
    a->~C();
  }
  typedef A B;
  void g(B *b) {
    b->~B();
    b->~C();
  }
  void h(B *b) {
    struct B {}; // expected-note {{declared here}}
    b->~B(); // expected-error {{does not match}}
  }

  template<typename T> struct X {};
  void i(X<int>* x) {
    struct X {};
    x->~X<int>();
    x->~X();
    x->~X<char>(); // expected-error {{no member named}}
  }

#if __cplusplus >= 201103L
  struct Y {
    template<typename T> using T1 = Y;
  };
  template<typename T> using T2 = Y;
  void j(Y *y) {
    y->~T1<int>();
    y->~T2<int>();
  }
  struct Z {
    template<typename T> using T2 = T;
  };
  void k(Z *z) {
    // FIXME: This diagnostic is terrible.
    z->~T1<int>(); // expected-error {{'T1' following the 'template' keyword does not refer to a template}} expected-error +{{}}
    z->~T2<int>(); // expected-error {{no member named '~int'}}
    z->~T2<Z>();
  }
#endif
}

namespace dr306 { // dr306: no
  // FIXME: dup 39
  // FIXME: This should be accepted.
  struct A { struct B {}; }; // expected-note 2{{member}}
  struct C { typedef A::B B; }; // expected-note {{member}}
  struct D : A, A::B, C {};
  D::B b; // expected-error {{found in multiple base classes of different types}}
}

// dr307: na

namespace dr308 { // dr308: yes
  // This is mostly an ABI library issue.
  struct A {};
  struct B : A {};
  struct C : A {};
  struct D : B, C {};
  void f() {
    try {
      throw D();
    } catch (const A&) {
      // unreachable
    } catch (const B&) {
      // get here instead
    }
  }
}

// dr309: dup 485

namespace dr311 { // dr311: yes
  namespace X { namespace Y {} }
  namespace X::Y {} // expected-error {{must define each namespace separately}}
  namespace X {
    namespace X::Y {} // expected-error {{must define each namespace separately}}
  }
  // FIXME: The diagnostics here are not very good.
  namespace ::dr311::X {} // expected-error 2+{{}} // expected-warning {{extra qual}}
}

// dr312: dup 616

namespace dr313 { // dr313: dup 299 c++11
  struct A { operator int() const; };
  int *p = new int[A()];
#if __cplusplus < 201103L
  // FIXME: should this be available in c++98 mode? expected-error@-2 {{extension}}
#endif
}

// dr315: na
// dr316: sup 1004

namespace dr317 { // dr317: no
  void f() {}
  inline void f(); // FIXME: ill-formed

  int g();
  int n = g();
  inline int g() { return 0; }

  int h();
  int m = h();
  int h() { return 0; }
  inline int h(); // FIXME: ill-formed
}

namespace dr318 { // dr318: sup 1310
  struct A {};
  struct A::A a;
}

namespace dr319 { // dr319: no
  // FIXME: dup dr389
  // FIXME: We don't have a diagnostic for a name with linkage
  //        having a type without linkage.
  typedef struct {
    int i;
  } *ps;
  extern "C" void f(ps);
  void g(ps); // FIXME: ill-formed, type 'ps' has no linkage

  static enum { e } a1;
  enum { e2 } a2; // FIXME: ill-formed, enum type has no linkage

  enum { n1 = 1u };
  typedef int (*pa)[n1];
  pa parr; // ok, type has linkage despite using 'n1'

  template<typename> struct X {};

  void f() {
    struct A { int n; };
    extern A a; // FIXME: ill-formed
    X<A> xa;

    typedef A B;
    extern B b; // FIXME: ill-formed
    X<B> xb;

    const int n = 1;
    typedef int (*C)[n];
    extern C c; // ok
    X<C> xc;
  }
#if __cplusplus < 201103L
  // expected-error@-12 {{uses local type 'A'}}
  // expected-error@-9 {{uses local type 'A'}}
#endif
}

namespace dr320 { // dr320: yes
#if __cplusplus >= 201103L
  struct X {
    constexpr X() {}
    constexpr X(const X &x) : copies(x.copies + 1) {}
    unsigned copies = 0;
  };
  constexpr X f(X x) { return x; }
  constexpr unsigned g(X x) { return x.copies; }
  static_assert(f(X()).copies == g(X()) + 1, "expected one extra copy for return value");
#endif
}

namespace dr321 { // dr321: dup 557
  namespace N {
    template<int> struct A {
      template<int> struct B;
    };
    template<> template<> struct A<0>::B<0>;
    void f(A<0>::B<0>);
  }
  template<> template<> struct N::A<0>::B<0> {};

  template<typename T> void g(T t) { f(t); }
  template void g(N::A<0>::B<0>);

  namespace N {
    template<typename> struct I { friend bool operator==(const I&, const I&); };
  }
  N::I<int> i, j;
  bool x = i == j;
}

namespace dr322 { // dr322: yes
  struct A {
    template<typename T> operator T&();
  } a;
  int &r = static_cast<int&>(a);
  int &s = a;
}

// dr323: no

namespace dr324 { // dr324: yes
  struct S { int n : 1; } s; // expected-note 3{{bit-field is declared here}}
  int &a = s.n; // expected-error {{non-const reference cannot bind to bit-field}}
  int *b = &s.n; // expected-error {{address of bit-field}}
  int &c = (s.n = 0); // expected-error {{non-const reference cannot bind to bit-field}}
  int *d = &(s.n = 0); // expected-error {{address of bit-field}}
  int &e = true ? s.n : s.n; // expected-error {{non-const reference cannot bind to bit-field}}
  int *f = &(true ? s.n : s.n); // expected-error {{address of bit-field}}
  int &g = (void(), s.n); // expected-error {{non-const reference cannot bind to bit-field}}
  int *h = &(void(), s.n); // expected-error {{address of bit-field}}
}

namespace dr326 { // dr326: yes
  struct S {};
  int test[__is_trivially_constructible(S, const S&) ? 1 : -1];
}

namespace dr327 { // dr327: dup 538
  struct A;
  class A {};

  class B;
  struct B {};
}

namespace dr328 { // dr328: yes
  struct A; // expected-note 3{{forward declaration}}
  struct B { A a; }; // expected-error {{incomplete}}
  template<typename> struct C { A a; }; // expected-error {{incomplete}}
  A *p = new A[0]; // expected-error {{incomplete}}
}

namespace dr329 { // dr329: 3.5
  struct B {};
  template<typename T> struct A : B {
    friend void f(A a) { g(a); }
    friend void h(A a) { g(a); } // expected-error {{undeclared}}
    friend void i(B b) {} // expected-error {{redefinition}} expected-note {{previous}}
  };
  A<int> a;
  A<char> b; // expected-note {{instantiation}}

  void test() {
    h(a); // expected-note {{instantiation}}
  }
}

namespace dr331 { // dr331: yes
  struct A {
    A(volatile A&); // expected-note {{candidate}}
  } const a, b(a); // expected-error {{no matching constructor}}
}

namespace dr332 { // dr332: dup 557
  void f(volatile void); // expected-error {{'void' as parameter must not have type qualifiers}}
  void g(const void); // expected-error {{'void' as parameter must not have type qualifiers}}
  void h(int n, volatile void); // expected-error {{'void' must be the first and only parameter}}
}

namespace dr333 { // dr333: yes
  int n = 0;
  int f(int(n));
  int g((int(n)));
  int h = f(g);
}

namespace dr334 { // dr334: yes
  template<typename T> void f() {
    T x;
    f((x, 123));
  }
  struct S {
    friend S operator,(S, int);
    friend void f(S);
  };
  template void f<S>();
}

// dr335: no

namespace dr336 { // dr336: yes
  namespace Pre {
    template<class T1> class A {
      template<class T2> class B {
        template<class T3> void mf1(T3);
        void mf2();
      };
    };
    template<> template<class X> class A<int>::B {};
    template<> template<> template<class T> void A<int>::B<double>::mf1(T t) {} // expected-error {{does not match}}
    template<class Y> template<> void A<Y>::B<double>::mf2() {} // expected-error {{does not refer into a class}}
  }
  namespace Post {
    template<class T1> class A {
      template<class T2> class B {
        template<class T3> void mf1(T3);
        void mf2();
      };
    };
    template<> template<class X> class A<int>::B {
      template<class T> void mf1(T);
    };
    template<> template<> template<class T> void A<int>::B<double>::mf1(T t) {}
    // FIXME: This diagnostic isn't very good.
    template<class Y> template<> void A<Y>::B<double>::mf2() {} // expected-error {{does not refer into a class}}
  }
}

namespace dr337 { // dr337: yes
  template<typename T> void f(T (*)[1]);
  template<typename T> int &f(...);

  struct A { virtual ~A() = 0; };
  int &r = f<A>(0);

  // FIXME: The language rules here are completely broken. We cannot determine
  // whether an incomplete type is abstract. See DR1640, which will probably
  // supersede this one and remove this rule.
  struct B;
  int &s = f<B>(0); // expected-error {{of type 'void'}}
  struct B { virtual ~B() = 0; };
}

namespace dr339 { // dr339: yes
  template <int I> struct A { static const int value = I; };

  char xxx(int);
  char (&xxx(float))[2];

  template<class T> A<sizeof(xxx((T)0))> f(T) {} // expected-note {{candidate}}

  void test() {
    A<1> a = f(0);
    A<2> b = f(0.0f);
    A<3> c = f("foo"); // expected-error {{no matching function}}
  }


  char f(int);
  int f(...);

  template <class T> struct conv_int {
    static const bool value = sizeof(f(T())) == 1;
  };

  template <class T> bool conv_int2(A<sizeof(f(T()))> p);

  template<typename T> A<sizeof(f(T()))> make_A();

  int a[conv_int<char>::value ? 1 : -1];
  bool b = conv_int2<char>(A<1>());
  A<1> c = make_A<char>();
}

namespace dr340 { // dr340: yes
  struct A { A(int); };
  struct B { B(A, A, int); };
  int x, y;
  B b(A(x), A(y), 3);
}

namespace dr341 { // dr341: sup 1708
  namespace A {
    int n;
    extern "C" int &dr341_a = n; // expected-note {{previous}} expected-note {{declared with C language linkage here}}
  }
  namespace B {
    extern "C" int &dr341_a = dr341_a; // expected-error {{redefinition}}
  }
  extern "C" void dr341_b(); // expected-note {{declared with C language linkage here}}
}
int dr341_a; // expected-error {{declaration of 'dr341_a' in global scope conflicts with declaration with C language linkage}}
int dr341_b; // expected-error {{declaration of 'dr341_b' in global scope conflicts with declaration with C language linkage}}
int dr341_c; // expected-note {{declared in global scope here}}
int dr341_d; // expected-note {{declared in global scope here}}
namespace dr341 {
  extern "C" int dr341_c; // expected-error {{declaration of 'dr341_c' with C language linkage conflicts with declaration in global scope}}
  extern "C" void dr341_d(); // expected-error {{declaration of 'dr341_d' with C language linkage conflicts with declaration in global scope}}

  namespace A { extern "C" int dr341_e; } // expected-note {{previous}}
  namespace B { extern "C" void dr341_e(); } // expected-error {{redefinition of 'dr341_e' as different kind of symbol}}
}

// dr342: na

namespace dr345 { // dr345: yes
  struct A {
    struct X {};
    int X; // expected-note {{here}}
  };
  struct B {
    struct X {};
  };
  template <class T> void f(T t) { typename T::X x; } // expected-error {{refers to non-type member 'X'}}
  void f(A a, B b) {
    f(b);
    f(a); // expected-note {{instantiation}}
  }
}

// dr346: na

namespace dr347 { // dr347: yes
  struct base {
    struct nested;
    static int n;
    static void f();
    void g();
  };

  struct derived : base {};

  struct derived::nested {}; // expected-error {{no struct named 'nested'}}
  int derived::n; // expected-error {{no member named 'n'}}
  void derived::f() {} // expected-error {{does not match any}}
  void derived::g() {} // expected-error {{does not match any}}
}

// dr348: na

namespace dr349 { // dr349: no
  struct A {
    template <class T> operator T ***() {
      int ***p = 0;
      return p; // expected-error {{cannot initialize return object of type 'const int ***' with an lvalue of type 'int ***'}}
    }
  };

  // FIXME: This is valid.
  A a;
  const int *const *const *p1 = a; // expected-note {{in instantiation of}}

  struct B {
    template <class T> operator T ***() {
      const int ***p = 0;
      return p;
    }
  };

  // FIXME: This is invalid.
  B b;
  const int *const *const *p2 = b;
}

// dr351: na

namespace dr352 { // dr352: yes
  namespace example1 {
    namespace A {
      enum E {};
      template<typename R, typename A> void foo(E, R (*)(A)); // expected-note 2{{couldn't infer template argument 'R'}}
    }

    template<typename T> void arg(T);
    template<typename T> int arg(T) = delete; // expected-note {{here}} expected-error 0-1{{extension}}

    void f(A::E e) {
      foo(e, &arg); // expected-error {{no matching function}}

      using A::foo;
      foo<int, int>(e, &arg); // expected-error {{deleted}}
    }

    int arg(int);

    void g(A::E e) {
      foo(e, &arg); // expected-error {{no matching function}}

      using A::foo;
      foo<int, int>(e, &arg); // ok, uses non-template
    }
  }

  namespace contexts {
    template<int I> void f1(int (&)[I]);
    template<int I> void f2(int (&)[I+1]); // expected-note {{couldn't infer}}
    template<int I> void f3(int (&)[I+1], int (&)[I]);
    void f() {
      int a[4];
      int b[3];
      f1(a);
      f2(a); // expected-error {{no matching function}}
      f3(a, b);
    }

    template<int I> struct S {};
    template<int I> void g1(S<I>);
    template<int I> void g2(S<I+1>); // expected-note {{couldn't infer}}
    template<int I> void g3(S<I+1>, S<I>);
    void g() {
      S<4> a;
      S<3> b;
      g1(a);
      g2(a); // expected-error {{no matching function}}
      g3(a, b);
    }

    template<typename T> void h1(T = 0); // expected-note {{couldn't infer}}
    template<typename T> void h2(T, T = 0);
    void h() {
      h1(); // expected-error {{no matching function}}
      h1(0);
      h1<int>();
      h2(0);
    }

    template<typename T> int tmpl(T);
    template<typename R, typename A> void i1(R (*)(A)); // expected-note 3{{couldn't infer}}
    template<typename R, typename A> void i2(R, A, R (*)(A)); // expected-note {{not viable}}
    void i() {
      extern int single(int);
      i1(single);
      i2(0, 0, single);

      extern int ambig(float), ambig(int);
      i1(ambig); // expected-error {{no matching function}}
      i2(0, 0, ambig);

      extern void no_match(float), no_match(int);
      i1(no_match); // expected-error {{no matching function}}
      i2(0, 0, no_match); // expected-error {{no matching function}}

      i1(tmpl); // expected-error {{no matching function}}
      i2(0, 0, tmpl);
    }
  }

  template<typename T> struct is_int;
  template<> struct is_int<int> {};

  namespace example2 {
    template<typename T> int f(T (*p)(T)) { is_int<T>(); }
    int g(int);
    int g(char);
    int i = f(g);
  }

  namespace example3 {
    template<typename T> int f(T, T (*p)(T)) { is_int<T>(); }
    int g(int);
    char g(char);
    int i = f(1, g);
  }

  namespace example4 {
    template <class T> int f(T, T (*p)(T)) { is_int<T>(); }
    char g(char);
    template <class T> T g(T);
    int i = f(1, g);
  }

  namespace example5 {
    template<int I> class A {};
    template<int I> void g(A<I+1>); // expected-note {{couldn't infer}}
    template<int I> void f(A<I>, A<I+1>);
    void h(A<1> a1, A<2> a2) {
      g(a1); // expected-error {{no matching function}}
      g<0>(a1);
      f(a1, a2);
    }
  }
}

// dr353 needs an IRGen test.

namespace dr354 { // dr354: yes c++11
  // FIXME: Should we allow this in C++98 too?
  struct S {};

  template<int*> struct ptr {}; // expected-note +{{here}}
  ptr<0> p0;
  ptr<(int*)0> p1;
  ptr<(float*)0> p2;
  ptr<(int S::*)0> p3;
#if __cplusplus < 201103L
  // expected-error@-5 {{does not refer to any decl}}
  // expected-error@-5 {{does not refer to any decl}}
  // expected-error@-5 {{does not refer to any decl}}
  // expected-error@-5 {{does not refer to any decl}}
#else
  // expected-error@-10 {{must be cast}}
  // ok
  // expected-error@-10 {{does not match}}
  // expected-error@-10 {{does not match}}
#endif

  template<int*> int both();
  template<int> int both();
  int b0 = both<0>();
  int b1 = both<(int*)0>();
#if __cplusplus < 201103L
  // expected-error@-2 {{no matching function}}
  // expected-note@-6 {{candidate}}
  // expected-note@-6 {{candidate}}
#endif

  template<int S::*> struct ptr_mem {}; // expected-note +{{here}}
  ptr_mem<0> m0;
  ptr_mem<(int S::*)0> m1;
  ptr_mem<(float S::*)0> m2;
  ptr_mem<(int *)0> m3;
#if __cplusplus < 201103L
  // expected-error@-5 {{cannot be converted}}
  // expected-error@-5 {{is not a pointer to member constant}}
  // expected-error@-5 {{cannot be converted}}
  // expected-error@-5 {{cannot be converted}}
#else
  // expected-error@-10 {{must be cast}}
  // ok
  // expected-error@-10 {{does not match}}
  // expected-error@-10 {{does not match}}
#endif
}

struct dr355_S; // dr355: yes
struct ::dr355_S {}; // expected-warning {{extra qualification}}
namespace dr355 { struct ::dr355_S s; }

// dr356: na

namespace dr357 { // dr357: yes
  template<typename T> struct A {
    void f() const; // expected-note {{const qualified}}
  };
  template<typename T> void A<T>::f() {} // expected-error {{does not match}}

  struct B {
    template<typename T> void f();
  };
  template<typename T> void B::f() const {} // expected-error {{does not match}}
}

namespace dr358 { // dr358: yes
  extern "C" void dr358_f();
  namespace N {
    int var;
    extern "C" void dr358_f() { var = 10; }
  }
}

namespace dr359 { // dr359: yes
  // Note, the example in the DR is wrong; it doesn't contain an anonymous
  // union.
  struct E {
    union {
      struct {
        int x;
      } s;
    } v;

    union {
      struct { // expected-error {{extension}}
        int x;
      } s;

      struct S { // expected-error {{types cannot be declared in an anonymous union}}
        int x;
      } t;

      union { // expected-error {{extension}}
        int u;
      };
    };
  };
}

// dr362: na
// dr363: na

namespace dr364 { // dr364: yes
  struct S {
    static void f(int);
    void f(char);
  };

  void g() {
    S::f('a'); // expected-error {{call to non-static}}
    S::f(0);
  }
}

#if "foo" // expected-error {{invalid token}} dr366: yes
#endif

namespace dr367 { // dr367: yes
  // FIXME: These diagnostics are terrible. Don't diagnose an ill-formed global
  // array as being a VLA!
  int a[true ? throw 0 : 4]; // expected-error 2{{variable length array}}
  int b[true ? 4 : throw 0];
  int c[true ? *new int : 4]; // expected-error 2{{variable length array}}
  int d[true ? 4 : *new int];
#if __cplusplus < 201103L
  // expected-error@-4 {{variable length array}} expected-error@-4 {{constant expression}}
  // expected-error@-3 {{variable length array}} expected-error@-3 {{constant expression}}
#endif
}

namespace dr368 { // dr368: yes
  template<typename T, T> struct S {}; // expected-note {{here}}
  template<typename T> int f(S<T, T()> *); // expected-error {{function type}}
  //template<typename T> int g(S<T, (T())> *); // FIXME: crashes clang
  template<typename T> int g(S<T, true ? T() : T()> *); // expected-note {{cannot have type 'dr368::X'}}
  struct X {};
  int n = g<X>(0); // expected-error {{no matching}}
}

// dr370: na
