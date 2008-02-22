/*	$NetBSD: strpbrk.c,v 1.15 2008/02/22 19:25:59 joerg Exp $	*/

/*-
 * Copyright (c) 2008 Joerg Sonnenberger
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: strpbrk.c,v 1.15 2008/02/22 19:25:59 joerg Exp $");

#include <assert.h>
#include <inttypes.h>
#include <limits.h>
#include <string.h>

char *
strpbrk(const char *s, const char *charset)
{
	static const size_t idx[8] = { 1, 2, 4, 8, 16, 32, 64, 128 };
	uint8_t set[32];

	_DIAGASSERT(s != NULL);
	_DIAGASSERT(charset != NULL);

	memset(set, 0, sizeof(set));

	for (; *charset != '\0'; ++charset)
		set[(unsigned char)*charset >> 3] |=
		    idx[(unsigned char)*charset & 7];

	for (; *s != '\0'; ++s) {
		if (set[(unsigned char)*s >> 3] &
		    idx[(unsigned char)*s & 7])
			return __UNCONST(s);
	}
	return NULL;
}
