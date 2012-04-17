/*	$NetBSD: printtqtable.c,v 1.1.1.1.2.2 2012/04/17 00:03:21 yamt Exp $	*/

/*
 * Copyright (C) 2009 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 */

#include <fcntl.h>
#include <sys/ioctl.h>
#include "ipf.h"


void
printtqtable(table)
	ipftq_t *table;
{
	int i;

	PRINTF("TCP Entries per state\n");
	for (i = 0; i < IPF_TCP_NSTATES; i++)
		PRINTF(" %5d", i);
	PRINTF("\n");

	for (i = 0; i < IPF_TCP_NSTATES; i++)
		PRINTF(" %5d", table[i].ifq_ref - 1);
	PRINTF("\n");
}
