/*	$NetBSD: Lint_Ovfork.c,v 1.1.2.2 1997/11/08 22:02:48 veego Exp $	*/

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#include <unistd.h>

/*ARGSUSED*/
pid_t
vfork()
{
	return (0);
}
