/* $NetBSD: fgetstr.c,v 1.9 2009/12/01 00:03:53 roy Exp $	*/

/*
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Roy Marples.
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: fgetstr.c,v 1.9 2009/12/01 00:03:53 roy Exp $");

#include "namespace.h"

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>

#include "reentrant.h"
#include "local.h"

/*
 * Get an input line.
 * This now uses getdelim(3) for a code reduction.
 * The upside is that strings are now always NULL terminated, but relying
 * on this is non portable - better to use the POSIX getdelim(3) function.
 */
char *
__fgetstr(FILE *__restrict fp, size_t *__restrict lenp, int sep)
{
	char *p;
	size_t size;
	ssize_t n;

	_DIAGASSERT(fp != NULL);
	_DIAGASSERT(lenp != NULL);

	p = (char *)fp->_lb._base;
	size = fp->_lb._size;
	n = getdelim(&p, &size, sep, fp);
	fp->_lb._base = (unsigned char *)p;
	/* The struct size variable is only an int .....
	 * This still works when exceeded, but the buffer could be
	 * realloced needlessly. */
	if (size > INT_MAX)
		fp->_lb._size = INT_MAX;
	else
		fp->_lb._size = (int)size;
	if (n == -1) {
		*lenp = 0;
		if (errno == EOVERFLOW) /* fixup errno */
			errno = EINVAL;
		return NULL;
	}
	*lenp = n;
	return p;
}
