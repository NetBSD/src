/*	$NetBSD: kmemcpywrap.c,v 1.1.1.1.2.2 2012/04/17 00:03:17 yamt Exp $	*/

/*
 * Copyright (C) 2009 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id
 */

#include "ipf.h"
#include "kmem.h"

int kmemcpywrap(from, to, size)
	void *from, *to;
	size_t size;
{
	int ret;

	ret = kmemcpy((caddr_t)to, (u_long)from, size);
	return ret;
}

