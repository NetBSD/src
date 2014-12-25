/*	$NetBSD: refnumtoa.c,v 1.1.1.1.14.1 2014/12/25 02:28:08 snj Exp $	*/

/*
 * refnumtoa - return asciized refclock addresses stored in local array space
 */
#include <config.h>
#include <stdio.h>

#include "ntp_net.h"
#include "lib_strbuf.h"
#include "ntp_stdlib.h"

const char *
refnumtoa(
	sockaddr_u *num
	)
{
	u_int32 netnum;
	char *buf;
	const char *rclock;

	if (!ISREFCLOCKADR(num))
		return socktoa(num);

	LIB_GETBUF(buf);
	netnum = SRCADR(num);
	rclock = clockname((int)((u_long)netnum >> 8) & 0xff);

	if (rclock != NULL)
		snprintf(buf, LIB_BUFLENGTH, "%s(%lu)",
			 rclock, (u_long)netnum & 0xff);
	else
		snprintf(buf, LIB_BUFLENGTH, "REFCLK(%lu,%lu)",
			 ((u_long)netnum >> 8) & 0xff,
			 (u_long)netnum & 0xff);

	return buf;
}
