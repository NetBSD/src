/* Check that unimplemented clone syscalls get the right treatment.
#notarget: cris*-*-elf
#xerror:
#output: Unimplemented clone syscall *
#output: program stopped with signal 4.\n
*/

#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

int pip[2];

int
process (void *arg)
{
  return 0;
}

int
main (void)
{
  int retcode;
  long stack[16384];

  clone (process, (char *) stack + sizeof (stack) - 64, 0, "cba");
  printf ("xyzzy\n");
  return 0;
}
