/*	$NetBSD: atexit.c,v 1.1.1.1 2016/01/10 21:36:18 christos Exp $	*/

/* Wrapper to implement ANSI C's atexit using SunOS's on_exit. */
/* This function is in the public domain.  --Mike Stump. */

#include "config.h"

int
atexit(f)
     void (*f)();
{
  /* If the system doesn't provide a definition for atexit, use on_exit
     if the system provides that.  */
  on_exit (f, 0);
  return 0;
}
