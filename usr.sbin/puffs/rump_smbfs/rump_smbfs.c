/*	$NetBSD: rump_smbfs.c,v 1.1 2009/09/06 20:26:47 pooka Exp $	*/

/*
 * Copyright (c) 2008 Antti Kantee.  All Rights Reserved.
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

#include <sys/types.h>
#include <sys/mount.h>

#include <fs/smbfs/smbfs.h>

#include <rump/rump.h>
#include <rump/p2k.h>

#include <err.h>
#include <puffs.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "mount_smbfs.h"

int
main(int argc, char *argv[])
{
	struct smbfs_args args;
	char canon_dev[MAXPATHLEN], canon_dir[MAXPATHLEN];
	int rv, mntflags;

	setprogname(argv[0]);

	/*
	 * XXX: rump_init() creates threads, we cannot detach after that.
	 * so let's call rump_smbfs work in progress and force debugging
	 * mode ;)  (I should go into marketing)
	 */
	printf("rump_smbfs is work-in-progress.  forcing debug mode\n");
	setenv("P2K_DEBUG", "1", 1);
	rump_init();
	mount_smbfs_parseargs(argc, argv, &args, &mntflags,
	    canon_dev, canon_dir);

	rv = p2k_run_fs(MOUNT_SMBFS, canon_dev, canon_dir, mntflags, &args,
	    sizeof(args), 0);
	if (rv == -1)
		err(1, "mount");

	return 0;
}
