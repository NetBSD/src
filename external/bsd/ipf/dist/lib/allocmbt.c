/*	$NetBSD: allocmbt.c,v 1.1.1.1 2012/03/23 21:20:08 christos Exp $	*/

/*
 * Copyright (C) 2006 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: allocmbt.c,v 1.1 2007/08/20 10:15:23 darren_r Exp 
 */

#include "ipf.h"

mb_t *allocmbt(size_t len)
{
	mb_t *m;

	m = (mb_t *)malloc(sizeof(mb_t));
	if (m == NULL)
		return NULL;
	m->mb_len = len;
	m->mb_next = NULL;
	m->mb_data = (char *)m->mb_buf;
	return m;
}
