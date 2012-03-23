/*	$NetBSD: prependmbt.c,v 1.1.1.1 2012/03/23 21:20:09 christos Exp $	*/

/*
 * Copyright (C) 2009 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: prependmbt.c,v 1.3.2.1 2012/01/26 05:44:26 darren_r Exp 
 */

#include "ipf.h"

int prependmbt(fin, m)
	fr_info_t *fin;
	mb_t *m;
{
	m->mb_next = *fin->fin_mp;
	*fin->fin_mp = m;
}
