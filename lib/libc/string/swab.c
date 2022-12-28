/*	$NetBSD: swab.c,v 1.20 2022/12/28 15:34:19 riastradh Exp $	*/


/*-
 * Copyright (c) 2022 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: swab.c,v 1.20 2022/12/28 15:34:19 riastradh Exp $");
#endif

#include <assert.h>
#include <unistd.h>

void
swab(const void *__restrict src, void *__restrict dst, ssize_t nbytes)
{
	const char *p = src;
	char *q = dst;
	size_t n;

	_DIAGASSERT(src != NULL);
	_DIAGASSERT(dst != NULL);

	if (nbytes <= 1)
		return;

	/*
	 * Round nbytes/2 down; we ignore the last byte altogether if
	 * nbytes is odd.  This way, if the destination array is
	 * uninitialized, sanitizers will recognize the last byte as
	 * still uninitialized and flag it as undefined.
	 */
	for (n = nbytes/2; n --> 0; p += 2, q += 2) {
		/*
		 * According to POSIX (2018), the behaviour is
		 * undefined if src and dst overlap.  However, there
		 * are uses in-tree (xsrc/external/mit/xfwp/dist/io.c)
		 * that rely on swab(ptr, ptr, n) to do the swabbing
		 * in-place.  So make sure this works if src == dst.
		 */
		const char p0 = p[0], p1 = p[1];

		q[0] = p1;
		q[1] = p0;
	}
}
