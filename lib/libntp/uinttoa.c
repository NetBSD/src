/*	$NetBSD: uinttoa.c,v 1.2 1998/01/09 03:16:37 perry Exp $	*/

/*
 * uinttoa - return an asciized unsigned integer
 */
#include <stdio.h>

#include "lib_strbuf.h"
#include "ntp_stdlib.h"

char *
uinttoa(uval)
	u_long uval;
{
	register char *buf;

	LIB_GETBUF(buf);

	(void) sprintf(buf, "%lu", (u_long)uval);
	return buf;
}
