/*	$NetBSD: pread.c,v 1.7.70.1 2019/09/17 19:32:00 martin Exp $	 */

/*
 * Copyright (c) 1996
 *	Matthias Drochner.  All rights reserved.
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
 *
 */

/* read into destination in flat addr space */

#include <lib/libsa/stand.h>

#include "libi386.h"

#ifdef SAVE_MEMORY
#define BUFSIZE (1*1024)
#else
#define BUFSIZE (4*1024)
#endif

static char     *buf;

ssize_t
pread(int fd, void *dest, size_t size)
{
	size_t             rsize;

	if (!buf)
		buf = alloc(BUFSIZE);

	rsize = size;
	while (rsize > 0) {
		size_t count;
		ssize_t got;

		count = (rsize < BUFSIZE ? rsize : BUFSIZE);

		got = read(fd, buf, count);
		if (got < 0)
			return -1;

		/* put to physical space */
		vpbcopy(buf, dest, got);

		dest += got;
		rsize -= got;
		if (got < count)
			break;	/* EOF */
	}
	return size - rsize;
}
