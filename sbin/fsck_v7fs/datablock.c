/*	$NetBSD: datablock.c,v 1.1 2011/06/27 11:52:58 uch Exp $	*/

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
__RCSID("$NetBSD: datablock.c,v 1.1 2011/06/27 11:52:58 uch Exp $");
#endif /* not lint */

#include <stdio.h>
#include <stdbool.h>

#include "v7fs.h"
#include "v7fs_endian.h"
#include "v7fs_superblock.h"
#include "v7fs_inode.h"
#include "v7fs_impl.h"
#include "v7fs_datablock.h"
#include "v7fs_file.h"
#include "fsck_v7fs.h"

static void datablock_dup_remove(struct v7fs_self *, v7fs_ino_t, v7fs_ino_t,
    v7fs_daddr_t);

struct loop_context {
	v7fs_ino_t i;
	v7fs_ino_t j;
	v7fs_daddr_t blk;
};

/*
 * datablock vs freeblock
 */
static int
freeblock_subr(struct v7fs_self *fs, void *ctx, v7fs_daddr_t freeblk)
{
	struct loop_context *arg = (struct loop_context *)ctx;

	progress(0);
	if (arg->blk == freeblk) {
		pwarn("*** ino%d(%s) data block %d found at freeblock",
		    arg->i, filename(fs, arg->i), freeblk);
		if (reply("CORRECT?")) {
			freeblock_dup_remove(fs, freeblk);
			return V7FS_ITERATOR_ERROR; /* Rescan needed. */
		}
	}

	return 0;
}

static int
datablock_subr(struct v7fs_self *fs, void *ctx, v7fs_daddr_t blk,
    size_t sz __unused)
{
	struct loop_context *arg = (struct loop_context *)ctx;
	int ret;

	arg->blk = blk;
	if ((ret = v7fs_freeblock_foreach(fs, freeblock_subr, ctx)) ==
	    V7FS_ITERATOR_ERROR)
		return ret;

	return 0;
}

static int
inode_subr(struct v7fs_self *fs, void *ctx, struct v7fs_inode *p,
    v7fs_ino_t ino)
{
	struct loop_context *arg = (struct loop_context *)ctx;
	int ret;

	arg->i = ino;

	if ((ret = v7fs_datablock_foreach(fs, p, datablock_subr, ctx)) ==
	    V7FS_ITERATOR_ERROR)
		return ret;

	return 0;
}

int
datablock_vs_freeblock_check(struct v7fs_self *fs)
{
	struct v7fs_superblock *sb = &fs->superblock;
	int nfree = sb->total_freeblock;
	int ndata = sb->volume_size - sb->datablock_start_sector - nfree;
	int ret;

	progress(&(struct progress_arg){ .label = "data-free", .tick = (ndata /
	    PROGRESS_BAR_GRANULE) * nfree });

	if ((ret = v7fs_ilist_foreach(fs, inode_subr, &(struct loop_context)
	    { .i = 0, .blk = 0 })) == V7FS_ITERATOR_ERROR)
		return FSCK_EXIT_UNRESOLVED;

	return 0;
}


/*
 * datablock vs datablock
 */
static int
datablock_i_j(struct v7fs_self *fs, void *ctx, v7fs_daddr_t blk,  size_t sz
    __unused)
{
	struct loop_context *arg = (struct loop_context *)ctx;

	progress(0);
	if (blk == arg->blk) {
		pwarn("*** duplicated block found."
		    "#%d(%s) and #%d(%s) refer block %d",
		    arg->i, filename(fs, arg->i),
		    arg->j, filename(fs, arg->j), blk);
		if (reply("CORRECT?")) {
			datablock_dup_remove(fs, arg->i, arg->j, blk);
			return V7FS_ITERATOR_ERROR; /* Rescan needed. */
		}
	}

	return 0;
}

