template<class T> T add2(T v1, T v2)
{
   T v3;
   v3 = v1;
   v3 += v2;
   return v3;
}

void subr2()
{
  char c;
  int i;
  float f;
  
  c = 'b';
  i = 3;
  f = 6.5;

  c = add2(c, c);
  i = add2(i, i);
  f = add2(f, f);
}
