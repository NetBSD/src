/*	$NetBSD: icmpcode.c,v 1.1.1.1 2012/03/23 21:20:08 christos Exp $	*/

/*
 * Copyright (C) 2006 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id
 */

#include <ctype.h>

#include "ipf.h"

#ifndef	MIN
# define	MIN(a,b)	((a) > (b) ? (b) : (a))
#endif


char	*icmpcodes[MAX_ICMPCODE + 1] = {
	"net-unr", "host-unr", "proto-unr", "port-unr", "needfrag", "srcfail",
	"net-unk", "host-unk", "isolate", "net-prohib", "host-prohib",
	"net-tos", "host-tos", "filter-prohib", "host-preced", "preced-cutoff",
	NULL };
