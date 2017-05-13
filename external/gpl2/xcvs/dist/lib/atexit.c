/* Wrapper to implement ANSI C's atexit using SunOS's on_exit. */
/* This function is in the public domain.  --Mike Stump. */
#include <sys/cdefs.h>
__RCSID("$NetBSD: atexit.c,v 1.1.1.1.26.1 2017/05/13 06:23:23 snj Exp $");

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

int
atexit (void (*f) (void))
{
  /* If the system doesn't provide a definition for atexit, use on_exit
     if the system provides that.  */
  on_exit (f, 0);
  return 0;
}
