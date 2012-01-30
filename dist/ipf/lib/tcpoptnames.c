/*	$NetBSD: tcpoptnames.c,v 1.3 2012/01/30 16:12:04 darrenr Exp $	*/

/*
 * Copyright (C) 2009 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: tcpoptnames.c,v 1.7.2.1 2012/01/26 05:29:17 darrenr Exp
 */

#include "ipf.h"


struct	ipopt_names	tcpoptnames[] ={
	{ TCPOPT_NOP,			0x000001,	1,	"nop" },
	{ TCPOPT_MAXSEG,		0x000002,	4,	"maxseg" },
	{ TCPOPT_WINDOW,		0x000004,	3,	"wscale" },
	{ TCPOPT_SACK_PERMITTED,	0x000008,	2,	"sackok" },
	{ TCPOPT_SACK,			0x000010,	3,	"sack" },
	{ TCPOPT_TIMESTAMP,		0x000020,	10,	"tstamp" },
	{ 0, 		0,	0,	(char *)NULL }     /* must be last */
};
