/*	$NetBSD$	*/

/*
 * Copyright (C) 2009 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: printlookup.c,v 1.6.2.1 2012/01/26 05:29:16 darrenr Exp
 */

#include "ipf.h"


void
printlookup(base, addr, mask)
	char *base;
	i6addr_t *addr, *mask;
{
	char name[32];

	switch (addr->iplookuptype)
	{
	case IPLT_POOL :
		PRINTF("pool/");
		break;
	case IPLT_HASH :
		PRINTF("hash/");
		break;
	case IPLT_DSTLIST :
		PRINTF("dstlist/");
		break;
	default :
		PRINTF("lookup(%x)=", addr->iplookuptype);
		break;
	}

	if (addr->iplookupsubtype == 0)
		PRINTF("%u", addr->iplookupnum);
	else if (addr->iplookupsubtype == 1) {
		strncpy(name, base + addr->iplookupname, sizeof(name));
		name[sizeof(name) - 1] = '\0';
		PRINTF("%s", name);
	}

	if (mask->iplookupptr == NULL)
		PRINTF("(!)");
}
