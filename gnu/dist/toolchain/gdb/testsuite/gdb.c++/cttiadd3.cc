template<class T> T add3(T v1, T v2)
{
   T v3;
   v3 = v1;
   v3 += v2;
   return v3;
}

template<class T> T add4(T v1, T v2)
{
   T v3;
   v3 = v1;
   v3 += v2;
   return v3;
}

void subr3()
{
  char c;
  int i;
  float f;
  
  c = 'b';
  i = 3;
  f = 6.5;

  c = add3(c, c);
  i = add3(i, i);
  f = add3(f, f);
  c = add4(c, c);
  i = add4(i, i);
  f = add4(f, f);
}
