/*	$NetBSD: pathfs.c,v 1.1 2012/01/18 23:12:21 nonaka Exp $	*/

/*-
 * Copyright (C) 2012 NONAKA Kimihiro <nonaka@netbsd.org>
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

#include "boot.h"
#include "pathfs.h"
#include "unixdev.h"
#include "compat_linux.h"

__compactcall int
pathfs_open(const char *path, struct open_file *fd)
{

	if (strcmp(fd->f_dev->dv_name, "path"))
		return EINVAL;

	(void) ulseek((int)fd->f_devdata, 0L, SEEK_SET);
	return 0;
}

__compactcall int
pathfs_read(struct open_file *fd, void *vbuf, size_t nbyte, size_t *resid)
{
	char *buf = vbuf;
	size_t off = 0;
	ssize_t rsz;

	while (off < nbyte) {
		rsz = uread((int)fd->f_devdata, buf + off, nbyte - off);
		if (rsz < 0)
			return errno;
		if (rsz == 0)
			break;
		off += rsz;
	}

	*resid -= off;
	return 0;
}

__compactcall int
pathfs_write(struct open_file *fd, void *vbuf, size_t size, size_t *resid)
{

	return EROFS;
}

__compactcall off_t
pathfs_seek(struct open_file *fd, off_t offset, int whence)
{

	return ulseek((int)fd->f_devdata, offset, whence);
}

__compactcall int
pathfs_close(struct open_file *fd)
{

	return 0;
}

__compactcall int
pathfs_stat(struct open_file *fd, struct stat *sb)
{
	struct linux_stat lsb;
	int rv;

	rv = ufstat((int)fd->f_devdata, &lsb);
	if (rv < 0)
		return errno;

	sb->st_ino = lsb.lst_ino;
	sb->st_mode = lsb.lst_mode;
	sb->st_nlink = lsb.lst_nlink;
	sb->st_uid = lsb.lst_uid;
	sb->st_gid = lsb.lst_gid;
	sb->st_size = lsb.lst_size;
	sb->st_blksize = lsb.lst_blksize;
	sb->st_blocks = lsb.lst_blocks;
	sb->st_atime = lsb.lst_atime;
	sb->st_mtime = lsb.lst_mtime;
	sb->st_ctime = lsb.lst_ctime;
	return 0;
}

__compactcall void
pathfs_ls(struct open_file *f, const char *pattern)
{

	printf("Currently ls command is unsupported by pathfs.\n");
}
