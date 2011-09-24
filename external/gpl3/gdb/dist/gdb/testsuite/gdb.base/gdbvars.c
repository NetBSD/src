/* Simple program to help exercise gdb's convenience variables.  */

typedef void *ptr;

ptr p = &p;

int
main ()
{
  p = &p;
  return 0;
}
