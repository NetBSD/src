/*	$NetBSD: print_toif.c,v 1.1.1.1.2.2 2012/04/17 00:03:19 yamt Exp $	*/

/*
 * Copyright (C) 2010 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id
 */

#include "ipf.h"


void
print_toif(tag, base, fdp)
	char *tag;
	char *base;
	frdest_t *fdp;
{
	switch (fdp->fd_type)
	{
	case FRD_NORMAL :
		PRINTF("%s %s%s", tag, base + fdp->fd_name,
		       (fdp->fd_ptr || (long)fdp->fd_ptr == -1) ? "" : "(!)");
#ifdef	USE_INET6
		if (use_inet6 && IP6_NOTZERO(&fdp->fd_ip6.in6)) {
			char ipv6addr[80];

			inet_ntop(AF_INET6, &fdp->fd_ip6, ipv6addr,
				  sizeof(fdp->fd_ip6));
			PRINTF(":%s", ipv6addr);
		} else
#endif
			if (fdp->fd_ip.s_addr)
				PRINTF(":%s", inet_ntoa(fdp->fd_ip));
		putchar(' ');
		break;

	case FRD_DSTLIST :
		PRINTF("%s dstlist/%s ", tag, base + fdp->fd_name);
		break;

	default :
		PRINTF("%s <%d>", tag, fdp->fd_type);
		break;
	}
}
