/*	$NetBSD: modetoa.c,v 1.9.8.1 2025/08/02 05:22:30 perseant Exp $	*/

/*
 * modetoa - return an asciized mode
 */
#include <config.h>
#include <stdio.h>

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
