/*	$NetBSD: prependmbt.c,v 1.2 2012/07/22 14:27:36 darrenr Exp $	*/

/*
 * Copyright (C) 2012 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: prependmbt.c,v 1.1.1.2 2012/07/22 13:44:40 darrenr Exp $
 */

#include "ipf.h"

int prependmbt(fin, m)
	fr_info_t *fin;
	mb_t *m;
{
	m->mb_next = *fin->fin_mp;
	*fin->fin_mp = m;
	return 0;
}
