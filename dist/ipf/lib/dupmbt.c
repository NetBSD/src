/*	$NetBSD$	*/

/*
 * Copyright (C) 2009 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: dupmbt.c,v 1.3.2.1 2012/01/26 05:29:15 darrenr Exp
 */

#include "ipf.h"

mb_t *dupmbt(orig)
	mb_t *orig;
{
	mb_t *m;

	m = (mb_t *)malloc(sizeof(mb_t));
	if (m == NULL)
		return NULL;
	m->mb_len = orig->mb_len;
	m->mb_next = NULL;
	m->mb_data = (char *)m->mb_buf + (orig->mb_data - (char *)orig->mb_buf);
	bcopy(orig->mb_data, m->mb_data, m->mb_len);
	return m;
}
