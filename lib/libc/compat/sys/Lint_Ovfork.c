/* $NetBSD: Lint_Ovfork.c,v 1.2 2024/01/20 14:52:46 christos Exp $ */

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#include <unistd.h>

/*ARGSUSED*/
pid_t
vfork(void)
{
	return (0);
}
