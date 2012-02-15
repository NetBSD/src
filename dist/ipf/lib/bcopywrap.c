/*	$NetBSD: bcopywrap.c,v 1.2 2012/02/15 17:55:06 riz Exp $	*/

/*
 * Copyright (C) 2002 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: bcopywrap.c,v 1.1.4.2 2009/12/27 06:58:06 darrenr Exp
 */

#include "ipf.h"

int bcopywrap(from, to, size)
void *from, *to;
size_t size;
{
	bcopy((caddr_t)from, (caddr_t)to, size);
	return 0;
}

