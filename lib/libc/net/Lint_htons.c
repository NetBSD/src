/*	$NetBSD: Lint_htons.c,v 1.2 1999/05/03 13:12:34 christos Exp $	*/

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#include <sys/types.h>
#undef htons

/*ARGSUSED*//*NOSTRICT*/
in_port_t
htons(host16)
	in_port_t host16;
{
	return (0);
}
