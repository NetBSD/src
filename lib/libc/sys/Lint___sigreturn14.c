/*	$NetBSD: Lint___sigreturn14.c,v 1.1 1998/09/26 23:58:14 christos Exp $	*/

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
