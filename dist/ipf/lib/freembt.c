/*	$NetBSD$	*/

/*
 * Copyright (C) 2009 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: freembt.c,v 1.3.2.1 2012/01/26 05:29:15 darrenr Exp
 */

#include "ipf.h"

void freembt(m)
	mb_t *m;
{

	free(m);
}
