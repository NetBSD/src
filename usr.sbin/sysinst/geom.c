/*	$NetBSD: geom.c,v 1.1 2014/07/26 19:30:44 dholland Exp $	*/

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
#include <sys/disklabel.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <util.h>
#include <errno.h>

#include "defs.h"

static int
get_label(const char *disk, struct disklabel *l, unsigned long cmd)
{
	char diskpath[MAXPATHLEN];
	int fd;
	int sv_errno;

	/* Open the disk. */
	fd = opendisk(disk, O_RDONLY, diskpath, sizeof(diskpath), 0);
	if (fd < 0) 
		return 0;

	if (ioctl(fd, cmd, l) < 0) {
		sv_errno = errno;
		(void)close(fd);
		errno = sv_errno;
		return 0;
	}
	(void)close(fd);
	return 1;
}

int
get_geom(const char *disk, struct disklabel *l)
{

	return get_label(disk, l, DIOCGDEFLABEL);
}

int
get_real_geom(const char *disk, struct disklabel *l)
{

	return get_label(disk, l, DIOCGDINFO);
}
