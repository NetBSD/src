/*	$NetBSD: wapbl.c,v 1.1.2.2 2008/06/11 12:09:59 simonb Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: wapbl.c,v 1.1.2.2 2008/06/11 12:09:59 simonb Exp $");

#include <sys/time.h>
#include <sys/types.h>
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
	int error;

	if (debug)
		wapbl_debug_print = WAPBL_PRINT_ERROR|WAPBL_PRINT_REPLAY;
	if (debug > 1)
		wapbl_debug_print |= WAPBL_PRINT_IO;
	error = wapbl_replay_start(&wapbl_replay,
			0, 
			fsbtodb(sblock, sblock->fs_size), /* journal is after file system */
			0 /* XXX */,
			dev_bsize);
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
			WAPBL_PRINTF(WAPBL_PRINT_REPLAY,("wapbl_replay: not cleaning inode %"PRIu32" mode %"PRIo32"\n",
			    wapbl_replay->wr_inodes[i].wr_inumber, wapbl_replay->wr_inodes[i].wr_imode));
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
