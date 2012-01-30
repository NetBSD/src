/*	$NetBSD: printifname.c,v 1.1.1.3 2012/01/30 16:03:25 darrenr Exp $	*/

/*
 * Copyright (C) 2009 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: printifname.c,v 1.6.2.1 2012/01/26 05:29:16 darrenr Exp
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
