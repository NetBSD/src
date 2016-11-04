#include <stdio.h>

void
foo (void)
{
  printf ("MAIN\n");
}

asm (".symver foo,foo@FOO");
asm (".set foo_alias,foo");
asm (".global foo_alias");
