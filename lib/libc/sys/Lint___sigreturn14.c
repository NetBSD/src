/* $NetBSD: Lint___sigreturn14.c,v 1.1.8.1 2000/06/23 16:18:07 minoura Exp $ */

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#define	__LIBC12_SOURCE__

#include <signal.h>

/*ARGSUSED*/
int
__sigreturn14(scp)
	struct sigcontext *scp;
{
	return (0);
}
