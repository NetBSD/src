/*       $NetBSD: frame.h,v 1.14 2005/12/11 12:19:10 christos Exp $        */

#include <sparc/frame.h>

#ifndef _LOCORE
#ifdef COMPAT_16
void sendsig_sigcontext(const ksiginfo_t *, const sigset_t *);
#endif

void *getframe(struct lwp *, int, int *);
#endif
