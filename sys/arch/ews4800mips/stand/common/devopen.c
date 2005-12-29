/*	$NetBSD: devopen.c,v 1.1 2005/12/29 15:20:09 tsutsui Exp $	*/

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

#include <netinet/in.h>
#include <lib/libsa/dev_net.h>
#include <lib/libsa/ufs.h>
#include <lib/libsa/nfs.h>
#include <machine/sbd.h>

#include "local.h"

extern uint8_t kernel_binary[];
extern int kernel_binary_size;

extern struct fs_ops datafs_ops;
extern struct fs_ops bfs_ops;
struct fs_ops ufs_ops = {
	ufs_open, ufs_close, ufs_read, ufs_write, ufs_seek, ufs_stat
};
struct fs_ops nfs_ops = {
	nfs_open, nfs_close, nfs_read, nfs_write, nfs_seek, nfs_stat
};

extern struct devsw netdevsw;
extern struct devsw dkdevsw;
char fname[16];

/* Referenced by libsa/open.c */
struct fs_ops file_system[1];
int nfsys = 1;
struct devsw devsw[1];
int ndevs = 1;

int
devopen(struct open_file *f, const char *request, char **file)
{
	char *p, *filename;
	int disk, partition;
	void *addr;
	size_t size;

	strcpy(fname, request);

	filename = 0;
	for (p = fname; *p; p++) {
		if (*p == ':') {
			filename = p + 1;
			*p = '\0';
			break;
		}
	}

	if (filename == 0) {	/* not a loader's request. probably ufs_ls() */
		printf("request=%s\n", request);
		f->f_dev = &dkdevsw;
		file_system[0] = ufs_ops;
		devsw[0] = dkdevsw;
		*file = "/";
		return 0;
	}

	/* Data section */
	if (strcmp(fname, "mem") == 0) {
		data_attach(kernel_binary, kernel_binary_size);
		*file = "noname";
		f->f_flags |= F_NODEV;
		file_system[0] = datafs_ops;
		printf("data(compiled):noname\n");
		return 0;
	}

	/* NFS boot */
	if (strcmp(fname, "nfs") == 0) {
		extern int try_bootp;
		if (!DEVICE_CAPABILITY.network_enabled) {
			printf("Network disabled.\n");
			return -1;
		}
		try_bootp = TRUE;
		file_system[0] = nfs_ops;
		f->f_dev = &netdevsw;
		if (*filename == '\0') {
			printf("set kernel filename. ex.) nfs:netbsd\n");
			return 1;
		}
		*--filename = '/';
		*file = filename;
		printf("nfs:/%s\n", filename);
		net_open(f);
		return 0;
	}

	/* FD boot */
	if (strcmp(fname, "fd") == 0) {
		printf("floppy(boot):/%s (ustarfs)\n", filename);
		f->f_dev = &dkdevsw;
		file_system[0] = datafs_ops;
		devsw[0] = dkdevsw;
		device_attach(NVSRAM_BOOTDEV_FLOPPYDISK, -1, -1);
		if (!ustarfs_load(filename, &addr, &size))
			return -1;
		data_attach(addr, size);
		*file = filename;
		return 0;
	}

	/* Disk boot */
	if (strncmp(fname, "sd", 2) == 0) {
		enum fstype fs;
		if (!DEVICE_CAPABILITY.disk_enabled) {
			printf("Disk disabled.\n");
			return -1;
		}

		disk = fname[2] - '0';
		partition = fname[3] - 'a';
		if (disk < 0 || disk > 9 || partition < 0 || partition > 15) {
			fs = FSTYPE_USTARFS;
			printf("disk(boot):%s ", filename);
			device_attach(NVSRAM_BOOTDEV_HARDDISK, -1, -1);
		} else {
			fs = fstype(partition);
			printf("disk(%d,%d):/%s ",
			    disk, partition, filename);
			device_attach(NVSRAM_BOOTDEV_HARDDISK, disk, partition);
		}

		switch (fs) {
		case FSTYPE_UFS:
			printf(" (ufs)\n");
			f->f_dev = &dkdevsw;
			file_system[0] = ufs_ops;
			devsw[0] = dkdevsw;
			break;
		case FSTYPE_BFS:
			printf(" (bfs)\n");
			f->f_flags |= F_NODEV;
			file_system[0] = bfs_ops;
			break;
		case FSTYPE_USTARFS:
			printf(" (ustarfs)\n");
			f->f_dev = &dkdevsw;
			file_system[0] = datafs_ops;
			devsw[0] = dkdevsw;
			if (!ustarfs_load(filename, &addr, &size))
				return -1;
			data_attach(addr, size);
			break;
		}
		*file = filename;
		return 0;
	}

	printf("%s invalid.\n", fname);

	return -1;
}
