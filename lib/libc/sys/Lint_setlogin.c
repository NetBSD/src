/* $NetBSD: Lint_setlogin.c,v 1.1.10.1 2000/06/23 16:18:08 minoura Exp $ */

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#include <unistd.h>

/*ARGSUSED*/
int
setlogin(name)
	const char *name;
{
	return (0);
}
