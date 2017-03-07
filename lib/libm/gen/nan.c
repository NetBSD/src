/* $NetBSD: nan.c,v 1.3 2017/03/07 11:15:08 maya Exp $ */

/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Klaus Klein.
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
#if defined(LIBM_SCCS) && !defined(lint) && !defined(NAN_FUNCTION)
__RCSID("$NetBSD: nan.c,v 1.3 2017/03/07 11:15:08 maya Exp $");
#endif /* LIBM_SCCS and not lint */

#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>


#ifndef NAN_FUNCTION
#define	NAN_FUNCTION	nan
#define	NAN_TYPE	double
#define	NAN_STRTOD	strtod
#endif

NAN_TYPE
NAN_FUNCTION(const char *tagp)
{
	const char *nstr;
	char *buf;
	NAN_TYPE res;

	_DIAGASSERT(tagp != NULL);

	nstr = "NAN()";
	buf = NULL;

	if (tagp[0] != '\0') {
		size_t l;

		l = strlen(tagp);
		buf = malloc(5 + l + 1);

		if (buf != NULL) {
			/* Avoiding stdio in libm. */
			memcpy(buf,		"NAN(",	4);
			memcpy(buf + 4,		tagp,	l);
			memcpy(buf + 4 + l,	")",	2);
			nstr = buf;
		} else {
			/* Best effort: Fall back to "NAN()". */
		}
	}

	res = NAN_STRTOD(nstr, NULL);

	free(buf);

	return res;
}
