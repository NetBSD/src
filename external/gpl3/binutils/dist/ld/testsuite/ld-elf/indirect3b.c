#include <stdio.h>

void
foo (void)
{
  printf ("MAIN\n");
}

asm (".symver foo,foo@FOO");
