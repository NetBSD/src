/* $NetBSD: tfs.c,v 1.1 1998/09/22 00:35:15 ross Exp $ */

/* [Notice revision 2.2]
 * Copyright (c) 1997, 1998 Avalon Computer Systems, Inc.
 * All rights reserved.
 *
 * Author: Ross Harvey
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright and
 *    author notice, this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of Avalon Computer Systems, Inc. nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 * 4. This copyright will be assigned to The NetBSD Foundation on
 *    1/1/2000 unless these terms (including possibly the assignment
 *    date) are updated in writing by Avalon prior to the latest specified
 *    assignment date.
 *
 * THIS SOFTWARE IS PROVIDED BY AVALON COMPUTER SYSTEMS, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL AVALON OR THE CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */


/*
 ******************************* USTAR FS *******************************
 */

/*
 * Implement an ROFS with an 8K boot area followed by ustar-format data.
 * The point: minimal FS overhead, and it's easy (well, `possible') to
 * split files over multiple volumes.
 *
 * XXX - TODO LIST
 * --- - ---- ----
 * XXX - tag volume numbers and verify that the correct volume is
 *       inserted after volume swaps.
 *
 * XXX - stop hardwiring FS metadata for floppies...embed it in a file,
 * 	 file name, or something. (Remember __SYMDEF? :-)
 *
 */

#include <lib/libkern/libkern.h>
#include "stand.h"
#include "tfs.h"

#define	USTAR_NAME_BLOCK 512

typedef struct ustar_struct {
	char	ust_name[100],
		ust_mode[8],
		ust_uid[8],
		ust_gid[8],
		ust_size[12],
		ust_misc[12 + 8 + 1 + 100],
		ust_magic[6];
	/* there is more, but we don't care */
} ustar_t;

/*
 * We buffer one even cylindar of data...it's actually only really one
 * cyl on a 1.44M floppy, but on other devices it's fast enough with any
 * kind of block buffering, so we optimize for the slowest device.
 */

typedef struct ust_active_struct {
	ustar_t	uas_active;
	char	uas_1cyl[18 * 2 * 512];
	off_t	uas_volsize;		/* XXX this is hardwired now */
	off_t	uas_windowbase;		/* relative to volume 0 */
	off_t	uas_filestart;		/* relative to volume 0 */
	off_t	uas_fseek;		/* relative to file */
	int	uas_init_window;	/* data present in window */
	int	uas_init_fs;		/* ust FS actually found */
	off_t	uas_filesize;		/* relative to volume 0 */
} ust_active_t;

#define	BBSIZE	8192

static int tfs_mode_offset = BBSIZE;

static int tfs_cylinder_read __P((struct open_file *, off_t));
static int read512block __P((struct open_file *, off_t, char block[512]));
static void tfs_sscanf __P((const char *, const char *, int *));
static int convert __P((const char *, int, int));

static int
convert(f, base, fw)
	const char *f;
	int base, fw;
{
	int	i, c, result = 0;

	while(fw > 0 && *f == ' ') {
		--fw;
		++f;
	}
	for(i = 0; i < fw; ++i) {
		c = f[i];
		if ('0' <= c && c < '0' + base) {
			c -= '0';
			result = result * base + c;
		} else	break;
	}
	return result;
}

static void
tfs_sscanf(s,f,xi)
	const char *s,*f;
	int *xi;
{
	*xi = convert(s, 8, convert(f + 1, 10, 99));
}

static int
tfs_cylinder_read(f, seek2)
	struct open_file *f;
	off_t seek2;
{
	int e;
	size_t	xfercount;
	ust_active_t *ustf;

	ustf = f->f_fsdata;
	e = f->f_dev->dv_strategy(f->f_devdata, F_READ, seek2/512,
		sizeof ustf->uas_1cyl, ustf->uas_1cyl, &xfercount);
	if (e == 0 && xfercount != sizeof ustf->uas_1cyl)
		printf("Warning, unexpected short transfer %d/%d\n",
			(int)xfercount, (int) sizeof ustf->uas_1cyl);
	return e;
}

static int
read512block(f, offset, block)
	struct open_file *f;
	off_t offset;
	char block[512];
{
	ssize_t	e;
	int	needvolume, havevolume, dienow = 0;
	off_t	lastbase, seek2;
	ust_active_t *ustf;

	ustf = f->f_fsdata;
tryagain:
	if(ustf->uas_init_window
	&& ustf->uas_windowbase <= offset
	&& offset - ustf->uas_windowbase < sizeof ustf->uas_1cyl) {
		memcpy(block, ustf->uas_1cyl + offset - ustf->uas_windowbase,
			512);
		return 0;
	}
	if (dienow++)
		panic("tfs read512block");
	lastbase = ustf->uas_windowbase;
	seek2 = ustf->uas_windowbase = offset - offset % sizeof ustf->uas_1cyl;
	if(ustf->uas_volsize) {
		havevolume = lastbase / ustf->uas_volsize;
		needvolume = ustf->uas_windowbase / ustf->uas_volsize;
		if (havevolume != needvolume) {
			if (needvolume != havevolume + 1)
				printf("Caution: the disk required is not the"
				" next disk in ascending sequence.\n");
			printf("Please insert disk number %d"
				" and type return...", needvolume + 1);
			getchar();
		}
		seek2 %= ustf->uas_volsize;
	}
	e = tfs_cylinder_read(f, seek2);
	if (e)
		return e;
	ustf->uas_init_window = 1;
	goto tryagain;
}

