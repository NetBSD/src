/* $NetBSD: lfs.c,v 1.5 1999/11/13 21:17:56 thorpej Exp $ */

/*-
 * Copyright (c) 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * The Mach Operating System project at Carnegie-Mellon University.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *
 * Copyright (c) 1990, 1991 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Author: David Golub
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

/*
 * XXX NOTE: ufs.c (FFS) and lfs.c (LFS) should eventually use much common
 * XXX code.  until then, the two files should be easily diffable.
 */

/*
 *	Stand-alone file reading package.
 */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/mount.h>			/* XXX for MNAMELEN */
#include <ufs/ufs/dinode.h>
#include <ufs/ufs/dir.h>
#include <ufs/lfs/lfs.h>
#ifdef _STANDALONE
#include <lib/libkern/libkern.h>
#else
#include <string.h>
inline u_int
max(a, b)
        u_int a, b;
{
        return (a > b ? a : b);
}
#endif

#include "stand.h"
#include "lfs.h"

#if defined(LIBSA_FS_SINGLECOMPONENT) && !defined(LIBSA_NO_FS_SYMLINK)
#define LIBSA_NO_FS_SYMLINK
#endif


/*
 * In-core LFS superblock.  This exists only to placate the macros in lfs.h,
 * and to diff easily against the UFS code.
 */
struct fs {
	struct dlfs	lfs_dlfs;
};
#define	fs_magic	lfs_magic
#define	fs_bsize	lfs_bsize
#define fs_maxsymlinklen lfs_maxsymlinklen

#define	FS_MAGIC	LFS_MAGIC
#define	SBSIZE		LFS_SBPAD
#define	SBLOCK		(LFS_LABELPAD / DEV_BSIZE)

/*
 * In-core open file.
 */
struct file {
	off_t		f_seekp;	/* seek pointer */
	struct fs	*f_fs;		/* pointer to super-block */
	struct dinode	f_di;		/* copy of on-disk inode */
	unsigned int	f_nindir[NIADDR];
					/* number of blocks mapped by
					   indirect block at level i */
	char		*f_blk[NIADDR];	/* buffer for indirect block at
					   level i */
	size_t		f_blksize[NIADDR];
					/* size of buffer */
	daddr_t		f_blkno[NIADDR];/* disk address of block in buffer */
	char		*f_buf;		/* buffer for data block */
	size_t		f_buf_size;	/* size of data block */
	daddr_t		f_buf_blkno;	/* block number of data block */
};

static int	find_inode_sector(ino_t inumber, struct open_file *f,
		    ufs_daddr_t *ibp);
static int	read_inode __P((ino_t, struct open_file *));
static int	block_map __P((struct open_file *, daddr_t, daddr_t *));
static int	buf_read_file __P((struct open_file *, char **, size_t *));
static int	search_directory __P((char *, struct open_file *, ino_t *));

/*
 * Find an inode's block.  Look it up in the ifile.  Whee!
 */
static int
find_inode_sector(ino_t inumber, struct open_file *f, ufs_daddr_t *isp)
{
	register struct file *fp = (struct file *)f->f_fsdata;
	register struct fs *fs = fp->f_fs;
	ufs_daddr_t ifileent_blkno;
	char *ent_in_buf;
	size_t buf_after_ent;
	int rc;

	rc = read_inode(fs->lfs_ifile, f);
	if (rc)
		return (rc);

	ifileent_blkno =
	    (inumber / fs->lfs_ifpb) + fs->lfs_cleansz + fs->lfs_segtabsz;
	fp->f_seekp = (off_t)ifileent_blkno * fs->fs_bsize +
	    (inumber % fs->lfs_ifpb) * sizeof (IFILE);
	rc = buf_read_file(f, &ent_in_buf, &buf_after_ent);
	if (rc)
		return (rc);
	/* make sure something's not badly wrong, but don't panic. */
	if (buf_after_ent < sizeof (IFILE))
		return (EINVAL);

#if 0	/* LFS daddr_t's are in sectors, fsbtodb() shouldn't be used on them */
	*isp = fsbtodb(fs, ((IFILE *)ent_in_buf)->if_daddr);
#else
	*isp = ((IFILE *)ent_in_buf)->if_daddr;
#endif

	if (*isp == LFS_UNUSED_DADDR)	/* again, something badly wrong */
		return (EINVAL);
	return (0);
}

