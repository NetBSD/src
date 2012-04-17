/*	$NetBSD: freembt.c,v 1.1.1.1.2.2 2012/04/17 00:03:16 yamt Exp $	*/

/*
 * Copyright (C) 2009 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: freembt.c,v 1.3.2.1 2012/01/26 05:44:26 darren_r Exp 
 */

#include "ipf.h"

void freembt(m)
	mb_t *m;
{

	free(m);
}
