#include <stdio.h>

extern int bar (void);

int times = -1;
int time;

int
main ()
{
  printf ("times: %d\n", times);
  times = 20;
  printf ("times: %d\n", times);

  printf ("time: %d\n", time);
  time = 10;
  printf ("time: %d\n", time);
  bar ();

  return 0;
}
