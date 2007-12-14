/*	$NetBSD: mount_tmpfs.c,v 1.19 2007/12/14 17:37:22 christos Exp $	*/

/*
 * Copyright (c) 2005, 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julio M. Merino Vidal, developed as part of Google's Summer of Code
 * 2005 program.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
#ifndef lint
__RCSID("$NetBSD: mount_tmpfs.c,v 1.19 2007/12/14 17:37:22 christos Exp $");
#endif /* not lint */

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/stat.h>

#include <fs/tmpfs/tmpfs.h>

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
#include "fattr.h"

/* --------------------------------------------------------------------- */

static const struct mntopt mopts[] = {
	MOPT_STDOPTS,
	MOPT_GETARGS,
	MOPT_NULL,
};

/* --------------------------------------------------------------------- */

static int	mount_tmpfs(int argc, char **argv);
static void	usage(void) __attribute__((__noreturn__));

/* --------------------------------------------------------------------- */

int
mount_tmpfs(int argc, char *argv[])
{
	char canon_dir[MAXPATHLEN];
	int gidset, modeset, uidset; /* Ought to be 'bool'. */
	int ch, mntflags;
	gid_t gid;
	uid_t uid;
	mode_t mode;
	int64_t tmpnumber;
	mntoptparse_t mp;
	struct tmpfs_args args;
	struct stat sb;

	setprogname(argv[0]);

	/* Set default values for mount point arguments. */
	args.ta_version = TMPFS_ARGS_VERSION;
	args.ta_size_max = 0;
	args.ta_nodes_max = 0;
	mntflags = 0;

	gidset = 0; gid = 0;
	uidset = 0; uid = 0;
	modeset = 0; mode = 0;

	optind = optreset = 1;
	while ((ch = getopt(argc, argv, "g:m:n:o:s:u:")) != -1 ) {
		switch (ch) {
		case 'g':
			gid = a_gid(optarg);
			gidset = 1;
			break;

		case 'm':
			mode = a_mask(optarg);
			modeset = 1;
			break;

		case 'n':
			if (dehumanize_number(optarg, &tmpnumber) == -1)
				err(EXIT_FAILURE, "failed to parse nodes `%s'",
				    optarg);
			args.ta_nodes_max = tmpnumber;
			break;

		case 'o':
			mp = getmntopts(optarg, mopts, &mntflags, 0);
			if (mp == NULL)
				err(EXIT_FAILURE, "getmntopts");
			freemntopts(mp);
			break;

		case 's':
			if (dehumanize_number(optarg, &tmpnumber) == -1)
				err(EXIT_FAILURE, "failed to parse size `%s'",
				    optarg);
			args.ta_size_max = tmpnumber;
			break;

		case 'u':
			uid = a_uid(optarg);
			uidset = 1;
			break;

		case '?':
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (argc != 2)
		usage();

	if (realpath(argv[1], canon_dir) == NULL)
		err(EXIT_FAILURE, "realpath %s", argv[0]);

	if (strncmp(argv[1], canon_dir, MAXPATHLEN) != 0) {
		warnx("\"%s\" is a relative path", argv[0]);
		warnx("using \"%s\" instead", canon_dir);
	}

	if (stat(canon_dir, &sb) == -1)
		err(EXIT_FAILURE, "cannot stat `%s'", canon_dir);

	args.ta_root_uid = uidset ? uid : sb.st_uid;
	args.ta_root_gid = gidset ? gid : sb.st_gid;
	args.ta_root_mode = modeset ? mode : sb.st_mode;

	if (mount(MOUNT_TMPFS, canon_dir, mntflags, &args, sizeof args) == -1)
		err(EXIT_FAILURE, "tmpfs on %s", canon_dir);

	if (mntflags & MNT_GETARGS) {
		struct passwd *pw;
		struct group *gr;

		(void)printf("version=%d, ", args.ta_version);
		(void)printf("size_max=%" PRIuMAX ", ",
		    (uintmax_t)args.ta_size_max);
		(void)printf("nodes_max=%" PRIuMAX ", ",
		    (uintmax_t)args.ta_nodes_max);

		pw = getpwuid(args.ta_root_uid);
		if (pw == NULL)
			(void)printf("root_uid=%" PRIuMAX ", ",
			    (uintmax_t)args.ta_root_uid);
		else
			(void)printf("root_uid=%s, ", pw->pw_name);

		gr = getgrgid(args.ta_root_gid);
		if (gr == NULL)
			(void)printf("root_gid=%" PRIuMAX ", ",
			    (uintmax_t)args.ta_root_gid);
		else
			(void)printf("root_gid=%s, ", gr->gr_name);

		(void)printf("root_mode=%o\n", args.ta_root_mode);
	}

	return EXIT_SUCCESS;
}

/* --------------------------------------------------------------------- */

static void
usage(void)
{
	(void)fprintf(stderr,
	    "Usage: %s [-g group] [-m mode] [-n nodes] [-o options] [-s size]\n"
	    "           [-u user] tmpfs mountpoint\n", getprogname());
	exit(1);
}

/* --------------------------------------------------------------------- */

#ifndef MOUNT_NOMAIN
int
main(int argc, char *argv[])
{

	return mount_tmpfs(argc, argv);
}
#endif
