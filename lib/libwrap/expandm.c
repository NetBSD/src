/*	$NetBSD: expandm.c,v 1.6 2019/01/13 01:32:51 christos Exp $	*/

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: expandm.c,v 1.6 2019/01/13 01:32:51 christos Exp $");

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
	const char *e = strerror(errno);
	char *buf, *m, *nbuf;
	const char *ptr;

	for (ptr = fmt, buf = NULL; (m = strstr(ptr, "%m")) != NULL;
	    ptr = m + 2)
	{
		size_t cnt = 0;

		for (char *p = m; p >= ptr && *p == '%'; p--)
			cnt++;

               if (__predict_false((m - ptr) >= INT_MAX)) {
                        size_t blen = buf ? strlen(buf) : 0;
                        size_t nlen = (size_t)(m - ptr);

                        nbuf = realloc(buf, blen + nlen + 1);
                        if (nbuf == NULL)
                                goto out;

                        memcpy(nbuf + blen, ptr, nlen);
                        nbuf[blen + nlen] = '\0';
                        ptr += nlen;
                        buf = nbuf;
                }

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
	return nbuf;
out:
	free(buf);
	if (rbuf)
		*rbuf = NULL;
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
