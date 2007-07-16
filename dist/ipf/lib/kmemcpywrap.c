/*	$NetBSD: kmemcpywrap.c,v 1.1.1.1.18.2 2007/07/16 11:05:02 liamjfoy Exp $	*/

/*
 * Copyright (C) 2002 by Darren Reed.
 * 
 * See the IPFILTER.LICENCE file for details on licencing.  
 *   
 * Id: kmemcpywrap.c,v 1.1.4.1 2006/06/16 17:21:05 darrenr Exp 
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

