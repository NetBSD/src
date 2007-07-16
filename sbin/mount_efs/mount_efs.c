/*	$NetBSD: mount_efs.c,v 1.3 2007/07/16 17:06:52 pooka Exp $	*/

/*
 * Copyright (c) 2006 Stephen M. Rumble <rumble@ephemeral.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/param.h>
#include <sys/mount.h>
#include <fs/efs/efs.h>
#include <fs/efs/efs_sb.h>
#include <fs/efs/efs_mount.h>

#include <err.h>
#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <unistd.h>

#include <mntopts.h>

static const struct mntopt mopts[] = {
	MOPT_STDOPTS,
	MOPT_GETARGS,
	MOPT_FORCE,
	MOPT_NULL
};

static void	usage(void);
int		mount_efs(int, char **);

static void
usage()
{
	
	fprintf(stderr, "usage: %s [-o options] special node\n", getprogname());
	exit(1);
}

int
mount_efs(int argc, char **argv)
{
	int ch, mntflags;
	extern int optind;
	extern char *optarg;
	struct efs_args args;
	char special[MAXPATHLEN], node[MAXPATHLEN];
	mntoptparse_t mp;

	setprogname(argv[0]);
	memset(&args, 0, sizeof(args));
	mntflags = 0;

	while ((ch = getopt(argc, argv, "o:")) != -1) {
		switch (ch) {
		case 'o':
			mp = getmntopts(optarg, mopts, &mntflags, NULL);
			if (mp == NULL)
				err(1, "getmntopts");
			freemntopts(mp);
			break;

		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (argc != 2)
		usage();

	if (realpath(argv[0], special) == NULL)
		err(EXIT_FAILURE, "realpath %s", argv[0]);

	if (realpath(argv[1], node) == NULL)
		err(EXIT_FAILURE, "realpath %s", argv[1]);

	args.fspec = special;
	args.version = EFS_MNT_VERSION;
	if (mount(MOUNT_EFS, node, mntflags, &args, sizeof args) == -1)
		err(EXIT_FAILURE, "%s on %s", special, node);

	return (0);
}

#ifndef MOUNT_NOMAIN
int
main(int argc, char **argv)
{

	return (mount_efs(argc, argv));
}
#endif
