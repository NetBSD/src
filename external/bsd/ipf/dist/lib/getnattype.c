/*	$NetBSD: getnattype.c,v 1.3 2018/02/04 08:19:42 mrg Exp $	*/

/*
 * Copyright (C) 2012 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Added redirect stuff and a variety of bug fixes. (mcn@EnGarde.com)
 */
#include "ipf.h"
#include "kmem.h"

#if !defined(lint)
static __attribute__((__used__)) const char rcsid[] = "@(#)Id: getnattype.c,v 1.1.1.2 2012/07/22 13:44:38 darrenr Exp $";
#endif


/*
 * Get a nat filter type given its kernel address.
 */
char *
getnattype(nat)
	nat_t *nat;
{
	static char unknownbuf[20];
	char *which;

	if (!nat)
		return "???";

	switch (nat->nat_redir)
	{
	case NAT_MAP :
		which = "MAP";
		break;
	case NAT_MAPBLK :
		which = "MAP-BLOCK";
		break;
	case NAT_REDIRECT :
		which = "RDR";
		break;
	case NAT_MAP|NAT_REWRITE :
		which = "RWR-MAP";
		break;
	case NAT_REDIRECT|NAT_REWRITE :
		which = "RWR-RDR";
		break;
	case NAT_BIMAP :
		which = "BIMAP";
		break;
	case NAT_REDIRECT|NAT_DIVERTUDP :
		which = "DIV-RDR";
		break;
	case NAT_MAP|NAT_DIVERTUDP :
		which = "DIV-MAP";
		break;
	case NAT_REDIRECT|NAT_ENCAP :
		which = "ENC-RDR";
		break;
	case NAT_MAP|NAT_ENCAP :
		which = "ENC-MAP";
		break;
	default :
		sprintf(unknownbuf, "unknown(%04x)",
			nat->nat_redir & 0xffffffff);
		which = unknownbuf;
		break;
	}
	return which;
}
