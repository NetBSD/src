/*	$NetBSD: dtfs.c,v 1.27 2007/06/05 12:20:35 pooka Exp $	*/

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

static struct puffs_usermount *gpu;
static struct dtfs_mount gdtm;
int dynamicfh;

static void usage(void);

static void
usage()
{

	errx(1, "usage: %s [-bsdt] [-c hashbuckets] [-o mntopt] [-o puffsopt]\n"
	    "    [-r rootnodetype] mountpath", getprogname());
}

/*
 * This is not perhaps entirely kosher, but this is test file system,
 * so I'm really not concerned.
 */
static void
dosuspend(int v)
{

	puffs_fs_suspend(gpu);
	puffs_fs_suspend(gpu);
}

static void
wipe_the_sleep_out_of_my_eyes(int v)
{

	gdtm.dtm_needwakeup++;
}

static void
loopfun(struct puffs_usermount *pu)
{
	struct dtfs_mount *dtm = puffs_getspecific(pu);
	struct dtfs_poll *dp;

	while (dtm->dtm_needwakeup) {
		dtm->dtm_needwakeup--;
		dp = LIST_FIRST(&dtm->dtm_pollent);
		if (dp == NULL)
			return;

		LIST_REMOVE(dp, dp_entries);
		puffs_cc_continue(dp->dp_pcc);
	}
}

int
main(int argc, char *argv[])
{
	extern char *optarg;
	extern int optind;
	struct puffs_usermount *pu;
	struct puffs_pathobj *po_root;
	struct puffs_ops *pops;
	struct timespec ts;
	char *rtstr;
	mntoptparse_t mp;
	int pflags, lflags, mntflags;
	int ch;
	int khashbuckets;

	setprogname(argv[0]);

	rtstr = NULL;
	lflags = mntflags = 0;
	khashbuckets = 256;
	pflags = PUFFS_KFLAG_IAONDEMAND;
	while ((ch = getopt(argc, argv, "bc:dio:r:st")) != -1) {
		switch (ch) {
		case 'b': /* build paths, for debugging the feature */
			pflags |= PUFFS_FLAG_BUILDPATH;
			break;
		case 'c':
			khashbuckets = atoi(optarg);
			break;
		case 'd':
			dynamicfh = 1;
			break;
		case 'i':
			pflags &= ~PUFFS_KFLAG_IAONDEMAND;
			break;
		case 'o':
			mp = getmntopts(optarg, puffsmopts, &mntflags, &pflags);
			if (mp == NULL)
				err(1, "getmntopts");
			freemntopts(mp);
			break;
		case 'r':
			rtstr = optarg;
			break;
		case 's': /* stay on top */
			lflags |= PUFFSLOOP_NODAEMON;
			break;
		case 't':
			pflags |= PUFFS_KFLAG_WTCACHE;
			break;
		default:
			usage();
			/*NOTREACHED*/
		}
	}
	if (pflags & PUFFS_FLAG_OPDUMP)
		lflags |= PUFFSLOOP_NODAEMON;
	argc -= optind;
	argv += optind;

	if (argc != 1)
		usage();

	PUFFSOP_INIT(pops);

	PUFFSOP_SET(pops, dtfs, fs, statvfs);
	PUFFSOP_SET(pops, dtfs, fs, unmount);
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
	PUFFSOP_SET(pops, dtfs, node, poll);
	PUFFSOP_SET(pops, dtfs, node, mkdir);
	PUFFSOP_SET(pops, dtfs, node, rmdir);
	PUFFSOP_SET(pops, dtfs, node, rename);
	PUFFSOP_SET(pops, dtfs, node, read);
	PUFFSOP_SET(pops, dtfs, node, write);
	PUFFSOP_SET(pops, dtfs, node, link);
	PUFFSOP_SET(pops, dtfs, node, symlink);
	PUFFSOP_SET(pops, dtfs, node, readlink);
	PUFFSOP_SET(pops, dtfs, node, mknod);
	PUFFSOP_SET(pops, dtfs, node, reclaim);

	srandom(time(NULL)); /* for random generation numbers */

	pu = puffs_init(pops, FSNAME, &gdtm, pflags);
	if (pu == NULL)
		err(1, "init");
	gpu = pu;

	puffs_setfhsize(pu, sizeof(struct dtfs_fid),
	    PUFFS_FHFLAG_NFSV2 | PUFFS_FHFLAG_NFSV3
	    | (dynamicfh ? PUFFS_FHFLAG_DYNAMIC : 0));
	puffs_setncookiehash(pu, khashbuckets);

	if (signal(SIGUSR1, dosuspend) == SIG_ERR)
		warn("cannot set suspend sighandler");
	if (signal(SIGALRM, wipe_the_sleep_out_of_my_eyes) == SIG_ERR)
		warn("cannot set alarm sighandler");

	/* init */
	if (dtfs_domount(pu, rtstr) != 0)
		errx(1, "dtfs_domount failed");

	po_root = puffs_getrootpathobj(pu);
	po_root->po_path = argv[0];
	po_root->po_len = strlen(argv[0]);

	/* often enough for testing poll */
	ts.tv_sec = 1;
	ts.tv_nsec = 0;
	puffs_ml_setloopfn(pu, loopfun);
	puffs_ml_settimeout(pu, &ts);

	if (puffs_mount(pu,  argv[0], mntflags, puffs_getroot(pu)) == -1)
		err(1, "mount");
	if (puffs_mainloop(pu, lflags) == -1)
		err(1, "mainloop");

	return 0;
}
