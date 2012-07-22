/*	$NetBSD: initparse.c,v 1.1.1.2 2012/07/22 13:44:38 darrenr Exp $	*/

/*
 * Copyright (C) 2012 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * $Id: initparse.c,v 1.1.1.2 2012/07/22 13:44:38 darrenr Exp $
 */
#include "ipf.h"


char	thishost[MAXHOSTNAMELEN];


void initparse __P((void))
{
	gethostname(thishost, sizeof(thishost));
	thishost[sizeof(thishost) - 1] = '\0';
}
