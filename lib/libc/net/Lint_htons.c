/*	$NetBSD: Lint_htons.c,v 1.1.2.2 1997/11/08 21:59:06 veego Exp $	*/

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#include <sys/types.h>

/*ARGSUSED*/
in_port_t
htons(host16)
	in_port_t host16;
{
	return (0);
}
