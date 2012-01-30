/*	$NetBSD: count6bits.c,v 1.1.1.3 2012/01/30 16:03:22 darrenr Exp $	*/

/*
 * Copyright (C) 2009 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: count6bits.c,v 1.7.2.1 2012/01/26 05:29:15 darrenr Exp
 */

#include "ipf.h"


int count6bits(msk)
	u_32_t *msk;
{
	int i = 0, k;
	u_32_t j;

	for (k = 3; k >= 0; k--)
		if (msk[k] == 0xffffffff)
			i += 32;
		else {
			for (j = msk[k]; j; j <<= 1)
				if (j & 0x80000000)
					i++;
		}
	return i;
}
