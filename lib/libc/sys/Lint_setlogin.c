/* $NetBSD: Lint_setlogin.c,v 1.2 2000/06/14 06:49:10 cgd Exp $ */

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
