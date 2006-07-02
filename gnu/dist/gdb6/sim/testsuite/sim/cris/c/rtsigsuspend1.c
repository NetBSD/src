/* Test that TRT happens for invalid rt_sigsuspend calls.  Single-thread.
#notarget: cris*-*-elf
#xerror:
#output: Unimplemented rt_sigsuspend syscall arguments (0x1, 0x2)\n
#output: program stopped with signal 4.\n
*/

#include <unistd.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <stdlib.h>

int main (void)
{
  syscall (SYS_rt_sigsuspend, 1, 2);
  printf ("xyzzy\n");
  exit (0);
}
