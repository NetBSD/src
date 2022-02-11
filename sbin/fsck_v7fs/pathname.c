/*	$NetBSD: pathname.c,v 1.2 2022/02/11 10:55:15 hannken Exp $	*/

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
__RCSID("$NetBSD: pathname.c,v 1.2 2022/02/11 10:55:15 hannken Exp $");
#endif /* not lint */

#include <sys/types.h>
#include <sys/syslimits.h>	/*PATH_MAX */
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "v7fs.h"
#include "v7fs_impl.h"
#include "v7fs_inode.h"
#include "v7fs_datablock.h"
#include "v7fs_superblock.h"
#include "v7fs_dirent.h"
#include "v7fs_file.h"
#include "fsck_v7fs.h"

/*
 * Check pathname. for each inode, check parent, if it is directory,
 * check child.
 */

static int
connect_lost_and_found(struct v7fs_self *fs, v7fs_ino_t ino)
{
	char name[V7FS_NAME_MAX];
	v7fs_time_t t;

	if (!lost_and_found.mode || !reply("CONNECT?"))
		return FSCK_EXIT_CHECK_FAILED;

	snprintf(name, sizeof(name), "%d", ino);
	v7fs_directory_add_entry(fs, &lost_and_found, ino, name, strlen(name));
	t = (v7fs_time_t)time(NULL);
	lost_and_found.mtime = lost_and_found.atime = t;
	v7fs_inode_writeback(fs, &lost_and_found);
	v7fs_superblock_writeback(fs); /* # of freeblocks may change. */

	return 0;
}

/* Check child (dir) */
struct lookup_child_arg {
	int dir_cnt;
	bool print;
	struct v7fs_inode *parent;
};

static int
lookup_child_subr(struct v7fs_self *fs, void *ctx, v7fs_daddr_t blk, size_t sz)
{
	struct lookup_child_arg *arg = (struct lookup_child_arg *)ctx;
	void *buf;
	int error = 0;

	if (!(buf = scratch_read(fs, blk)))
		return 0;
	struct v7fs_dirent *dir = (struct v7fs_dirent *)buf;
	size_t i, n = sz / sizeof(*dir);
	if (!v7fs_dirent_endian_convert(fs, dir, n)) {
		pwarn("*** bogus entry found *** dir#%d entry=%zu\n",
		    arg->parent->inode_number, n);
		arg->print = true;
	}

	for (i = 0; i < n; i++, dir++) {
		struct v7fs_inode inode;
		if (arg->print)
			pwarn("%s %d\n", dir->name, dir->inode_number);
		/* Bogus enties are removed here. */
		if ((error = v7fs_inode_load(fs, &inode, dir->inode_number)))
		{
			pwarn("entry #%d not found.", dir->inode_number);
			if (reply("REMOVE?"))
				v7fs_directory_remove_entry(fs, arg->parent,
				    dir->name, strlen(dir->name));
		} else {
			/* Count child dir. */
			if (v7fs_inode_isdir(&inode))
				arg->dir_cnt++;
		}
	}
	scratch_free(fs, buf);

	return error;
}

static int
lookup_child_from_dir(struct v7fs_self *fs, struct v7fs_inode *p, bool print)
{
	struct lookup_child_arg arg = { .dir_cnt = 0, .print = print,
	    .parent = p };

	v7fs_datablock_foreach(fs, p, lookup_child_subr, &arg);

	return arg.dir_cnt;
}

/* Find parent directory (file) */
struct lookup_parent_arg {
	v7fs_ino_t target_ino;
	v7fs_ino_t parent_ino;
};

static int
lookup_parent_from_file_subr(struct v7fs_self *fs, void *ctx,
    struct v7fs_inode *p, v7fs_ino_t ino)
{
	struct lookup_parent_arg *arg = (struct lookup_parent_arg *)ctx;

	if (!v7fs_inode_isdir(p))
		return 0;

	if (v7fs_file_lookup_by_number(fs, p, arg->target_ino, NULL)) {
		arg->parent_ino = ino; /* My inode found. */
		return V7FS_ITERATOR_BREAK;
	}

	return 0; /* not found. */
}


static v7fs_ino_t
lookup_parent_from_file(struct v7fs_self *fs, v7fs_ino_t ino)
{
	struct lookup_parent_arg arg = { .target_ino = ino, .parent_ino = 0 };

	v7fs_ilist_foreach(fs, lookup_parent_from_file_subr, &arg);

	return arg.parent_ino;
}

/* Find parent directory (dir) */
static int
lookup_parent_from_dir_subr(struct v7fs_self *fs, void *ctx, v7fs_daddr_t blk,
    size_t sz)
{
	struct lookup_parent_arg *arg = (struct lookup_parent_arg *)ctx;
	void *buf;
	int ret = 0;

	if (!(buf = scratch_read(fs, blk)))
		return 0;
	struct v7fs_dirent *dir = (struct v7fs_dirent *)buf;
	size_t i, n = sz / sizeof(*dir);
	if (!v7fs_dirent_endian_convert(fs, dir, n)) {
		scratch_free(fs, buf);
		return V7FS_ITERATOR_ERROR;
	}

	for (i = 0; i < n; i++, dir++) {
		if (strncmp(dir->name, "..", 2) != 0)
			continue;

		arg->parent_ino = dir->inode_number;
		ret = V7FS_ITERATOR_BREAK;
		break;
	}

	scratch_free(fs, buf);
	return ret;
}

