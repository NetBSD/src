/*	$NetBSD: modetoa.c,v 1.2 2010/12/04 23:08:34 christos Exp $	*/

/*
 * modetoa - return an asciized mode
 */
#include <stdio.h>

#include "lib_strbuf.h"
#include "ntp_stdlib.h"

const char *
modetoa(
	size_t mode
	)
{
	char *bp;
	static const char *modestrings[] = {
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

	if (mode >= (sizeof modestrings)/sizeof(char *)) {
		LIB_GETBUF(bp);
		(void)sprintf(bp, "mode#%zu", mode);
		return bp;
	}

	return modestrings[mode];
}
