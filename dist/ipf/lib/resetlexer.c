/*	$NetBSD: resetlexer.c,v 1.1.1.3 2010/04/17 20:45:56 darrenr Exp $	*/

/*
 * Copyright (C) 2002 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: resetlexer.c,v 1.1.4.2 2009/12/27 06:58:07 darrenr Exp
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
