/* $NetBSD: Lint___sigreturn14.c,v 1.3 2003/09/11 15:29:10 christos Exp $ */

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#define	__LIBC12_SOURCE__

#include <signal.h>

#ifndef __HAVE_SIGINFO
/*ARGSUSED*/
int
__sigreturn14(scp)
	struct sigcontext *scp;
{
	return (0);
}
#endif
