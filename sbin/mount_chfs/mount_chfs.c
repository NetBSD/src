/*	$NetBSD: mount_chfs.c,v 1.1.8.1 2014/08/20 00:02:26 tls Exp $	*/

/*-
 * Copyright (c) 2010 Department of Software Engineering,
 *		      University of Szeged, Hungary
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by the Department of Software Engineering, University of Szeged, Hungary
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

#include <sys/cdefs.h>
#ifndef lint
#endif /* not lint */

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <ufs/ufs/ufsmount.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <grp.h>
#include <mntopts.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mountprog.h"
#include "mount_chfs.h"

/* --------------------------------------------------------------------- */

static void	usage(void) __dead;

/* --------------------------------------------------------------------- */

void
mount_chfs_parseargs(int argc, char *argv[], struct ufs_args *args,
    int *mntflags, char *canon_dev, char *canon_dir)
{
	int ch;
	struct stat sb;

	/* Set default values for mount point arguments. */
	memset(args, 0, sizeof(*args));
	*mntflags = 0;

	optind = optreset = 1;

	while ((ch = getopt(argc, argv, "i:")) != -1) {
		switch (ch) {
		case '?':
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (argc != 2)
		usage();

	//strlcpy(canon_dev, argv[0], MAXPATHLEN);
	pathadj(argv[0], canon_dev);
	pathadj(argv[1], canon_dir);

	args->fspec = canon_dev;

	if (stat(canon_dir, &sb) == -1) {
		err(EXIT_FAILURE, "cannot stat `%s'", canon_dir);
	}

}

/* --------------------------------------------------------------------- */

static void
usage(void)
{
	(void)fprintf(stderr,
	    "usage: %s special mountpath\n",
	    getprogname());
	exit(1);
}

/* --------------------------------------------------------------------- */

int
mount_chfs(int argc, char *argv[])
{
	struct ufs_args args;
	char canon_dev[MAXPATHLEN], fs_name[MAXPATHLEN];
	int mntflags;
	

	mount_chfs_parseargs(argc, argv, &args, &mntflags,
	    canon_dev, fs_name);

	if (mount(MOUNT_CHEWIEFS, fs_name, mntflags, &args, sizeof args) == -1) {
		err(EXIT_FAILURE, "chfs on %s", fs_name);
	}

	if (mntflags & MNT_GETARGS) {

		//(void)printf("flash index=%d\n",  args.fl_index);
	}

	return EXIT_SUCCESS;
}

#ifndef MOUNT_NOMAIN
int
main(int argc, char *argv[])
{
	setprogname(argv[0]);
	return mount_chfs(argc, argv);
}
#endif
