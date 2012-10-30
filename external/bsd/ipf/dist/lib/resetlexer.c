/*	$NetBSD: resetlexer.c,v 1.1.1.1.2.3 2012/10/30 18:55:11 yamt Exp $	*/

/*
 * Copyright (C) 2012 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: resetlexer.c,v 1.1.1.2 2012/07/22 13:44:42 darrenr Exp $
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
