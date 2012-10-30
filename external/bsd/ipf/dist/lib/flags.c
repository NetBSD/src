/*	$NetBSD: flags.c,v 1.1.1.1.2.3 2012/10/30 18:55:04 yamt Exp $	*/

/*
 * Copyright (C) 2012 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: flags.c,v 1.1.1.2 2012/07/22 13:44:38 darrenr Exp $
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

char	flagset[] = "FSRPAUEC";
u_char	flags[] = { TH_FIN, TH_SYN, TH_RST, TH_PUSH, TH_ACK, TH_URG,
		    TH_ECN, TH_CWR };
