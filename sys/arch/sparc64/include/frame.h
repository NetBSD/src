/*       $NetBSD: frame.h,v 1.14.78.1 2009/05/04 08:11:58 yamt Exp $        */

#include <sparc/frame.h>

#ifndef _LOCORE
void *getframe(struct lwp *, int, int *);
#endif
