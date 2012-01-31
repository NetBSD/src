/*	$NetBSD: uinttoa.c,v 1.1.1.2 2012/01/31 21:23:58 kardel Exp $	*/

/*
 * uinttoa - return an asciized unsigned integer
 */
#include <stdio.h>

#include "lib_strbuf.h"
#include "ntp_stdlib.h"

char *
uinttoa(
	u_long uval
	)
{
	register char *buf;

	LIB_GETBUF(buf);
	snprintf(buf, LIB_BUFLENGTH, "%lu", uval);

	return buf;
}
