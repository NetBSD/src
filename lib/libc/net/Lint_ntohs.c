/* $NetBSD: Lint_ntohs.c,v 1.2.6.1 2000/06/23 16:17:34 minoura Exp $ */

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#include <sys/types.h>
#undef ntohs

/*ARGSUSED*//*NOSTRICT*/
in_port_t
ntohs(net16)
	in_port_t net16;
{
	return (0);
}
