/*	$NetBSD: mount_tmpfs.c,v 1.14 2006/05/27 09:14:17 yamt Exp $	*/

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
__RCSID("$NetBSD: mount_tmpfs.c,v 1.14 2006/05/27 09:14:17 yamt Exp $");
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

/* --------------------------------------------------------------------- */

static const struct mntopt mopts[] = {
	MOPT_STDOPTS,
	MOPT_GETARGS,
	{ NULL }
};

/* --------------------------------------------------------------------- */

static int	mount_tmpfs(int argc, char **argv);
static void	usage(void);
static int	dehumanize_group(const char *str, gid_t *gid);
static int	dehumanize_mode(const char *str, mode_t *mode);
static int	dehumanize_off(const char *str, off_t *size);
static int	dehumanize_user(const char *str, uid_t *uid);

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
	off_t offtmp;
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
			if (!dehumanize_group(optarg, &gid)) {
				errx(EXIT_FAILURE, "failed to parse group "
				    "'%s'", optarg);
				/* NOTREACHED */
			}
			gidset = 1;
			break;

		case 'm':
			if (!dehumanize_mode(optarg, &mode)) {
				errx(EXIT_FAILURE, "failed to parse mode "
				    "'%s'", optarg);
				/* NOTREACHED */
			}
			modeset = 1;
			break;

		case 'n':
			if (!dehumanize_off(optarg, &offtmp)) {
				errx(EXIT_FAILURE, "failed to parse size "
				    "'%s'", optarg);
				/* NOTREACHED */
			}
			args.ta_nodes_max = offtmp;
			break;

		case 'o':
			mp = getmntopts(optarg, mopts, &mntflags, 0);
			if (mp == NULL)
				err(1, "getmntopts");
			freemntopts(mp);
			break;

		case 's':
			if (!dehumanize_off(optarg, &offtmp)) {
				errx(EXIT_FAILURE, "failed to parse size "
				    "'%s'", optarg);
				/* NOTREACHED */
			}
			args.ta_size_max = offtmp;
			break;

		case 'u':
			if (!dehumanize_user(optarg, &uid)) {
				errx(EXIT_FAILURE, "failed to parse user "
				    "'%s'", optarg);
				/* NOTREACHED */
			}
			uidset = 1;
			break;

		case '?':
		default:
			usage();
			/* NOTREACHED */
		}
	}
	argc -= optind;
	argv += optind;

	if (argc != 2) {
		usage();
		/* NOTREACHED */
	}

	if (realpath(argv[1], canon_dir) == NULL) {
		err(EXIT_FAILURE, "realpath %s", argv[0]);
		/* NOTREACHED */
	}

	if (strncmp(argv[1], canon_dir, MAXPATHLEN) != 0) {
		warnx("\"%s\" is a relative path", argv[0]);
		warnx("using \"%s\" instead", canon_dir);
	}

	if (stat(canon_dir, &sb) == -1) {
		err(EXIT_FAILURE, "cannot stat");
		/* NOTREACHED */
	}
	args.ta_root_uid = uidset ? uid : sb.st_uid;
	args.ta_root_gid = gidset ? gid : sb.st_gid;
	args.ta_root_mode = modeset ? mode : sb.st_mode;

	if (mount(MOUNT_TMPFS, canon_dir, mntflags, &args)) {
		err(EXIT_FAILURE, "tmpfs on %s", canon_dir);
		/* NOTREACHED */
	}
	if (mntflags & MNT_GETARGS) {
		struct passwd *pw;
		struct group *gr;

		(void)printf("version=%d\n", args.ta_version);
		(void)printf("size_max=%" PRIuMAX "\n",
		    (uintmax_t)args.ta_size_max);
		(void)printf("nodes_max=%" PRIuMAX "\n",
		    (uintmax_t)args.ta_nodes_max);

		pw = getpwuid(args.ta_root_uid);
		if (pw == NULL)
			(void)printf("root_uid=%" PRIuMAX "\n",
			    (uintmax_t)args.ta_root_uid);
		else
			(void)printf("root_uid=%s\n", pw->pw_name);

		gr = getgrgid(args.ta_root_gid);
		if (gr == NULL)
			(void)printf("root_gid=%" PRIuMAX "\n",
			    (uintmax_t)args.ta_root_gid);
		else
			(void)printf("root_gid=%s\n", gr->gr_name);

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

/*
 * Obtains a GID number based on the contents of 'str'.  If it is a string
 * and is found in group(5), its corresponding ID is used.  Otherwise, an
 * attempt is made to convert the string to a number and use that as a GID.
 * In case of success, true is returned and *gid holds the GID value.
 * Otherwise, false is returned and *gid is untouched.
 */
static int
dehumanize_group(const char *str, gid_t *gid)
{
	int error;
	struct group *gr;

	gr = getgrnam(str);
	if (gr != NULL) {
		*gid = gr->gr_gid;
		error = 1;
	} else {
		char *ep;
		unsigned long tmp;

		errno = 0;
		tmp = strtoul(str, &ep, 0);
		if (str[0] == '\0' || *ep != '\0')
			error = 0; /* Not a number. */
		else if (errno == ERANGE)
			error = 0; /* Out of range. */
		else {
			*gid = (gid_t)tmp;
			error = 1;
		}
	}

	return error;
}

/* --------------------------------------------------------------------- */

/*
 * Obtains a mode number based on the contents of 'str'.
 * In case of success, true is returned and *mode holds the mode value.
 * Otherwise, false is returned and *mode is untouched.
 */
static int
dehumanize_mode(const char *str, mode_t *mode)
{
	char *ep;
	int error;
	long tmp;

	errno = 0;
	tmp = strtol(str, &ep, 8);
	if (str[0] == '\0' || *ep != '\0')
		error = 0; /* Not a number. */
	else if (errno == ERANGE)
		error = 0; /* Out of range. */
	else {
		*mode = (mode_t)tmp;
		error = 1;
	}

	return error;
}

/* --------------------------------------------------------------------- */

/*
 * Converts the number given in 'str', which may be given in a humanized
 * form (as described in humanize_number(3), but with some limitations),
 * to a file size (off_t) without units.
 * In case of success, true is returned and *size holds the value.
 * Otherwise, false is returned and *size is untouched.
 */
static int
dehumanize_off(const char *str, off_t *size)
{
	char *ep, unit;
	const char *delimit;
	long multiplier;
	long long tmp, tmp2;
	size_t len;

	len = strlen(str);
	if (len < 1)
		return 0;

	multiplier = 1;

	unit = str[len - 1];
	if (isalpha((int)unit)) {
		switch (tolower((int)unit)) {
		case 'b':
			multiplier = 1;
			break;

		case 'k':
			multiplier = 1024;
			break;

		case 'm':
			multiplier = 1024 * 1024;
			break;

		case 'g':
			multiplier = 1024 * 1024 * 1024;
			break;

		default:
			return 0; /* Invalid suffix. */
		}

		delimit = &str[len - 1];
	} else
		delimit = NULL;

	errno = 0;
	tmp = strtoll(str, &ep, 10);
	if (str[0] == '\0' || (ep != delimit && *ep != '\0'))
		return 0; /* Not a number. */
	else if (errno == ERANGE)
		return 0; /* Out of range. */

	tmp2 = tmp * multiplier;
	tmp2 = tmp2 / multiplier;
	if (tmp != tmp2)
		return 0; /* Out of range. */
	*size = tmp * multiplier;

	return 1;
}

/* --------------------------------------------------------------------- */

/*
 * Obtains a UID number based on the contents of 'str'.  If it is a string
 * and is found in passwd(5), its corresponding ID is used.  Otherwise, an
 * attempt is made to convert the string to a number and use that as a UID.
 * In case of success, true is returned and *uid holds the UID value.
 * Otherwise, false is returned and *uid is untouched.
 */
static int
dehumanize_user(const char *str, uid_t *uid)
{
	int error;
	struct passwd *pw;

	pw = getpwnam(str);
	if (pw != NULL) {
		*uid = pw->pw_uid;
		error = 1;
	} else {
		char *ep;
		unsigned long tmp;

		errno = 0;
		tmp = strtoul(str, &ep, 0);
		if (str[0] == '\0' || *ep != '\0')
			error = 0; /* Not a number. */
		else if (errno == ERANGE)
			error = 0; /* Out of range. */
		else {
			*uid = (uid_t)tmp;
			error = 1;
		}
	}

	return error;
}

/* --------------------------------------------------------------------- */

#ifndef MOUNT_NOMAIN
int
main(int argc, char *argv[])
{

	return mount_tmpfs(argc, argv);
}
#endif
