/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Fleischer <paul@xpg.dk>
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
#include <sys/disklabel.h>

#include <netinet/in.h>

#include <lib/libkern/libkern.h>
#include <lib/libsa/stand.h>
#include <lib/libsa/nfs.h>
#include <lib/libsa/tftp.h>
#include <lib/libsa/ext2fs.h>
#include <lib/libsa/dosfs.h>
#include <lib/libsa/ufs.h>

#include "dev_sdmmc.h"
#include <arch/evbarm/mini2440/mini2440_bootinfo.h>

/* Implemented in dev_net.c */
int net_open(struct open_file *, ...);
int net_close(struct open_file *);
int net_strategy(void *, int, daddr_t, size_t, void *, size_t *);

/* Implemented in dev_sdmmc.c */
int sdmmc_open(struct open_file *, ...);
int sdmmc_close(struct open_file *);
int sdmmc_strategy(void *, int, daddr_t, size_t, void *, size_t *);
int sdmmc_get_fstype(void*);

struct devsw devsw[] = {
	{ "dm9000", net_strategy, net_open, net_close, noioctl },
	{ "s3c sdio", sdmmc_strategy, sdmmc_open, sdmmc_close, noioctl }
};

struct fs_ops ops_nfs = FS_OPS(nfs);
struct fs_ops ops_tftp = FS_OPS(tftp);
struct fs_ops ops_ext2fs = FS_OPS(ext2fs);
struct fs_ops ops_dosfs = FS_OPS(dosfs);
struct fs_ops ops_bsdfs = FS_OPS(ufs);
struct fs_ops file_system[1];
int nfsys = 1;

extern struct btinfo_bootpath		bi_path;
extern struct btinfo_rootdevice		bi_rdev;

static void parseunit(const char *name, int *unitp, int *partp, char **pathp);

int
devopen(struct open_file *of, const char *name, char **file)
{
	int error;
	extern char bootfile[];

	if (strncmp("net:", name, 4) == 0 ||
	    strncmp("nfs:", name, 4) == 0 ) {
		of->f_dev = &devsw[0];
		if ((error = net_open(of, name+4, "nfs")) != 0)
			return error;
		file_system[0] = ops_nfs;
		*file = bootfile;
		strncpy(bi_path.bootpath, bootfile, sizeof(bi_path.bootpath));

		return 0;
	} else if (strncmp("tftp:", name, 5) == 0) {
		of->f_dev = &devsw[0];
		if ((error = net_open(of, name+5, "tftp")) != 0) {
			return error;
		}
		file_system[0] = ops_tftp;
		*file = bootfile;
		strncpy(bi_path.bootpath, bootfile, sizeof(bi_path.bootpath));

		return 0;
	} else if (name[0] == 'l' && name[1] == 'd') {
		int unit, part;
		parseunit(&name[2], &unit, &part, file);
		if (*file == NULL || strlen(*file) == 0) {
			strcpy(*file, "netbsd");
		}
		of->f_dev = &devsw[1];
		if ((error = sdmmc_open(of, unit, part)) != 0)
			return error;

		strncpy(bi_path.bootpath, *file, sizeof(bi_path.bootpath));

		snprintf(bi_rdev.devname, sizeof(bi_rdev.devname), "ld");
		bi_rdev.cookie = unit;
		bi_rdev.partition = part;
		switch(sdmmc_get_fstype(of->f_devdata)) {
		case FS_EX2FS:
			file_system[0] = ops_ext2fs;
			break;
		case FS_MSDOS:
			file_system[0] = ops_dosfs;
			break;
		case FS_BSDFFS:
			file_system[0] = ops_bsdfs;
			break;
		default:
			return ENOENT;
		}

		return 0;
	}
	return ENOENT;
}

static void
parseunit(const char *name, int *unitp, int *partp, char **pathp)
{
	const char *p = name;
	int unit, part;

	unit = part = -1;
	while (*p != ':' && *p != '\0') {
		if (unit == -1 && *p >= '0' && *p <= '9')
			unit = *p - '0';
		if (part == -1 && *p >= 'a' && *p < 'a' + 16)
			part = *p - 'a';
		p += 1;
	}
	*unitp = (unit == -1) ? 0 : unit;
	*partp = (part == -1) ? 0 : part;
	*pathp = (*p == ':') ? (char *)p + 1 : NULL;
}
