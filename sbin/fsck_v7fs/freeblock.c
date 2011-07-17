/*	$NetBSD: freeblock.c,v 1.2 2011/07/17 12:47:38 uch Exp $	*/

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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
__RCSID("$NetBSD: freeblock.c,v 1.2 2011/07/17 12:47:38 uch Exp $");
#endif /* not lint */

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "v7fs.h"
#include "v7fs_superblock.h"
#include "v7fs_inode.h"
#include "v7fs_impl.h"
#include "v7fs_datablock.h"
#include "fsck_v7fs.h"

struct freeblock_arg {
	v7fs_daddr_t i;
	v7fs_daddr_t j;
	v7fs_daddr_t blk;
};

static int
freeblock_subr_cnt(struct v7fs_self *fs __unused, void *ctx,
    v7fs_daddr_t blk __unused)
{
	((struct freeblock_arg *)ctx)->blk++;
	progress(0);

	return 0;
}

static int
freeblock_subr_j(struct v7fs_self *fs, void *ctx, v7fs_daddr_t blk)
{
	struct freeblock_arg *arg = (struct freeblock_arg *)ctx;

	if (!datablock_number_sanity(fs, blk)) {
		pwarn("invalid block#%d in freeblock", blk);
		/* This problem should be fixed at freeblock_check(). */
		return FSCK_EXIT_CHECK_FAILED;
	}

	if (arg->j >= arg->i)
		return V7FS_ITERATOR_BREAK;

	progress(0);

	if (arg->blk == blk) {
		pwarn("freeblock duplicate %d %d blk=%d", arg->i, arg->j, blk);
		if (reply("CORRECT?")) {
			freeblock_dup_remove(fs, blk);
		}
		return FSCK_EXIT_UNRESOLVED; /* Rescan needed. */
	}

	arg->j++;

	return 0;	/*continue */
}

static int
freeblock_subr_i(struct v7fs_self *fs, void *ctx, v7fs_daddr_t blk)
{
	struct freeblock_arg *arg = (struct freeblock_arg *)ctx;
	int ret;

	if (!datablock_number_sanity(fs, blk)) {
		pwarn("invalid block#%d in freeblock", blk);
		/* This problem should be fixed at freeblock_check(). */
		return FSCK_EXIT_CHECK_FAILED;
	}

	arg->j = 0;
	arg->blk = blk;
	ret = v7fs_freeblock_foreach(fs, freeblock_subr_j, ctx);
	if (!((ret == 0) || (ret == V7FS_ITERATOR_BREAK)))
		return ret;

	arg->i++;

	return 0;
}

int
freeblock_vs_freeblock_check(struct v7fs_self *fs)
{
	struct v7fs_superblock *sb = &fs->superblock;
	int n = sb->total_freeblock;

	progress(&(struct progress_arg){ .label = "free-free", .tick = (n / 2)
	    * ((n - 1) / PROGRESS_BAR_GRANULE) });

	return v7fs_freeblock_foreach(fs, freeblock_subr_i,
	    &(struct freeblock_arg){ .i = 0 });
}

/*
 * Remove duplicated block.
 */
