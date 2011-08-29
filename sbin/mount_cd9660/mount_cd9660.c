/*	$NetBSD: mount_cd9660.c,v 1.32 2011/08/29 14:35:00 joerg Exp $	*/

/*
 * Copyright (c) 1992, 1993, 1994
 *      The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley
 * by Pace Willisson (pace@blitz.com).  The Rock Ridge Extension
 * Support code is derived from software contributed to Berkeley
 * by Atsushi Murai (amurai@spec.co.jp).
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
 *
 *      @(#)mount_cd9660.c	8.7 (Berkeley) 5/1/95
 */

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1992, 1993, 1994\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)mount_cd9660.c	8.7 (Berkeley) 5/1/95";
#else
__RCSID("$NetBSD: mount_cd9660.c,v 1.32 2011/08/29 14:35:00 joerg Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/mount.h>

#include <err.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#include <isofs/cd9660/cd9660_mount.h>

#include <mntopts.h>

#include "mountprog.h"
#include "mount_cd9660.h"

static const struct mntopt mopts[] = {
	MOPT_STDOPTS,
	MOPT_UPDATE,
	MOPT_GETARGS,
	{ "extatt", 0, ISOFSMNT_EXTATT, 1 },
	{ "gens", 0, ISOFSMNT_GENS, 1 },
	{ "maplcase", 1, ISOFSMNT_NOCASETRANS, 1 },
	{ "casetrans", 1, ISOFSMNT_NOCASETRANS, 1 },
	{ "nrr", 0, ISOFSMNT_NORRIP, 1 },
	{ "rrip", 1, ISOFSMNT_NORRIP, 1 },
	{ "joliet", 1, ISOFSMNT_NOJOLIET, 1 },
	{ "rrcaseins", 0, ISOFSMNT_RRCASEINS, 1 },
	MOPT_NULL,
};

__dead static void	usage(void);

#ifndef MOUNT_NOMAIN
int
main(int argc, char **argv)
{

	setprogname(argv[0]);
	return mount_cd9660(argc, argv);
}
#endif

void
mount_cd9660_parseargs(int argc, char **argv,
	struct iso_args *args, int *mntflags,
	char *canon_dev, char *canon_dir)
{
	int ch, opts;
	mntoptparse_t mp;
	char *dev, *dir;

	memset(args, 0, sizeof(*args));
	*mntflags = opts = 0;
	optind = optreset = 1;
	while ((ch = getopt(argc, argv, "egijo:r")) != -1)
		switch (ch) {
		case 'e':
			/* obsolete, retained for compatibility only, use
			 * -o extatt */
			opts |= ISOFSMNT_EXTATT;
			break;
		case 'g':
			/* obsolete, retained for compatibility only, use
			 * -o gens */
			opts |= ISOFSMNT_GENS;
			break;
		case 'j':
			/* obsolete, retained fo compatibility only, use
			 * -o nojoliet */
			opts |= ISOFSMNT_NOJOLIET;
			break;
		case 'o':
			mp = getmntopts(optarg, mopts, mntflags, &opts);
			if (mp == NULL)
				err(1, "getmntopts");
			freemntopts(mp);
			break;
		case 'r':
			/* obsolete, retained for compatibility only, use
			 * -o norrip */
			opts |= ISOFSMNT_NORRIP;
			break;
		case '?':
		default:
			usage();
		}

	argc -= optind;
	argv += optind;

	if (argc != 2)
		usage();

	dev = argv[0];
	dir = argv[1];

	pathadj(dev, canon_dev);
	pathadj(dir, canon_dir);

#define DEFAULT_ROOTUID	-2
	/*
	 * ISO 9660 filesystems are not writable.
	 */
	if ((*mntflags & MNT_GETARGS) == 0)
		*mntflags |= MNT_RDONLY;
	args->fspec = canon_dev;
	args->flags = opts;
}

int
mount_cd9660(int argc, char **argv)
{
	struct iso_args args;
	char canon_dev[MAXPATHLEN], canon_dir[MAXPATHLEN];
	int mntflags;

	mount_cd9660_parseargs(argc, argv, &args, &mntflags,
	    canon_dev, canon_dir);

	if (mount(MOUNT_CD9660, canon_dir, mntflags, &args, sizeof args) == -1)
		err(1, "%s on %s", canon_dev, canon_dir);
	if (mntflags & MNT_GETARGS) {
		char buf[2048];
		(void)snprintb(buf, sizeof(buf), ISOFSMNT_BITS, args.flags);
		printf("%s\n", buf);
	}

	exit(0);
}

static void
usage(void)
{
	(void)fprintf(stderr,
		"usage: %s [-o options] special node\n", getprogname());
	exit(1);
}
