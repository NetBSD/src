template<class T> T add(T v1, T v2);

void add1()
{
  char c;
  int i;
  float f;
  
  c = 'b';
  i = 3;
  f = 6.5;

  c = add(c, c);
  i = add(i, i);
  f = add(f, f);
}
