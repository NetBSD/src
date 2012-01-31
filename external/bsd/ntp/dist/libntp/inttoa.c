/*	$NetBSD: inttoa.c,v 1.1.1.2 2012/01/31 21:24:09 kardel Exp $	*/

/*
 * inttoa - return an asciized signed integer
 */
#include <config.h>
#include <stdio.h>

#include "lib_strbuf.h"
#include "ntp_stdlib.h"

char *
inttoa(
	long val
	)
{
	register char *buf;

	LIB_GETBUF(buf);
	snprintf(buf, LIB_BUFLENGTH, "%ld", val);

	return buf;
}
