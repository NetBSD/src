/* $NetBSD: nbfs.c,v 1.9.2.2 2014/08/20 00:02:41 tls Exp $ */

/*-
 * Copyright (c) 2006 Ben Harris
 * Copyright (c) 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 */

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
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/disklabel.h>
#include <sys/queue.h>
#include <ufs/ufs/dinode.h>
#include <ufs/ufs/dir.h>
#include <lib/libkern/libkern.h>
#include <lib/libsa/stand.h>
#include <lib/libsa/lfs.h>
#include <lib/libsa/ufs.h>
#include <riscoscalls.h>
#include <riscosdisk.h>

#include "nbfs.h"

struct fs_ops file_system[] = {
	FS_OPS(ffsv1), FS_OPS(ffsv2), FS_OPS(lfsv1), FS_OPS(lfsv2)
};

int nfsys = __arraycount(file_system);

struct nbfs_open_file {
	struct	open_file f;
	int	fileswitch_handle;
	LIST_ENTRY(nbfs_open_file) link;
};

static LIST_HEAD(, nbfs_open_file) nbfs_open_files;

static os_error const *maperr(int saerr);

/*
 * Given a RISC OS special field and pathname, open the relevant
 * device and return a pointer to the remainder of the pathname.
 */
static int
nbfs_devopen(struct open_file *f, char const *special, char const *fname,
    char const **rest)
{
	unsigned int drive = 0, part = RAW_PART;
	int err;

	if (*fname++ != ':')
		return EINVAL;
	while (isdigit((unsigned char)*fname))
		drive = drive * 10 + *fname++ - '0';
	if (islower((unsigned char)*fname))
		part = *fname++ - 'a';
	else if (isupper((unsigned char)*fname))
		part = *fname++ - 'A';
	if (*fname != '.' && *fname != '\0')
		return EINVAL;
	err = rodisk_open(f, special, drive, part);
	if (err != 0)
		return err;
	*rest = fname;
	if (**rest == '.')
		(*rest)++;
	return 0;
}

static int
nbfs_fileopen(struct open_file *f, char const *tail)
{
	char *file, *p;
	int i, error = ENOENT;

	if (tail[0] == '$' && tail[1] == '.')
		tail += 2;
	file = alloc(strlen(tail) + 2);
	strcpy(file, "/");
	strcat(file, tail);
	for (p = file + 1; *p != '\0'; p++) {
		if (*p == '.')
			*p = '/';
		else if (*p == '/')
			*p = '.';
	}
	if (strcmp(tail, "$") == 0)
		strcpy(file, "/");

	for (i = 0; i < nfsys; i++) {
		error = FS_OPEN(&file_system[i])(file, f);
		if (error == 0 || error == ENOENT) {
			f->f_ops = &file_system[i];
			break;
		}
	}
	dealloc(file, strlen(file) + 1);
	return error;
}

static int
nbfs_fopen(struct open_file *f, char const *special, char const *path)
{
	char const *tail;
	int err;

	err = nbfs_devopen(f, special, path, &tail);
	if (err != 0)
		return err;
	err = nbfs_fileopen(f, tail);
	if (err != 0)
		DEV_CLOSE(f->f_dev)(f);
	return err;
}

static int
nbfs_fclose(struct open_file *f)
{
	int ferr, derr;

	ferr = FS_CLOSE(f->f_ops)(f);
	derr = DEV_CLOSE(f->f_dev)(f);
	return ferr != 0 ? ferr : derr;
}

os_error const *
nbfs_open(struct nbfs_reg *r)
{
	int reason = r->r0;
	char const *fname = (char const *)r->r1;
	int fh = r->r3;
	char const *special = (char const *)r->r6;
	int err;
	struct nbfs_open_file *nof = NULL;
	struct stat st;

	switch (reason) {
	case 0: /* Open for read */
	case 1: /* Create and open for update */
	case 2: /* Open for update */
		nof = alloc(sizeof(*nof));
		memset(nof, 0, sizeof(*nof));
		err = nbfs_fopen(&nof->f, special, fname);
		if (err != 0)
			goto fail;
		err = FS_STAT(nof->f.f_ops)(&nof->f, &st);
		if (err != 0)
			goto fail;
		nof->fileswitch_handle = fh;
		LIST_INSERT_HEAD(&nbfs_open_files, nof, link);
		r->r0 = 0x40000000;
		if (S_ISDIR(st.st_mode))
			r->r0 |= 0x20000000;
		r->r1 = (uint32_t)nof;
		r->r2 = DEV_BSIZE;
		r->r3 = st.st_size;
		r->r4 = st.st_size;
		return NULL;
	default:
		err = EINVAL;
		goto fail;
	}
fail:
	if (nof != NULL)
		dealloc(nof, sizeof(*nof));
	return maperr(err);
}

