/*	$NetBSD: geom.c,v 1.4 2021/01/31 22:45:46 rillig Exp $	*/

/*
 * Copyright (c) 1995, 1997 Jason R. Thorpe.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed for the NetBSD Project
 *	by Jason R. Thorpe.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

/* Modified by Philip A. Nelson for use in sysinst. */

#include <sys/param.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <util.h>
#include <stdint.h>
#include <errno.h>
#include "partutil.h"

#include "defs.h"

bool
disk_ioctl(const char *disk, unsigned long cmd, void *d)
{
	char diskpath[MAXPATHLEN];
	int fd;
	int sv_errno;

	/* Open the disk. */
	fd = opendisk(disk, O_RDONLY, diskpath, sizeof(diskpath), 0);
	if (fd == -1)
		return false;

	if (ioctl(fd, cmd, d) == -1) {
		sv_errno = errno;
		(void)close(fd);
		errno = sv_errno;
		return false;
	}
	(void)close(fd);
	return true;
}

bool
get_wedge_list(const char *disk, struct dkwedge_list *dkwl)
{
	struct dkwedge_info *dkw;
	memset(dkwl, 0, sizeof(*dkwl));

	for (;;) {
		if (!disk_ioctl(disk, DIOCLWEDGES, dkwl))
			goto out;
		if (dkwl->dkwl_nwedges == dkwl->dkwl_ncopied)
			return true;
		dkwl->dkwl_bufsize = dkwl->dkwl_nwedges * sizeof(*dkw);
		dkw = realloc(dkwl->dkwl_buf, dkwl->dkwl_bufsize);
		if (dkw == NULL)
			goto out;
		dkwl->dkwl_buf = dkw;
	}
out:
	free(dkwl->dkwl_buf);
	return false;
}

bool
get_wedge_info(const char *disk, struct dkwedge_info *dkw)
{

	return disk_ioctl(disk, DIOCGWEDGEINFO, dkw);
}

bool
get_disk_geom(const char *disk, struct disk_geom *d)
{
	char buf[MAXPATHLEN];
	int fd, error;

	if ((fd = opendisk(disk, O_RDONLY, buf, sizeof(buf), 0)) == -1)
		return false;

	error = getdiskinfo(disk, fd, NULL, d, NULL);
	close(fd);
	if (error < 0) {
		errno = error;
		return false;
	}
	return true;
}

bool
get_label_geom(const char *disk, struct disklabel *l)
{

	return disk_ioctl(disk, DIOCGDINFO, l);
}
