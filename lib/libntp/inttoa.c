/*	$NetBSD: inttoa.c,v 1.2 1998/01/09 03:16:16 perry Exp $	*/

/*
 * inttoa - return an asciized signed integer
 */
#include <stdio.h>

#include "lib_strbuf.h"
#include "ntp_stdlib.h"

char *
inttoa(ival)
	long ival;
{
	register char *buf;

	LIB_GETBUF(buf);

	(void) sprintf(buf, "%ld", (long)ival);
	return buf;
}
