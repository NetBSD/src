/* $NetBSD: Lint_clone.c,v 1.3 2024/01/20 14:52:49 christos Exp $ */

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
	return (0);
}
