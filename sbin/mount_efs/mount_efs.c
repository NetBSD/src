/*	$NetBSD: mount_efs.c,v 1.5 2011/08/29 14:35:00 joerg Exp $	*/

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

#include "mountprog.h"
#include "mount_efs.h"

static const struct mntopt mopts[] = {
	MOPT_STDOPTS,
	MOPT_GETARGS,
	MOPT_FORCE,
	MOPT_NULL
};

__dead static void	usage(void);

static void
usage(void)
{
	
	fprintf(stderr, "usage: %s [-o options] special node\n", getprogname());
	exit(1);
}

void
mount_efs_parseargs(int argc, char **argv,
	struct efs_args *args, int *mntflags,
	char *canon_dev, char *canon_dir)
{
	int ch;
	extern int optind;
	extern char *optarg;
	mntoptparse_t mp;

	memset(args, 0, sizeof(*args));
	*mntflags = 0;

	while ((ch = getopt(argc, argv, "o:")) != -1) {
		switch (ch) {
		case 'o':
			mp = getmntopts(optarg, mopts, mntflags, NULL);
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

	pathadj(argv[0], canon_dev);
	pathadj(argv[1], canon_dir);

	args->fspec = canon_dev;
	args->version = EFS_MNT_VERSION;
}

int
mount_efs(int argc, char **argv)
{
	char special[MAXPATHLEN], node[MAXPATHLEN];
	struct efs_args args;
	int mntflags;

	mount_efs_parseargs(argc, argv, &args, &mntflags, special, node);

	if (mount(MOUNT_EFS, node, mntflags, &args, sizeof args) == -1)
		err(EXIT_FAILURE, "%s on %s", special, node);

	return (0);
}

#ifndef MOUNT_NOMAIN
int
main(int argc, char **argv)
{

	setprogname(argv[0]);
	return (mount_efs(argc, argv));
}
#endif
