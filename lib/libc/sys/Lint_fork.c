/* $NetBSD: Lint_fork.c,v 1.1.10.1 2000/06/23 16:18:07 minoura Exp $ */

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#include <unistd.h>

/*ARGSUSED*/
pid_t
fork()
{
	return (0);
}
