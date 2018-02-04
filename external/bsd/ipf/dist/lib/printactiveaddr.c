/*	$NetBSD: printactiveaddr.c,v 1.3 2018/02/04 08:19:42 mrg Exp $	*/

/*
 * Copyright (C) 2012 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Added redirect stuff and a variety of bug fixes. (mcn@EnGarde.com)
 */

#include "ipf.h"


#if !defined(lint)
static __attribute__((__used__)) const char rcsid[] = "@(#)Id: printactiveaddr.c,v 1.1.1.2 2012/07/22 13:44:40 darrenr Exp $";
#endif


void
printactiveaddress(v, fmt, addr, ifname)
	int v;
	char *fmt, *ifname;
	i6addr_t *addr;
{
	switch (v)
	{
	case 4 :
		PRINTF(fmt, inet_ntoa(addr->in4));
		break;
#ifdef USE_INET6
	case 6 :
		printaddr(AF_INET6, FRI_NORMAL, ifname, 0,
			  (u_32_t *)&addr->in6, NULL);
		break;
#endif
	default :
		break;
	}
}