/*
 * Read a new inode into a file structure.
 */
static int
read_inode(inumber, f)
	ino_t inumber;
	struct open_file *f;
{
	register struct file *fp = (struct file *)f->f_fsdata;
	register struct fs *fs = fp->f_fs;
	struct dinode *dip;
	ufs_daddr_t inode_sector;
	size_t rsize;
	char *buf;
	int rc, cnt;

	if (inumber == fs->lfs_ifile)
		inode_sector = fs->lfs_idaddr;
	else if ((rc = find_inode_sector(inumber, f, &inode_sector)) != 0)
		return (rc);

	/*
	 * Read inode and save it.
	 */
	buf = alloc(fs->fs_bsize);
#if !defined(LIBSA_NO_TWIDDLE)
	twiddle();
#endif
	rc = DEV_STRATEGY(f->f_dev)(f->f_devdata, F_READ,
		inode_sector, fs->fs_bsize,
		buf, &rsize);
	if (rc)
		goto out;
	if (rsize != fs->fs_bsize) {
		rc = EIO;
		goto out;
	}

	rc = EINVAL;
	cnt = INOPB(fs);
        for (dip = (struct dinode *)buf + (cnt - 1); cnt--; --dip) {
                if (dip->di_inumber == inumber) {
                        rc = 0;
			break;
		}
	}
	/* kernel code panics, but boot blocks which panic are Bad. */
	if (rc)
		goto out;
	fp->f_di = *dip;

	/*
	 * Clear out the old buffers
	 */
	{
		register int level;

		for (level = 0; level < NIADDR; level++)
			fp->f_blkno[level] = -1;
		fp->f_buf_blkno = -1;
	}
out:
	free(buf, fs->fs_bsize);
	return (rc);
}

/*
 * Given an offset in a file, find the disk block number that
 * contains that block.
 */
static int
block_map(f, file_block, disk_block_p)
	struct open_file *f;
	daddr_t file_block;
	daddr_t *disk_block_p;	/* out */
{
	register struct file *fp = (struct file *)f->f_fsdata;
	register struct fs *fs = fp->f_fs;
	int level;
	int idx;
	daddr_t ind_block_num;
	daddr_t *ind_p;
	int rc;

	/*
	 * Index structure of an inode:
	 *
	 * di_db[0..NDADDR-1]	hold block numbers for blocks
	 *			0..NDADDR-1
	 *
	 * di_ib[0]		index block 0 is the single indirect block
	 *			holds block numbers for blocks
	 *			NDADDR .. NDADDR + NINDIR(fs)-1
	 *
	 * di_ib[1]		index block 1 is the double indirect block
	 *			holds block numbers for INDEX blocks for blocks
	 *			NDADDR + NINDIR(fs) ..
	 *			NDADDR + NINDIR(fs) + NINDIR(fs)**2 - 1
	 *
	 * di_ib[2]		index block 2 is the triple indirect block
	 *			holds block numbers for double-indirect
	 *			blocks for blocks
	 *			NDADDR + NINDIR(fs) + NINDIR(fs)**2 ..
	 *			NDADDR + NINDIR(fs) + NINDIR(fs)**2
	 *				+ NINDIR(fs)**3 - 1
	 */

	if (file_block < NDADDR) {
		/* Direct block. */
		*disk_block_p = fp->f_di.di_db[file_block];
		return (0);
	}

	file_block -= NDADDR;

	/*
	 * nindir[0] = NINDIR
	 * nindir[1] = NINDIR**2
	 * nindir[2] = NINDIR**3
	 *	etc
	 */
	for (level = 0; level < NIADDR; level++) {
		if (file_block < fp->f_nindir[level])
			break;
		file_block -= fp->f_nindir[level];
	}
	if (level == NIADDR) {
		/* Block number too high */
		return (EFBIG);
	}

	ind_block_num = fp->f_di.di_ib[level];

	for (; level >= 0; level--) {
		if (ind_block_num == 0) {
			*disk_block_p = 0;	/* missing */
			return (0);
		}

		if (fp->f_blkno[level] != ind_block_num) {
			if (fp->f_blk[level] == (char *)0)
				fp->f_blk[level] =
					alloc(fs->fs_bsize);
#if !defined(LIBSA_NO_TWIDDLE)
			twiddle();
#endif
			rc = DEV_STRATEGY(f->f_dev)(f->f_devdata, F_READ,
#if 0	/* LFS daddr_t's are in sectors, fsbtodb() shouldn't be used on them */
				fsbtodb(fp->f_fs, ind_block_num),
#else
				ind_block_num,
#endif
				fs->fs_bsize,
				fp->f_blk[level],
				&fp->f_blksize[level]);
			if (rc)
				return (rc);
			if (fp->f_blksize[level] != fs->fs_bsize)
				return (EIO);
			fp->f_blkno[level] = ind_block_num;
		}

		ind_p = (daddr_t *)fp->f_blk[level];

		if (level > 0) {
			idx = file_block / fp->f_nindir[level - 1];
			file_block %= fp->f_nindir[level - 1];
		} else
			idx = file_block;

		ind_block_num = ind_p[idx];
	}

	*disk_block_p = ind_block_num;

	return (0);
}

