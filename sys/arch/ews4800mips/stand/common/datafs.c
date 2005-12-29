/*	$NetBSD: datafs.c,v 1.1 2005/12/29 15:20:09 tsutsui Exp $	*/

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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

#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>

#include "local.h"

FS_DEF(data);

struct fs_ops datafs_ops = {
	data_open, data_close, data_read, data_write, data_seek, data_stat
};

struct datafs {
	uint8_t *ptr;
	uint8_t *end;
	uint8_t *start;
	size_t size;
} __data;

void
data_attach(void *addr, size_t size)
{

	__data.start = addr;
	__data.size = size;
}

int
data_open(const char *name, struct open_file *f)
{

	if (__data.size == 0) {
		printf("no data\n");
		return -1;
	}

	__data.ptr = __data.start;
	__data.end = __data.ptr + __data.size;

	return 0;
}

int
data_close(struct open_file *f)
{

	return 0;
}

int
data_read(struct open_file *f, void *buf, size_t size, size_t *resid)
{
	uint8_t *b = buf;
	size_t sz;

	twiddle();
	if (__data.ptr + size < __data.end) {
		sz = size;
		memcpy(b, __data.ptr, sz);
		__data.ptr += sz;
	} else {
		sz = (size_t)(__data.end - __data.ptr);
		while (__data.ptr < __data.end)
			*b++ = *__data.ptr++;
	}

	if (resid)
		*resid = size - sz;

	return 0;
}

int
data_write(struct open_file *f, void *start, size_t size, size_t *resid)
{

	return 0;
}

off_t
data_seek(struct open_file *f, off_t offset, int where)
{
	uint8_t *p = 0;

	switch (where) {
	case SEEK_SET:
		p = __data.start + offset;
		break;
	case SEEK_CUR:
		p = __data.ptr + offset;
		break;
	case SEEK_END:
		p = __data.end + offset;
		break;
	default:
		return EINVAL;
	}

	if (p < __data.start || p >= __data.end) {
		return EINVAL;
	}

	__data.ptr = p;

	return (off_t)(p - __data.start);
}

int
data_stat(struct open_file *f, struct stat *stat)
{

	stat->st_size = __data.size;

	return 0;
}
