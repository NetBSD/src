/*	$NetBSD: snapshot.c,v 1.4 2008/04/28 20:23:08 martin Exp $	*/

/*-
 * Copyright (c) 2005 The NetBSD Foundation, Inc.
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

#include <dev/fssvar.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "snapshot.h"

/*
 * Create a snapshot of the file system currently mounted on the first argument
 * using the second argument as backing store and return an open file
 * descriptor for the snapshot.  If the second argument is NULL, use the first
 * as backing store.  If the third argument is not NULL, it gets the time the
 * snapshot was created.  If the fourth argument is not NULL, it gets the
 * snapshot device path.
 */
int
snap_open(char *mountpoint, char *backup, time_t *snap_date, char **snap_dev)
{
	int i, fd, israw, fsinternal, dounlink, flags;
	char path[MAXPATHLEN], fss_dev[14];
	dev_t mountdev;
	struct fss_set fss;
	struct fss_get fsg;
	struct stat sb;
	struct statvfs fsb;

	dounlink = 0;
	fd = -1;

	fss.fss_mount = mountpoint;
	fss.fss_bstore = backup ? backup : fss.fss_mount;
	fss.fss_csize = 0;

	if (stat(fss.fss_mount, &sb) < 0)
		goto fail;
	mountdev = sb.st_dev;

	/*
	 * Prepare the backing store. `backup' is either a raw device,
	 * a file or a directory.  If it is a file, it must not exist.
	 */
	israw = 0;
	if (stat(fss.fss_bstore, &sb) == 0) {
		if (S_ISDIR(sb.st_mode)) {
			snprintf(path, sizeof(path),
			    "%s/XXXXXXXXXX", fss.fss_bstore);
			fd = mkstemp(path);
			fss.fss_bstore = path;
			dounlink = 1;
		} else if (S_ISCHR(sb.st_mode)) {
			fd = open(fss.fss_bstore, O_RDWR);
			israw = 1;
		} else
			goto fail;
	} else {
		fd = open(fss.fss_bstore, O_CREAT|O_EXCL|O_WRONLY, 0600);
		dounlink = 1;
	}
	if (fd < 0)
		goto fail;

	if (fstat(fd, &sb) < 0)
		goto fail;
	fsinternal = (!israw && sb.st_dev == mountdev);

	/*
	 * If the backing store is a plain file and the snapshot
	 * is not file system internal, truncate to file system
	 * free space.
	 */
	if (!israw && !fsinternal) {
		if (statvfs(fss.fss_bstore, &fsb) < 0)
			goto fail;
		if (ftruncate(fd, (off_t)fsb.f_frsize*fsb.f_bavail) < 0)
			goto fail;
	}

	if (close(fd) < 0)
		goto fail;

	/*
	 * Create the snapshot on the first free snapshot device.
	 */
	for (i = 0; ; i++) {
		snprintf(fss_dev, sizeof(fss_dev), "/dev/rfss%d", i);
		if ((fd = open(fss_dev, O_RDWR, 0)) < 0)
			goto fail;

		if (ioctl(fd, FSSIOFGET, &flags) < 0)
			goto fail;

		if (ioctl(fd, FSSIOCSET, &fss) < 0) {
			if (errno != EBUSY)
				goto fail;
			close(fd);
			fd = -1;
			continue;
		}

		if (snap_dev != NULL) {
			*snap_dev = strdup(fss_dev);
			if (*snap_dev == NULL) {
				ioctl(fd, FSSIOCCLR);
				goto fail;
			}
		}

		flags |= FSS_UNCONFIG_ON_CLOSE;
		if (ioctl(fd, FSSIOCGET, &fsg) < 0 ||
		    ioctl(fd, FSSIOFSET, &flags) < 0 ||
		    (!israw && unlink(fss.fss_bstore) < 0)) {
			ioctl(fd, FSSIOCCLR);
			goto fail;
		}

		if (snap_date != NULL)
			*snap_date = fsg.fsg_time.tv_sec;
		return fd;
	}

fail:
	if (dounlink)
		unlink(fss.fss_bstore);
	if (fd >= 0)
		close(fd);

	return -1;
}
