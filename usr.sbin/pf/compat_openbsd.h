/*	$NetBSD: compat_openbsd.h,v 1.5 2008/06/18 09:06:28 yamt Exp $	*/

/*-
 * Copyright (c)2004 YAMAMOTO Takashi,
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * misc OpenBSD compatibility hacks.
 */

#include <sys/cdefs.h>

#include <stdarg.h>

/*
 * sys/queue.h
 */

#define	TAILQ_END(h)	NULL
#define	LIST_END(h)	NULL


/*
 * libc/stdlib/strtonum.c
 */

#include <limits.h>
#include <stdlib.h>

static __inline long long
strtonum(const char *, long long, long long, const char **) __unused;

static __inline long long
strtonum(const char *numstr, long long minval, long long maxval,
    const char **errstrp)
{
	char *p;
	long long n;

	n = strtol(numstr, &p, 10);
	if (*p != '\0' || *numstr == '\0') {
		*errstrp = "invalid";
		n = 0;
	}
	return (n);
}
