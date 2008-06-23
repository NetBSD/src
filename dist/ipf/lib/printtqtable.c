/*	$NetBSD: printtqtable.c,v 1.1.1.1.16.1 2008/06/23 04:28:46 wrstuden Exp $	*/

/*
 * Copyright (C) 2007 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 */

#include <fcntl.h>
#include <sys/ioctl.h>
#include "ipf.h"


void printtqtable(table)
ipftq_t *table;
{
	int i;

	printf("TCP Entries per state\n");
	for (i = 0; i < IPF_TCP_NSTATES; i++)
		printf(" %5d", i);
	printf("\n");

	for (i = 0; i < IPF_TCP_NSTATES; i++)
		printf(" %5d", table[i].ifq_ref - 1);
	printf("\n");
}