/*
 * Read a portion of a file into an internal buffer.  Return
 * the location in the buffer and the amount in the buffer.
 */
static int
buf_read_file(f, buf_p, size_p)
	struct open_file *f;
	char **buf_p;		/* out */
	size_t *size_p;		/* out */
{
	register struct file *fp = (struct file *)f->f_fsdata;
	register struct fs *fs = fp->f_fs;
	long off;
	register daddr_t file_block;
	daddr_t	disk_block;
	size_t block_size;
	int rc;

	off = blkoff(fs, fp->f_seekp);
	file_block = lblkno(fs, fp->f_seekp);
	block_size = dblksize(fs, &fp->f_di, file_block);

	if (file_block != fp->f_buf_blkno) {
		rc = block_map(f, file_block, &disk_block);
		if (rc)
			return (rc);

		if (fp->f_buf == (char *)0)
			fp->f_buf = alloc(fs->fs_bsize);

		if (disk_block == 0) {
			bzero(fp->f_buf, block_size);
			fp->f_buf_size = block_size;
		} else {
#if !defined(LIBSA_NO_TWIDDLE)
			twiddle();
#endif
			rc = DEV_STRATEGY(f->f_dev)(f->f_devdata, F_READ,
#if 0	/* LFS daddr_t's are in sectors, fsbtodb() shouldn't be used on them */
				fsbtodb(fs, disk_block),
#else
				disk_block,
#endif
				block_size, fp->f_buf, &fp->f_buf_size);
			if (rc)
				return (rc);
		}

		fp->f_buf_blkno = file_block;
	}

	/*
	 * Return address of byte in buffer corresponding to
	 * offset, and size of remainder of buffer after that
	 * byte.
	 */
	*buf_p = fp->f_buf + off;
	*size_p = block_size - off;

	/*
	 * But truncate buffer at end of file.
	 */
	if (*size_p > fp->f_di.di_size - fp->f_seekp)
		*size_p = fp->f_di.di_size - fp->f_seekp;

	return (0);
}

/*
 * Search a directory for a name and return its
 * i_number.
 */
static int
search_directory(name, f, inumber_p)
	char *name;
	struct open_file *f;
	ino_t *inumber_p;		/* out */
{
	register struct file *fp = (struct file *)f->f_fsdata;
	register struct direct *dp;
	struct direct *edp;
	char *buf;
	size_t buf_size;
	int namlen, length;
	int rc;

	length = strlen(name);

	fp->f_seekp = 0;
	while (fp->f_seekp < fp->f_di.di_size) {
		rc = buf_read_file(f, &buf, &buf_size);
		if (rc)
			return (rc);

		dp = (struct direct *)buf;
		edp = (struct direct *)(buf + buf_size);
		while (dp < edp) {
			if (dp->d_ino == (ino_t)0)
				goto next;
#if BYTE_ORDER == LITTLE_ENDIAN
			if (fp->f_fs->fs_maxsymlinklen <= 0)
				namlen = dp->d_type;
			else
#endif
				namlen = dp->d_namlen;
			if (namlen == length &&
			    !strcmp(name, dp->d_name)) {
				/* found entry */
				*inumber_p = dp->d_ino;
				return (0);
			}
		next:
			dp = (struct direct *)((char *)dp + dp->d_reclen);
		}
		fp->f_seekp += buf_size;
	}
	return (ENOENT);
}

/*
 * Open a file.
 */
