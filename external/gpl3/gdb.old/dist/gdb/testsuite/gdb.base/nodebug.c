#include <stdlib.h>
/* Test that things still (sort of) work when compiled without -g.  */

int dataglobal = 3;			/* Should go in global data */
static int datalocal = 4;		/* Should go in local data */
int bssglobal;				/* Should go in global bss */
static int bsslocal;			/* Should go in local bss */

int
inner (int x)
{
  return x + dataglobal + datalocal + bssglobal + bsslocal;
}

static short
middle (int x)
{
  return 2 * inner (x);
}

short
top (int x)
{
  return 2 * middle (x);
}

int
main (int argc, char **argv)
{
  return top (argc);
}

int *x;

int array_index (char *arr, int i)
{
  /* The basic concept is just "return arr[i];".  But call malloc so that gdb
     will be able to call functions.  */
  char retval;
  x = (int *) malloc (sizeof (int));
  *x = i;
  retval = arr[*x];
  free (x);
  return retval;
}
