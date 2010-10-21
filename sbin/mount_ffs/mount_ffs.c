/*	$NetBSD: mount_ffs.c,v 1.25.10.3 2010/10/21 08:45:04 uebayasi Exp $	*/

/*-
 * Copyright (c) 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
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
 */

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1993, 1994\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)mount_ufs.c	8.4 (Berkeley) 4/26/95";
#else
__RCSID("$NetBSD: mount_ffs.c,v 1.25.10.3 2010/10/21 08:45:04 uebayasi Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/mount.h>
#include <ufs/ufs/ufsmount.h>

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <unistd.h>

#include <mntopts.h>

#include "mountprog.h"
#include "mount_ffs.h"

static void	ffs_usage(void);

static const struct mntopt mopts[] = {
	MOPT_STDOPTS,
	MOPT_ASYNC,
	MOPT_SYNC,
	MOPT_UPDATE,
	MOPT_RELOAD,
	MOPT_NOATIME,
	MOPT_NODEVMTIME,
	MOPT_FORCE,
	MOPT_SOFTDEP,
	MOPT_LOG,
	MOPT_GETARGS,
	MOPT_XIP,
	MOPT_NULL,
};

#ifndef MOUNT_NOMAIN
int
main(int argc, char **argv)
{

	setprogname(argv[0]);
	return mount_ffs(argc, argv);
}
#endif

void
mount_ffs_parseargs(int argc, char *argv[],
	struct ufs_args *args, int *mntflags,
	char *canon_dev, char *canon_dir)
{
	int ch;
	mntoptparse_t mp;

	memset(args, 0, sizeof(*args));
	*mntflags = 0;
	optind = optreset = 1;		/* Reset for parse of new argv. */
	while ((ch = getopt(argc, argv, "o:")) != -1)
		switch (ch) {
		case 'o':
			mp = getmntopts(optarg, mopts, mntflags, 0);
			if (mp == NULL)
				err(1, "getmntopts");
			freemntopts(mp);
			break;
		case '?':
		default:
			ffs_usage();
		}
	argc -= optind;
	argv += optind;

	if (argc != 2)
		ffs_usage();

	pathadj(argv[0], canon_dev);
	args->fspec = canon_dev;

	pathadj(argv[1], canon_dir);
}

int
mount_ffs(int argc, char *argv[])
{
	char fs_name[MAXPATHLEN], canon_dev[MAXPATHLEN];
	struct ufs_args args;
	const char *errcause;
	int mntflags;

	mount_ffs_parseargs(argc, argv, &args, &mntflags, canon_dev, fs_name);

	if (mount(MOUNT_FFS, fs_name, mntflags, &args, sizeof args) == -1) {
		switch (errno) {
		case EMFILE:
			errcause = "mount table full";
			break;
		case EINVAL:
			if (mntflags & MNT_UPDATE)
				errcause =
			    "specified device does not match mounted device";
			else 
				errcause = "incorrect super block";
			break;
		default:
			errcause = strerror(errno);
			break;
		}
		errx(1, "%s on %s: %s", args.fspec, fs_name, errcause);
	}
	exit(0);
}

static void
ffs_usage(void)
{
	fprintf(stderr, "usage: %s [-o options] special node\n", getprogname());
	exit(1);
}
