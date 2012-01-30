/*	$NetBSD: kmemcpywrap.c,v 1.1.1.4 2012/01/30 16:03:25 darrenr Exp $	*/

/*
 * Copyright (C) 2009 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: kmemcpywrap.c,v 1.5.2.1 2012/01/26 05:29:16 darrenr Exp
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

