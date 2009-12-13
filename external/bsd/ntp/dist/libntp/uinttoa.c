/*	$NetBSD: uinttoa.c,v 1.1.1.1 2009/12/13 16:55:06 kardel Exp $	*/

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

	(void) sprintf(buf, "%lu", (u_long)uval);
	return buf;
}
