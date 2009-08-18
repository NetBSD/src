int unused_var = 7;
int used_var = 7;

int
unused_func (int v)
{
  return 3 * unused_var;
}

int
__attribute__((noinline))
used_func (int v)
{
  return 2 * used_var;
}

int
main (void)
{
  return used_func (5);
}
