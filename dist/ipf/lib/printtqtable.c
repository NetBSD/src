/*	$NetBSD: printtqtable.c,v 1.1.1.1.2.3 2007/07/16 11:05:20 liamjfoy Exp $	*/

/*
 * Copyright (C) 2007 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 */

#include <fcntl.h>
#include <sys/ioctl.h>
#include "ipf.h"
#include "ipl.h"


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
