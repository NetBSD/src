#include <stdio.h>

void
foo2 (void)
{
  printf ("MAIN2\n");
}

asm (".symver foo2,foo@@FOO2");

void
foo1 (void)
{
  printf ("MAIN1\n");
}

asm (".symver foo1,foo@FOO1");
