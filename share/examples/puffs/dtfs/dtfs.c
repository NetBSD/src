/*	$NetBSD: dtfs.c,v 1.18 2007/04/13 13:35:46 pooka Exp $	*/

/*
 * Copyright (c) 2006  Antti Kantee.  All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Delectable Test File System: a simple in-memory file system which
 * demonstrates the use of puffs.
 * (a.k.a. Detrempe FS ...)
 */

#include <sys/types.h>

#include <err.h>
#include <mntopts.h>
#include <puffs.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "dtfs.h"

#ifdef DEEP_ROOTED_CLUE
#define FSNAME "detrempe"
#else
#define FSNAME "dt"
#endif

static struct puffs_usermount *pu;

static void usage(void);

static void
usage()
{

	errx(1, "usage: %s [-bs] [-o mntopt] [-o puffsopt] mountpath",
	    getprogname());
}

/*
 * This is not perhaps entirely kosher, but this is test file system,
 * so I'm really not concerned.
 */
static void
dosuspend(int v)
{

	puffs_fs_suspend(pu);
}

int
main(int argc, char *argv[])
{
	extern char *optarg;
	extern int optind;
	struct dtfs_mount dtm;
	struct puffs_pathobj *po_root;
	struct puffs_ops *pops;
	mntoptparse_t mp;
	int pflags, lflags, mntflags;
	int ch;

	setprogname(argv[0]);

	pflags = lflags = mntflags = 0;
	while ((ch = getopt(argc, argv, "bo:s")) != -1) {
		switch (ch) {
		case 'b': /* build paths, for debugging the feature */
			pflags |= PUFFS_FLAG_BUILDPATH;
			break;
		case 'o':
			mp = getmntopts(optarg, puffsmopts, &mntflags, &pflags);
			if (mp == NULL)
				err(1, "getmntopts");
			freemntopts(mp);
			break;
		case 's': /* stay on top */
			lflags |= PUFFSLOOP_NODAEMON;
			break;
		default:
			usage();
			/*NOTREACHED*/
		}
	}
	if (pflags & PUFFS_FLAG_OPDUMP)
		lflags |= PUFFSLOOP_NODAEMON;
	pflags |= PUFFS_KFLAG_CANEXPORT;
	argc -= optind;
	argv += optind;

	if (argc != 1)
		usage();

	PUFFSOP_INIT(pops);

	PUFFSOP_SET(pops, dtfs, fs, statvfs);
	PUFFSOP_SETFSNOP(pops, unmount);
	PUFFSOP_SETFSNOP(pops, sync);
	PUFFSOP_SET(pops, dtfs, fs, fhtonode);
	PUFFSOP_SET(pops, dtfs, fs, nodetofh);
	PUFFSOP_SET(pops, dtfs, fs, suspend);

	PUFFSOP_SET(pops, dtfs, node, lookup);
	PUFFSOP_SET(pops, dtfs, node, access);
	PUFFSOP_SET(pops, dtfs, node, getattr);
	PUFFSOP_SET(pops, dtfs, node, setattr);
	PUFFSOP_SET(pops, dtfs, node, create);
	PUFFSOP_SET(pops, dtfs, node, remove);
	PUFFSOP_SET(pops, dtfs, node, readdir);
	PUFFSOP_SET(pops, dtfs, node, mkdir);
	PUFFSOP_SET(pops, dtfs, node, rmdir);
	PUFFSOP_SET(pops, dtfs, node, rename);
	PUFFSOP_SET(pops, dtfs, node, read);
	PUFFSOP_SET(pops, dtfs, node, write);
	PUFFSOP_SET(pops, dtfs, node, link);
	PUFFSOP_SET(pops, dtfs, node, symlink);
	PUFFSOP_SET(pops, dtfs, node, readlink);
	PUFFSOP_SET(pops, dtfs, node, mknod);
	PUFFSOP_SET(pops, dtfs, node, inactive);
	PUFFSOP_SET(pops, dtfs, node, reclaim);

	srandom(time(NULL)); /* for random generation numbers */

	if ((pu = puffs_mount(pops, argv[0], mntflags, FSNAME, &dtm, pflags))
	    == NULL)
		err(1, "mount");

	if (signal(SIGUSR1, dosuspend) == SIG_ERR)
		warn("cannot set suspend sighandler");

	/* init & call puffs_start() */
	if (dtfs_domount(pu) != 0)
		errx(1, "dtfs_domount failed");

	/* XXX: wrong order, but I need to refactor this further anyway */
	po_root = puffs_getrootpathobj(pu);
	po_root->po_path = argv[0];
	po_root->po_len = strlen(argv[0]);

	if (puffs_mainloop(pu, lflags) == -1)
		err(1, "mainloop");

	return 0;
}
