/* Check that the simulator has chdir:ed to the --sysroot argument
#sim: --sysroot=@srcdir@
   (or that  --sysroot is applied to relative file paths).  */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
int main (int argc, char *argv[])
{
  FILE *f = fopen ("openpf2.c", "rb");
  if (f == NULL)
    abort ();
  close (f);
  printf ("pass\n");
  return 0;
}
