#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main (void)
{
  printf ("foll-exec is about to execl(crashme)...\n");

  execl (BASEDIR "/crashme",
         BASEDIR "/crashme",
         (char *)0);
}