int
lfs_open(path, f)
	char *path;
	struct open_file *f;
{
#ifndef LIBSA_FS_SINGLECOMPONENT
	register char *cp, *ncp;
	register int c;
#endif
	ino_t inumber;
	struct file *fp;
	struct fs *fs;
	int rc;
	size_t buf_size;
#ifndef LIBSA_NO_FS_SYMLINK
	ino_t parent_inumber;
	int nlinks = 0;
	char namebuf[MAXPATHLEN+1];
	char *buf = NULL;
#endif

	/* allocate file system specific data structure */
	fp = alloc(sizeof(struct file));
	bzero(fp, sizeof(struct file));
	f->f_fsdata = (void *)fp;

	/* allocate space and read super block */
	fs = alloc(SBSIZE);
	fp->f_fs = fs;
#if !defined(LIBSA_NO_TWIDDLE)
	twiddle();
#endif
	rc = DEV_STRATEGY(f->f_dev)(f->f_devdata, F_READ,
		SBLOCK, SBSIZE, (char *)fs, &buf_size);
	if (rc)
		goto out;

	if (buf_size != SBSIZE || fs->fs_magic != FS_MAGIC ||
	    fs->fs_bsize > MAXBSIZE || fs->fs_bsize < sizeof(struct fs)) {
		rc = EINVAL;
		goto out;
	}
	/*
	 * Calculate indirect block levels.
	 */
	{
		register int mult;
		register int level;

		mult = 1;
		for (level = 0; level < NIADDR; level++) {
			mult *= NINDIR(fs);
			fp->f_nindir[level] = mult;
		}
	}

	inumber = ROOTINO;
	if ((rc = read_inode(inumber, f)) != 0)
		goto out;

#ifndef LIBSA_FS_SINGLECOMPONENT
	cp = path;
	while (*cp) {

		/*
		 * Remove extra separators
		 */
		while (*cp == '/')
			cp++;
		if (*cp == '\0')
			break;

		/*
		 * Check that current node is a directory.
		 */
		if ((fp->f_di.di_mode & IFMT) != IFDIR) {
			rc = ENOTDIR;
			goto out;
		}

		/*
		 * Get next component of path name.
		 */
		{
			register int len = 0;

			ncp = cp;
			while ((c = *cp) != '\0' && c != '/') {
				if (++len > MAXNAMLEN) {
					rc = ENOENT;
					goto out;
				}
				cp++;
			}
			*cp = '\0';
		}

		/*
		 * Look up component in current directory.
		 * Save directory inumber in case we find a
		 * symbolic link.
		 */
#ifndef LIBSA_NO_FS_SYMLINK
		parent_inumber = inumber;
#endif
		rc = search_directory(ncp, f, &inumber);
		*cp = c;
		if (rc)
			goto out;

		/*
		 * Open next component.
		 */
		if ((rc = read_inode(inumber, f)) != 0)
			goto out;

#ifndef LIBSA_NO_FS_SYMLINK
		/*
		 * Check for symbolic link.
		 */
		if ((fp->f_di.di_mode & IFMT) == IFLNK) {
			int link_len = fp->f_di.di_size;
			int len;

			len = strlen(cp);

			if (link_len + len > MAXPATHLEN ||
			    ++nlinks > MAXSYMLINKS) {
				rc = ENOENT;
				goto out;
			}

			bcopy(cp, &namebuf[link_len], len + 1);

			if (link_len < fs->fs_maxsymlinklen) {
				bcopy(fp->f_di.di_shortlink, namebuf,
				      (unsigned) link_len);
			} else {
				/*
				 * Read file for symbolic link
				 */
				size_t buf_size;
				daddr_t	disk_block;
				register struct fs *fs = fp->f_fs;

				if (!buf)
					buf = alloc(fs->fs_bsize);
				rc = block_map(f, (daddr_t)0, &disk_block);
				if (rc)
					goto out;

#if !defined(LIBSA_NO_TWIDDLE)
				twiddle();
#endif
				rc = DEV_STRATEGY(f->f_dev)(f->f_devdata,
#if 0	/* LFS daddr_t's are in sectors, fsbtodb() shouldn't be used on them */
					F_READ, fsbtodb(fs, disk_block),
#else
					F_READ, disk_block,
#endif
					fs->fs_bsize, buf, &buf_size);
				if (rc)
					goto out;

				bcopy((char *)buf, namebuf, (unsigned)link_len);
			}

			/*
			 * If relative pathname, restart at parent directory.
			 * If absolute pathname, restart at root.
			 */
			cp = namebuf;
			if (*cp != '/')
				inumber = parent_inumber;
			else
				inumber = (ino_t)ROOTINO;

			if ((rc = read_inode(inumber, f)) != 0)
				goto out;
		}
#endif	/* !LIBSA_NO_FS_SYMLINK */
	}

