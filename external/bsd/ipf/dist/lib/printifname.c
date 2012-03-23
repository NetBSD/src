/*	$NetBSD: printifname.c,v 1.1.1.1 2012/03/23 21:20:09 christos Exp $	*/

/*
 * Copyright (C) 2009 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id
 */

#include "ipf.h"


void
printifname(format, name, ifp)
	char *format, *name;
	void *ifp;
{
	PRINTF("%s%s", format, name);
	if ((ifp == NULL) && strcmp(name, "-") && strcmp(name, "*"))
		PRINTF("(!)");
}
