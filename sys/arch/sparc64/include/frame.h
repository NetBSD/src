/*       $NetBSD: frame.h,v 1.14.86.1 2009/01/19 13:16:51 skrll Exp $        */

#include <sparc/frame.h>

#ifndef _LOCORE
void *getframe(struct lwp *, int, int *);
#endif
