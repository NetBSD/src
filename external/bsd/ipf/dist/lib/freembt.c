/*	$NetBSD: freembt.c,v 1.1.1.2 2012/07/22 13:44:38 darrenr Exp $	*/

/*
 * Copyright (C) 2012 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * $Id: freembt.c,v 1.1.1.2 2012/07/22 13:44:38 darrenr Exp $
 */

#include "ipf.h"

void freembt(m)
	mb_t *m;
{

	free(m);
}
