/*	$NetBSD: mount_ext2fs.c,v 1.19 2007/07/16 17:06:52 pooka Exp $	*/

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
__COPYRIGHT("@(#) Copyright (c) 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)mount_ufs.c	8.4 (Berkeley) 4/26/95";
#else
__RCSID("$NetBSD: mount_ext2fs.c,v 1.19 2007/07/16 17:06:52 pooka Exp $");
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

static void	ext2fs_usage(void);
int	mount_ext2fs(int argc, char **argv);

static const struct mntopt mopts[] = {
	MOPT_STDOPTS,
	MOPT_ASYNC,
	MOPT_SYNC,
	MOPT_UPDATE,
	MOPT_RELOAD,
	MOPT_FORCE,
	MOPT_GETARGS,
	MOPT_NODEVMTIME,
	MOPT_NULL,
};

#ifndef MOUNT_NOMAIN
int
main(int argc, char *argv[])
{
	return mount_ext2fs(argc, argv);
}
#endif

int
mount_ext2fs(int argc, char *argv[])
{
	struct ufs_args args;		/* XXX ffs_args */
	int ch, mntflags;
	char fs_name[MAXPATHLEN], canon_dev[MAXPATHLEN];
	const char *errcause;
	mntoptparse_t mp;

	mntflags = 0;
	optind = optreset = 1;		/* Reset for parse of new argv. */
	while ((ch = getopt(argc, argv, "o:")) != -1)
		switch (ch) {
		case 'o':
			mp = getmntopts(optarg, mopts, &mntflags, 0);
			if (mp == NULL)
				err(1, "getmntopts");
			freemntopts(mp);
			break;
		case '?':
		default:
			ext2fs_usage();
		}
	argc -= optind;
	argv += optind;

	if (argc != 2)
		ext2fs_usage();

	if (realpath(argv[0], canon_dev) == NULL)     /* Check device path */
		err(1, "realpath %s", argv[0]);
	if (strncmp(argv[0], canon_dev, MAXPATHLEN)) {
		warnx("\"%s\" is a relative path.", argv[0]);
		warnx("using \"%s\" instead.", canon_dev);
	}
	args.fspec = canon_dev;

	if (realpath(argv[1], fs_name) == NULL)       /* Check mounton path */
		err(1, "realpath %s", argv[1]);
	if (strncmp(argv[1], fs_name, MAXPATHLEN)) {
		warnx("\"%s\" is a relative path.", argv[1]);
		warnx("using \"%s\" instead.", fs_name);
	}

	if (mount(MOUNT_EXT2FS, fs_name, mntflags, &args, sizeof args) == -1) {
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
ext2fs_usage(void)
{
	(void)fprintf(stderr,
		"usage: mount_ext2fs [-o options] special node\n");
	exit(1);
}