static int
loopover_j(struct v7fs_self *fs, void *ctx, struct v7fs_inode *p,
    v7fs_ino_t ino)
{
	struct loop_context *arg = (struct loop_context *)ctx;
	int ret;

	arg->j = ino;

	if (arg->j >= arg->i)
		return V7FS_ITERATOR_BREAK;

	if ((ret = v7fs_datablock_foreach(fs, p, datablock_i_j, ctx)) ==
	    V7FS_ITERATOR_ERROR)
		return V7FS_ITERATOR_ERROR;

	return 0;
}

static int
datablock_i(struct v7fs_self *fs, void *ctx, v7fs_daddr_t blk,
    size_t sz __unused)
{
	struct loop_context *arg = (struct loop_context *)ctx;
	int ret;

	arg->blk = blk;

	if ((ret = v7fs_ilist_foreach(fs, loopover_j, ctx)) ==
	    V7FS_ITERATOR_ERROR)
		return V7FS_ITERATOR_ERROR;

	return 0;
}

static int
loopover_i(struct v7fs_self *fs, void *ctx, struct v7fs_inode *inode,
    v7fs_ino_t ino)
{
	struct loop_context *arg = (struct loop_context *)ctx;
	int ret;

	arg->i = ino;

	if ((ret = v7fs_datablock_foreach(fs, inode, datablock_i, ctx)) ==
	    V7FS_ITERATOR_ERROR)
		return V7FS_ITERATOR_ERROR;

	return 0;
}


int
datablock_vs_datablock_check(struct v7fs_self *fs)
{
	const struct v7fs_superblock *sb = &fs->superblock;
	int n = sb->volume_size - sb->total_freeblock;
	int ret;

	progress(&(struct progress_arg){ .label = "data-data", .tick = (n / 2)
	    * ((n - 1) / PROGRESS_BAR_GRANULE) });

	if ((ret = v7fs_ilist_foreach(fs, loopover_i, &(struct loop_context){
	    .i = 0, .j = 0 })) == V7FS_ITERATOR_ERROR)
		return FSCK_EXIT_UNRESOLVED;

	return 0;
}

/*
 * Remove duplicated block.
 */
static void
copy_block(struct v7fs_self *fs, v7fs_daddr_t dst, v7fs_daddr_t src)
{
	void *buf;

	if (!(buf = scratch_read(fs, src)))
		return;
	fs->io.write(fs->io.cookie, buf, dst);
	scratch_free(fs, buf);

	pwarn("copy block %d->%d\n", src, dst);
}

static void
replace_block_direct(struct v7fs_self *fs, struct v7fs_inode *p, int dupidx,
    v7fs_daddr_t newblk)
{
	v7fs_daddr_t oldblk;

	oldblk = p->addr[dupidx];
	p->addr[dupidx] = newblk;
	v7fs_inode_writeback(fs, p);	/* endian conversion done by here. */

	copy_block(fs, newblk, oldblk);
}

static void
prepare_list(struct v7fs_self *fs, v7fs_daddr_t listblk, v7fs_daddr_t *p)
{
	size_t i;

	fs->io.read(fs->io.cookie, (void *)p, listblk);
	for (i = 0; i < V7FS_DADDR_PER_BLOCK; i++)
		p[i] = V7FS_VAL32(fs, p[i]);
}

static void
replace_block_indexed(struct v7fs_self *fs, v7fs_daddr_t listblk, int dupidx,
    v7fs_daddr_t newblk)
{
	void *buf;
	v7fs_daddr_t *list;
	v7fs_daddr_t oldblk;

	if (!(buf = scratch_read(fs, listblk)))
		return;
	list = (v7fs_daddr_t *)buf;
	oldblk = V7FS_VAL32(fs, list[dupidx]);
	list[dupidx] = V7FS_VAL32(fs, newblk);
	fs->io.write(fs->io.cookie, buf, listblk);
	scratch_free(fs, buf);

	copy_block(fs, newblk, oldblk);
	pwarn("dup block replaced by %d\n", newblk);
}

