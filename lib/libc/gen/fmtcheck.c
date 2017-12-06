/*	$NetBSD: fmtcheck.c,v 1.15 2017/12/06 14:05:14 rin Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code was contributed to The NetBSD Foundation by Allen Briggs.
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

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: fmtcheck.c,v 1.15 2017/12/06 14:05:14 rin Exp $");
#endif

#include "namespace.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#ifdef __weak_alias
__weak_alias(fmtcheck,__fmtcheck)
#endif

enum __e_fmtcheck_types {
	FMTCHECK_START,
	FMTCHECK_SHORT,
	FMTCHECK_INT,
	FMTCHECK_WINTT,
	FMTCHECK_LONG,
	FMTCHECK_QUAD,
	FMTCHECK_INTMAXT,
	FMTCHECK_PTRDIFFT,
	FMTCHECK_SIZET,
	FMTCHECK_POINTER,
	FMTCHECK_CHARPOINTER,
	FMTCHECK_SHORTPOINTER,
	FMTCHECK_INTPOINTER,
	FMTCHECK_LONGPOINTER,
	FMTCHECK_QUADPOINTER,
	FMTCHECK_INTMAXTPOINTER,
	FMTCHECK_PTRDIFFTPOINTER,
	FMTCHECK_SIZETPOINTER,
	FMTCHECK_DOUBLE,
	FMTCHECK_LONGDOUBLE,
	FMTCHECK_STRING,
	FMTCHECK_WSTRING,
	FMTCHECK_WIDTH,
	FMTCHECK_PRECISION,
	FMTCHECK_DONE,
	FMTCHECK_UNKNOWN
};
typedef enum __e_fmtcheck_types EFT;

enum e_modifier {
	MOD_NONE,
	MOD_CHAR,
	MOD_SHORT,
	MOD_LONG,
	MOD_QUAD,
	MOD_INTMAXT,
	MOD_LONGDOUBLE,
	MOD_PTRDIFFT,
	MOD_SIZET,
};

#define RETURN(pf,f,r) do { \
			*(pf) = (f); \
			return r; \
		       } /*NOTREACHED*/ /*CONSTCOND*/ while (0)

static EFT
get_next_format_from_precision(const char **pf)
{
	enum e_modifier	modifier;
	const char	*f;

	f = *pf;
	switch (*f) {
	case 'h':
		f++;
		if (!*f) RETURN(pf,f,FMTCHECK_UNKNOWN);
		if (*f == 'h') {
			f++;
			modifier = MOD_CHAR;
		} else {
			modifier = MOD_SHORT;
		}
		break;
	case 'j':
		f++;
		modifier = MOD_INTMAXT;
		break;
	case 'l':
		f++;
		if (!*f) RETURN(pf,f,FMTCHECK_UNKNOWN);
		if (*f == 'l') {
			f++;
			modifier = MOD_QUAD;
		} else {
			modifier = MOD_LONG;
		}
		break;
	case 'q':
		f++;
		modifier = MOD_QUAD;
		break;
	case 't':
		f++;
		modifier = MOD_PTRDIFFT;
		break;
	case 'z':
		f++;
		modifier = MOD_SIZET;
		break;
	case 'L':
		f++;
		modifier = MOD_LONGDOUBLE;
		break;
#ifdef WIN32
	case 'I':
		f++;
		if (!*f) RETURN(pf,f,FMTCHECK_UNKNOWN);
		if (*f == '3' && f[1] == '2') {
			f += 2;
			modifier = MOD_NONE;
		} else if (*f == '6' && f[1] == '4') {
			f += 2;
			modifier = MOD_QUAD;
		}
		else {
#ifdef _WIN64
			modifier = MOD_QUAD;
#else
			modifier = MOD_NONE;
#endif
		}
		break;
#endif
	default:
		modifier = MOD_NONE;
		break;
	}
	if (!*f) RETURN(pf,f,FMTCHECK_UNKNOWN);
	if (strchr("diouxX", *f)) {
		switch (modifier) {
		case MOD_LONG:
			RETURN(pf,f,FMTCHECK_LONG);
		case MOD_QUAD:
			RETURN(pf,f,FMTCHECK_QUAD);
		case MOD_INTMAXT:
			RETURN(pf,f,FMTCHECK_INTMAXT);
		case MOD_PTRDIFFT:
			RETURN(pf,f,FMTCHECK_PTRDIFFT);
		case MOD_SIZET:
			RETURN(pf,f,FMTCHECK_SIZET);
		case MOD_CHAR:
		case MOD_SHORT:
		case MOD_NONE:
			RETURN(pf,f,FMTCHECK_INT);
		default:
			RETURN(pf,f,FMTCHECK_UNKNOWN);
		}
	}
	if (*f == 'n') {
		switch (modifier) {
		case MOD_CHAR:
			RETURN(pf,f,FMTCHECK_CHARPOINTER);
		case MOD_SHORT:
			RETURN(pf,f,FMTCHECK_SHORTPOINTER);
		case MOD_LONG:
			RETURN(pf,f,FMTCHECK_LONGPOINTER);
		case MOD_QUAD:
			RETURN(pf,f,FMTCHECK_QUADPOINTER);
		case MOD_INTMAXT:
			RETURN(pf,f,FMTCHECK_INTMAXTPOINTER);
		case MOD_PTRDIFFT:
			RETURN(pf,f,FMTCHECK_PTRDIFFTPOINTER);
		case MOD_SIZET:
			RETURN(pf,f,FMTCHECK_SIZETPOINTER);
		case MOD_NONE:
			RETURN(pf,f,FMTCHECK_INTPOINTER);
		default:
			RETURN(pf,f,FMTCHECK_UNKNOWN);
		}
	}
	if (strchr("DOU", *f)) {
		if (modifier != MOD_NONE)
			RETURN(pf,f,FMTCHECK_UNKNOWN);
		RETURN(pf,f,FMTCHECK_LONG);
	}
	if (strchr("aAeEfFgG", *f)) {
		switch (modifier) {
		case MOD_LONGDOUBLE:
			RETURN(pf,f,FMTCHECK_LONGDOUBLE);
		case MOD_LONG:
		case MOD_NONE:
			RETURN(pf,f,FMTCHECK_DOUBLE);
		default:
			RETURN(pf,f,FMTCHECK_UNKNOWN);
		}
	}
	if (*f == 'c') {
		switch (modifier) {
		case MOD_LONG:
			RETURN(pf,f,FMTCHECK_WINTT);
		case MOD_NONE:
			RETURN(pf,f,FMTCHECK_INT);
		default:
			RETURN(pf,f,FMTCHECK_UNKNOWN);
		}
	}
	if (*f == 'C') {
		if (modifier != MOD_NONE)
			RETURN(pf,f,FMTCHECK_UNKNOWN);
		RETURN(pf,f,FMTCHECK_WINTT);
	}
	if (*f == 's') {
		switch (modifier) {
		case MOD_LONG:
			RETURN(pf,f,FMTCHECK_WSTRING);
		case MOD_NONE:
			RETURN(pf,f,FMTCHECK_STRING);
		default:
			RETURN(pf,f,FMTCHECK_UNKNOWN);
		}
	}
	if (*f == 'S') {
		if (modifier != MOD_NONE)
			RETURN(pf,f,FMTCHECK_UNKNOWN);
		RETURN(pf,f,FMTCHECK_WSTRING);
	}
	if (*f == 'p') {
		if (modifier != MOD_NONE)
			RETURN(pf,f,FMTCHECK_UNKNOWN);
		RETURN(pf,f,FMTCHECK_POINTER);
	}
	RETURN(pf,f,FMTCHECK_UNKNOWN);
	/*NOTREACHED*/
}

