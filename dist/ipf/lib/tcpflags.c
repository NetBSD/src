/*	$NetBSD: tcpflags.c,v 1.1.1.3 2012/01/30 16:03:24 darrenr Exp $	*/

/*
 * Copyright (C) 2009 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: tcpflags.c,v 1.6.2.1 2012/01/26 05:29:17 darrenr Exp
 */

#include "ipf.h"


/*
 * ECN is a new addition to TCP - RFC 2481
 */
#ifndef TH_ECN
# define	TH_ECN  0x40
#endif
#ifndef TH_CWR
# define	TH_CWR  0x80
#endif

extern	char	flagset[];
extern	u_char	flags[];


u_char tcpflags(flgs)
	char *flgs;
{
	u_char tcpf = 0;
	char *s, *t;

	for (s = flgs; *s; s++) {
		if (*s == 'W')
			tcpf |= TH_CWR;
		else {
			if (!(t = strchr(flagset, *s))) {
				return 0;
			}
			tcpf |= flags[t - flagset];
		}
	}
	return tcpf;
}
