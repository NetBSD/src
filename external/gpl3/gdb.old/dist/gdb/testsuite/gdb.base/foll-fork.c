#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#ifdef PROTOTYPES
void callee (int i)
#else
void callee (i)
  int  i;
#endif
{
  /* Any output corrupts GDB CLI expect strings.
     printf("callee: %d\n", i);  */
}

#ifdef PROTOTYPES
int main (void)
#else
main ()
#endif
{
  int  pid;
  int  v = 5;

  pid = fork ();
  if (pid == 0) /* set breakpoint here */
    {
      v++;
      /* printf ("I'm the child!\n"); */
      callee (getpid ());
    }
  else
    {
      v--;
      /* printf ("I'm the proud parent of child #%d!\n", pid); */
      callee (getpid ());
    }

  exit (0); /* at exit */
}
