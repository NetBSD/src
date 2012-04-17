/*	$NetBSD: printdstlistpolicy.c,v 1.1.1.1.2.2 2012/04/17 00:03:19 yamt Exp $	*/

/*
 * Copyright (C) 2009 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 */

#include "ipf.h"


void
printdstlistpolicy(policy)
	ippool_policy_t policy;
{
	switch (policy)
	{
	case IPLDP_NONE :
		PRINTF("none");
		break;
	case IPLDP_ROUNDROBIN :
		PRINTF("round-robin");
		break;
	case IPLDP_CONNECTION :
		PRINTF("weighting connection");
		break;
	case IPLDP_RANDOM :
		PRINTF("random");
		break;
	default :
		break;
	}
}