static v7fs_ino_t
lookup_parent_from_dir(struct v7fs_self *fs, struct v7fs_inode *p)
{
	struct lookup_parent_arg arg = { .target_ino = 0, .parent_ino = 0 };

	/* Search parent("..") from my dirent. */
	v7fs_datablock_foreach(fs, p, lookup_parent_from_dir_subr, &arg);

	return arg.parent_ino;
}

static int
pathname_check_file(struct v7fs_self *fs, struct v7fs_inode *p, v7fs_ino_t ino)
{
	v7fs_ino_t parent_ino;
	struct v7fs_inode parent_inode;
	int error = 0;

	if (ino == 1)	/* reserved. */
		return 0;

	/* Check parent. */
	if (!(parent_ino = lookup_parent_from_file(fs, ino)) ||
	    (error = v7fs_inode_load(fs, &parent_inode, parent_ino)) ||
	    !v7fs_inode_isdir(&parent_inode)) {
		pwarn("*** inode#%d don't have parent.", ino);
		v7fs_inode_dump(p);
		error = connect_lost_and_found(fs, ino);
	}

	return error;
}

static int
pathname_check_dir(struct v7fs_self *fs, struct v7fs_inode *p, v7fs_ino_t ino)
{
	v7fs_ino_t parent_ino;
	struct v7fs_inode parent_inode;
	int error = 0;

	/* Check parent */
	if (!(parent_ino = lookup_parent_from_dir(fs, p)) ||
	    (error = v7fs_inode_load(fs, &parent_inode, parent_ino)) ||
	    !v7fs_inode_isdir(&parent_inode)) {
		pwarn("*** ino#%d parent dir missing parent=%d", ino,
		    parent_ino);
		/* link to lost+found */
		v7fs_inode_dump(p);
		if ((error = connect_lost_and_found(fs, ino)))
			return error;
	}

	/* Check child */
	int cnt = lookup_child_from_dir(fs, p, false);
	if ((error = (cnt != p->nlink))) {
		pwarn("*** ino#%d corrupt link # of child"
		    " dir:%d(inode) != %d(cnt)", ino, p->nlink, cnt);
		v7fs_inode_dump(p);
		lookup_child_from_dir(fs, p, true);

		if (reply("CORRECT?")) {
			p->nlink = cnt;
			v7fs_inode_writeback(fs, p);
			error = 0;
		} else {
			error = FSCK_EXIT_CHECK_FAILED;
		}
	}

	return error;
}

static int
pathname_subr(struct v7fs_self *fs, void *ctx __unused, struct v7fs_inode *p,
    v7fs_ino_t ino)
{
	int error = 0;

	if (!v7fs_inode_allocated(p))
		return 0;

	progress(0);

	if (v7fs_inode_isdir(p)) {
		error = pathname_check_dir(fs, p, ino);
	} else if (v7fs_inode_isfile(p)) {
		error = pathname_check_file(fs, p, ino);
	}

	return error;
}

int
pathname_check(struct v7fs_self *fs)
{
	struct v7fs_superblock *sb = &fs->superblock;
	int inodes = V7FS_MAX_INODE(sb) - sb->total_freeinode;

	progress(&(struct progress_arg){ .label = "pathname", .tick = inodes /
	    PROGRESS_BAR_GRANULE });

	return v7fs_ilist_foreach(fs, pathname_subr, 0);
}


char *
filename(struct v7fs_self *fs, v7fs_ino_t ino)
{
	static char path[V7FS_PATH_MAX];
	struct v7fs_inode inode;
	v7fs_ino_t parent;
	int error;
	char name[V7FS_NAME_MAX + 1];
	char *p = path + V7FS_PATH_MAX;
	size_t n;

	if ((error = v7fs_inode_load(fs, &inode, ino)))
		return 0;

	/* Lookup the 1st parent. */
	if (v7fs_inode_isdir(&inode))
		parent = lookup_parent_from_dir(fs, &inode);
	else
		parent = lookup_parent_from_file(fs, ino);

	if ((error = v7fs_inode_load(fs, &inode, parent)))
		return 0;

	if (!v7fs_file_lookup_by_number(fs, &inode, ino, name))
		return 0;

	n = strlen(name) + 1;
	strcpy(p -= n, name);

	/* Lookup until / */
	ino = parent;
	while (parent != V7FS_ROOT_INODE) {
		parent = lookup_parent_from_dir(fs, &inode);
		if ((error = v7fs_inode_load(fs, &inode, parent)))
			return 0;
		if (!v7fs_file_lookup_by_number(fs, &inode, ino, name))
			return 0;
		ino = parent;
		n = strlen(name) + 1;
		strcpy(p - n, name);
		p[-1] = '/';
		p -= n;
	}
	*--p = '/';

	return p;
}
