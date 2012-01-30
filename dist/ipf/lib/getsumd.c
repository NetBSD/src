/*	$NetBSD: getsumd.c,v 1.1.1.4 2012/01/30 16:03:24 darrenr Exp $	*/

/*
 * Copyright (C) 2009 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: getsumd.c,v 1.6.2.1 2012/01/26 05:29:15 darrenr Exp
 */

#include "ipf.h"

char *getsumd(sum)
	u_32_t sum;
{
	static char sumdbuf[17];

	if (sum & NAT_HW_CKSUM)
		sprintf(sumdbuf, "hw(%#0x)", sum & 0xffff);
	else
		sprintf(sumdbuf, "%#0x", sum);
	return sumdbuf;
}
