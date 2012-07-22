/*	$NetBSD: bcopywrap.c,v 1.1.1.2 2012/07/22 13:44:38 darrenr Exp $	*/

/*
 * Copyright (C) 2012 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * $Id: bcopywrap.c,v 1.1.1.2 2012/07/22 13:44:38 darrenr Exp $
 */

#include "ipf.h"

int bcopywrap(from, to, size)
	void *from, *to;
	size_t size;
{
	bcopy((caddr_t)from, (caddr_t)to, size);
	return 0;
}

