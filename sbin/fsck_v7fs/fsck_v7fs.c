/*	$NetBSD: fsck_v7fs.c,v 1.1 2011/06/27 11:52:58 uch Exp $ */

/*-
 * Copyright (c) 2004, 2011 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: fsck_v7fs.c,v 1.1 2011/06/27 11:52:58 uch Exp $");
#endif /* not lint */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <err.h>
#include <sys/ioctl.h>
#include <sys/disklabel.h>

#include <fs/v7fs/v7fs.h>
#include "v7fs_impl.h"
#include "fsck_v7fs.h"
#include "progress.h"

static void usage(void) __dead;
static void catopt(char **, const char *);

enum fsck_operate fsck_operate;
bool verbose = true;
#define	VPRINTF(fmt, args...)	{ if (verbose) printf(fmt, ##args); }

int
main(int argc, char **argv)
{
	const char *device;
	struct disklabel d;
	struct partition *p;
	struct stat st;
	int Fflag = 0;
	int part;
	int fd, ch;
	int endian = _BYTE_ORDER;
	int openflags = O_RDWR;
	size_t part_sectors;
	int fsck_flags = 0;
	char *options = 0;
	bool progress_bar_enable = false;

	fsck_operate = ASK;

	if (argc < 2)
		usage();

	while ((ch = getopt(argc, argv, "pPqynfx:dFB:o:")) != -1) {
		switch (ch) {
			/*
			 * generic fsck options
			 */
		case 'd':	/* Not supported */
			break;
		case 'f':	/* Always forced */
			break;
		case 'p':
			fsck_operate = PREEN;
			break;
		case 'y':
			fsck_operate = ALWAYS_YES;
			break;
		case 'n':
			fsck_operate = ALWAYS_NO;
			openflags = O_RDONLY;
			break;
		case 'P':
			progress_bar_enable = true;
			break;
		case 'q':	/* Not supported */
			break;
		case 'x':	/* Not supported */
			break;
			/*
			 * v7fs fsck options
			 */
		case 'F':
			Fflag = 1;
			break;
		case 'B':
			switch (optarg[0]) {
			case 'l':
				endian = _LITTLE_ENDIAN;
				break;
			case 'b':
				endian = _BIG_ENDIAN;
				break;
			case 'p':
				endian = _PDP_ENDIAN;
				break;
			}
			break;
		case 'o': /* datablock, freeblock duplication check */
			if (*optarg)
				catopt(&options, optarg);
			break;
		default:
			usage();
			/*NOTREACHED*/
		}
	}

	argc -= optind;
	argv += optind;

	if (argc != 1)
		usage();
	device = argv[0];

	if (options) {
		if (strstr(options, "data"))
			fsck_flags |= V7FS_FSCK_DATABLOCK_DUP;
		if (strstr(options, "free"))
			fsck_flags |= V7FS_FSCK_FREEBLOCK_DUP;
	}

	if (Fflag) {
		if ((fd = open(device, openflags)) == -1) {
			pfatal("%s", device);
		}
		if (fstat(fd, &st)) {
			pfatal("stat");
		}
		part_sectors = st.st_size >> V7FS_BSHIFT;
		setcdevname(device, fsck_operate == PREEN);
	} else {
		/* blockcheck sets 'hot' */
		device = blockcheck(device);
		setcdevname(device, fsck_operate == PREEN);

		if ((fd = open(device, openflags)) == -1) {
			pfatal("%s", device);
		}
		part = DISKPART(st.st_rdev);

		if (ioctl(fd, DIOCGDINFO, &d) == -1) {
			pfatal("DIOCGDINFO");
		}
		p = &d.d_partitions[part];
		part_sectors = p->p_size;
		VPRINTF("partition=%d size=%d offset=%d fstype=%d secsize=%d\n",
		    part, p->p_size, p->p_offset, p->p_fstype, d.d_secsize);
		if (p->p_fstype != FS_V7) {
			pfatal("not a Version 7 partition.");
		}
	}

	progress_switch(progress_bar_enable);
	progress_init();
	progress(&(struct progress_arg){ .cdev = device });

	struct v7fs_mount_device mount;
	mount.device.fd = fd;
	mount.endian = endian;
	mount.sectors = part_sectors;
	int error = v7fs_fsck(&mount, fsck_flags);

	close(fd);

	return error;
}

static void
catopt(char **sp, const char *o)
{
	char *s, *n;

	s = *sp;
	if (s) {
		if (asprintf(&n, "%s,%s", s, o) < 0)
			err(1, "asprintf");
		free(s);
		s = n;
	} else
		s = strdup(o);
	*sp = s;
}

static void
usage(void)
{

	(void)fprintf(stderr, "usage: %s [-ynpP] [-o options] [-B endian] "
	    "special-device\n",
	    getprogname());
	(void)fprintf(stderr, "usage: %s -F [-ynpP] [-o options] [-B endian] "
	    "file\n",
	    getprogname());

	exit(FSCK_EXIT_USAGE);
}
