/*	$NetBSD: riscosfile.c,v 1.2.4.2 2002/05/28 14:38:56 bjh21 Exp $	*/

/*-
 * Copyright (c) 2001 Ben Harris
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#include <lib/libsa/stand.h>
#include <riscoscalls.h>
#include <riscosfile.h>
#include <sys/stat.h>

struct riscosfile {
	int file;
};

int
riscos_open(char *path, struct open_file *f)
{
	struct riscosfile *rf;
	os_error *error;
	int flags;

	rf = (struct riscosfile *) alloc(sizeof(*rf));
	if (!rf)
		return -1;

	switch (f->f_flags & (F_READ | F_WRITE)) {
	case F_READ:
		flags = OSFind_Openin;
		break;
	case F_WRITE:
		flags = OSFind_Openout;
		break;
	case F_READ | F_WRITE:
		flags = OSFind_Openup;
		break;
	default:
		/* Erm... */
		return EINVAL;
	}

	error = xosfind_open(flags | osfind_ERROR_IF_DIR |
	    osfind_ERROR_IF_ABSENT, path, NULL, &rf->file);
	if (error) {
		free(rf, sizeof(*rf));
		return riscos_errno(error);
	}
	f->f_fsdata = rf;
	return 0;
}

#ifndef LIBSA_NO_FS_CLOSE
int
riscos_close(struct open_file *f)
{
	struct riscosfile *rf;
	struct os_error *error;
	int err = 0;

	rf = f->f_fsdata;

	error = xosfind_close(rf->file);
	if (error)
		err = riscos_errno(error);
	free(rf, sizeof(*rf));
	return err;
}
#endif

int
riscos_read(struct open_file *f, void *buf, size_t size, size_t *residp)
{
	struct riscosfile *rf;
	int resid;
	os_error *error;

	rf = f->f_fsdata;

#ifndef LIBSA_NO_TWIDDLE
	twiddle();
#endif
	error = xosgbpb_read(rf->file, buf, size, &resid);
	*residp = resid;
	if (error)
		return riscos_errno(error);
	return 0;
}

#ifndef LIBSA_NO_FS_WRITE
int
riscos_write(struct open_file *f, void *buf, size_t size, size_t *residp)
{
	struct riscosfile *rf;
	int resid;
	os_error *error;

	rf = f->f_fsdata;

#ifndef LIBSA_NO_TWIDDLE
	twiddle();
#endif
	error = xosgbpb_write(rf->file, buf, size, &resid);
	*residp = resid;
	if (error)
		return riscos_errno(error);
	return 0;
}
#endif

int
riscos_stat(struct open_file *f, struct stat *sb)
{
	struct riscosfile *rf;
	os_error *error;
	int extent;

	rf = f->f_fsdata;

	error = xosargs_read_ext(rf->file, &extent);
	if (error)
		return riscos_errno(error);

	sb->st_mode = S_IFREG | 0444;
	sb->st_nlink = 1;
	sb->st_uid = 0;
	sb->st_gid = 0;
	sb->st_size = extent;
	return 0;
}

#ifndef LIBSA_NO_FS_SEEK
off_t
riscos_seek(struct open_file *f, off_t offset, int where)
{
	struct riscosfile *rf;
	int base, result;
	os_error *error;

	rf = f->f_fsdata;

	switch (where) {
	case SEEK_SET:
		base = 0;
		break;
	case SEEK_CUR:
		error = xosargs_read_ptr(rf->file, &base);
		if (error)
			goto err;
		break;
	case SEEK_END:
		error = xosargs_read_ext(rf->file, &base);
		if (error)
			goto err;
		break;
	default:
		errno = EOFFSET;
		return -1;
	}
	offset = base + offset;
	error = xosargs_set_ptr(rf->file, offset);
	if (error)
		goto err;
	error = xosargs_read_ptr(rf->file, &result);
	if (error)
		goto err;
	return result;

err:
	errno = riscos_errno(error);
	return -1;
}
#endif
