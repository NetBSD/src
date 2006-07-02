/* Check that closing a pipe with a nonempty buffer works.
#notarget: cris*-*-elf
#output: got: a\nexit: 0\n
*/


#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <unistd.h>
#include <sched.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

int pip[2];

int pipemax;

int
process (void *arg)
{
  char *s = arg;
  char *buf = malloc (pipemax * 100);
  int ret;

  if (buf == NULL)
    abort ();

  *buf = *s;

  /* The first write should go straight through.  */
  if (write (pip[1], buf, 1) != 1)
    abort ();

  *buf = s[1];

  /* The second write should only successful for at most the PIPE_MAX
     part, but no error.  */
  ret = write (pip[1], buf, pipemax * 10);
  if (ret != 0 && ret != pipemax - 1 && ret != pipemax)
    {
      fprintf (stderr, "ret: %d\n", ret);
      fflush (0);
      abort ();
    }

  return 0;
}

int
main (void)
{
  int retcode;
  int pid;
  int st = 0;
  long stack[16384];
  char buf[1];

  /* We need to turn this off because we don't want (to have to model) a
     SIGPIPE resulting from the close.  */
  if (signal (SIGPIPE, SIG_IGN) != SIG_DFL)
    abort ();

  retcode = pipe (pip);

  if (retcode != 0)
    {
      fprintf (stderr, "Bad pipe %d\n", retcode);
      abort ();
    }

#ifdef PIPE_MAX
  pipemax = PIPE_MAX;
#else
  pipemax = fpathconf (pip[1], _PC_PIPE_BUF);
#endif

  if (pipemax <= 0)
    {
      fprintf (stderr, "Bad pipemax %d\n", pipemax);
      abort ();
    }

  pid = clone (process, (char *) stack + sizeof (stack) - 64,
	       (CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND)
	       | SIGCHLD, "ab");
  if (pid <= 0)
    {
      fprintf (stderr, "Bad clone %d\n", pid);
      abort ();
    }

  while ((retcode = read (pip[0], buf, 1)) == 0)
    ;

  if (retcode != 1)
    {
      fprintf (stderr, "Bad read 1: %d\n", retcode);
      abort ();
    }

  printf ("got: %c\n", buf[0]);

  if (close (pip[0]) != 0)
    {
      perror ("pip close");
      abort ();
    }

  retcode = waitpid (pid, &st, __WALL);

  if (retcode != pid || !WIFEXITED (st))
    {
      fprintf (stderr, "Bad wait %d:%d %x\n", pid, retcode, st);
      perror ("errno");
      abort ();
    }

  printf ("exit: %d\n", WEXITSTATUS (st));
  return 0;
}
