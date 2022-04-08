/*	$NetBSD: inode.c,v 1.2 2022/04/08 10:17:53 andvar Exp $	*/

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
__RCSID("$NetBSD: inode.c,v 1.2 2022/04/08 10:17:53 andvar Exp $");
#endif /* not lint */

#include <sys/types.h>
#include <stdio.h>
#include <string.h>

#include "v7fs.h"
#include "v7fs_impl.h"
#include "v7fs_inode.h"
#include "v7fs_superblock.h"
#include "fsck_v7fs.h"

struct ilistcheck_arg {
	int total;
	int alloc;
};

int
freeinode_check(struct v7fs_self *fs)
{
	struct v7fs_superblock *sb = &fs->superblock;
	v7fs_ino_t *f = sb->freeinode;
	int16_t n = sb->nfreeinode;
	int16_t i, j;
	int bogus = false;

	/* Check # of cached free inode. */
	if (n > V7FS_MAX_FREEINODE || n < 0) {
		pwarn("*** corrupt nfreeinode %d (0-%d)***", n,
		    V7FS_MAX_FREEINODE);

		if (reply("PURGE?")) {
			sb->nfreeinode = 0;
			sb->modified = 1;
			v7fs_superblock_writeback(fs);
			return FSCK_EXIT_UNRESOLVED;
		}
		return FSCK_EXIT_CHECK_FAILED;
	}

	/* Check dup. */
	for (i = 0; i < n; i++)
		for (j = 0; j < i; j++)
			if (f[i] == f[j]) {
				pwarn("*** freeinode DUP %d %d", i, j);
				bogus = true;
			}
	if (bogus) {
		if (reply("PURGE?")) {
			memset(sb->freeinode, 0, sizeof(*sb->freeinode));
			sb->nfreeinode = 0;
			sb->modified = 1;
			v7fs_superblock_writeback(fs);
			return FSCK_EXIT_UNRESOLVED;
		} else {
			return FSCK_EXIT_CHECK_FAILED;
		}
	}

	return FSCK_EXIT_OK;
}

/* Counting freeinode and find partially allocated inode. */
static int
v7fs_inode_check(struct v7fs_self *fs, struct v7fs_inode *p, v7fs_ino_t ino)
{
	int error = 0;

	if (v7fs_inode_allocated(p) && !p->nlink) {
		pwarn("*** partially allocated inode #%d", ino);
		v7fs_inode_dump(p);
		if (reply_trivial("REMOVE?")) {
			memset(p, 0, sizeof(*p));
			v7fs_inode_deallocate(fs, ino);
		} else {
			error = FSCK_EXIT_CHECK_FAILED;
		}
	}

	return error;
}

static int
ilistcheck_subr(struct v7fs_self *fs, void *ctx, struct v7fs_inode *p,
    v7fs_ino_t ino)
{
	struct ilistcheck_arg *arg = (struct ilistcheck_arg *)ctx;
	int error = 0;

	if (ino != 1)
		error = v7fs_inode_check(fs, p, ino);

	arg->total++;
	if (v7fs_inode_allocated(p))
		arg->alloc++;

	return error;
}

int
ilist_check(struct v7fs_self *fs)
{
	struct v7fs_superblock *sb = &fs->superblock;
	struct ilistcheck_arg arg = { .total = 0, .alloc = 0 };
	int error = 0;

	if ((error = v7fs_ilist_foreach(fs, ilistcheck_subr, &arg)))
		return error;
	int nfree = arg.total - arg.alloc;

	if (nfree != sb->total_freeinode) {
		pwarn("*** corrupt total freeinode. %d(sb) != %d(cnt)\n",
		    sb->total_freeinode, nfree);
		if (reply_trivial("CORRECT?")) {
			sb->total_freeinode = nfree;
			sb->modified = true;
			v7fs_superblock_writeback(fs);
			v7fs_superblock_dump(fs);
		} else {
			error = FSCK_EXIT_CHECK_FAILED;
		}
	}

	pwarn("\ninode usage: %d/%d (%d)\n", arg.alloc, arg.total, nfree);
	return error;
}
