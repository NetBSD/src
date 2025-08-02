/* $NetBSD: Lint___clone.c,v 1.2.110.1 2025/08/02 05:54:42 perseant Exp $ */

/*
 * This file placed in the public domain.
 * Jason R. Thorpe, July 16, 2001.
 */

#include <sched.h>

/*ARGSUSED*/
pid_t
__clone(int (*func)(void *), void *stack, int flags, void *arg)
{
	return 0;
}
