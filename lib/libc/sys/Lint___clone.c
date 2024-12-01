/* $NetBSD: Lint___clone.c,v 1.3 2024/12/01 16:16:57 rillig Exp $ */

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
