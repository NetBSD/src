/*       $NetBSD: frame.h,v 1.11.6.1 2004/08/03 10:41:34 skrll Exp $        */

#include <sparc/frame.h>

#ifndef _LOCORE
#ifdef COMPAT_16
void sendsig_sigcontext(const ksiginfo_t *, const sigset_t *);
#endif

void *getframe(struct lwp *, int, int *);
#endif
