/*	$NetBSD: wapbl.c,v 1.2 2008/07/31 05:38:04 simonb Exp $	*/

/*-
 * Copyright (c) 2005,2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Wasabi Systems, Inc.
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

/* This file contains fsck support for wapbl
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: wapbl.c,v 1.2 2008/07/31 05:38:04 simonb Exp $");

#include <sys/stat.h>
#include <sys/time.h>
#include <sys/uio.h>

#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <ufs/ufs/dinode.h>
#include <ufs/ufs/dir.h>
#include <ufs/ufs/ufs_bswap.h>
#include <ufs/ufs/ufs_wapbl.h>
#include <ufs/ffs/fs.h>
#include <ufs/ffs/ffs_extern.h>

#include <sys/wapbl.h>

#include "fsck.h"
#include "fsutil.h"
#include "extern.h"
#include "exitvalues.h"

int
wapbl_write(void *data, size_t len, struct vnode *devvp, daddr_t pbn)
{

	WAPBL_PRINTF(WAPBL_PRINT_IO,
		("wapbl_write: %zd bytes at block %"PRId64" on fd 0x%x\n",
		len, pbn, fswritefd));
	bwrite(fswritefd, data, pbn, len);
	return 0;
}

int
wapbl_read(void *data, size_t len, struct vnode *devvp, daddr_t pbn)
{

	WAPBL_PRINTF(WAPBL_PRINT_IO,
		("wapbl_read: %zd bytes at block %"PRId64" on fd 0x%x\n",
		len, pbn, fsreadfd));
	bread(fsreadfd, data, pbn, len);
	return 0;
}

struct wapbl_replay *wapbl_replay;

void
replay_wapbl(void)
{
	uint64_t addr, count, blksize;
	int error;

	if (debug)
		wapbl_debug_print = WAPBL_PRINT_ERROR | WAPBL_PRINT_REPLAY;
	if (debug > 1)
		wapbl_debug_print |= WAPBL_PRINT_IO;

	if (sblock->fs_journal_version != UFS_WAPBL_VERSION) {
		pfatal("INVALID JOURNAL VERSION %d",
		    sblock->fs_journal_version);
		if (reply("CONTINUE") == 0) {
			exit(FSCK_EXIT_CHECK_FAILED);
		}
		return;
	}

	switch (sblock->fs_journal_location) {
	case UFS_WAPBL_JOURNALLOC_NONE:
		pfatal("INVALID JOURNAL LOCATION 'NONE'");
		if (reply("CONTINUE") == 0) {
			exit(FSCK_EXIT_CHECK_FAILED);
		}
		return;

	case UFS_WAPBL_JOURNALLOC_END_PARTITION:
		addr = sblock->fs_journallocs[UFS_WAPBL_EPART_ADDR];
		count = sblock->fs_journallocs[UFS_WAPBL_EPART_COUNT];
		blksize = sblock->fs_journallocs[UFS_WAPBL_EPART_BLKSZ];
		break;

	case UFS_WAPBL_JOURNALLOC_IN_FILESYSTEM:
		addr = sblock->fs_journallocs[UFS_WAPBL_INFS_ADDR];
		count = sblock->fs_journallocs[UFS_WAPBL_INFS_COUNT];
		blksize = sblock->fs_journallocs[UFS_WAPBL_INFS_BLKSZ];
		break;

	default:
		pfatal("INVALID JOURNAL LOCATION %d",
		    sblock->fs_journal_location);
		if (reply("CONTINUE") == 0) {
			exit(FSCK_EXIT_CHECK_FAILED);
		}
		return;
	}

	error = wapbl_replay_start(&wapbl_replay, 0, addr, count, blksize);
	if (error) {
		pfatal("UNABLE TO READ JOURNAL FOR REPLAY");
		if (reply("CONTINUE") == 0) {
			exit(FSCK_EXIT_CHECK_FAILED);
		}
		return;
	}
	if (!nflag) {
		error = wapbl_replay_write(wapbl_replay, 0);
		if (error) {
			pfatal("UNABLE TO REPLAY JOURNAL BLOCKS");
			if (reply("CONTINUE") == 0) {
				exit(FSCK_EXIT_CHECK_FAILED);
			}
		} else {
			wapbl_replay_stop(wapbl_replay);
		}
	}
	{
		int i;
		for (i = 0; i < wapbl_replay->wr_inodescnt; i++) {
			WAPBL_PRINTF(WAPBL_PRINT_REPLAY,("wapbl_replay: "
			    "not cleaning inode %"PRIu32" mode %"PRIo32"\n",
			    wapbl_replay->wr_inodes[i].wr_inumber,
			    wapbl_replay->wr_inodes[i].wr_imode));
		}
	}
}

void
cleanup_wapbl(void)
{

	if (wapbl_replay) {
		if (wapbl_replay_isopen(wapbl_replay))
			wapbl_replay_stop(wapbl_replay);
		wapbl_replay_free(wapbl_replay);
		wapbl_replay = 0;
	}
}

int
read_wapbl(char *buf, long size, daddr_t blk)
{

	if (!wapbl_replay || !wapbl_replay_isopen(wapbl_replay))
		return 0;
	return wapbl_replay_read(wapbl_replay, buf, blk, size);
}

int
is_journal_inode(ino_t ino)
{
	union dinode *dp;

	dp = ginode(ino);
	if ((iswap32(DIP(dp, flags)) & SF_LOG) != 0 &&
	    sblock->fs_journal_version == UFS_WAPBL_VERSION &&
	    sblock->fs_journal_location == UFS_WAPBL_JOURNALLOC_IN_FILESYSTEM &&
	    sblock->fs_journallocs[UFS_WAPBL_INFS_INO] == ino)
		return 1;

	return 0;
}
