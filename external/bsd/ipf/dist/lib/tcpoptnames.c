/*	$NetBSD: tcpoptnames.c,v 1.1.1.1.2.2 2012/04/17 00:03:21 yamt Exp $	*/

/*
 * Copyright (C) 2009 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id
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
