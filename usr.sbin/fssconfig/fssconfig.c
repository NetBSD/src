/*	$NetBSD: fssconfig.c,v 1.3 2004/04/21 01:05:47 christos Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/stat.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <util.h>

#include <dev/fssvar.h>

int	vflag = 0;
int	xflag = 0;

int	mkfile(int, off_t);
void	config(int, char **);
void	unconfig(int, char **);
void	list(int, char **);
void	usage(void);

int
main(int argc, char **argv)
{
	int ch;
	void (*action)(int, char **);

	action = config;

	while ((ch = getopt(argc, argv, "cluvx")) != -1) {
		switch (ch) {
		case 'c':
			action = config;
			break;
		case 'l':
			action = list;
			break;
		case 'u':
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

	argc -= optind;
	argv += optind;

	(*action)(argc, argv);

	exit(0);
}

int
mkfile(int fd, off_t size)
{
	char buf[64*1024];
	ssize_t l;

	memset(buf, 0, sizeof(buf));
	while (size > 0) {
		if ((l = write(fd, buf, sizeof(buf))) < 0)
			return -1;
		size -= l;
	}

	return 0;
}

void
config(int argc, char **argv)
{
	int fd, isreg, istmp;
	char full[64], path[MAXPATHLEN];
	off_t bssize;
	struct stat sbuf;
	struct statvfs fsbuf;
	struct fss_set fss;

	if (argc < 3)
		usage();

	istmp = 0;

	if (statvfs(argv[1], &fsbuf) != 0)
		err(1, "statvfs %s", argv[1]);

	fss.fss_mount = argv[1];
	fss.fss_bstore = argv[2];
	if (argc > 3)
		fss.fss_csize = strsuftoll("cluster size", argv[3], 0, INT_MAX);
	else
		fss.fss_csize = 0;
	if (argc > 4)
		bssize = strsuftoll("bs size", argv[4], 0, LLONG_MAX);
	else
		bssize = (off_t)fsbuf.f_blocks*fsbuf.f_frsize;

	/*
	 * Create the backing store. If it is a directory, create a temporary
	 * file and set the unlink flag.
	 */
	if ((fd = open(fss.fss_bstore, O_CREAT|O_TRUNC|O_WRONLY, 0600)) < 0) {
		if (errno != EISDIR)
			err(1, "create: %s", fss.fss_bstore);
		snprintf(path, sizeof(path), "%s/XXXXXXXXXX", fss.fss_bstore);
		if ((fd = mkstemp(path)) < 0)
			err(1, "mkstemp: %s", path);
		fss.fss_bstore = path;
		istmp = 1;
	}
	if (fstat(fd, &sbuf) < 0)
		err(1, "stat: %s", fss.fss_bstore);
	isreg = S_ISREG(sbuf.st_mode);
	if (isreg && ftruncate(fd, bssize) < 0)
		err(1, "truncate %s", fss.fss_bstore);
	close(fd);

	if ((fd = opendisk(argv[0], O_RDWR, full, sizeof(full), 0)) < 0) {
		if (istmp)
			unlink(fss.fss_bstore);
		err(1, "open: %s", argv[0]);
	}

	if (ioctl(fd, FSSIOCSET, &fss) < 0) {
		if (istmp)
			unlink(fss.fss_bstore);
		err(1, "%s: FSSIOCSET", full);
	}

	if ((xflag || istmp) && isreg && unlink(fss.fss_bstore) < 0)
		err(1, "unlink: %s", fss.fss_bstore);

	if (vflag)
		list(1, argv);
}

void
unconfig(int argc, char **argv)
{
	int fd;
	char full[64];

	if (argc != 1)
		usage();

	if (vflag)
		list(1, argv);

	if ((fd = opendisk(argv[0], O_RDWR, full, sizeof(full), 0)) < 0)
		err(1, "open: %s", argv[0]);

	if (ioctl(fd, FSSIOCCLR) < 0)
		err(1, "%s: FSSIOCCLR", full);
}

void
list(int argc, char **argv)
{
	int n, fd;
	char *dev, path[64], full[64];
	char clbuf[5], bsbuf[5], tmbuf[64];
	time_t t;
	struct fss_get fsg;

	if (argc > 1)
		usage();

	if (argc > 0) 
		dev = argv[0];
	else
		dev = path;

	for (n = 0; ; n++) {
		if (argc == 0)
			snprintf(path, sizeof(path), "fss%d", n);
		if ((fd = opendisk(dev, O_RDONLY, full, sizeof(full), 0)) < 0) {
			if (argc == 0 && (errno == ENOENT || errno == ENXIO))
				break;
			err(1, "open: %s", dev);
		}

		if (ioctl(fd, FSSIOCGET, &fsg) < 0) {
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

			printf("%s: %s, taken %s, %" PRId64 " clusters of %s"
			    ", %s backup\n", dev, fsg.fsg_mount, tmbuf,
			    fsg.fsg_mount_size, clbuf, bsbuf);
		} else
			printf("%s: %s\n", dev, fsg.fsg_mount);

		close(fd);

		if (argc > 0)
			break;
	}
}

void
usage(void)
{
	fprintf(stderr, "%s",
	    "usage: fssconfig [-cx] device path backup [cluster [size]]\n"
	    "       fssconfig -u device\n"
	    "       fssconfig -l [-v] [device]\n");
	exit(1);
}
