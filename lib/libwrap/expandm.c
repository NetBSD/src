/*	$NetBSD: expandm.c,v 1.13 2024/10/04 08:37:20 rillig Exp $	*/

/*-
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: expandm.c,v 1.13 2024/10/04 08:37:20 rillig Exp $");

#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "expandm.h"

#ifdef TEST
#undef INT_MAX
#define INT_MAX 31
#endif


const char * __attribute__((__format_arg__(1)))
expandm(const char *fmt, const char *sf, char **rbuf)
{
	const int err = errno;
	const char *e = NULL;
	char *buf, *nbuf;
	const char *ptr, *m;

	buf = NULL;
	for (ptr = fmt; (m = strstr(ptr, "%m")) != NULL; ptr = m + 2) {
		size_t cnt = 0;
		size_t nlen;

		for (const char *p = m; p >= ptr && *p == '%'; p--)
			cnt++;

		nlen = (size_t)(m - ptr);
		/*
		 * we can't exceed INT_MAX because int is used as 
		 * a format width
		 */
		if (__predict_false(nlen >= INT_MAX)) {
			size_t blen = buf ? strlen(buf) : 0;
			size_t tlen;

			/*
			 * if we would overflow a ptrdiff_t when computing
			 * tlen, then don't bother.  The format string is
			 * simply too large to be converted.
			 */
			if (blen >= PTRDIFF_MAX ||
			    nlen >= PTRDIFF_MAX - blen ||
			    nlen >= SIZE_T_MAX - blen)
				goto out;

			tlen = nlen + blen;

			nbuf = realloc(buf, tlen + 1);
			if (nbuf == NULL)
				goto out;

			memcpy(nbuf + blen, ptr, nlen);
			nbuf[tlen] = '\0';
			ptr += nlen;
			buf = nbuf;
		}

		if (__predict_true(e == NULL && (cnt & 1) != 0))
			e = strerror(err);
		if (asprintf(&nbuf, "%s%.*s%s", buf ? buf : "",
		    (int)(m - ptr), ptr, (cnt & 1) ? e : "%m") == -1)
			goto out;
		free(buf);
		buf = nbuf;
	}

	if (asprintf(&nbuf, "%s%s%s", buf ? buf : "", ptr, sf ? sf : "") == -1)
		goto out;

	free(buf);
	if (rbuf)
		*rbuf = nbuf;
	errno = err;
	return nbuf;
out:
	free(buf);
	if (rbuf)
		*rbuf = NULL;
	errno = err;
	return fmt;
}

#ifdef TEST
int
main(int argc, char *argv[])
{
	errno = ERANGE;
	printf("%s\n", expandm(argc > 1 ? argv[1] : "Message %%m=%m: %%%m%%",
	    "...", NULL));
	return 0;
}
#endif
