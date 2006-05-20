/*	$NetBSD: rawfs.c,v 1.9 2006/05/20 20:38:39 mrg Exp $	*/

/*
 * Copyright (c) 1995 Gordon W. Ross
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
 * 4. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Gordon W. Ross
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

/*
 * Raw file system - for stream devices like tapes.
 * No random access, only sequential read allowed.
 * This exists only to allow upper level code to be
 * shielded from the fact that the device must be
 * read only with whole block position and size.
 */

#include <sys/param.h>
#include <lib/libsa/stand.h>
#include <rawfs.h>

extern int debug;

#define	RAWFS_BSIZE	8192

/*
 * In-core open file.
 */
struct file {
	daddr_t		fs_nextblk;	/* block number to read next */
	daddr_t		fs_curblk;	/* block number currently in buffer */
	int		fs_len;		/* amount left in f_buf */
	char *		fs_ptr;		/* read pointer into f_buf */
	char		fs_buf[RAWFS_BSIZE];
};

static int
rawfs_get_block __P((struct open_file *));

int	rawfs_open(path, f)
	const char *path;
	struct open_file *f;
{
	struct file *fs;

	/*
	 * The actual PROM driver has already been opened.
	 * Just allocate the I/O buffer, etc.
	 */
	fs = alloc(sizeof(struct file));
	fs->fs_nextblk = 0;
	fs->fs_curblk = -1;
	fs->fs_len = 0;
	fs->fs_ptr = fs->fs_buf;

	f->f_fsdata = fs;
	return (0);
}

int	rawfs_close(f)
	struct open_file *f;
{
	struct file *fs;

	fs = (struct file *) f->f_fsdata;
	f->f_fsdata = (void *)0;

	if (fs != (struct file *)0)
		dealloc(fs, sizeof(*fs));

	return (0);
}

int	rawfs_read(f, start, size, resid)
	struct open_file *f;
	void *start;
	u_int size;
	u_int *resid;
{
	struct file *fs = (struct file *)f->f_fsdata;
	char *addr = start;
	int error = 0;
	size_t csize;

	while (size != 0) {

		if (fs->fs_len == 0)
			if ((error = rawfs_get_block(f)) != 0)
				break;

		if (fs->fs_len <= 0)
			break;	/* EOF */

		csize = size;
		if (csize > fs->fs_len)
			csize = fs->fs_len;

		memcpy(addr, fs->fs_ptr, csize);
		fs->fs_ptr += csize;
		fs->fs_len -= csize;
		addr += csize;
		size -= csize;
	}
	if (resid)
		*resid = size;

	if (error) {
		errno = error;
		error = -1;
	}

	return (error);
}

int	rawfs_write(f, start, size, resid)
	struct open_file *f;
	void *start;
	size_t size;
	size_t *resid;	/* out */
{
	errno = EROFS;
	return (-1);
}

off_t	rawfs_seek(f, offset, where)
	struct open_file *f;
	off_t offset;
	int where;
{
	struct file *fs = (struct file *)f->f_fsdata;
	daddr_t curblk, targblk;
	off_t newoff;
	int err, idx;

	/*
	 * We support a very minimal feature set for lseek(2); just
	 * enough to allow loadfile() to work with the parameters
	 * we pass to it on boot.
	 *
	 * In all cases, we can't seek back past the start of the
	 * current block.
	 */
	curblk = (fs->fs_curblk < 0) ? 0 : fs->fs_curblk;

	/*
	 * Only support SEEK_SET and SEEK_CUR which result in offsets
	 * which don't require seeking backwards.
	 */
	switch (where) {
	case SEEK_SET:
		newoff = offset;
		break;

	case SEEK_CUR:
		if (fs->fs_curblk < 0)
			newoff = 0;
		else {
			newoff = fs->fs_curblk * RAWFS_BSIZE;
			newoff += RAWFS_BSIZE - fs->fs_len;
		}
		newoff += offset;
		break;

	default:
		errno = EINVAL;
		return (-1);
	}

	if (newoff < (curblk * RAWFS_BSIZE)) {
		errno = EINVAL;
		return (-1);
	}

	targblk = newoff / RAWFS_BSIZE;

	/*
	 * If necessary, skip blocks until we hit the required target
	 */
	err = 0;
	while (fs->fs_curblk != targblk && (err = rawfs_get_block(f)) == 0)
		;

	if (err) {
		errno = err;
		return (-1);
	}

	/*
	 * Update the index within the loaded block
	 */
	idx = newoff % RAWFS_BSIZE;
	fs->fs_len = RAWFS_BSIZE - idx;
	fs->fs_ptr = &fs->fs_buf[idx];

	return (newoff);
}

int	rawfs_stat(f, sb)
	struct open_file *f;
	struct stat *sb;
{
	errno = EFTYPE;
	return (-1);
}


/*
 * Read a block from the underlying stream device
 * (In our case, a tape drive.)
 */
static int
rawfs_get_block(f)
	struct open_file *f;
{
	struct file *fs;
	int error;
	size_t len;

	fs = (struct file *)f->f_fsdata;
	fs->fs_ptr = fs->fs_buf;

	twiddle();
	error = f->f_dev->dv_strategy(f->f_devdata, F_READ,
		fs->fs_nextblk * (RAWFS_BSIZE / DEV_BSIZE),
		RAWFS_BSIZE, fs->fs_buf, &len);

	if (!error) {
		fs->fs_len = len;
		fs->fs_curblk = fs->fs_nextblk;
		fs->fs_nextblk += 1;
	} else {
		errno = error;
		error = -1;
	}

	return (error);
}
