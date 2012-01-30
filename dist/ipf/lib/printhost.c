/*	$NetBSD$	*/

/*
 * Copyright (C) 2009 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: printhost.c,v 1.3.2.1 2012/01/26 05:29:16 darrenr Exp
 */

#include "ipf.h"


void
printhost(family, addr)
	int	family;
	u_32_t	*addr;
{
#ifdef  USE_INET6
	char ipbuf[64];
#else
	struct in_addr ipa;
#endif

	if ((family == -1) || !*addr)
		PRINTF("any");
	else {
		void *ptr = addr;

#ifdef  USE_INET6
		PRINTF("%s", inet_ntop(family, ptr, ipbuf, sizeof(ipbuf)));
#else
		ipa.s_addr = *addr;
		PRINTF("%s", inet_ntoa(ipa));
#endif
	}
}
