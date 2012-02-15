/*	$NetBSD: kmemcpywrap.c,v 1.2 2012/02/15 17:55:06 riz Exp $	*/

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

