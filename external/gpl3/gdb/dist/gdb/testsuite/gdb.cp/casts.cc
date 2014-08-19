struct A
{
  int a;
  A (int aa): a (aa) {}
};

struct B: public A
{
  int b;
  B (int aa, int bb): A (aa), b(bb) {}
};


struct Alpha
{
  virtual void x() { }
};

struct Gamma
{
};

struct Derived : public Alpha
{
};

struct VirtuallyDerived : public virtual Alpha
{
};

struct DoublyDerived : public VirtuallyDerived,
		       public virtual Alpha,
		       public Gamma
{
};

// Confuse a simpler approach.

double
decltype(int x)
{
  return x + 2.0;
}

int
main (int argc, char **argv)
{
  A *a = new B(42, 1729);
  B *b = (B *) a;
  A &ar = *b;
  B &br = (B&)ar;

  Derived derived;
  DoublyDerived doublyderived;

  Alpha *ad = &derived;
  Alpha *add = &doublyderived;

  double y = decltype(2);

  return 0;  /* breakpoint spot: casts.exp: 1 */
}
