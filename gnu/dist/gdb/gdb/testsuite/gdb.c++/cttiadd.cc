template<class T> T add(T v1, T v2)
{
   T v3;
   v3 = v1;
   v3 += v2;
   return v3;
 }

int main()
{
  char c;
  int i;
  float f;
  extern void add1();
  extern void subr2();
  extern void subr3();
  
  c = 'a';
  i = 2;
  f = 4.5;

  c = add(c, c);
  i = add(i, i);
  f = add(f, f);

  add1();
  subr2();
  subr3();
}
