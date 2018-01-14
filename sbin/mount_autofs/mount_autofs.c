/*	$NetBSD: mount_autofs.c,v 1.1 2018/01/14 22:44:04 christos Exp $	*/

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
__RCSID("$NetBSD: mount_autofs.c,v 1.1 2018/01/14 22:44:04 christos Exp $");
#endif /* not lint */

#include <sys/param.h>
#include <sys/mount.h>

#include <err.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mntopts.h>

#include <fs/autofs/autofs_mount.h>

#include "mount_autofs.h"

static const struct mntopt mopts[] = {
	MOPT_STDOPTS,
	MOPT_GETARGS,
	MOPT_NULL,
};

int	mount_autofs(int argc, char **argv);
__dead static void	usage(void);

#ifndef MOUNT_NOMAIN
int
main(int argc, char **argv)
{
	return mount_autofs(argc, argv);
}
#endif

void
mount_autofs_parseargs(int argc, char *argv[], void *v, int *mntflags,
	char *canon_dev, char *canon_dir)
{
	int ch;
	mntoptparse_t mp;
	struct autofs_args *am = v;

	*mntflags = 0;
	while ((ch = getopt(argc, argv, "f:o:O:p:")) != -1)
		switch (ch) {
		case 'f':
			strlcpy(am->from, optarg, MAXPATHLEN);
			break;
		case 'o':
			mp = getmntopts(optarg, mopts, mntflags, 0);
			if (mp == NULL)
				err(EXIT_FAILURE, "getmntopts");
			freemntopts(mp);
			break;
		case 'O':
			strlcpy(am->master_options, optarg, MAXPATHLEN);
			break;
		case 'p':
			strlcpy(am->master_prefix, optarg, MAXPATHLEN);
			break;
			
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (argc != 2)
		usage();

	strlcpy(canon_dev, argv[0], MAXPATHLEN);
	if (realpath(argv[1], canon_dir) == NULL)    /* Check mounton path */
		err(EXIT_FAILURE, "realpath %s", canon_dir);
	if (strncmp(argv[1], canon_dir, MAXPATHLEN)) {
		warnx("\"%s\" is a relative path.", argv[1]);
		warnx("using \"%s\" instead.", canon_dir);
	}
}

int
mount_autofs(int argc, char *argv[])
{
	char canon_dev[MAXPATHLEN], canon_dir[MAXPATHLEN];
	char from[MAXPATHLEN], master_options[MAXPATHLEN];
	char master_prefix[MAXPATHLEN];
	int mntflags;
	struct autofs_args am = {
		.from = from,
		.master_options = master_options,
		.master_prefix = master_prefix,
	};

	mount_autofs_parseargs(argc, argv, &am, &mntflags,
	    canon_dev, canon_dir);
	if (mount(MOUNT_KERNFS, canon_dir, mntflags, &am, sizeof(am)) == -1)
		err(EXIT_FAILURE, "kernfs on %s", canon_dir);
        if (mntflags & MNT_GETARGS) {
		printf("from=%s, master_options=%s, master_prefix=%s\n",
		    am.from, am.master_options, am.master_prefix);
	}

	return EXIT_SUCCESS;
}

static void
usage(void)
{
	(void)fprintf(stderr,
	    "Usage: %s [-o options] [-O autofs_options] [-p <prefix>] "
		"[-f <from>] autofs mount_point\n", getprogname());
	exit(EXIT_FAILURE);
}
