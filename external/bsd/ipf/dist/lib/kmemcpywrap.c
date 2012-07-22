/*	$NetBSD: kmemcpywrap.c,v 1.1.1.2 2012/07/22 13:44:39 darrenr Exp $	*/

/*
 * Copyright (C) 2012 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * $Id: kmemcpywrap.c,v 1.1.1.2 2012/07/22 13:44:39 darrenr Exp $
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

