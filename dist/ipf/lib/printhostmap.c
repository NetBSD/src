/*	$NetBSD: printhostmap.c,v 1.1.1.1 2004/03/28 08:56:20 martti Exp $	*/

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
