/*	$NetBSD: vnconfig.c,v 1.44.14.1 2018/03/15 09:12:08 pgoyette Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

/*
 * Copyright (c) 1993 University of Utah.
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 *
 * from: Utah $Hdr: vnconfig.c 1.1 93/12/15$
 *
 *	@(#)vnconfig.c	8.1 (Berkeley) 12/15/93
 */

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/buf.h>
#include <sys/disklabel.h>
#include <sys/disk.h>

#include <dev/vndvar.h>

#include <disktab.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>
#include <paths.h>
#include <limits.h>

#define VND_CONFIG	1
#define VND_UNCONFIG	2
#define VND_GET		3

/* with -l we always print at least this many entries */
#define	DUMMY_FREE	4

static int	verbose = 0;
static int	readonly = 0;
static int	force = 0;
static int	compressed = 0;
static int	minimum = DUMMY_FREE;
static char	*tabname;

static int	show(int, int, const char * const);
static int	config(char *, char *, char *, int);
static int	getgeom(struct vndgeom *, char *);
__dead static void	usage(void);
static void	show_unused(int);

int
main(int argc, char *argv[])
{
	int ch, rv, action = VND_CONFIG;
	char *end;
	unsigned long cnt;

	while ((ch = getopt(argc, argv, "Fcf:lm:rt:uvz")) != -1) {
		switch (ch) {
		case 'F':
			force = 1;
			break;
		case 'c':
			action = VND_CONFIG;
			break;
		case 'f':
			if (setdisktab(optarg) == -1)
				usage();
			break;
		case 'l':
			action = VND_GET;
			break;
		case 'm':
			cnt = strtoul(optarg, &end, 10);
			if (cnt >= INT_MAX || end == optarg || *end != '\0')
				usage();
			minimum = (int)cnt;
			break;
		case 'r':
			readonly = 1;
			break;
		case 't':
			tabname = optarg;
			break;
		case 'u':
			action = VND_UNCONFIG;
			break;
		case 'v':
			verbose = 1;
			break;
		case 'z':
			compressed = 1;
			readonly = 1;
			break;
		default:
		case '?':
			usage();
			/* NOTREACHED */
		}
	}
	argc -= optind;
	argv += optind;

	if (action == VND_CONFIG) {
		if ((argc < 2 || argc > 3) ||
		    (argc == 3 && tabname != NULL))
			usage();
		rv = config(argv[0], argv[1], (argc == 3) ? argv[2] : NULL,
		    action);
	} else if (action == VND_UNCONFIG) {
		if (argc != 1 || tabname != NULL)
			usage();
		rv = config(argv[0], NULL, NULL, action);
	} else { /* VND_GET */
		int n, vdisk;
		const char *vn;
		char path[64];

		if (argc == 0) {
			vn = "vnd0";

			vdisk = opendisk(vn, O_RDONLY, path, sizeof(path), 0);
			if (vdisk == -1) {
				if (minimum == 0)
					return 1;
				err(1, "open: %s", vn);
			}

			for (n = 0; show(vdisk, n, 0); n++)
				continue;
			while (n < minimum)
				show_unused(n++);
			close(vdisk);
			return 0;
		}
			
		rv = 0;
		while (--argc >= 0) {
			vn = *argv++;

			vdisk = opendisk(vn, O_RDONLY, path, sizeof(path), 0);
			if (vdisk == -1) {
				warn("open: %s", vn);
				rv = 1;
				continue;
			}

			if (!show(vdisk, -1, vn))
				rv = 1;
			close(vdisk);
		}
	}
	return rv;
}

static void
show_unused(int n)
{
	if (minimum == 0) 
		return;

	printf("vnd%d: not in use\n", n);
}

static int
show(int v, int n, const char * const name)
{
	struct vnd_user vnu;
	char *dev;
	struct statvfs *mnt;
	int i, nmount;

	vnu.vnu_unit = n;
	if (ioctl(v, VNDIOCGET, &vnu) == -1) {
		if (errno != ENXIO) {
			if (n != -1)
				err(1, "VNDIOCGET");
			warn("%s: VNDIOCGET", name);
		}
		return 0;
	}

	if (vnu.vnu_ino == 0) {
		show_unused(vnu.vnu_unit);
		return -1;
	}

	printf("vnd%d: ", vnu.vnu_unit);

	dev = devname(vnu.vnu_dev, S_IFBLK);
	if (dev != NULL)
		nmount = getmntinfo(&mnt, MNT_NOWAIT);
	else {
		mnt = NULL;
		nmount = 0;
	}

	if (mnt != NULL) {
		for (i = 0; i < nmount; i++) {
			if (strncmp(mnt[i].f_mntfromname, "/dev/", 5) == 0 &&
			    strcmp(mnt[i].f_mntfromname + 5, dev) == 0)
				break;
		}
		if (i < nmount)
			printf("%s (%s) ", mnt[i].f_mntonname,
			    mnt[i].f_mntfromname);
		else
			printf("%s ", dev);
	}
	else if (dev != NULL)
		printf("%s ", dev);
	else
		printf("dev %llu,%llu ",
		    (unsigned long long)major(vnu.vnu_dev),
		    (unsigned long long)minor(vnu.vnu_dev));

	printf("inode %llu\n", (unsigned long long)vnu.vnu_ino);
	return 1;
}

