/*	$NetBSD: rump_lfs.c,v 1.5 2009/07/21 00:40:44 pooka Exp $	*/

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

#include <ufs/ufs/ufsmount.h>

#include <err.h>
#include <puffs.h>
#include <stdlib.h>
#include <unistd.h>

#include <rump/p2k.h>
#include <rump/ukfs.h>

#include "mount_lfs.h"

int
main(int argc, char *argv[])
{
	struct ufs_args args;
	char canon_dev[MAXPATHLEN], canon_dir[MAXPATHLEN];
	int mntflags;
	int rv;

	setprogname(argv[0]);

	/*
	 * Load FFS and LFS already here.  we need both and link set
	 * lossage does not allow them to be linked on the command line.
	 */
	ukfs_init();
	if (ukfs_modload("librumpfs_ffs.so") != 1)
		err(1, "modload ffs");
	if (ukfs_modload("librumpfs_lfs.so") != 1)
		err(1, "modload lfs");

	mount_lfs_parseargs(argc, argv, &args, &mntflags, canon_dev, canon_dir);
	rv = p2k_run_fs(MOUNT_LFS, canon_dev, canon_dir, mntflags,
	    &args, sizeof(args), 0);
	if (rv)
		err(1, "mount");

	return 0;
}
