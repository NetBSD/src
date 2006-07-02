/* Compiler options:
#notarget: cris*-*-elf
#cc: additional_flags=-pthread
#xerror:
#output: Unimplemented rt_sigprocmask syscall (0x3, 0x0, 0x3dff*\n
#output: program stopped with signal 4.\n

   Testing a signal handler corner case.  */

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

static void *
process (void *arg)
{
  while (1)
    sched_yield ();
  return NULL;
}

int
main (void)
{
  int retcode;
  pthread_t th_a;
  void *retval;
  sigset_t sigs;

  if (sigemptyset (&sigs) != 0)
    abort ();

  retcode = pthread_create (&th_a, NULL, process, NULL);
  if (retcode != 0)
    abort ();

  /* An invalid parameter 1 should cause this to halt the simulator.  */
  pthread_sigmask (SIG_BLOCK + SIG_UNBLOCK + SIG_SETMASK,
		   NULL, &sigs);
  printf ("xyzzy\n");
  return 0;
}
