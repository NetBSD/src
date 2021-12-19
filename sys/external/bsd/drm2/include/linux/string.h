/*	$NetBSD: string.h,v 1.9 2021/12/19 10:48:00 riastradh Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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

#ifndef _LINUX_STRING_H_
#define _LINUX_STRING_H_

#include <sys/types.h>
#include <sys/cdefs.h>
#include <sys/errno.h>
#include <sys/null.h>

#include <linux/slab.h>

static inline void *
memchr_inv(const void *buffer, int c, size_t len)
{
	const uint8_t byte = c;	/* XXX lose */
	const char *p;

	for (p = buffer; len-- > 0; p++)
		if (*p != byte)
			return __UNCONST(p);

	return NULL;
}

static inline void *
kmemdup(const void *src, size_t len, gfp_t gfp)
{
	void *dst;

	dst = kmalloc(len, gfp);
	if (dst == NULL)
		return NULL;

	(void)memcpy(dst, src, len);
	return dst;
}

static inline char *
kstrndup(const char *src, size_t maxlen, gfp_t gfp)
{
	char *dst;
	size_t len;

	if (src == NULL)
		return NULL;

	len = strnlen(src, maxlen);
	dst = kmalloc(len + 1, gfp);
	if (dst == NULL)
		return NULL;

	(void)memcpy(dst, src, len);
	dst[len] = '\0';

	return dst;
}

static inline char *
kstrdup(const char *src, gfp_t gfp)
{

	if (src == NULL)
		return NULL;
	return kstrndup(src, strlen(src), gfp);
}

static inline ssize_t
strscpy(char *dst, const char *src, size_t dstsize)
{
	size_t n = dstsize;

	/* If no space for a NUL terminator, fail.  */
	if (n == 0)
		return -E2BIG;

	/* Copy until we get a NUL terminator or the end of the buffer.  */
	while ((*dst++ = *src++) != '\0') {
		if (__predict_false(--n == 0)) {
			dst[-1] = '\0'; /* NUL-terminate */
			return -E2BIG;
		}
	}

	/* Return the number of bytes copied, excluding NUL.  */
	return dstsize - n;
}

static inline void
memset64(uint64_t *p, uint64_t v, size_t n)
{

	while (n --> 0)
		*p++ = v;
}

static inline void
memset_p(void **p, void *v, size_t n)
{

	while (n --> 0)
		*p++ = v;
}

#define str_has_prefix(str, prefix) strncmp(str, prefix, strlen(prefix))

static inline int
match_string(const char *const *haystack, size_t n, const char *needle)
{
	int i;

	for (i = 0; i < n; i++) {
		if (haystack[i] == NULL)
			break;
		if (strcmp(haystack[i], needle) == 0)
			return i;
		if (i == INT_MAX)
			break;
	}
	return -EINVAL;
}

#endif  /* _LINUX_STRING_H_ */
