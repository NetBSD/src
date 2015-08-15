int foo();
int bar();
int baz(int);

int main()
{
  /* Use comma operator to sequence evaluation of bar and foo. */
  return baz((bar(), foo()));
}

int foo()
{
  return 0;
}