int
tfs_open(path, f)
	char *path;
	struct open_file *f;

{
	ust_active_t *ustf;
	off_t offset;
	char	block[512];
	int	filesize;
	int	e, e2;

	if (*path == '/')
		++path;
	e = EINVAL;
	f->f_fsdata = ustf = alloc(sizeof *ustf);
	memset(ustf, 0, sizeof *ustf);
	offset = tfs_mode_offset;
	ustf->uas_fseek = 0;
	for(;;) {
		ustf->uas_filestart = offset;
		e2 = read512block(f, offset, block);
		if (e2) {
			e = e2;
			break;
		}
		memcpy(&ustf->uas_active, block, sizeof ustf->uas_active);
		if(strncmp(ustf->uas_active.ust_magic, "ustar", 5))
			break;
		e = ENOENT;	/* it must be an actual TFS */
		ustf->uas_init_fs = 1;
		/*
		 * XXX - the right way to store FS metadata on tfs
		 * is to embed the data within a file. For now, we
		 * will avoid complexity by hardwiring metadata for
		 * a floppy.
		 */
		ustf->uas_volsize = 80 * 2 * 18 * 512;	/* XXX */
		tfs_sscanf(ustf->uas_active.ust_size,"%12o",&filesize);
		if(strncmp(ustf->uas_active.ust_name, path,
		    sizeof ustf->uas_active.ust_name) == 0) {
			ustf->uas_filesize = filesize;
			e = 0;
			break;
		}
		offset += USTAR_NAME_BLOCK + filesize;
		filesize %= 512;
		if (filesize)
			offset += 512 - filesize;
	}
	if (e) {
		free(ustf, sizeof *ustf);
		f->f_fsdata = 0;
	}
	return e;
}

int
tfs_write(f, start, size, resid)
	struct open_file *f;
	void *start;
	size_t size;
	size_t *resid;
{
	return (EROFS);
}

off_t
tfs_seek(f, offs, whence)
	struct open_file *f;
	off_t offs;
	int whence;
{
	ust_active_t *ustf;

	ustf = f->f_fsdata;
	switch (whence) {
	    case SEEK_SET:
		ustf->uas_fseek = offs;
		break;
	    case SEEK_CUR:
		ustf->uas_fseek += offs;
		break;
	    case SEEK_END:
		ustf->uas_fseek = ustf->uas_filesize - offs;
		break;
	    default:
		return -1;
	}
	return ustf->uas_fseek;
}

int
tfs_read(f, start, size, resid)
	struct open_file *f;
	void *start;
	size_t size;
	size_t *resid;
{
	ust_active_t *ustf;
	int	e;
	char	*space512;
	int	blkoffs,
		readoffs,
		bufferoffset;
	size_t	seg;
	int	infile,
		inbuffer;

	e = 0;
	space512 = alloc(512);
	ustf = f->f_fsdata;
	while(size != 0) {
		if (ustf->uas_fseek >= ustf->uas_filesize)
			break;
		bufferoffset = ustf->uas_fseek % 512;
		blkoffs  = ustf->uas_fseek - bufferoffset;
		readoffs = ustf->uas_filestart + 512 + blkoffs;
		e = read512block(f, readoffs, space512);
		if (e)
			break;
		seg = size;
		inbuffer = 512 - bufferoffset;
		if (inbuffer < seg)
			seg = inbuffer;
		infile = ustf->uas_filesize - ustf->uas_fseek;
		if (infile < seg)
			seg = infile;
		memcpy(start, space512 + bufferoffset, seg);
		ustf->uas_fseek += seg;
		start += seg;
		size  -= seg;
	}
	if (resid)
		*resid = size;
	free(space512, 512);
	return e;
}

int
tfs_stat(f, sb)
	struct open_file *f;
	struct stat *sb;
{
	int	mode, uid, gid;
	ust_active_t *ustf;

	if (f == NULL)
		return EINVAL;
	ustf = f->f_fsdata;
	memset(sb, 0, sizeof *sb);
	tfs_sscanf(ustf->uas_active.ust_mode, "%8o", &mode);
	tfs_sscanf(ustf->uas_active.ust_uid, "%8o", &uid);
	tfs_sscanf(ustf->uas_active.ust_gid, "%8o", &gid);
	sb->st_mode = mode;
	sb->st_uid  = uid;
	sb->st_gid  = gid;
	sb->st_size = ustf->uas_filesize;
	return 0;
}

int
tfs_close(f)
	struct open_file *f;
{
	if (f == NULL || f->f_fsdata == NULL)
		return EINVAL;
	free(f->f_fsdata, sizeof(ust_active_t));
	f->f_fsdata = 0;
	return 0;
}
