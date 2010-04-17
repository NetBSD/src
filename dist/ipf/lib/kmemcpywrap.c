/*	$NetBSD: kmemcpywrap.c,v 1.1.1.3 2010/04/17 20:45:56 darrenr Exp $	*/

/*
 * Copyright (C) 2002 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: kmemcpywrap.c,v 1.1.4.2 2009/12/27 06:58:06 darrenr Exp
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

