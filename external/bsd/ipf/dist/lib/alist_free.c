/*	$NetBSD: alist_free.c,v 1.1.1.1 2012/03/23 21:20:08 christos Exp $	*/

/*
 * Copyright (C) 2009 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: alist_free.c,v 1.3.2.1 2012/01/26 05:44:26 darren_r Exp 
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