void
freeblock_dup_remove(struct v7fs_self *fs, v7fs_daddr_t dupblk)
{
	struct v7fs_superblock *sb = &fs->superblock;
	struct v7fs_freeblock *fb;
	int i, total, n;
	void *buf;
	v7fs_daddr_t blk;

	n = sb->total_freeblock;

	/* Superblock cache. */
	total = 0;
	for (i = sb->nfreeblock - 1; (i > 0) && (n >= 0); i--, n--, total++) {
		if (sb->freeblock[i] == dupblk) {      /* Duplicate found. */
			memmove(sb->freeblock + i, sb->freeblock + i + 1,
			    sb->nfreeblock - 1 - i);
			sb->nfreeblock--;
			sb->modified = 1;
			v7fs_superblock_writeback(fs);
			pwarn("remove duplicated freeblock %d"
			    "from superblock", dupblk);
			return;
		}
	}
	if (!n)
		return;
	blk = sb->freeblock[0];

	do {
		if (!blk)
			break;
		buf = scratch_read(fs, blk);
		fb = (struct v7fs_freeblock *)buf;
		v7fs_freeblock_endian_convert(fs, fb);

		if (blk == dupblk) {
			/* This is difficult probrem. give up! */
			/* or newly allocate block, and copy it and link. */
			pwarn("duplicated block is freeblock list."
			    "Shortage freeblock %d->%d.",
			    sb->nfreeblock, total);
			sb->nfreeblock = total; /*shotage freeblock list. */
			sb->modified = 1;
			v7fs_superblock_writeback(fs);
			return;
		}
		total++;

		blk = fb->freeblock[0]; /* next freeblock list */

		for (i = fb->nfreeblock - 1; (i > 0) && (n >= 0);
		    i--, n--, total++) {
			if (fb->freeblock[i] == dupblk)	{
				pwarn("remove duplicated freeblock"
				    "%d from list %d", dupblk, blk);
				memmove(fb->freeblock + i, fb->freeblock + i +
				    1, fb->nfreeblock - 1 - i);
				/* Writeback superblock. */
				sb->nfreeblock--;
				sb->modified = 1;
				v7fs_superblock_writeback(fs);
				/* Writeback freeblock list block. */
				v7fs_freeblock_endian_convert(fs, fb);
				fs->io.write(fs->io.cookie, buf, blk);
				return;
			}
		}
		scratch_free(fs, buf);
	} while (n);

	return;
}

int
v7fs_freeblock_foreach(struct v7fs_self *fs,
    int (*func)(struct v7fs_self *, void *, v7fs_daddr_t), void *ctx)
{
	struct v7fs_superblock *sb = &fs->superblock;
	struct v7fs_freeblock *fb;
	int i, n;
	void *buf;
	v7fs_daddr_t blk;
	int ret;

	n = sb->total_freeblock;

	/* Superblock cache. */
	for (i = sb->nfreeblock - 1; (i > 0) && (n >= 0); i--, n--) {
		if ((ret = func(fs, ctx, sb->freeblock[i])))
			return ret;
	}
	if (!n)
		return 0;
	blk = sb->freeblock[0];
	if (!datablock_number_sanity(fs, blk)) {
		pwarn("invalid freeblock list block#%d.", blk);
		return 0;
	}
	do {
		if (!blk)
			break;
		if (!(buf = scratch_read(fs, blk)))
			return 0;
		fb = (struct v7fs_freeblock *)buf;

		if (v7fs_freeblock_endian_convert(fs, fb)) {
			pwarn("***corrupt freeblock list blk#%d", blk);
			return 0;
		}

		/* freeblock list is used as freeblock. */
		n--;
		if ((ret = func(fs, ctx, blk)))
			return ret;

		blk = fb->freeblock[0]; /* next freeblock list */

		for (i = fb->nfreeblock - 1; (i > 0) && (n >= 0); i--, n--)
			if ((ret = func(fs, ctx, fb->freeblock[i]))) {
				scratch_free(fs, buf);
				return ret;
			}
		scratch_free(fs, buf);
	} while (n);

	return 0;
}

int
freeblock_check(struct v7fs_self *fs)
{
	struct v7fs_superblock *sb = &fs->superblock;
	struct freeblock_arg freeblock_arg = { .blk = 0 };
	v7fs_daddr_t blk;
	v7fs_daddr_t datablock_size = sb->volume_size -
	    sb->datablock_start_sector;
	int error = 0;
	struct progress_arg progress_arg = { .label = "freeblock", .tick =
	    sb->total_freeblock / PROGRESS_BAR_GRANULE };

	progress(&progress_arg);
	v7fs_freeblock_foreach(fs, freeblock_subr_cnt, &freeblock_arg);
	progress(&progress_arg);

	blk = freeblock_arg.blk;
	pwarn("\ndatablock usage: %d/%d (%d)\n",  datablock_size - blk,
	    datablock_size, blk);

	if (sb->total_freeblock != blk) {
		pwarn("corrupt # of freeblocks. %d(sb) != %d(cnt)",
		    sb->total_freeblock, blk);
		if (reply_trivial("CORRECT?")) {
			sb->total_freeblock = blk;
			sb->modified = 1;
			v7fs_superblock_writeback(fs);
		} else {
			error = FSCK_EXIT_CHECK_FAILED;
		}
	}


	return error;
}
