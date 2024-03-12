/*	$NetBSD: decode.c,v 1.1.26.1 2024/03/12 10:04:22 martin Exp $	*/

/*
 * Copyright (c) 1999 Matthew R. Green
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>

#ifndef lint
__RCSID("$NetBSD: decode.c,v 1.1.26.1 2024/03/12 10:04:22 martin Exp $");
#endif

#include <sys/types.h>
#include <sys/time.h>

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <err.h>

#include "libaudio.h"

void
decode_int(const char *arg, int *intp)
{
	char	*ep;
	int	ret;

	ret = (int)strtoul(arg, &ep, 10);

	if (ep[0] == '\0') {
		*intp = ret;
		return;
	}
	errx(1, "argument `%s' not a valid integer", arg);
}

void
decode_uint(const char *arg, unsigned *intp)
{
	char	*ep;
	unsigned	ret;

	ret = (unsigned)strtoul(arg, &ep, 10);

	if (ep[0] == '\0') {
		*intp = ret;
		return;
	}
	errx(1, "argument `%s' not a valid integer", arg);
}

void
decode_time(const char *arg, struct timeval *tvp)
{
	char	*s, *colon, *dot;
	char	*copy = strdup(arg);
	int	first;

	if (copy == NULL)
		err(1, "could not allocate a copy of %s", arg);

	tvp->tv_sec = tvp->tv_usec = 0;
	s = copy;

	/* handle [hh:]mm:ss.dd */
	if ((colon = strchr(s, ':')) != NULL) {
		*colon++ = '\0';
		decode_int(s, &first);
		tvp->tv_sec = first * 60;	/* minutes */
		s = colon;

		if ((colon = strchr(s, ':')) != NULL) {
			*colon++ = '\0';
			decode_int(s, &first);
			tvp->tv_sec += first;	/* minutes and hours */
			tvp->tv_sec *= 60;
			s = colon;
		}
	}
	if ((dot = strchr(s, '.')) != NULL) {
		int 	i, base = 100000;

		*dot++ = '\0';

		for (i = 0; i < 6; i++, base /= 10) {
			if (!dot[i])
				break;
			if (!isdigit((unsigned char)dot[i]))
				errx(1, "argument `%s' is not a value time specification", arg);
			tvp->tv_usec += base * (dot[i] - '0');
		}
	}
	decode_int(s, &first);
	tvp->tv_sec += first;

	free(copy);
}