os_error const *
nbfs_getbytes(struct nbfs_reg *r)
{
	struct nbfs_open_file *nof = (struct nbfs_open_file *)r->r1;
	void *buf = (void *)r->r2;
	size_t size = r->r3;
	off_t off = r->r4;
	int err;

	err = FS_SEEK(nof->f.f_ops)(&nof->f, off, SEEK_SET);
	if (err == -1)
		return maperr(err);
	err = FS_READ(nof->f.f_ops)(&nof->f, buf, size, NULL);
	if (err != 0)
		return maperr(err);
	return NULL;
}

os_error const *
nbfs_putbytes(struct nbfs_reg *r)
{
	static os_error const err = {0, "nbfs_putbytes"};

	return &err;
}

os_error const *
nbfs_args(struct nbfs_reg *r)
{
	static os_error const err = {0, "nbfs_args"};

	return &err;
}

os_error const *
nbfs_close(struct nbfs_reg *r)
{
	struct nbfs_open_file *nof = (struct nbfs_open_file *)r->r1;
	/* uint32_t loadaddr = r->r2; */
	/* uint32_t execaddr = r->r3; */
	int err;

	err = nbfs_fclose(&nof->f);
	if (err != 0)
		return maperr(err);
	LIST_REMOVE(nof, link);
	dealloc(nof, sizeof(*nof));
	return NULL;
}

os_error const *
nbfs_file(struct nbfs_reg *r)
{
	int reason = r->r0;
	char const *fname = (char const *)r->r1;
	void *buf = (void *)r->r2;
	char const *special = (char const *)r->r6;
	struct open_file f;
	int err;
	struct stat st;

	memset(&f, 0, sizeof(f));
	err = nbfs_fopen(&f, special, fname);
	if (err != 0 && err != ENOENT)
		return maperr(err);
	switch (reason) {
	case 0: /* Save file */
	case 1: /* Write catalogue information */
	case 2: /* Write load address */
	case 3: /* Write execution address */
	case 4: /* Write attributes */
	case 6: /* Delete object */
	case 7: /* Create file */
	case 8: /* Create directory */
		nbfs_fclose(&f);
		err = EROFS;
		goto fail;
	case 5: /* Read catalogue information */
	case 255: /* Load file */
		if (err == ENOENT)
			r->r0 = r->r2 = r->r3 = r->r4 = r->r5 = 0;
		else {
			err = FS_STAT(f.f_ops)(&f, &st);
			if (err != 0)
				goto fail;
			r->r0 = S_ISDIR(st.st_mode) ?
			    fileswitch_IS_DIR : fileswitch_IS_FILE;
			r->r2 = r->r3 = 0;
			r->r4 = st.st_size;
			r->r5 = fileswitch_ATTR_OWNER_READ |
			    fileswitch_ATTR_WORLD_READ;
			if (reason == 255) {
				err = FS_READ(f.f_ops)
				    (&f, buf, st.st_size, NULL);
				if (err != 0)
					goto fail;
				/* R6 should really be the leaf name */
				r->r6 = r->r1;
			}
		}
		break;
	default:
		nbfs_fclose(&f);
		err = EINVAL;
		goto fail;
	}
	nbfs_fclose(&f);
	return NULL;
fail:
	nbfs_fclose(&f);
	return maperr(err);
}

static int
nbfs_filename_ok(char const *f)
{

	while (*f)
		if (strchr(":*#$&@^%\\", *f++) != NULL)
			return 0;
	return 1;
}

