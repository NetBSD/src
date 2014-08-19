
/*	$NetBSD: mount_tmpfs.c,v 1.24.24.2 2014/08/20 00:02:26 tls Exp $	*/

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
__RCSID("$NetBSD: mount_tmpfs.c,v 1.24.24.2 2014/08/20 00:02:26 tls Exp $");
#endif /* not lint */

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/sysctl.h>

#include <fs/tmpfs/tmpfs_args.h>

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
#include "mount_tmpfs.h"

/* --------------------------------------------------------------------- */

static const struct mntopt mopts[] = {
	MOPT_UPDATE,
	MOPT_STDOPTS,
	MOPT_GETARGS,
	MOPT_NULL,
};

/* --------------------------------------------------------------------- */

static void	usage(void) __dead;
static int64_t	ram_fract(const char *arg);
static int64_t	ram_percent(const char *arg);
static int64_t	ram_factor(float f);

/* --------------------------------------------------------------------- */

/* return f * available system ram */
static int64_t
ram_factor(float f)
{
	uint64_t ram;
	size_t len;

	len = sizeof(ram);
	if (sysctlbyname("hw.physmem64", &ram, &len, NULL, 0))
		err(EXIT_FAILURE, "can't get \"hw.physmem64\"");

	return (int64_t)((float)ram * f);
}

/* return fraction of available ram given by arg */
static int64_t
ram_fract(const char *arg)
{
	char *endp;
	float f;

	f = strtof(arg, &endp);
	if (endp && *endp != 0)
		errx(EXIT_FAILURE, "syntax error in ram fraction: ram/%s"
		    " at %s", arg, endp);
	if (f <= 0.0f)
		errx(EXIT_FAILURE, "ram fraction must be a positive number:"
		     " ram/%s", arg);

	return ram_factor(1.0f/f);
}

/* --------------------------------------------------------------------- */

/* return percentage of available ram given by arg */
static int64_t
ram_percent(const char *arg)
{
	char *endp;
	float f;

	f = strtof(arg, &endp);
	if (endp && *endp != 0)
		errx(EXIT_FAILURE, "syntax error in ram percentage: ram%%%s"
		    " at %s", arg, endp);
	if (f <= 0.0f || f >= 100.0f)
		errx(EXIT_FAILURE, "ram percentage must be a between 0 and 100"
		     " ram%%%s", arg);

	return ram_factor(f/100.0f);
}

/* --------------------------------------------------------------------- */

void
mount_tmpfs_parseargs(int argc, char *argv[],
	struct tmpfs_args *args, int *mntflags,
	char *canon_dev, char *canon_dir)
{
	int gidset, modeset, uidset; /* Ought to be 'bool'. */
	int ch;
	gid_t gid;
	uid_t uid;
	mode_t mode;
	int64_t tmpnumber;
	mntoptparse_t mp;
	struct stat sb;

	/* Set default values for mount point arguments. */
	memset(args, 0, sizeof(*args));
	args->ta_version = TMPFS_ARGS_VERSION;
	args->ta_size_max = 0;
	args->ta_nodes_max = 0;
	*mntflags = 0;

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
			args->ta_nodes_max = tmpnumber;
			break;

		case 'o':
			mp = getmntopts(optarg, mopts, mntflags, 0);
			if (mp == NULL)
				err(EXIT_FAILURE, "getmntopts");
			freemntopts(mp);
			break;

		case 's':
			if (strncmp(optarg, "ram/", 4) == 0)
				tmpnumber = ram_fract(optarg+4);
			else if (strncmp(optarg, "ram%", 4) == 0)
				tmpnumber = ram_percent(optarg+4);
			else if (dehumanize_number(optarg, &tmpnumber) == -1)
				err(EXIT_FAILURE, "failed to parse size `%s'",
				    optarg);
			args->ta_size_max = tmpnumber;
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

	strlcpy(canon_dev, argv[0], MAXPATHLEN);
	pathadj(argv[1], canon_dir);

	if (stat(canon_dir, &sb) == -1)
		err(EXIT_FAILURE, "cannot stat `%s'", canon_dir);

	args->ta_root_uid = uidset ? uid : sb.st_uid;
	args->ta_root_gid = gidset ? gid : sb.st_gid;
	args->ta_root_mode = modeset ? mode : sb.st_mode;
}

/* --------------------------------------------------------------------- */

static void
usage(void)
{
	(void)fprintf(stderr,
	    "usage: %s [-g group] [-m mode] [-n nodes] [-o options] [-s size]\n"
	    "           [-u user] tmpfs mount_point\n", getprogname());
	exit(1);
}

/* --------------------------------------------------------------------- */

int
mount_tmpfs(int argc, char *argv[])
{
	struct tmpfs_args args;
	char canon_dev[MAXPATHLEN], canon_dir[MAXPATHLEN];
	int mntflags;

	mount_tmpfs_parseargs(argc, argv, &args, &mntflags,
	    canon_dev, canon_dir);

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

#ifndef MOUNT_NOMAIN
int
main(int argc, char *argv[])
{

	setprogname(argv[0]);
	return mount_tmpfs(argc, argv);
}
#endif
