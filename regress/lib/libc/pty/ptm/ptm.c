/*	$NetBSD: ptm.c,v 1.5 2008/04/28 20:23:05 martin Exp $	*/

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
__RCSID("$NetBSD: ptm.c,v 1.5 2008/04/28 20:23:05 martin Exp $");

#include <stdlib.h>
#include <unistd.h>
#include <grp.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

int
main(int argc, char *argv[])
{
	struct stat stm, sts;
	struct ptmget ptm;
	char *pty;
	int fdm, fds;
	struct group *gp;

	if ((fdm = open("/dev/ptm", O_RDWR)) == -1) {
		if (errno == ENOENT || errno == ENODEV)
			return 0;
		err(1, "open multiplexor");
	}

	if (fstat(fdm, &stm) == -1)
		err(1, "fstat multiplexor");

	if (major(stm.st_rdev) != 165)
		errx(1, "bad multiplexor major number %d", major(stm.st_rdev));

	if (ioctl(fdm, TIOCPTMGET, &ptm) == -1)
		err(1, "ioctl(TIOCPTMGET)");

	if (strncmp(ptm.cn, "/dev/pty", 8) != 0)
		if (strncmp(ptm.cn, "/dev/null", 9) != 0)
			errx(1, "bad master name %s", ptm.cn);

	if (strncmp(ptm.sn, "/dev/tty", 8) != 0)
		if (strncmp(ptm.sn, "/dev/pts/", 9) != 0)
			errx(1, "bad slave name %s", ptm.sn);

	if (strncmp(ptm.cn, "/dev/null", 9) != 0) {
		if (fstat(ptm.cfd, &stm) == -1)
			err(1, "fstat master");
		if (stat(ptm.cn, &sts) == -1)
			err(1, "stat master");
		if (stm.st_rdev != sts.st_rdev)
			errx(1, "master device mismatch %lu != %lu",
			    (unsigned long)stm.st_rdev,
			    (unsigned long)sts.st_rdev);
	}

	if (fstat(ptm.sfd, &stm) == -1)
		err(1, "fstat slave");
	if (stat(ptm.sn, &sts) == -1)
		err(1, "stat slave");
	if (stm.st_rdev != sts.st_rdev)
		errx(1, "slave device mismatch %lu != %lu",
		    (unsigned long)stm.st_rdev, (unsigned long)sts.st_rdev);

	if (sts.st_uid != getuid())
		errx(1, "bad slave uid %lu != %lu", (unsigned long)stm.st_uid,
		    getuid());
	if ((gp = getgrnam("tty")) == NULL)
		errx(1, "cannot find `tty' group");
	if (sts.st_gid != gp->gr_gid)
		errx(1, "bad slave gid %lu != %lu", (unsigned long)stm.st_gid,
		    gp->gr_gid);
	(void)close(ptm.sfd);
	(void)close(ptm.cfd);
	(void)close(fdm);
	return 0;
}
