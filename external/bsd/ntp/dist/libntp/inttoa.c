/*	$NetBSD: inttoa.c,v 1.1.1.1 2009/12/13 16:55:03 kardel Exp $	*/

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
	snprintf(buf, sizeof(buf), "%ld", val);

	return buf;
}
