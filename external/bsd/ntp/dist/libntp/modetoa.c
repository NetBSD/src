/*	$NetBSD: modetoa.c,v 1.3.16.1 2014/12/25 02:13:05 snj Exp $	*/

/*
 * modetoa - return an asciized mode
 */
#include <config.h>
#include <stdio.h>

#include "lib_strbuf.h"
#include "ntp_stdlib.h"

const char *
modetoa(
	size_t mode
	)
{
	char *bp;
	static const char * const modestrings[] = {
		"unspec",
		"sym_active",
		"sym_passive",
		"client",
		"server",
		"broadcast",
		"control",
		"private",
		"bclient",
	};

	if (mode >= COUNTOF(modestrings)) {
		LIB_GETBUF(bp);
		snprintf(bp, LIB_BUFLENGTH, "mode#%zu", mode);
		return bp;
	}

	return modestrings[mode];
}
