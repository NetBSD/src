#include <stdio.h>

extern char bar[];
extern char size_of_bar asm ("bar@SIZE");
extern void set_bar (int, int);

int
main ()
{
  set_bar (1, 20);
  if (10 == (long) &size_of_bar && bar[1] == 20)
    printf ("OK\n");

  return 0;
}
