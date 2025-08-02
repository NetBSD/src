/* $NetBSD: Lint_ptrace.c,v 1.4.2.1 2025/08/02 05:54:42 perseant Exp $ */

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#include <signal.h>	/* XXX: alpha <machine/signal.h> needs sigset_t! */
#include <sys/types.h>
#include <sys/ptrace.h>

/*ARGSUSED*/
int
ptrace(int request, pid_t pid, void *addr, int data)
{
	return 0;
}
