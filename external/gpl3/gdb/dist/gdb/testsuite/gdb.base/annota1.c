#include <stdio.h>
#include <signal.h>


#ifdef PROTOTYPES
void
handle_USR1 (int sig)
{
}
#else
void
handle_USR1 (sig)
     int sig;
{
}
#endif

int value;

#ifdef PROTOTYPES
int
main (void)
#else
int
main ()
#endif
{
  int my_array[3] = { 1, 2, 3 };  /* break main */
  
  value = 7;
  
#ifdef SIGUSR1
  signal (SIGUSR1, handle_USR1);
#endif

  printf ("value is %d\n", value);
  printf ("my_array[2] is %d\n", my_array[2]);
  
  {
    int i;
    for (i = 0; i < 5; i++)
      value++;  /* increment value */
  }

  return 0;  /* after loop */
}

