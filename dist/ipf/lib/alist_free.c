/*	$NetBSD: alist_free.c,v 1.1.1.2 2012/01/30 16:03:23 darrenr Exp $	*/

/*
 * Copyright (C) 2009 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: alist_free.c,v 1.3.2.1 2012/01/26 05:29:15 darrenr Exp
 */
#include "ipf.h"

void
alist_free(hosts)
	alist_t *hosts;
{
	alist_t *a, *next;

	for (a = hosts; a != NULL; a = next) {
		next = a->al_next;
		free(a);
	}
}
