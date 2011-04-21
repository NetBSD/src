/* $NetBSD: memfs.c,v 1.1.2.2 2011/04/21 01:41:22 rmind Exp $ */

/*-
 * Copyright (c) 2011 Frank Wille.
 * All rights reserved.
 *
 * Written by Frank Wille for The NetBSD Project.
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

#include <sys/param.h>

#include <lib/libsa/stand.h>

#include "globals.h"
#include "memfs.h"

struct memhandle {
	char *base;
	off_t off;
};

int
mem_open(const char *path, struct open_file *f)
{
	struct memhandle *mh;

	mh = alloc(sizeof(struct memhandle));
	if (mh == NULL)
		return ENOMEM;
	mh->base = (char *)read_hex(path);
	mh->off = 0;
	f->f_fsdata = mh;
	return 0;
}

#ifndef LIBSA_NO_FS_CLOSE
int
mem_close(struct open_file *f)
{

	dealloc(f->f_fsdata, sizeof(struct memhandle));
	return 0;
}
#endif

int
mem_read(struct open_file *f, void *buf, size_t size, size_t *resid)
{
	struct memhandle *mh;

	mh = f->f_fsdata;
	memcpy(buf, mh->base + mh->off, size);
	mh->off += size;
	if (resid)
		*resid = 0;
	return 0;
}

#ifndef LIBSA_NO_FS_WRITE
int
mem_write(struct open_file *f, void *buf, size_t size, size_t *resid)
{
	struct memhandle *mh;

	mh = f->f_fsdata;
	memcpy(mh->base + mh->off, buf, size);
	mh->off += size;
	if (resid)
		*resid = 0;
	return 0;
}
#endif

#ifndef LIBSA_NO_FS_SEEK
off_t
mem_seek(struct open_file *f, off_t offset, int where)
{
	struct memhandle *mh;

	mh = f->f_fsdata;
	switch (where) {
	case SEEK_SET:
		mh->off = offset;
		break;
	case SEEK_CUR:
		mh->off += offset;
		break;
	default:
		errno = EOFFSET;
		return -1;
	}
	return mh->off;
}
#endif

int
mem_stat(struct open_file *f, struct stat *sb)
{

	return EIO;
}
