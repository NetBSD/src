/* $NetBSD: Lint_ptrace.c,v 1.3 2024/01/20 14:52:49 christos Exp $ */

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#include <sys/types.h>
#include <sys/ptrace.h>

/*ARGSUSED*/
int
ptrace(int request, pid_t pid, void *addr, int data)
{
	return (0);
}
