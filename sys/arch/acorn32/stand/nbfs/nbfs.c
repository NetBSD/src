/* $NetBSD: nbfs.c,v 1.3.10.2 2006/05/24 15:47:49 tron Exp $ */

/*-
 * Copyright (c) 2006 Ben Harris
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


#include <sys/types.h>
#include <sys/param.h>
#include <sys/disklabel.h>
#include <sys/queue.h>
#include <lib/libkern/libkern.h>
#include <lib/libsa/stand.h>
#include <lib/libsa/ufs.h>
#include <riscoscalls.h>
#include <riscosdisk.h>

#include "nbfs.h"

struct fs_ops file_system[] = { FS_OPS(ffsv1), FS_OPS(ffsv2) };

int nfsys = __arraycount(file_system);

struct nbfs_open_file {
	struct	open_file f;
	int	fileswitch_handle;
	LIST_ENTRY(nbfs_open_file) link;
};

static LIST_HEAD(, nbfs_open_file) nbfs_open_files;

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
	if (err != 0) return err;
	*rest = fname;
	if (**rest == '.') (*rest)++;
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
		if (*p == '.') *p = '/';
		else if (*p == '/') *p = '.';
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
	if (err != 0) return err;
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

os_error *
nbfs_open(struct nbfs_reg *r)
{
	static os_error error = {0, "nbfs_open"};
	int reason = r->r0;
	char const *fname = (char const *)r->r1;
	int fh = r->r3;
	char const *special = (char const *)r->r6;
	int err;
	struct nbfs_open_file *nof = NULL;
	struct stat st;

	if (reason == 0) {
		nof = alloc(sizeof(*nof));
		memset(nof, 0, sizeof(*nof));
		err = nbfs_fopen(&nof->f, special, fname);
		if (err != 0) goto fail;
		err = FS_STAT(nof->f.f_ops)(&nof->f, &st);
		if (err != 0) goto fail;
		nof->fileswitch_handle = fh;
		LIST_INSERT_HEAD(&nbfs_open_files, nof, link);
		r->r0 = 0x40000000;
		if (S_ISDIR(st.st_mode)) r->r0 |= 0x20000000;
		r->r1 = (uint32_t)nof;
		r->r2 = DEV_BSIZE;
		r->r3 = st.st_size;
		r->r4 = st.st_size;
		return NULL;
	}
fail:
	if (nof != NULL)
		dealloc(nof, sizeof(*nof));
	return &error;
}

os_error *
nbfs_getbytes(struct nbfs_reg *r)
{
	static os_error error = {0, "nbfs_getbytes"};
	struct nbfs_open_file *nof = (struct nbfs_open_file *)r->r1;
	void *buf = (void *)r->r2;
	size_t size = r->r3;
	off_t off = r->r4;
	int err;

	err = FS_SEEK(nof->f.f_ops)(&nof->f, off, SEEK_SET);
	if (err == -1) return &error;
	err = FS_READ(nof->f.f_ops)(&nof->f, buf, size, NULL);
	if (err != 0) return &error;
	return NULL;
}

os_error *
nbfs_putbytes(struct nbfs_reg *r)
{
	static os_error err = {0, "nbfs_putbytes"};

	return &err;
}

os_error *
nbfs_args(struct nbfs_reg *r)
{
	static os_error err = {0, "nbfs_args"};

	return &err;
}

os_error *
nbfs_close(struct nbfs_reg *r)
{
	static os_error error = {0, "nbfs_close"};
	struct nbfs_open_file *nof = (struct nbfs_open_file *)r->r1;
	/* uint32_t loadaddr = r->r2; */
	/* uint32_t execaddr = r->r3; */
	int err;

	err = nbfs_fclose(&nof->f);
	if (err != 0) return &error;
	LIST_REMOVE(nof, link);
	dealloc(nof, sizeof(*nof));
	return NULL;
}

os_error *
nbfs_file(struct nbfs_reg *r)
{
	static os_error error = {0, "nbfs_file"};
	int reason = r->r0;
	char const *fname = (char const *)r->r1;
	char const *special = (char const *)r->r6;
	struct open_file f;
	int err;
	struct stat st;

	memset(&f, 0, sizeof(f));
	err = nbfs_fopen(&f, special, fname);
	if (err != 0 && err != ENOENT)
		return &error;
	switch (reason) {
	case 5:
		if (err == ENOENT)
			r->r0 = r->r2 = r->r3 = r->r4 = r->r5 = 0;
		else {
			err = FS_STAT(f.f_ops)(&f, &st);
			if (err != 0) goto fail;
			r->r0 = S_ISDIR(st.st_mode) ?
			    fileswitch_IS_DIR : fileswitch_IS_FILE;
			r->r2 = r->r3 = 0;
			r->r4 = st.st_size;
			r->r5 = fileswitch_ATTR_OWNER_READ |
			    fileswitch_ATTR_WORLD_READ;
		}
		break;
	default:
		goto fail;
	}
	nbfs_fclose(&f);
	return NULL;
fail:
	nbfs_fclose(&f);
	return &error;
}

os_error *
nbfs_func(struct nbfs_reg *r)
{
	static os_error err = {0, "nbfs_func"};
	int reason = r->r0;

	/* We do nothing on shutdown */
	if (reason == 16) return NULL;

	return &err;
}
