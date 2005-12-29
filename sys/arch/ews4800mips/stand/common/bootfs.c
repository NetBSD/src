/*	$NetBSD: bootfs.c,v 1.1 2005/12/29 15:20:09 tsutsui Exp $	*/

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

#include <machine/param.h>
#include <machine/bfs.h>
#include <machine/sector.h>

#include "local.h"

/* System V bfs */

FS_DEF(bfs);

struct fs_ops bfs_ops = {
	bfs_open, bfs_close, bfs_read, bfs_write, bfs_seek, bfs_stat
};

struct bfs_file {
	struct bfs *bfs;
	int start, end;
	size_t size;
	int cur;
	uint8_t buf[DEV_BSIZE];
};

int
bfs_open(const char *name, struct open_file *f)
{
	struct bfs_file *file;

	if ((file = alloc(sizeof *file)) == 0)
		return -1;
	memset(file, 0, sizeof *file);

	if (bfs_init(&file->bfs) != 0) {
		free(file, sizeof *file);
		return -2;
	}

	if (!bfs_file_lookup(file->bfs, name, &file->start, &file->end,
	    &file->size)) {
		bfs_fini(file->bfs);
		free(file, sizeof *file);
		return -3;
	}

	printf("%s: %s %d %d %d\n", __FUNCTION__, name, file->start,
	    file->end, file->size);

	f->f_fsdata = file;

	return 0;
}

int
bfs_close(struct open_file *f)
{
	struct bfs_file *file = f->f_fsdata;

	bfs_fini(file->bfs);

	return 0;
}

int
bfs_read(struct open_file *f, void *buf, size_t size, size_t *resid)
{
	struct bfs_file *file = f->f_fsdata;
	int n, start, end;
	uint8_t *p = buf;
	size_t bufsz = size;
	int cur = file->cur;

	if (cur + size > file->size)
		size = file->size - cur;

	start = file->start + (cur >> DEV_BSHIFT);
	end = file->start + ((cur + size) >> DEV_BSHIFT);

	/* first sector */
	if (!sector_read(0, file->buf, start))
		return -2;
	n = TRUNC_SECTOR(cur) + DEV_BSIZE - cur;
	if (n >= size) {
		memcpy(p, file->buf + DEV_BSIZE - n, size);
		goto done;
	}
	memcpy(p, file->buf + DEV_BSIZE - n, n);
	p += n;

	if ((end - start - 1) > 0) {
		if (!sector_read_n(0, p, start + 1, end - start - 1))
			return -2;
		p += (end - start - 1) * DEV_BSIZE;
	}

	/* last sector */
	if (!sector_read(0, file->buf, end))
		return -2;
	n = cur + size - TRUNC_SECTOR(cur + size);
	memcpy(p, file->buf, n);

 done:
	file->cur += size;

	if (resid)
		*resid = bufsz - size;

	return 0;
}

int
bfs_write(struct open_file *f, void *start, size_t size, size_t *resid)
{

	return -1;
}

off_t
bfs_seek(struct open_file *f, off_t offset, int where)
{
	struct bfs_file *file = f->f_fsdata;
	int cur;

	switch (where) {
	case SEEK_SET:
		cur = offset;
		break;
	case SEEK_CUR:
		cur = file->cur + offset;
		break;
	case SEEK_END:
		cur = file->size + offset;
		break;
	default:
		return EINVAL;
	}

	if (cur  < 0 || cur >= file->size) {
		return EINVAL;
	}

	file->cur = cur;

	return (off_t)cur;
}

int
bfs_stat(struct open_file *f, struct stat *stat)
{
	struct bfs_file *file = f->f_fsdata;

	stat->st_size = file->size;

	return 0;
}
