/*	$NetBSD: append.c,v 1.21 2009/09/05 12:00:25 dsl Exp $	*/

/*-
 * Copyright (c) 2000-2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Ben Harris and Jaromir Dolecek.
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

/*-
 * Copyright (c) 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Peter McIlroy.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "sort.h"

#ifndef lint
__RCSID("$NetBSD: append.c,v 1.21 2009/09/05 12:00:25 dsl Exp $");
__SCCSID("@(#)append.c	8.1 (Berkeley) 6/6/93");
#endif /* not lint */

#include <stdlib.h>
#include <string.h>

static int
wt_cmp(const u_char *a, const u_char *b, size_t len, u_char *wts)
{
    size_t i;

    for (i = 0; i < len; i++) {
	    if (wts[*a++] != wts[*b++])
		return 1;
    }

    return 0;
}

/*
 * copy sorted lines to output; check for uniqueness
 */
void
append(const RECHEADER **keylist, int nelem, FILE *fp, put_func_t put, u_char *wts)
{
	const RECHEADER **cpos, **lastkey;
	const RECHEADER *crec, *prec;
	size_t plen;

	lastkey = keylist + nelem;
	if (!UNIQUE || wts == NULL) {
		for (cpos = keylist; cpos < lastkey; cpos++)
			put(*cpos, fp);
		return;
	}

	if (nelem == 0)
		return;

	cpos = keylist;
	prec = *cpos;

	if (!SINGL_FLD) {
		/* Key for each line is already in adjacent bytes */
		plen = prec->offset;
		for (cpos = &keylist[1]; cpos < lastkey; cpos++) {
			crec = *cpos;
			if (crec->offset == plen
			    && memcmp(crec->data, prec->data, plen) == 0) {
				/* Duplicate key */
				continue;
			}
			put(prec, fp);
			prec = crec;
			plen = prec->offset;
		}
		put(prec, fp);
		return;
	}

	/* We have to compare the raw data - which means applying weight */

	/* Key for each line is already in adjacent bytes */
	plen = prec->length;
	for (cpos = &keylist[1]; cpos < lastkey; cpos++) {
		crec = *cpos;
		if (crec->length == plen
		    && wt_cmp(crec->data, prec->data, plen, wts) == 0) {
			/* Duplicate key */
			continue;
		}
		put(prec, fp);
		prec = crec;
		plen = prec->length;
	}
	put(prec, fp);
	return;
}
