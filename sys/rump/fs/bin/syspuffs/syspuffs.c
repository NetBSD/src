/*	$NetBSD: syspuffs.c,v 1.1.6.2 2008/01/21 09:47:39 yamt Exp $	*/

/*
 * Copyright (c) 2008 Antti Kantee.  All Rights Reserved.
 *
 * Development of this software was supported by the
 * Research Foundation of Helsinki University of Technology
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/mount.h>
#include <sys/socket.h>

#include <assert.h>
#include <err.h>
#include <paths.h>
#include <puffs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "p2k.h"
#include "puffs_rumpglue.h"

static void
usage(void)
{

	errx(1, "usage: %s file_server [fs opts] dev mountpath", getprogname());
}

int
main(int argc, char *argv[])
{
	struct puffs_kargs kargs;
	extern char **environ;
	char *mntpath, *fromname;
	size_t len;
	char comfd[16];
	int sv[2];
	int mntflags, pflags, rv;

#if 1
	extern int puffsdebug;
	extern int putterdebug;

	puffsdebug = putterdebug = 1;
#endif

	setprogname(argv[0]);

	if (argc < 2)
		usage();

	/* Create sucketpair for communication with the real file server */
	if (socketpair(PF_LOCAL, SOCK_STREAM, 0, sv) == -1)
		err(1, "socketpair");
	snprintf(comfd, sizeof(sv[0]), "%d", sv[0]);
	if (setenv("PUFFS_COMFD", comfd, 1) == -1)
		err(1, "setenv");

	switch (fork()) {
	case 0:
		argv++;
		if (execve(argv[0], argv, environ) == -1)
			err(1, "execvp");
		/*NOTREACHED*/
	case -1:
		err(1, "fork");
		/*NOTREACHED*/
	default:
		unsetenv("PUFFS_COMFD");
		break;
	}

	/* read args */
	if (read(sv[1], &len, sizeof(len)) != sizeof(len))
		err(1, "mp 1");
	mntpath = malloc(len);
	if (mntpath == NULL)
		err(1, "malloc %zu", len);
	if (read(sv[1], mntpath, len) != len)
		err(1, "mp 2");
	if (read(sv[1], &len, sizeof(len)) != sizeof(len))
		err(1, "fn 1");
	fromname = malloc(len);
	if (fromname == NULL)
		err(1, "malloc %zu", len);
	if (read(sv[1], fromname, len) != len)
		err(1, "fn 2");
	if (read(sv[1], &mntflags, sizeof(mntflags)) != sizeof(mntflags))
		err(1, "mntflags");
	if (read(sv[1], &kargs, sizeof(kargs)) != sizeof(kargs))
		err(1, "puffs_args");
	if (read(sv[1], &pflags, sizeof(kargs)) != sizeof(pflags))
		err(1, "pflags");

	/* XXX: some adjustments */
	pflags |= PUFFS_KFLAG_NOCACHE;
	pflags &= ~PUFFS_FLAG_BUILDPATH;

	rv = puffs_rumpglue_init(sv[1], &kargs.pa_fd);
	assert(rv == 0);
	rv = p2k_run_fs(MOUNT_PUFFS, fromname, mntpath, mntflags, 
		&kargs, sizeof(kargs), pflags);
	if (rv)
		err(1, "mount");

	return 0;
}
