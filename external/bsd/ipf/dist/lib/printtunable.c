/*	$NetBSD: printtunable.c,v 1.1.1.1.2.3 2012/10/30 18:55:11 yamt Exp $	*/

/*
 * Copyright (C) 2012 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: printtunable.c,v 1.1.1.2 2012/07/22 13:44:42 darrenr Exp $
 */

#include "ipf.h"

void
printtunable(tup)
	ipftune_t *tup;
{
	PRINTF("%s\tmin %lu\tmax %lu\tcurrent ",
		tup->ipft_name, tup->ipft_min, tup->ipft_max);
	if (tup->ipft_sz == sizeof(u_long))
		PRINTF("%lu\n", tup->ipft_vlong);
	else if (tup->ipft_sz == sizeof(u_int))
		PRINTF("%u\n", tup->ipft_vint);
	else if (tup->ipft_sz == sizeof(u_short))
		PRINTF("%hu\n", tup->ipft_vshort);
	else if (tup->ipft_sz == sizeof(u_char))
		PRINTF("%u\n", (u_int)tup->ipft_vchar);
	else {
		PRINTF("sz = %d\n", tup->ipft_sz);
	}
}
