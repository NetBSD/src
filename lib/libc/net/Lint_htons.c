/* $NetBSD: Lint_htons.c,v 1.2.6.1 2000/06/23 16:17:33 minoura Exp $ */

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
