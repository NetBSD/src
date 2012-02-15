/*	$NetBSD: printhostmap.c,v 1.2 2012/02/15 17:55:07 riz Exp $	*/

/*
 * Copyright (C) 2002-2005 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: printhostmap.c,v 1.3.2.4 2009/12/27 06:58:07 darrenr Exp
 */

#include "ipf.h"

void printhostmap(hmp, hv)
hostmap_t *hmp;
u_int hv;
{

	printf("%s,", inet_ntoa(hmp->hm_srcip));
	printf("%s -> ", inet_ntoa(hmp->hm_dstip));
	printf("%s ", inet_ntoa(hmp->hm_mapip));
	printf("(use = %d hv = %u)\n", hmp->hm_ref, hv);
}
