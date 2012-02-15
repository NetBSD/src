/*	$NetBSD: printtunable.c,v 1.2 2012/02/15 17:55:07 riz Exp $	*/

/*
 * Copyright (C) 2003 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: printtunable.c,v 1.1.4.2 2009/12/27 06:58:07 darrenr Exp
 */

#include "ipf.h"

void printtunable(tup)
ipftune_t *tup;
{
	printf("%s\tmin %#lx\tmax %#lx\tcurrent ",
		tup->ipft_name, tup->ipft_min, tup->ipft_max);
	if (tup->ipft_sz == sizeof(u_long))
		printf("%lu\n", tup->ipft_vlong);
	else if (tup->ipft_sz == sizeof(u_int))
		printf("%u\n", tup->ipft_vint);
	else if (tup->ipft_sz == sizeof(u_short))
		printf("%hu\n", tup->ipft_vshort);
	else if (tup->ipft_sz == sizeof(u_char))
		printf("%u\n", (u_int)tup->ipft_vchar);
	else {
		printf("sz = %d\n", tup->ipft_sz);
	}
}