static os_error const *
nbfs_func_dirents(struct nbfs_reg *r)
{
	int reason = r->r0;
	char const *fname = (char const *)r->r1;
	char const *special = (char const *)r->r6;
	struct open_file f;
	struct stat st;
	int err;
	struct fileswitch_dirent *fdp;
	size_t resid;
	size_t maxcount = r->r3;
	size_t count = 0;
	size_t skip = r->r4;
	ssize_t off = 0;
	size_t buflen = r->r5;
	char dirbuf[UFS_DIRBLKSIZ];
	char *outp = (char *)r->r2;

	err = nbfs_fopen(&f, special, fname);
	if (err != 0)
		return maperr(err);
	err = FS_STAT(f.f_ops)(&f, &st);
	if (err != 0)
		goto fail;
	if (!S_ISDIR(st.st_mode)) {
		err = ENOTDIR;
		goto fail;
	}
	while (FS_READ(f.f_ops)(&f, dirbuf, UFS_DIRBLKSIZ, &resid) == 0 &&
	    resid == 0) {
		struct direct  *dp, *edp;

		dp = (struct direct *) dirbuf;
		edp = (struct direct *) (dirbuf + UFS_DIRBLKSIZ);

		for (; dp < edp; dp = (void *)((char *)dp + dp->d_reclen)) {
			size_t entsiz = 0;
			int i;

			if (dp->d_ino ==  0)
				continue;
			/*
			 * Skip ., .., and names with characters that RISC
			 * OS doesn't allow.
			 */
			if (strcmp(dp->d_name, ".") == 0 ||
			    strcmp(dp->d_name, "..") == 0 ||
			    !nbfs_filename_ok(dp->d_name))
				continue;
			if (off++ < skip)
				continue;

			switch (reason) {
			case 14:
				entsiz = strlen(dp->d_name) + 1;
				if (buflen < entsiz)
					goto out;
				strcpy(outp, dp->d_name);
				break;
			case 15:
				entsiz = ALIGN(offsetof(
					  struct fileswitch_dirent, name)
				    + strlen(dp->d_name) + 1);
				if (buflen < entsiz)
					goto out;

				fdp = (struct fileswitch_dirent *)outp;
				fdp->loadaddr = 0;
				fdp->execaddr = 0;
				fdp->length = 0;
				fdp->attr = 0;
				fdp->objtype = dp->d_type == DT_DIR ?
				    fileswitch_IS_DIR : fileswitch_IS_FILE;
				strcpy(fdp->name, dp->d_name);
				for (i = 0; fdp->name[i] != '\0'; i++)
					if (fdp->name[i] == '.')
						fdp->name[i] = '/';
				break;
			}
			outp += entsiz;
			buflen -= entsiz;
			if (++count == maxcount)
				goto out;
		}
	}
	off = -1;
out:
	nbfs_fclose(&f);
	r->r3 = count;
	r->r4 = off;
	return NULL;
fail:
	nbfs_fclose(&f);
	return maperr(err);
}

os_error const *
nbfs_func(struct nbfs_reg *r)
{
	static os_error error = {0, "nbfs_func"};
	int reason = r->r0;

	switch (reason) {
	case 14:
	case 15:
		return nbfs_func_dirents(r);

	case 16: /* Shut down */
		return NULL;
	default:
		snprintf(error.errmess, sizeof(error.errmess),
		    "nbfs_func %d not implemented", reason);
		return &error;
	}
}

#define FSERR(x) (0x10000 | (NBFS_FSNUM << 8) | (x))

static struct {
	int saerr;
	os_error roerr;
} const errmap[] = {
	{ ECTLR,   { FSERR(ECTLR),   "Bad parent filing system" } },
	{ EUNIT,   { FSERR(0xAC),    "Bad drive number" } },
	{ EPART,   { FSERR(EPART),   "Bad partition" } },
	{ ERDLAB,  { FSERR(ERDLAB),  "Can't read disk label" } },
	{ EUNLAB,  { FSERR(EUNLAB),  "Unlabeled" } },
	{ ENOENT,  { FSERR(0xD6),    "No such file or directory" } },
	{ EIO,     { FSERR(EIO),     "Input/output error" } },
	{ EINVAL,  { FSERR(EINVAL),  "Invalid argument" } },
	{ ENOTDIR, { FSERR(ENOTDIR), "Not a directory" } },
	{ EROFS,   { FSERR(0xC9),    "Read-only file system" } },
};

static os_error const *maperr(int err)
{
	int i;
	static const os_error defaulterr = { FSERR(0), "Unknown NBFS error" };

	for (i = 0; i < sizeof(errmap) / sizeof(errmap[0]); i++)
		if (err == errmap[i].saerr)
			return &errmap[i].roerr;
	return &defaulterr;
}
