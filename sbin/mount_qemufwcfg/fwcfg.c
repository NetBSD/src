/* $NetBSD: fwcfg.c,v 1.7 2022/01/22 07:53:06 pho Exp $ */

/*-
 * Copyright (c) 2017 Jared McNeill <jmcneill@invisible.ca>
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: fwcfg.c,v 1.7 2022/01/22 07:53:06 pho Exp $");

#define FUSE_USE_VERSION FUSE_MAKE_VERSION(2, 6)

#include <sys/ioctl.h>
#include <sys/param.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <dev/ic/qemufwcfgio.h>

#include "virtdir.h"

#define _PATH_FWCFG	"/dev/qemufwcfg"

struct fwcfg_file {
	uint32_t size;
	uint16_t select;
	uint16_t reserved;
	char name[56];
};

static int fwcfg_fd;
static mode_t fwcfg_dir_mask = 0555;
static mode_t fwcfg_file_mask = 0444;
static uid_t fwcfg_uid;
static gid_t fwcfg_gid;

static virtdir_t fwcfg_virtdir;

static void
set_index(uint16_t index)
{
	if (ioctl(fwcfg_fd, FWCFGIO_SET_INDEX, &index) != 0)
		err(EXIT_FAILURE, "failed to set index 0x%04x", index);
}

static void
read_data(void *buf, size_t buflen)
{
	if (read(fwcfg_fd, buf, buflen) != (ssize_t)buflen)
		err(EXIT_FAILURE, "failed to read data");
}

static int
fwcfg_getattr(const char *path, struct stat *st)
{
	virt_dirent_t *ep;

	if (strcmp(path, "/") == 0) {
		memset(st, 0, sizeof(*st));
		st->st_mode = S_IFDIR | fwcfg_dir_mask;
		st->st_nlink = 2;
		return 0;
	}

	if ((ep = virtdir_find(&fwcfg_virtdir, path, strlen(path))) == NULL)
		return -ENOENT;

	switch (ep->type) {
	case 'f':
		memcpy(st, &fwcfg_virtdir.file, sizeof(*st));
		st->st_size = (off_t)ep->tgtlen;
		st->st_mode = S_IFREG | fwcfg_file_mask;
		break;
	case 'd':
		memcpy(st, &fwcfg_virtdir.dir, sizeof(*st));
		st->st_mode = S_IFDIR | fwcfg_dir_mask;
		break;
	}
	st->st_ino = (ino_t)virtdir_offset(&fwcfg_virtdir, ep) + 10;

	return 0;
}

static int
fwcfg_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
    off_t offset, struct fuse_file_info *fi)
{
	static VIRTDIR *dirp;
	virt_dirent_t *dp;

	if (offset == 0) {
		if ((dirp = openvirtdir(&fwcfg_virtdir, path)) == NULL)
			return 0;
		filler(buf, ".", NULL, 0);
		filler(buf, "..", NULL, 0);
	}
	while ((dp = readvirtdir(dirp)) != NULL) {
		if (filler(buf, dp->d_name, NULL, 0) != 0)
			return 0;
	}
	closevirtdir(dirp);
	dirp = NULL;

	return 0;
}

static int
fwcfg_open(const char *path, struct fuse_file_info *fi)
{
	return 0;
}

static int
fwcfg_read(const char *path, char *buf, size_t size, off_t offset,
    struct fuse_file_info *fi)
{
	virt_dirent_t *ep;
	uint8_t tmp[64];

	if ((ep = virtdir_find(&fwcfg_virtdir, path, strlen(path))) == NULL)
		return -ENOENT;

	if (ep->select == 0)
		return -ENOENT;

	set_index(ep->select);

	/* Seek to correct offset */
	while (offset > 0) {
		const size_t len = MIN(sizeof(tmp), (size_t)offset);
		read_data(tmp, len);
		offset -= (off_t)len;
	}

	/* Read the data */
	read_data(buf, size);

	return (int)size;
}

static int
fwcfg_statfs(const char *path, struct statvfs *st)
{
	uint32_t count;

	set_index(FW_CFG_FILE_DIR);
	read_data(&count, sizeof(count));

	memset(st, 0, sizeof(*st));
	st->f_files = be32toh(count);

	return 0;
}

static struct fuse_operations fwcfg_ops = {
	.getattr = fwcfg_getattr,
	.readdir = fwcfg_readdir,
	.open = fwcfg_open,
	.read = fwcfg_read,
	.statfs = fwcfg_statfs,
};

static void
build_tree(virtdir_t *v)
{
	char path[PATH_MAX];
	struct fwcfg_file f;
	uint32_t count, n;
	struct stat st;

	memset(&st, 0, sizeof(st));
	st.st_uid = fwcfg_uid;
	st.st_gid = fwcfg_gid;
	virtdir_init(v, NULL, &st, &st, &st);

	set_index(FW_CFG_FILE_DIR);
	read_data(&count, sizeof(count));
	for (n = 0; n < be32toh(count); n++) {
		read_data(&f, sizeof(f));
		snprintf(path, sizeof(path), "/%s", f.name);
		virtdir_add(v, path, strlen(path), 'f', NULL,
		    be32toh(f.size), be16toh(f.select));
	}
}

#if 0
static __dead void
usage(void)
{
	fprintf(stderr, "Usage: %s [-F path] [-g gid] [-M dir-mode] "
	    "[-m file-mode] [-u uid] [fuse-options]", getprogname());
	exit(EXIT_FAILURE);
}
#endif

int
main(int argc, char *argv[])
{
	const char *path = _PATH_FWCFG;
	int ch;
	long m;
	char *ep;

	fwcfg_uid = geteuid();
	fwcfg_gid = getegid();

	while ((ch = getopt(argc, argv, "F:g:m:M:u:")) != -1) {
		switch (ch) {
		case 'F':
			path = optarg;
			break;
		case 'g':
			fwcfg_gid = (gid_t)atoi(optarg);
			break;
		case 'm':
			m = strtol(optarg, &ep, 8);
			if (optarg == ep || *ep || m < 0)
				errx(EXIT_FAILURE, "invalid file mode: %s",
				    optarg);
			fwcfg_file_mask = (mode_t)m;
			break;
		case 'M':
			m = strtol(optarg, &ep, 8);
			if (optarg == ep || *ep || m < 0)
				errx(EXIT_FAILURE, "invalid directory mode: %s",
				    optarg);
			fwcfg_dir_mask = (mode_t)m;
			break;
		case 'u':
			fwcfg_uid = (uid_t)atoi(optarg);
			break;
#ifdef notyet
		default:
			usage();
#endif
		}
	}

	fwcfg_fd = open(path, O_RDWR);
	if (fwcfg_fd == -1)
		err(EXIT_FAILURE, "failed to open %s", path);

	build_tree(&fwcfg_virtdir);

	return fuse_main(argc, argv, &fwcfg_ops, NULL);
}
