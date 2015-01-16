/*	$NetBSD: strtonum.c,v 1.1 2015/01/16 18:41:33 christos Exp $	*/
/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
__RCSID("$NetBSD: strtonum.c,v 1.1 2015/01/16 18:41:33 christos Exp $");

#define _OPENBSD_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <inttypes.h>

/*
 * Problems with the strtonum(3) API:
 *   - will return 0 on failure; 0 might not be in range, so
 *     that necessitates an error check even if you want to avoid it.
 *   - does not differentiate 'illegal' returns, so we can't tell
 *     the difference between partial and no conversions.
 *   - returns english strings
 *   - can't set the base, or find where the conversion ended
 */
long long
strtonum(const char * __restrict ptr, long long lo, long long hi,
    const char ** __restrict res)
{
	int e;
	intmax_t rv;
	const char *resp;

	if (res == NULL)
		res = &resp;

	rv = strtoi(ptr, NULL, 0, lo, hi, &e);
	if (e == 0) {
		*res = NULL;
		return rv;
	}
	*res = e != ERANGE ? "invalid" : (rv == hi ? "too large" : "too small");
	return 0;
}
