// RUN: %clang_cc1 -triple %itanium_abi_triple -fsyntax-only -verify %s
// RUN: %clang_cc1 -triple %ms_abi_triple -DMSABI -fsyntax-only -verify %s

namespace PR5557 {
template <class T> struct A {
  A();
  virtual void anchor();
  virtual int a(T x);
};
template<class T> A<T>::A() {}
template<class T> void A<T>::anchor() { }

template<class T> int A<T>::a(T x) { 
  return *x; // expected-error{{requires pointer operand}}
}

void f(A<int> x) {
  x.anchor(); // expected-note{{instantiation}}
}

template<typename T>
struct X {
  virtual void f();
};

template<>
void X<int>::f() { }
}

template<typename T>
struct Base {
  virtual ~Base() { 
    int *ptr = 0;
    T t = ptr; // expected-error{{cannot initialize}}
  }
};

template<typename T>
struct Derived : Base<T> {
  virtual void foo() { }
};

template struct Derived<int>; // expected-note {{in instantiation of member function 'Base<int>::~Base' requested here}}

template<typename T>
struct HasOutOfLineKey {
  HasOutOfLineKey() { } 
  virtual T *f(float *fp);
};

template<typename T>
T *HasOutOfLineKey<T>::f(float *fp) {
  return fp; // expected-error{{cannot initialize return object of type 'int *' with an lvalue of type 'float *'}}
}

HasOutOfLineKey<int> out_of_line; // expected-note{{in instantiation of member function 'HasOutOfLineKey<int>::f' requested here}}

namespace std {
  class type_info;
}

namespace PR7114 {
  class A { virtual ~A(); }; // expected-note{{declared private here}}

  template<typename T>
  class B {
  public:
    class Inner : public A { }; // expected-error{{base class 'PR7114::A' has private destructor}}
    static Inner i;
    static const unsigned value = sizeof(i) == 4;
  };

  int f() { return B<int>::value; }

#ifdef MSABI
  void test_typeid(B<float>::Inner bfi) { // expected-note{{implicit destructor}}
    (void)typeid(bfi);
#else
  void test_typeid(B<float>::Inner bfi) {
    (void)typeid(bfi); // expected-note{{implicit destructor}}
#endif
  }

  template<typename T>
  struct X : A {
    void f() { }
  };

  void test_X(X<int> &xi, X<float> &xf) {
    xi.f();
  }
}

namespace DynamicCast {
  struct Y {};
  template<typename T> struct X : virtual Y {
    virtual void foo() { T x; } // expected-error {{variable has incomplete type 'void'}}
  };
  template<typename T> struct X2 : virtual Y {
    virtual void foo() { T x; }
  };
  Y* f(X<void>* x) { return dynamic_cast<Y*>(x); } // expected-note {{in instantiation of member function 'DynamicCast::X<void>::foo' requested here}}
  Y* f2(X<void>* x) { return dynamic_cast<Y*>(x); }
}
