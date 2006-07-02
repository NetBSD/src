/* Check unsupported case of sigaction syscall.
#notarget: cris*-*-elf
#xerror:
#output: Unimplemented rt_sigaction syscall (0x8, 0x3df*\n
#output: program stopped with signal 4.\n
*/
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

int
main (void)
{
  struct sigaction sa;
  sa.sa_sigaction = NULL;
  sa.sa_flags = SA_RESTART | SA_SIGINFO;
  sigemptyset (&sa.sa_mask);

  if (sigaction (SIGFPE, &sa, NULL) != 0)
    abort ();

  printf ("xyzzy\n");
  exit (0);
}
