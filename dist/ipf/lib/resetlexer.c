/*	$NetBSD: resetlexer.c,v 1.1.1.4 2012/01/30 16:03:22 darrenr Exp $	*/

/*
 * Copyright (C) 2009 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: resetlexer.c,v 1.4.2.1 2012/01/26 05:29:17 darrenr Exp
 */

#include "ipf.h"

long	string_start = -1;
long	string_end = -1;
char	*string_val = NULL;
long	pos = 0;


void resetlexer()
{
	string_start = -1;
	string_end = -1;
	string_val = NULL;
	pos = 0;
}
