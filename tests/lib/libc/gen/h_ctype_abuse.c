/*	$NetBSD: h_ctype_abuse.c,v 1.2 2025/09/15 17:32:01 riastradh Exp $	*/

/*-
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Helper program to verify effects of ctype(3) abuse.
 *
 * NOTE: This program intentionally triggers undefined behaviour by
 * passing int values to the ctype(3) functions which are neither EOF
 * nor representable by unsigned char.  The purpose is to verify that
 * NetBSD's ctype(3) _does not_ trap the undefined behaviour when the
 * environment variable LIBC_ALLOWCTYPEABUSE.  (It does not check
 * anything about the results, which are perforce nonsense -- just that
 * it gets a result without crashing.)
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: h_ctype_abuse.c,v 1.2 2025/09/15 17:32:01 riastradh Exp $");

#include <ctype.h>
#include <err.h>
#include <limits.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define	FOREACHCTYPE(M)							      \
	M(ISALPHA, isalpha)						      \
	M(ISUPPER, isupper)						      \
	M(ISLOWER, islower)						      \
	M(ISDIGIT, isdigit)						      \
	M(ISXDIGIT, isxdigit)						      \
	M(ISALNUM, isalnum)						      \
	M(ISSPACE, isspace)						      \
	M(ISPUNCT, ispunct)						      \
	M(ISPRINT, isprint)						      \
	M(ISGRAPH, isgraph)						      \
	M(ISCNTRL, iscntrl)						      \
	M(ISBLANK, isblank)						      \
	M(TOUPPER, toupper)						      \
	M(TOLOWER, tolower)

int
main(int argc, char **argv)
{
	enum {
#define	M(upper, lower) upper,
		FOREACHCTYPE(M)
#undef M
	} fn;
	enum {
		MACRO,
		FUNCTION,
	} mode;
	int ch;
	volatile int result;

	setprogname(argv[0]);
	if (argc != 3 && argc != 4) {
		errx(1, "Usage: %s <function> <mode> [<locale>]",
		    getprogname());
	}

#define	M(upper, lower)							      \
	else if (strcmp(argv[1], #lower) == 0)				      \
		fn = upper;

	if (0)
		;
	FOREACHCTYPE(M)
	else
		errx(1, "unknown ctype function");
#undef M

	if (strcmp(argv[2], "macro") == 0)
		mode = MACRO;
	else if (strcmp(argv[2], "function") == 0)
		mode = FUNCTION;
	else
		errx(1, "unknown usage mode");

	if (argc == 4) {
		if (setlocale(LC_CTYPE, argv[3]) == NULL)
			err(1, "setlocale");
	}

	/*
	 * Make sure we cover EOF as well.
	 */
	__CTASSERT(CHAR_MIN == 0 || CHAR_MIN <= EOF);
	__CTASSERT(EOF <= UCHAR_MAX);

	for (ch = (CHAR_MIN == 0 ? EOF : CHAR_MIN); ch <= UCHAR_MAX; ch++) {
		switch (fn) {
#define	M(upper, lower)							      \
		case upper:						      \
			switch (mode) {					      \
			case MACRO:					      \
				result = lower(ch);			      \
				break;					      \
			case FUNCTION:					      \
				result = (lower)(ch);			      \
				break;					      \
			}						      \
			break;
		FOREACHCTYPE(M)
#undef M
		}
		(void)result;
	}

	return 0;
}
