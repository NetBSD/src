/*	$NetBSD: mount_union.c,v 1.20 2008/02/05 16:54:07 ad Exp $	*/

/*
 * Copyright (c) 1992, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software donated to Berkeley by
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

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1992, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)mount_union.c	8.6 (Berkeley) 4/26/95";
#else
__RCSID("$NetBSD: mount_union.c,v 1.20 2008/02/05 16:54:07 ad Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/mount.h>

#include <miscfs/union/union.h>

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#include <mntopts.h>

static const struct mntopt mopts[] = {
	MOPT_STDOPTS,
	MOPT_GETARGS,
	MOPT_NULL,
};

int	mount_union(int argc, char **argv);
static int	subdir(const char *, const char *);
static void	usage(void);

#ifndef MOUNT_NOMAIN
int
main(int argc, char **argv)
{
	return mount_union(argc, argv);
}
#endif

int
mount_union(int argc, char *argv[])
{
	struct union_args args;
	int ch, mntflags;
	char target[MAXPATHLEN], canon_dir[MAXPATHLEN];
	mntoptparse_t mp;


	mntflags = 0;
	args.mntflags = UNMNT_ABOVE;
	while ((ch = getopt(argc, argv, "bo:")) != -1)
		switch (ch) {
		case 'b':
			args.mntflags &= ~UNMNT_OPMASK;
			args.mntflags |= UNMNT_BELOW;
			break;
		case 'o':
			mp = getmntopts(optarg, mopts, &mntflags, 0);
			if (mp == NULL)
				err(1, "getmntopts");
			freemntopts(mp);
			break;
		case '?':
		default:
			usage();
			/* NOTREACHED */
		}
	argc -= optind;
	argv += optind;

	if (argc != 2)
		usage();

	if (realpath(argv[0], target) == NULL)       /* Check device path */
		err(1, "realpath %s", argv[0]);
	if (strncmp(argv[0], target, MAXPATHLEN)) {
		warnx("\"%s\" is a relative path.", argv[0]);
		warnx("using \"%s\" instead.", target);
	}

	if (realpath(argv[1], canon_dir) == NULL)    /* Check mounton path */
		err(1, "realpath %s", argv[1]);
	if (strncmp(argv[1], canon_dir, MAXPATHLEN)) {
		warnx("\"%s\" is a relative path.", argv[1]);
		warnx("using \"%s\" instead.", canon_dir);
	}

	if (subdir(target, canon_dir) || subdir(canon_dir, target))
		errx(1, "%s (%s) and %s are not distinct paths",
		    argv[0], target, canon_dir);

	args.target = target;

	if (mount(MOUNT_UNION, canon_dir, mntflags, &args, sizeof args) == -1)
		err(1, "%s on %s", target, canon_dir);
	if (mntflags & MNT_GETARGS) {
		char buf[1024];
		(void)snprintb(buf, sizeof(buf), UNMNT_BITS, args.mntflags);
		printf("flags=%s\n", buf);
	}
	exit(0);
}

static int
subdir(const char *p, const char *dir)
{
	int l;

	l = strlen(dir);
	if (l <= 1)
		return (1);

	if ((strncmp(p, dir, l) == 0) && (p[l] == '/' || p[l] == '\0'))
		return (1);

	return (0);
}

static void
usage(void)
{
	(void)fprintf(stderr,
		"usage: mount_union [-br] [-o options] target_fs mount_point\n");
	exit(1);
}
