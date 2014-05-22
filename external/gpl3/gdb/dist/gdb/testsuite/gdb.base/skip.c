int foo();
int bar();
int baz(int, int);

int main()
{
  return baz(foo(), bar());
}

int foo()
{
  return 0;
}
