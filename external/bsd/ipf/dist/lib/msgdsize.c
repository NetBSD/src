/*	$NetBSD: msgdsize.c,v 1.1.1.1 2012/03/23 21:20:09 christos Exp $	*/

/*
 * Copyright (C) 2011 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: msgdsize.c,v 1.2.4.2 2012/01/26 05:44:26 darren_r Exp 
 */

#include "ipf.h"

size_t msgdsize(orig)
	mb_t *orig;
{
	size_t sz = 0;
	mb_t *m;

	for (m = orig; m != NULL; m = m->mb_next)
		sz += m->mb_len;
	return sz;
}
