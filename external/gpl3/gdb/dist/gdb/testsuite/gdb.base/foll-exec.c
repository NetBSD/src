#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>


int  global_i = 100;

#ifdef PROTOTYPES
int main (void)
#else
main ()
#endif
{
  int  local_j = global_i+1;
  int  local_k = local_j+1;

  printf ("foll-exec is about to execlp(execd-prog)...\n");

  execlp (BASEDIR "/execd-prog",
          BASEDIR "/execd-prog",
          "execlp arg1 from foll-exec",
          (char *)0);

  printf ("foll-exec is about to execl(execd-prog)...\n");

  execl (BASEDIR "/execd-prog",
         BASEDIR "/execd-prog",
         "execl arg1 from foll-exec",
         "execl arg2 from foll-exec",
         (char *)0);

  {
    static char * argv[] = {
      (char *)BASEDIR "/execd-prog",
      (char *)"execv arg1 from foll-exec",
      (char *)0};

    printf ("foll-exec is about to execv(execd-prog)...\n");

    execv (BASEDIR "/execd-prog", argv);
  }
}
