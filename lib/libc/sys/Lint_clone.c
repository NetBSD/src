/* $NetBSD: Lint_clone.c,v 1.1.2.2 2001/10/08 20:21:31 nathanw Exp $ */

/*
 * This file placed in the public domain.
 * Jason R. Thorpe, July 16, 2001.
 */

#include <sched.h>

/*ARGSUSED*/
pid_t
clone(int (*func)(void *), void *stack, int flags, void *arg)
{
	return (0);
}
