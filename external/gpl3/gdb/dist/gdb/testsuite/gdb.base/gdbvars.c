/* Simple program to help exercise gdb's convenience variables.  */

typedef void *ptr;

ptr p = &p;

static void
foo_void (void)
{
}

static int
foo_int (void)
{
  return 0;
}

int
main ()
{
  p = &p;
  return 0;
}
