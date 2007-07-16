/*	$NetBSD: mount_ptyfs.c,v 1.8 2007/07/16 17:06:54 pooka Exp $	*/

/*
 * Copyright (c) 1992, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Jan-Simon Pendry.
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

/*
 * Copyright (c) 1990, 1992 Jan-Simon Pendry
 *
 * This code is derived from software contributed to Berkeley by
 * Jan-Simon Pendry.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
__COPYRIGHT("@(#) Copyright (c) 1992, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)mount_ptyfs.c	8.3 (Berkeley) 5/4/95";
#else
__RCSID("$NetBSD: mount_ptyfs.c,v 1.8 2007/07/16 17:06:54 pooka Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <sys/stat.h>

#include <fs/ptyfs/ptyfs.h>

#include <err.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grp.h>

#include <mntopts.h>

#define ALTF_GROUP	1
#define ALTF_MODE	2

static const struct mntopt mopts[] = {
	MOPT_STDOPTS,
	MOPT_GETARGS,
	{ "group", 0, ALTF_GROUP, 1 },
	{ "mode", 0, ALTF_MODE, 1 },
	MOPT_NULL,
};

int	main(int, char *[]);
int	mount_ptyfs(int argc, char **argv);

static gid_t	getgrp(const char *name);
static void	usage(void);

#ifndef MOUNT_NOMAIN
int
main(int argc, char *argv[])
{
	return mount_ptyfs(argc, argv);
}
#endif

static gid_t
getgrp(const char *name)
{
	char *ep;
	struct group *grp;
	long l;

	if (name == NULL)
		errx(1, "Missing group name");

	l = strtol(name, &ep, 0);

	if (name == ep || *ep)
		grp = getgrnam(name);
	else
		grp = getgrgid((gid_t)l);

	if (grp == NULL)
		errx(1, "Cannot find group `%s'", name);

	return grp->gr_gid;
}


int
mount_ptyfs(int argc, char *argv[])
{
	int ch, mntflags = 0, altflags = 0;
	struct ptyfs_args args;
	mntoptparse_t mp;
	char canon_dir[MAXPATHLEN];


	setprogname(argv[0]);

	args.version = PTYFS_ARGSVERSION;
	args.gid = getgrp("tty");
	args.mode = S_IRUSR|S_IWUSR|S_IWGRP;

	while ((ch = getopt(argc, argv, "g:m:o:")) != -1)
		switch (ch) {
		case 'o':
			altflags = 0;
			mp = getmntopts(optarg, mopts, &mntflags, &altflags);
			if (mp == NULL)
				err(1, "getmntopts");
			if (altflags & ALTF_GROUP)
				args.gid = getgrp(getmntoptstr(mp, "group"));
			if (altflags & ALTF_MODE)
				args.mode = (mode_t)getmntoptnum(mp, "mode");
			freemntopts(mp);
			break;
		case 'g':
			args.gid = getgrp(optarg);
			break;
		case 'm':
			args.mode = (mode_t)strtol(optarg, NULL, 0);
			break;
		case '?':
		default:
			usage();
		}

	argc -= optind;
	argv += optind;

	if (argc != 2)
		usage();

	if (realpath(argv[1], canon_dir) == NULL)   /* Check mounton path */
		err(1, "realpath %s", argv[1]);
	if (strncmp(argv[1], canon_dir, MAXPATHLEN)) {
		warnx("\"%s\" is a relative path.", argv[1]);
		warnx("using \"%s\" instead.", canon_dir);
	}

	if (mount(MOUNT_PTYFS, canon_dir, mntflags, &args, sizeof args) == -1)
		err(1, "ptyfs on %s", canon_dir);
	if (mntflags & MNT_GETARGS)
		printf("version=%d, gid=%lu, mode=0%o\n", args.version,
		    (unsigned long)args.gid, args.mode);
	return 0;
}

static void
usage(void)
{
	(void)fprintf(stderr,
	    "Usage: %s [-g <group|gid>] [-m <mode>] [-o options] ptyfs mountpoint\n", getprogname());
	exit(1);
}
