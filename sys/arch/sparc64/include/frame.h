/*       $NetBSD: frame.h,v 1.13 2003/10/27 00:16:42 christos Exp $        */

#include <sparc/frame.h>

#ifndef _LOCORE
#ifdef COMPAT_16
void sendsig_sigcontext(const ksiginfo_t *, const sigset_t *);
#endif

void *getframe(struct lwp *, int, int *);
#endif