	/*
	 * Found terminal component.
	 */
	rc = 0;

#else /* !LIBSA_FS_SINGLECOMPONENT */

	/* look up component in the current (root) directory */
	rc = search_directory(path, f, &inumber);
	if (rc)
		goto out;

	/* open it */
	rc = read_inode(inumber, f);

#endif /* !LIBSA_FS_SINGLECOMPONENT */

        fp->f_seekp = 0;		/* reset seek pointer */

out:
#ifndef LIBSA_NO_FS_SYMLINK
	if (buf)
		free(buf, fs->fs_bsize);
#endif
	if (rc) {
		if (fp->f_buf)
			free(fp->f_buf, fp->f_fs->fs_bsize);
		free(fp->f_fs, SBSIZE);
		free(fp, sizeof(struct file));
	}
	return (rc);
}

#ifndef LIBSA_NO_FS_CLOSE
int
lfs_close(f)
	struct open_file *f;
{
	register struct file *fp = (struct file *)f->f_fsdata;
	int level;

	f->f_fsdata = (void *)0;
	if (fp == (struct file *)0)
		return (0);

	for (level = 0; level < NIADDR; level++) {
		if (fp->f_blk[level])
			free(fp->f_blk[level], fp->f_fs->fs_bsize);
	}
	if (fp->f_buf)
		free(fp->f_buf, fp->f_fs->fs_bsize);
	free(fp->f_fs, SBSIZE);
	free(fp, sizeof(struct file));
	return (0);
}
#endif /* !LIBSA_NO_FS_CLOSE */

/*
 * Copy a portion of a file into kernel memory.
 * Cross block boundaries when necessary.
 */
int
lfs_read(f, start, size, resid)
	struct open_file *f;
	void *start;
	size_t size;
	size_t *resid;	/* out */
{
	register struct file *fp = (struct file *)f->f_fsdata;
	register size_t csize;
	char *buf;
	size_t buf_size;
	int rc = 0;
	register char *addr = start;

	while (size != 0) {
		if (fp->f_seekp >= fp->f_di.di_size)
			break;

		rc = buf_read_file(f, &buf, &buf_size);
		if (rc)
			break;

		csize = size;
		if (csize > buf_size)
			csize = buf_size;

		bcopy(buf, addr, csize);

		fp->f_seekp += csize;
		addr += csize;
		size -= csize;
	}
	if (resid)
		*resid = size;
	return (rc);
}

/*
 * Not implemented.
 */
#ifndef LIBSA_NO_FS_WRITE
int
lfs_write(f, start, size, resid)
	struct open_file *f;
	void *start;
	size_t size;
	size_t *resid;	/* out */
{

	return (EROFS);
}
#endif /* !LIBSA_NO_FS_WRITE */

#ifndef LIBSA_NO_FS_SEEK
off_t
lfs_seek(f, offset, where)
	struct open_file *f;
	off_t offset;
	int where;
{
	register struct file *fp = (struct file *)f->f_fsdata;

	switch (where) {
	case SEEK_SET:
		fp->f_seekp = offset;
		break;
	case SEEK_CUR:
		fp->f_seekp += offset;
		break;
	case SEEK_END:
		fp->f_seekp = fp->f_di.di_size - offset;
		break;
	default:
		return (-1);
	}
	return (fp->f_seekp);
}
#endif /* !LIBSA_NO_FS_SEEK */

int
lfs_stat(f, sb)
	struct open_file *f;
	struct stat *sb;
{
	register struct file *fp = (struct file *)f->f_fsdata;

	/* only important stuff */
	sb->st_mode = fp->f_di.di_mode;
	sb->st_uid = fp->f_di.di_uid;
	sb->st_gid = fp->f_di.di_gid;
	sb->st_size = fp->f_di.di_size;
	return (0);
}
