/*	$NetBSD: uinttoa.c,v 1.1.1.1.6.1 2012/04/17 00:03:46 yamt Exp $	*/

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
