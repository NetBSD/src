/* $NetBSD: Lint_clone.c,v 1.4 2024/12/01 16:16:57 rillig Exp $ */

/*
 * This file placed in the public domain.
 * Jason R. Thorpe, July 16, 2001.
 */

#define _GNU_SOURCE
#include <sched.h>

/*ARGSUSED*/
pid_t
clone(int (*func)(void *), void *stack, int flags, void *arg)
{
	return 0;
}
