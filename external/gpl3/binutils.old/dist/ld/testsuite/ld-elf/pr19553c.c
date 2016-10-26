#include <stdio.h>

void
foo (void)
{
  printf ("pr19553c\n");
}

asm (".symver foo,foo@FOO");
