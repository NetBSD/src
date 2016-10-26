#include <stdio.h>

extern char size_of_bar asm ("bar@SIZE");

int
main ()
{
  if (10 == (long) &size_of_bar)
    printf ("OK\n");

  return 0;
}
