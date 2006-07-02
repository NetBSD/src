/* Check unimplemented-output for mprotect call.
#notarget: cris*-*-elf
#xerror:
#output: Unimplemented mprotect call (0x0, 0x2001, 0x4)\n
#output: program stopped with signal 4.\n
 */
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>

int main (int argc, char *argv[])
{
  mprotect (0, 8193, PROT_EXEC);
  printf ("xyzzy\n");
  exit (0);
}
