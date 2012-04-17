/*	$NetBSD: printunit.c,v 1.1.1.1.2.2 2012/04/17 00:03:21 yamt Exp $	*/

/*
 * Copyright (C) 2009 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 */

#include "ipf.h"


void
printunit(unit)
	int unit;
{

	switch (unit)
	{
	case IPL_LOGIPF :
		PRINTF("ipf");
		break;
	case IPL_LOGNAT :
		PRINTF("nat");
		break;
	case IPL_LOGSTATE :
		PRINTF("state");
		break;
	case IPL_LOGAUTH :
		PRINTF("auth");
		break;
	case IPL_LOGSYNC :
		PRINTF("sync");
		break;
	case IPL_LOGSCAN :
		PRINTF("scan");
		break;
	case IPL_LOGLOOKUP :
		PRINTF("lookup");
		break;
	case IPL_LOGCOUNT :
		PRINTF("count");
		break;
	case IPL_LOGALL :
		PRINTF("all");
		break;
	default :
		PRINTF("unknown(%d)", unit);
	}
}
