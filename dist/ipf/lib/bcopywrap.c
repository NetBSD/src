/*	$NetBSD: bcopywrap.c,v 1.1.1.4 2012/01/30 16:03:24 darrenr Exp $	*/

/*
 * Copyright (C) 2009 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: bcopywrap.c,v 1.5.2.1 2012/01/26 05:29:15 darrenr Exp
 */

#include "ipf.h"

int bcopywrap(from, to, size)
	void *from, *to;
	size_t size;
{
	bcopy((caddr_t)from, (caddr_t)to, size);
	return 0;
}

