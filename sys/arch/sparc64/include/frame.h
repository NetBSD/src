/*       $NetBSD: frame.h,v 1.14.84.1 2008/12/13 01:13:29 haad Exp $        */

#include <sparc/frame.h>

#ifndef _LOCORE
void *getframe(struct lwp *, int, int *);
#endif
