/*	$NetBSD: rump_lfs.c,v 1.6 2009/08/06 01:00:04 pooka Exp $	*/

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
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <rump/rump.h>
#include <rump/p2k.h>
#include <rump/ukfs.h>

#include "mount_lfs.h"

static void *
cleaner(void *arg)
{
	const char *the_argv[7];

	the_argv[0] = "megamaid";
	the_argv[1] = "-D"; /* don't fork() & detach */
	the_argv[2] = arg;

	sleep(1); /* XXXtehsuck: wait until mount is complete is other thread */
	lfs_cleaner_main(3, __UNCONST(the_argv));

	return NULL;
}

int
main(int argc, char *argv[])
{
	struct ufs_args args;
	char canon_dev[MAXPATHLEN], canon_dir[MAXPATHLEN], rawdev[MAXPATHLEN];
	pthread_t cleanerthread;
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

	/*
	 * XXX: this particular piece inspired by the cleaner code.
	 * obviously FIXXXME along with the cleaner.
	 */
	sprintf(rawdev, "/dev/r%s", canon_dev+5);
	rump_etfs_register(rawdev, canon_dev, RUMP_ETFS_CHR);

	/*
	 * We basically have two choices:
	 *  1) run the cleaner in another process and do rump ipc syscalls
	 *  2) run it off a thread in this process and do rump syscalls
	 *     as function calls.
	 *
	 * opt for "2" for now
	 */
#ifndef CLEANER_TESTING
	if ((mntflags & MNT_RDONLY) == 0) {
		if (pthread_create(&cleanerthread, NULL,
		    cleaner, canon_dir) == -1)
			err(1, "cannot start cleaner");
	}
#else
	ukfs_mount(MOUNT_LFS, canon_dev, canon_dir, mntflags,
	    &args, sizeof(args));
	cleaner(canon_dir);
#endif

	rv = p2k_run_fs(MOUNT_LFS, canon_dev, canon_dir, mntflags,
	    &args, sizeof(args), 0);
	if (rv)
		err(1, "mount");

	return 0;
}
