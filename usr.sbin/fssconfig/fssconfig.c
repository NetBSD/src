/*	$NetBSD: fssconfig.c,v 1.13 2017/12/21 15:52:19 hannken Exp $	*/

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Juergen Hannken-Illjes.
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
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/stat.h>

#include <stdio.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <util.h>

#include <dev/fssvar.h>

#include "prog_ops.h"

static int	vflag = 0;
static int	xflag = 0;

static void	config(int, char **);
static void	unconfig(int, char **);
static void	list(int, char **);
__dead static void	usage(void);

int
main(int argc, char **argv)
{
	int ch;
	void (*action)(int, char **);

	action = NULL;

	while ((ch = getopt(argc, argv, "cluvx")) != -1) {
		switch (ch) {
		case 'c':
			if (action)
				usage();
			else
				action = config;
			break;
		case 'l':
			if (action)
				usage();
			else
				action = list;
			break;
		case 'u':
			if (action)
				usage();
			else
				action = unconfig;
			break;
		case 'v':
			vflag++;
			break;
		case 'x':
			xflag++;
			break;
		default:
		case '?':
			usage();
			/* NOTREACHED */
		}
	}
	if (action == NULL)
		action = config;

	argc -= optind;
	argv += optind;

	(*action)(argc, argv);

	exit(0);
}

static void
config(int argc, char **argv)
{
	int fd, isreg, istmp, ispersistent;
	char full[64], path[MAXPATHLEN];
	off_t bssize;
	dev_t mountdev;
	struct stat sbuf;
	struct statvfs fsbuf;
	struct fss_set fss;

	if (argc < 3)
		usage();

	istmp = ispersistent = 0;

	fss.fss_mount = argv[1];
	fss.fss_bstore = argv[2];

	if (prog_statvfs1(argv[1], &fsbuf, ST_WAIT) != 0 ||
	    prog_stat(argv[1], &sbuf) != 0)
		err(1, "stat %s", argv[1]);
	mountdev = sbuf.st_dev;
	if (stat(argv[2], &sbuf) == 0) {
		if (S_ISREG(sbuf.st_mode) && sbuf.st_dev == mountdev) {
			if ((sbuf.st_flags & SF_SNAPSHOT) == 0)
				errx(1, "%s: exists and is not a snapshot",
				    argv[2]);
			if (argc != 3)
				usage();
			isreg = ispersistent = 1;

			goto configure;
		} else if (S_ISDIR(sbuf.st_mode)) {
			istmp = 1;
		}
	}

	if (argc > 5)
		usage();

	if (argc > 3)
		fss.fss_csize = strsuftoll("cluster size", argv[3], 0, INT_MAX);
	else
		fss.fss_csize = 0;
	if (argc > 4)
		bssize = strsuftoll("bs size", argv[4], 0, LLONG_MAX);
	else
		bssize = (off_t)fsbuf.f_blocks*fsbuf.f_frsize;

	/*
	 * Create the backing store.
	 */
	if (istmp) {
		snprintf(path, sizeof(path), "%s/XXXXXXXXXX", argv[2]);
		fss.fss_bstore = path;
		fd = mkstemp(fss.fss_bstore);
	} else {
		fd = prog_open(fss.fss_bstore, O_CREAT|O_TRUNC|O_WRONLY, 0600);
	}
	if (fd < 0) {
		err(1, "create: %s", fss.fss_bstore);
	}
	if (prog_fstat(fd, &sbuf) < 0)
		err(1, "stat: %s", fss.fss_bstore);
	if (!ispersistent && sbuf.st_dev == mountdev)
		ispersistent = 1;
	isreg = S_ISREG(sbuf.st_mode);
	if (!ispersistent && isreg && ftruncate(fd, bssize) < 0)
		err(1, "truncate %s", fss.fss_bstore);
	prog_close(fd);

configure:
	fd = opendisk1(argv[0], O_RDWR, full, sizeof(full), 0, prog_open);
	if (fd < 0) {
		if (istmp)
			unlink(fss.fss_bstore);
		err(1, "open: %s", argv[0]);
	}

	fss.fss_flags = 0;
	if ((xflag || istmp) && isreg)
		fss.fss_flags |= FSS_UNLINK_ON_CREATE;

	if (prog_ioctl(fd, FSSIOCSET, &fss) < 0) {
		if (istmp)
			unlink(fss.fss_bstore);
		err(1, "%s: FSSIOCSET", full);
	}

	if (vflag)
		list(1, argv);
}

static void
unconfig(int argc, char **argv)
{
	int fd;
	char full[64];

	if (argc != 1 || xflag)
		usage();

	if (vflag)
		list(1, argv);

	fd = opendisk1(argv[0], O_RDWR, full, sizeof(full), 0, prog_open);
	if (fd < 0)
		err(1, "open: %s", argv[0]);

	if (prog_ioctl(fd, FSSIOCCLR) < 0)
		err(1, "%s: FSSIOCCLR", full);
}

static void
list(int argc, char **argv)
{
	int n, fd, flags;
	char *dev, path[64], full[64];
	char clbuf[5], bsbuf[5], tmbuf[64];
	time_t t;
	struct fss_get fsg;

	if (argc > 1 || xflag)
		usage();

	if (argc > 0) 
		dev = argv[0];
	else
		dev = path;

	for (n = 0; ; n++) {
		if (argc == 0)
			snprintf(path, sizeof(path), "fss%d", n);
		fd = opendisk1(dev, O_RDONLY, full, sizeof(full), 0, prog_open);
		if (fd < 0) {
			if (argc == 0 && (errno == ENOENT || errno == ENXIO))
				break;
			err(1, "open: %s", dev);
		}

		if (prog_ioctl(fd, FSSIOFGET, &flags) < 0)
			flags = 0;

		if (prog_ioctl(fd, FSSIOCGET, &fsg) < 0) {
			if (errno == ENXIO)
				printf("%s: not in use\n", dev);
			else
				err(1, "%s: FSSIOCGET", full);
		} else if (vflag) {
			humanize_number(clbuf, sizeof(clbuf),
			    (int64_t)fsg.fsg_csize,
			    "", HN_AUTOSCALE, HN_B|HN_NOSPACE);

			humanize_number(bsbuf, sizeof(bsbuf),
			    (int64_t)fsg.fsg_bs_size*fsg.fsg_csize,
			    "", HN_AUTOSCALE, HN_B|HN_NOSPACE);

			t = fsg.fsg_time.tv_sec;
			strftime(tmbuf, sizeof(tmbuf), "%F %T", localtime(&t));

			printf("%s: %s, taken %s", dev, fsg.fsg_mount, tmbuf);
			if ((flags & FSS_UNCONFIG_ON_CLOSE) != 0)
				printf(", unconfig on close");
			if (fsg.fsg_csize == 0) 
				printf(", file system internal\n");
			else
				printf(", %"PRId64" cluster of %s, %s backup\n",
				    fsg.fsg_mount_size, clbuf, bsbuf);
		} else
			printf("%s: %s\n", dev, fsg.fsg_mount);

		prog_close(fd);

		if (argc > 0)
			break;
	}
}

static void
usage(void)
{
	fprintf(stderr, "%s",
	    "usage: fssconfig [-cxv] device path backup [cluster [size]]\n"
	    "       fssconfig -u [-v] device\n"
	    "       fssconfig -l [-v] [device]\n");
	exit(1);
}