static EFT
get_next_format_from_width(const char **pf)
{
	const char	*f;

	f = *pf;
	if (*f == '.') {
		f++;
		if (*f == '*') {
			RETURN(pf,f,FMTCHECK_PRECISION);
		}
		/* eat any precision (empty is allowed) */
		while (isdigit((unsigned char)*f)) f++;
		if (!*f) RETURN(pf,f,FMTCHECK_UNKNOWN);
	}
	RETURN(pf,f,get_next_format_from_precision(pf));
	/*NOTREACHED*/
}

static EFT
get_next_format(const char **pf, EFT eft)
{
	int		infmt;
	const char	*f;

	if (eft == FMTCHECK_WIDTH) {
		(*pf)++;
		return get_next_format_from_width(pf);
	} else if (eft == FMTCHECK_PRECISION) {
		(*pf)++;
		return get_next_format_from_precision(pf);
	}

	f = *pf;
	infmt = 0;
	while (!infmt) {
		f = strchr(f, '%');
		if (f == NULL)
			RETURN(pf,f,FMTCHECK_DONE);
		f++;
		if (!*f)
			RETURN(pf,f,FMTCHECK_UNKNOWN);
		if (*f != '%')
			infmt = 1;
		else
			f++;
	}

	/* Eat any of the flags */
	while (*f && (strchr("#'0- +", *f)))
		f++;

	if (*f == '*') {
		RETURN(pf,f,FMTCHECK_WIDTH);
	}
	/* eat any width */
	while (isdigit((unsigned char)*f)) f++;
	if (!*f) {
		RETURN(pf,f,FMTCHECK_UNKNOWN);
	}

	RETURN(pf,f,get_next_format_from_width(pf));
	/*NOTREACHED*/
}

const char *
fmtcheck(const char *f1, const char *f2)
{
	const char	*f1p, *f2p;
	EFT		f1t, f2t;

	if (!f1) return f2;
	
	f1p = f1;
	f1t = FMTCHECK_START;
	f2p = f2;
	f2t = FMTCHECK_START;
	while ((f1t = get_next_format(&f1p, f1t)) != FMTCHECK_DONE) {
		if (f1t == FMTCHECK_UNKNOWN)
			return f2;
		f2t = get_next_format(&f2p, f2t);
		if (f1t != f2t)
			return f2;
	}
	if (get_next_format(&f2p, f2t) != FMTCHECK_DONE)
		return f2;
	else
		return f1;
}
