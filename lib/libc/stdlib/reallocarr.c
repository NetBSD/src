/* $NetBSD: reallocarr.c,v 1.2 2015/07/16 00:03:59 kamil Exp $ */

/*-
 * Copyright (c) 2015 Joerg Sonnenberger <joerg@NetBSD.org>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: reallocarr.c,v 1.2 2015/07/16 00:03:59 kamil Exp $");

#include "namespace.h"
#include <errno.h>
/* Old POSIX has SIZE_MAX in limits.h */
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

__CTASSERT(65535 < SIZE_MAX / 65535);

#ifdef _LIBC
#ifdef __weak_alias
__weak_alias(reallocarr, _reallocarr)
#endif
#endif

int
reallocarr(void *ptr, size_t num, size_t size)
{
	int saved_errno, result;
	void *optr;
	void *nptr;

	saved_errno = errno;
	memcpy(&optr, ptr, sizeof(ptr));
	if (num == 0 || size == 0) {
		free(optr);
		nptr = NULL;
		memcpy(ptr, &nptr, sizeof(ptr));
		errno = saved_errno;
		return 0;
	}
	if ((num >= 65535 || size >= 65535) && num > SIZE_MAX / size)
		return EOVERFLOW;
	nptr = realloc(optr, num * size);
	if (nptr == NULL) {
		result = errno;
	} else {
		result = 0;
		memcpy(ptr, &nptr, sizeof(ptr));
	}
	errno = saved_errno;
	return result;
}
