/*	$NetBSD: printdstlistpolicy.c,v 1.1.1.1.2.3 2012/10/30 18:55:09 yamt Exp $	*/

/*
 * Copyright (C) 2012 by Darren Reed.
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
