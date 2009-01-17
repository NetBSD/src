/*       $NetBSD: frame.h,v 1.14.74.1 2009/01/17 13:28:32 mjf Exp $        */

#include <sparc/frame.h>

#ifndef _LOCORE
void *getframe(struct lwp *, int, int *);
#endif
