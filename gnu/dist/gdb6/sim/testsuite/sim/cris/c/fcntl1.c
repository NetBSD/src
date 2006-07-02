/* Check that we get the expected message for unsupported fcntl calls.
#notarget: cris*-*-elf
#xerror:
#output: Unimplemented fcntl*
#output: program stopped with signal 4.\n
*/
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

int main (void)
{
  fcntl (1, 42);
  printf ("pass\n");
  exit (0);
}