static int
config(char *dev, char *file, char *geom, int action)
{
	struct vnd_ioctl vndio;
	struct disklabel *lp;
	char rdev[MAXPATHLEN + 1];
	int fd, rv;

	fd = opendisk(dev, O_RDWR, rdev, sizeof(rdev), 0);
	if (fd < 0) {
		warn("%s: opendisk", rdev);
		return (1);
	}

	memset(&vndio, 0, sizeof(vndio));
#ifdef __GNUC__
	rv = 0;			/* XXX */
#endif

	vndio.vnd_file = file;
	if (geom != NULL) {
		rv = getgeom(&vndio.vnd_geom, geom);
		if (rv != 0)
			errx(1, "invalid geometry: %s", geom);
		vndio.vnd_flags = VNDIOF_HASGEOM;
	} else if (tabname != NULL) {
		lp = getdiskbyname(tabname);
		if (lp == NULL)
			errx(1, "unknown disk type: %s", tabname);
		vndio.vnd_geom.vng_secsize = lp->d_secsize;
		vndio.vnd_geom.vng_nsectors = lp->d_nsectors;
		vndio.vnd_geom.vng_ntracks = lp->d_ntracks;
		vndio.vnd_geom.vng_ncylinders = lp->d_ncylinders;
		vndio.vnd_flags = VNDIOF_HASGEOM;
	}

	if (readonly)
		vndio.vnd_flags |= VNDIOF_READONLY;

	if (compressed)
		vndio.vnd_flags |= VNF_COMP;

	/*
	 * Clear (un-configure) the device
	 */
	if (action == VND_UNCONFIG) {
		if (force)
			vndio.vnd_flags |= VNDIOF_FORCE;
		rv = ioctl(fd, VNDIOCCLR, &vndio);
#ifdef VNDIOOCCLR
		if (rv && errno == ENOTTY)
			rv = ioctl(fd, VNDIOOCCLR, &vndio);
#endif
		if (rv)
			warn("%s: VNDIOCCLR", rdev);
		else if (verbose)
			printf("%s: cleared\n", rdev);
	}
	/*
	 * Configure the device
	 */
	if (action == VND_CONFIG) {
		int	ffd;

		ffd = open(file, readonly ? O_RDONLY : O_RDWR);
		if (ffd < 0) {
			warn("%s", file);
			rv = -1;
		} else {
			(void) close(ffd);

			rv = ioctl(fd, VNDIOCSET, &vndio);
#ifdef VNDIOOCSET
			if (rv && errno == ENOTTY) {
				rv = ioctl(fd, VNDIOOCSET, &vndio);
				vndio.vnd_size = vndio.vnd_osize;
			}
#endif
			if (rv)
				warn("%s: VNDIOCSET", rdev);
			else if (verbose) {
				printf("%s: %" PRIu64 " bytes on %s", rdev,
				    vndio.vnd_size, file);
				if (vndio.vnd_flags & VNDIOF_HASGEOM)
					printf(" using geometry %d/%d/%d/%d",
					    vndio.vnd_geom.vng_secsize,
					    vndio.vnd_geom.vng_nsectors,
					    vndio.vnd_geom.vng_ntracks,
				    vndio.vnd_geom.vng_ncylinders);
				printf("\n");
			}
		}
	}

	(void) close(fd);
	fflush(stdout);
	return (rv < 0);
}

static int
getgeom(struct vndgeom *vng, char *cp)
{
	char *secsize, *nsectors, *ntracks, *ncylinders;

#define	GETARG(arg) \
	do { \
		if (cp == NULL || *cp == '\0') \
			return (1); \
		arg = strsep(&cp, "/"); \
		if (arg == NULL) \
			return (1); \
	} while (0)

	GETARG(secsize);
	GETARG(nsectors);
	GETARG(ntracks);
	GETARG(ncylinders);

#undef GETARG

	/* Too many? */
	if (cp != NULL)
		return (1);

#define	CVTARG(str, num) \
	do { \
		num = strtol(str, &cp, 10); \
		if (*cp != '\0') \
			return (1); \
	} while (0)

	CVTARG(secsize, vng->vng_secsize);
	CVTARG(nsectors, vng->vng_nsectors);
	CVTARG(ntracks, vng->vng_ntracks);
	CVTARG(ncylinders, vng->vng_ncylinders);

#undef CVTARG

	return (0);
}

static void
usage(void)
{
	const char *p = getprogname();
	(void)fprintf(stderr, 
	    "Usage: %s [-crvz] [-f dsktab] [-t type] vnode_disk"
		" reg-file [geomspec]\n"
	    "       %s -u [-Fv] vnode_disk\n"
	    "       %s -l [-m num | vnode_disk...]\n", p, p, p);
	exit(1);
}
