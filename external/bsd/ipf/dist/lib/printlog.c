/*	$NetBSD: printlog.c,v 1.1.1.2 2012/07/22 13:44:41 darrenr Exp $	*/

/*
 * Copyright (C) 2012 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * $Id: printlog.c,v 1.1.1.2 2012/07/22 13:44:41 darrenr Exp $
 */

#include "ipf.h"

#include <syslog.h>


void
printlog(fp)
	frentry_t *fp;
{
	char *s, *u;

	PRINTF("log");
	if (fp->fr_flags & FR_LOGBODY)
		PRINTF(" body");
	if (fp->fr_flags & FR_LOGFIRST)
		PRINTF(" first");
	if (fp->fr_flags & FR_LOGORBLOCK)
		PRINTF(" or-block");
	if (fp->fr_loglevel != 0xffff) {
		PRINTF(" level ");
		s = fac_toname(fp->fr_loglevel);
		if (s == NULL || *s == '\0')
			s = "!!!";
		u = pri_toname(fp->fr_loglevel);
		if (u == NULL || *u == '\0')
			u = "!!!";
		PRINTF("%s.%s", s, u);
	}
}
