/*	$NetBSD: bcopywrap.c,v 1.1.1.1.2.2 2012/04/17 00:03:16 yamt Exp $	*/

/*
 * Copyright (C) 2009 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id
 */

#include "ipf.h"

int bcopywrap(from, to, size)
	void *from, *to;
	size_t size;
{
	bcopy((caddr_t)from, (caddr_t)to, size);
	return 0;
}

