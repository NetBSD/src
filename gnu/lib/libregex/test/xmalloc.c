#ifndef lint
static char rcsid[] = "$Id: xmalloc.c,v 1.2 1993/08/02 17:24:36 mycroft Exp $";
#endif /* not lint */

#include <stdio.h>
extern char *malloc ();

#ifndef NULL
#define NULL 0
#endif

void *
xmalloc (size)
  unsigned size;
{
  char *new_mem = malloc (size); 
  
  if (new_mem == NULL)
    {
      fprintf (stderr, "xmalloc: request for %u bytes failed.\n", size);
      abort ();
    }

  return new_mem;
}
