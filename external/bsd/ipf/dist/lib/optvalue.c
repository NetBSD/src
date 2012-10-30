/*	$NetBSD: optvalue.c,v 1.1.1.1.2.3 2012/10/30 18:55:08 yamt Exp $	*/

/*
 * Copyright (C) 2012 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: optvalue.c,v 1.1.1.2 2012/07/22 13:44:39 darrenr Exp $
 */
#include "ipf.h"


u_32_t getoptbyname(optname)
	char *optname;
{
	struct ipopt_names *io;

	for (io = ionames; io->on_name; io++)
		if (!strcasecmp(optname, io->on_name))
			return io->on_bit;
	return -1;
}


u_32_t getoptbyvalue(optval)
	int optval;
{
	struct ipopt_names *io;

	for (io = ionames; io->on_name; io++)
		if (io->on_value == optval)
			return io->on_bit;
	return -1;
}