static bool
dupfind_loop1(struct v7fs_self *fs, v7fs_daddr_t listblk, v7fs_daddr_t dupblk,
    v7fs_daddr_t newblk)
{
	v7fs_daddr_t list[V7FS_DADDR_PER_BLOCK];
	size_t i;

	prepare_list(fs, listblk, list);
	for (i = 0; i < V7FS_DADDR_PER_BLOCK; i++) {
		if (list[i] == dupblk) {
			replace_block_indexed(fs, listblk, i, newblk);
			return true;
		}
	}

	return false;
}

static bool
dupfind_loop2(struct v7fs_self *fs, v7fs_daddr_t listblk, v7fs_daddr_t dupblk,
    v7fs_daddr_t newblk)
{
	v7fs_daddr_t list[V7FS_DADDR_PER_BLOCK];
	v7fs_daddr_t blk;
	size_t i;

	prepare_list(fs, listblk, list);
	for (i = 0; i < V7FS_DADDR_PER_BLOCK; i++) {
		if ((blk = list[i]) == dupblk) {
			replace_block_indexed(fs, listblk, i, newblk);
			return true;
		}
		if (dupfind_loop1(fs, blk, dupblk, newblk))
			return true;
	}

	return false;
}

static void
do_replace(struct v7fs_self *fs, struct v7fs_inode *p, v7fs_daddr_t dupblk,
    v7fs_daddr_t newblk)
{
	size_t i;
	v7fs_daddr_t blk, blk2;
	v7fs_daddr_t list[V7FS_DADDR_PER_BLOCK];

	/* Direct */
	for (i = 0; i < V7FS_NADDR_DIRECT; i++)	{
		if (p->addr[i] == dupblk) {
			replace_block_direct(fs, p, i, newblk);
			return;
		}
	}

	/* Index 1 */
	if ((blk = p->addr[V7FS_NADDR_INDEX1]) == dupblk) {
		replace_block_direct(fs, p, V7FS_NADDR_INDEX1, newblk);
		return;
	}
	if (dupfind_loop1(fs, blk, dupblk, newblk))
		return;

	/* Index 2 */
	if ((blk = p->addr[V7FS_NADDR_INDEX2]) == dupblk) {
		replace_block_direct(fs, p, V7FS_NADDR_INDEX2, newblk);
		return;
	}
	if (dupfind_loop2(fs, blk, dupblk, newblk))
		return;

	/* Index 3 */
	if ((blk = p->addr[V7FS_NADDR_INDEX3]) == dupblk) {
		replace_block_direct(fs, p, V7FS_NADDR_INDEX3, newblk);
		return;
	}
	prepare_list(fs, blk, list);
	for (i = 0; i < V7FS_DADDR_PER_BLOCK; i++) {
		if ((blk2 = list[i]) == dupblk) {
			replace_block_indexed(fs, blk, i, newblk);
			return;
		}
		if (dupfind_loop2(fs, blk2, dupblk, newblk))
			return;
	}
}

static void
datablock_dup_remove(struct v7fs_self *fs, v7fs_ino_t i, v7fs_ino_t j,
    v7fs_daddr_t dupblk)
{
	struct v7fs_inode inode;
	v7fs_ino_t victim;
	v7fs_daddr_t newblk;
	int error;

	pwarn("Is victim %s (%s is preserved)", filename(fs, i),
	    filename(fs, j));
	if (reply("?"))	{
		victim =  i;
	} else {
		pwarn("Is victim %s (%s is preserved)",
		    filename(fs, j), filename(fs, i));
		if (reply("?")) {
			victim = j;
		} else {
			pwarn("Don't correct.\n");
			return;
		}
	}

	if ((error = v7fs_inode_load(fs, &inode, victim)))
		return;

	/* replace block. */
	if ((error = v7fs_datablock_allocate(fs, &newblk))) {
		pwarn("Can't allocate substitute block.");
		return;
	}

	do_replace(fs, &inode, dupblk, newblk);
}

