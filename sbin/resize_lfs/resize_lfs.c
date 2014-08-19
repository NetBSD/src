/*	$NetBSD: resize_lfs.c,v 1.6.26.2 2014/08/20 00:02:27 tls Exp $	*/
/*-
 * Copyright (c) 2005 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Konrad E. Schroder <perseant@hhhh.org>.
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
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/disklabel.h>
#include <sys/disk.h>
#include <sys/file.h>
#include <sys/mount.h>
#include <sys/statvfs.h>

#include <ufs/ufs/dinode.h>
#include <ufs/lfs/lfs.h>

#include <disktab.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "partutil.h"

static void
usage(void)
{
	errx(1, "usage: resize_lfs [-v] [-s new-size] [filesystem]");
}

int
main(int argc, char **argv)
{
	char *rdev, *fsname, buf[LFS_SBPAD];
	size_t rdevlen;
	daddr_t newsize, newnsegs;
	int devfd, rootfd;
	int ch, i, verbose;
	off_t secsize, sboff;
	struct disk_geom geo;
	struct dkwedge_info dkw;
	struct lfs *fs;
	struct statvfs vfs;

	/* Initialize and parse arguments */
	verbose = 0;
	newsize = 0;
	while ((ch = getopt(argc, argv, "s:v")) != -1) {
		switch(ch) {
		case 's':	/* New size, in sectors */
			newsize = strtoll(optarg, NULL, 10);
			break;
		case 'v':
			++verbose;
			break;
		default:
			usage();
		}
	}
	fsname = argv[optind];

	if (fsname == NULL)
		usage();

	/*
	 * If the user did not supply a filesystem size, use the
	 * length of the mounted partition.
	 */
	if (statvfs(fsname, &vfs) < 0)
		err(1, "%s", fsname);
	rdevlen = strlen(vfs.f_mntfromname) + 2;
	rdev = malloc(rdevlen);
	snprintf(rdev, rdevlen, "/dev/r%s", vfs.f_mntfromname + 5);
	devfd = open(rdev, O_RDONLY);
	if (devfd < 0)
		err(1, "open raw device");

	/*
	 * Read the disklabel to find the sector size.  Check the
	 * given size against the partition size.  We can skip some
	 * error checking here since we know the fs is mountable.
	 */
	if (getdiskinfo(rdev, devfd, NULL, &geo, &dkw) == -1)
		err(1, "%s: could not get info", rdev);
	secsize = geo.dg_secsize;
	if (newsize > dkw.dkw_size)
		errx(1, "new size must be <= the partition size");
	if (newsize == 0)
		newsize = dkw.dkw_size;

	/* Open the root of the filesystem so we can fcntl() it */
	rootfd = open(fsname, O_RDONLY);
	if (rootfd < 0)
		err(1, "open filesystem root");

	/* Read the superblock, finding alternates if necessary */
	fs = (struct lfs *)malloc(sizeof(*fs));
	for (sboff = LFS_LABELPAD;;) {
		pread(devfd, buf, sboff, LFS_SBPAD);
		memcpy(&fs->lfs_dlfs, buf, sizeof(struct dlfs));
		if (sboff == LFS_LABELPAD && lfs_fsbtob(fs, 1) > LFS_LABELPAD)
			sboff = lfs_fsbtob(fs, (off_t)fs->lfs_sboffs[0]);
		else
			break;
	}
	close(devfd);

	/* Calculate new number of segments. */
	newnsegs = (newsize * secsize) / fs->lfs_ssize;
	if (newnsegs == fs->lfs_nseg) {
		errx(0, "the filesystem is unchanged.");
	}

	/*
	 * If the new filesystem is smaller than the old, we have to
	 * invalidate the segments that extend beyond the new boundary.
	 * Make the cleaner do this for us.
	 * (XXX make the kernel able to do this instead?)
	 */
	for (i = fs->lfs_nseg - 1; i >= newnsegs; --i) {
		char cmd[128];

		/* If it's already empty, don't call the cleaner */
		if (fcntl(rootfd, LFCNINVAL, &i) == 0)
			continue;

		snprintf(cmd, sizeof(cmd), "/libexec/lfs_cleanerd -q -i %d %s",
			 i, fsname);
		if (system(cmd) != 0)
			err(1, "invalidating segment %d", i);
	}

	/* Tell the filesystem to resize itself. */
	if (fcntl(rootfd, LFCNRESIZE, &newnsegs) == -1) {
		err(1, "resizing");
	}

	if (verbose)
		printf("Successfully resized %s from %d to %lld segments\n",
			fsname, fs->lfs_nseg, (long long)newnsegs);
	return 0;
}
