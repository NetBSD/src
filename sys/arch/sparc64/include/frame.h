/*       $NetBSD: frame.h,v 1.12 2003/10/26 08:06:56 christos Exp $        */

#include <sparc/frame.h>

#ifndef _LOCORE
#ifdef COMPAT_16
void sendsig_sigcontext(const ksiginfo_t *, const sigset_t *);
#endif

void *getframe(struct lwp *, int, int *);
void buildcontext(struct lwp *, void *, const void *, void *);
#endif
