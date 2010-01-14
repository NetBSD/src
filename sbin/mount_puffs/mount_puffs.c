/*	$NetBSD: mount_puffs.c,v 1.1 2010/01/14 21:25:48 pooka Exp $	*/

/*
 * Copyright (c) 2010 Antti Kantee.  All Rights Reserved.
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
 * This is to support -o getargs without having to replicate
 * it in every file server.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: mount_puffs.c,v 1.1 2010/01/14 21:25:48 pooka Exp $");
#endif /* !lint */

#include <sys/param.h>
#include <sys/mount.h>

#include <fs/puffs/puffs_msgif.h>

#include <err.h>
#include <mntopts.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

const struct mntopt getargmopt[] = {
	MOPT_GETARGS,
	MOPT_NULL,
};

static void
usage(void)
{

	fprintf(stderr, "usage: %s -o getargs spec dir\n", getprogname());
	exit(1);
}

int
main(int argc, char *argv[])
{
	const char *vtypes[] = { VNODE_TYPES };
	struct puffs_kargs kargs;
	mntoptparse_t mp;
	int mntflags, f;
	int ch;

	if (argc < 3)
		usage();

	mntflags = 0;
	while ((ch = getopt(argc, argv, "o:")) != -1) {
		switch (ch) {
		case 'o':
			mp = getmntopts(optarg, getargmopt, &mntflags, &f);
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

	if (mntflags != MNT_GETARGS)
		usage();

	if (mount(MOUNT_PUFFS, argv[1], mntflags, &kargs, sizeof(kargs)) == -1)
		err(1, "mount");
	
	printf("version=%d, ", kargs.pa_vers & ~PUFFSDEVELVERS);
	printf("flags=0x%x, ", kargs.pa_flags);

	printf("root cookie=%p, ", kargs.pa_root_cookie);
	printf("root type=%s", vtypes[kargs.pa_root_vtype]);

	if (kargs.pa_root_vtype != VDIR)
		printf(", root size=%llu",
		    (unsigned long long)kargs.pa_root_vsize);
	if (kargs.pa_root_vtype == VCHR || kargs.pa_root_vtype == VBLK)
		printf(", root rdev=0x%" PRIx64, (uint64_t)kargs.pa_root_rdev);
	printf("\n");
}
