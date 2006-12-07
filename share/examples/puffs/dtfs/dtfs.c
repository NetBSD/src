/*	$NetBSD: dtfs.c,v 1.11 2006/12/07 22:49:04 pooka Exp $	*/

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
#include <puffs.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "dtfs.h"

#ifdef DEEP_ROOTED_CLUE
#define FSNAME "detrempe"
#else
#define FSNAME "dt"
#endif

static void usage(void);

static void
usage()
{

	errx(1, "usage: %s [-acds] mountpath", getprogname());
}

int
main(int argc, char *argv[])
{
	extern char *optarg;
	extern int optind;
	struct puffs_usermount *pu;
	struct puffs_ops pops;
	int pflags, lflags;
	int ch;

	setprogname(argv[0]);

	pflags = lflags = 0;
	while ((ch = getopt(argc, argv, "acds")) != -1) {
		switch (ch) {
		case 'a': /* all ops */
			pflags |= PUFFS_KFLAG_ALLOPS;
			break;
		case 'c': /* no cache */
			pflags |= PUFFS_KFLAG_NOCACHE;
			break;
		case 'd': /* dump ops */
			pflags |= PUFFS_FLAG_OPDUMP;
			break;
		case 's': /* stay on top */
			lflags |= PUFFSLOOP_NODAEMON;
			break;
		default:
			usage();
			/*NOTREACHED*/
		}
	}
	argc -= optind;
	argv += optind;

	if (argc != 1)
		usage();

	PUFFSOP_INIT(&pops);

	PUFFSOP_SET(&pops, dtfs, fs, mount);
	PUFFSOP_SET(&pops, dtfs, fs, statvfs);
	PUFFSOP_SETFSNOP(&pops, unmount);
	PUFFSOP_SETFSNOP(&pops, sync);

	PUFFSOP_SET(&pops, dtfs, node, lookup);
	PUFFSOP_SET(&pops, dtfs, node, getattr);
	PUFFSOP_SET(&pops, dtfs, node, setattr);
	PUFFSOP_SET(&pops, dtfs, node, create);
	PUFFSOP_SET(&pops, dtfs, node, remove);
	PUFFSOP_SET(&pops, dtfs, node, readdir);
	PUFFSOP_SET(&pops, dtfs, node, mkdir);
	PUFFSOP_SET(&pops, dtfs, node, rmdir);
	PUFFSOP_SET(&pops, dtfs, node, rename);
	PUFFSOP_SET(&pops, dtfs, node, read);
	PUFFSOP_SET(&pops, dtfs, node, write);
	PUFFSOP_SET(&pops, dtfs, node, link);
	PUFFSOP_SET(&pops, dtfs, node, symlink);
	PUFFSOP_SET(&pops, dtfs, node, readlink);
	PUFFSOP_SET(&pops, dtfs, node, mknod);
	PUFFSOP_SET(&pops, dtfs, node, inactive);
	PUFFSOP_SET(&pops, dtfs, node, reclaim);

	if ((pu = puffs_mount(&pops, argv[0], 0, FSNAME, pflags, 0)) == NULL)
		err(1, "mount");

	if (puffs_mainloop(pu, lflags) == -1)
		err(1, "mainloop");

	return 0;
}
