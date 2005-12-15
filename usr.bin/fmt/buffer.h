/*	$NetBSD: buffer.h,v 1.1 2005/12/15 21:32:00 christos Exp $	*/

/*-
 * Copyright (c) 2005 The NetBSD Foundation, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <err.h>

struct buffer {
	char *ptr;
	char *bptr;
	char *eptr;
};

static void
buf_init(struct buffer *buf)
{
	buf->ptr = buf->bptr = malloc(BUFSIZ);
	if (buf->ptr == NULL)
		err(1, "Cannot allocate buffer");
	buf->eptr = buf->ptr + BUFSIZ;
}

static void
buf_end(struct buffer *buf)
{
	free(buf->bptr);
}

static void
buf_grow(struct buffer *buf, size_t minsize)
{
	ptrdiff_t diff;
	size_t len = (buf->eptr - buf->bptr) + 
	    minsize > BUFSIZ ? minsize : BUFSIZ;
	char *nptr = realloc(buf->bptr, len);

	if (nptr == NULL)
		err(1, "Cannot grow buffer");

	if (nptr == buf->bptr)
		return;

	diff = nptr - buf->bptr;
	buf->bptr += diff;
	buf->eptr = buf->bptr + len;
	buf->ptr += diff;
}

static __inline void
buf_putc(struct buffer *buf, char c)
{
	if (buf->ptr >= buf->eptr)
		buf_grow(buf, 1);
	*buf->ptr++ = c;
}

static __inline void
buf_reset(struct buffer *buf)
{
	buf->ptr = buf->bptr;
}

static __inline char 
buf_unputc(struct buffer *buf)
{
	return buf->ptr > buf->bptr ? *--buf->ptr : '\0';
}
