/*	$NetBSD: main.c,v 1.2 2022/02/11 10:55:15 hannken Exp $	*/

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
__RCSID("$NetBSD: main.c,v 1.2 2022/02/11 10:55:15 hannken Exp $");
#endif /* not lint */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <err.h>
#include <time.h>

#include "v7fs.h"
#include "v7fs_impl.h"
#include "v7fs_superblock.h"
#include "v7fs_inode.h"
#include "v7fs_file.h"

#include "fsck_v7fs.h"
#include "progress.h"

static int check_filesystem(struct v7fs_self *, int);
static int make_lost_and_found(struct v7fs_self *, struct v7fs_inode *);

struct v7fs_inode lost_and_found;

int
v7fs_fsck(const struct v7fs_mount_device *mount, int flags)
{
	struct v7fs_self *fs;
	int error;

	if ((error = v7fs_io_init(&fs, mount, V7FS_BSIZE))) {
		pfatal("I/O setup failed");
		return FSCK_EXIT_CHECK_FAILED;
	}

	if ((error = v7fs_superblock_load(fs))) {
		if ((error != EINVAL)) {
			pfatal("Can't load superblock");
			return FSCK_EXIT_CHECK_FAILED;
		}
		pwarn("Inavalid superblock");
		if (!reply_trivial("CONTINUE?")) {
			return FSCK_EXIT_CHECK_FAILED;
		}
	}

	error = check_filesystem(fs, flags);

	printf("write backing...(no progress report)"); fflush(stdout);
	v7fs_io_fini(fs);
	printf("done.\n");

	return error;
}

int
check_filesystem(struct v7fs_self *fs, int flags)
{
	int error;

	/* Check superblock cached freeinode list. */
	pwarn("[Superblock information]\n");
	v7fs_superblock_dump(fs);
	pwarn("[1] checking free inode in superblock...\n");
	if ((error = freeinode_check(fs)))
		return error;

	/* Check free block linked list. */
	pwarn("[2] checking free block link...\n");
	if ((error = freeblock_check(fs)))
		return error;

	/* Check inode all. */
	pwarn("[3] checking all ilist...\n");
	if ((error = ilist_check(fs)))
		return error;

	/* Setup lost+found. */
	pwarn("prepare lost+found\n");
	if ((error = make_lost_and_found(fs, &lost_and_found)))
		return FSCK_EXIT_CHECK_FAILED;

	/* Check path(child and parent). Orphans are linked to lost+found. */
	pwarn("[4] checking path name...\n");
	if ((error = pathname_check(fs)))
		return error;

	if (flags & V7FS_FSCK_FREEBLOCK_DUP) {
		/* Check duplicated block in freeblock. */
		pwarn("[5] checking freeblock duplication...\n");
		if ((error = freeblock_vs_freeblock_check(fs)))
			return error;
	}

	if (flags & V7FS_FSCK_DATABLOCK_DUP) {
		/* Check duplicated block in datablock. */
		pwarn("[6] checking datablock duplication(vs datablock)...\n");
		if ((error = datablock_vs_datablock_check(fs)))
			return error;
	}

	if ((flags & V7FS_FSCK_DATABLOCK_DUP) && (flags &
		V7FS_FSCK_FREEBLOCK_DUP)) {
		/* Check off-diagonal. */
		pwarn("[7] checking datablock duplication (vs freeblock)...\n");
		if ((error = datablock_vs_freeblock_check(fs)))
			return error;
	}

	return 0;
}

static int
reply_subr(const char *str, bool trivial)
{
	int c;
	char *line = NULL;
	size_t linesize = 0;
	ssize_t linelen;

	printf("%s ", str);
	switch (fsck_operate) {
	case PREEN:
		return trivial;
	case ALWAYS_YES:
		printf("[Y]\n");
		return 1;
	case ALWAYS_NO:
		if (strstr(str, "CONTINUE")) {
			printf("[Y]\n");
			return 1;
		}
		printf("[N]\n");
		return 0;
	case ASK:
		break;
	}
	fflush(stdout);

	if ((linelen = getline(&line, &linesize, stdin)) == -1)	{
		clearerr(stdin);
		return 0;
	}
	c = line[0];

	return c == 'y' || c == 'Y';
}

int
reply(const char *str)
{
	return reply_subr(str, false);
}

int
reply_trivial(const char *str)
{
	return reply_subr(str, true);
}

void
progress(const struct progress_arg *p)
{
	static struct progress_arg Progress;
	static char cdev[32];
	static char label[32];

	if (p) {
		if (Progress.tick) {
			progress_done();
		}
		Progress = *p;
		if (p->cdev)
			strcpy(cdev, p->cdev);
		if (p->label)
			strcpy(label, p->label);
	}

	if (fsck_operate == PREEN)
		return;
	if (!Progress.tick)
		return;
	if (++Progress.cnt > Progress.tick) {
		Progress.cnt = 0;
		Progress.total++;
		progress_bar(cdev, label, Progress.total, PROGRESS_BAR_GRANULE);
	}
}

int
make_lost_and_found(struct v7fs_self *fs, struct v7fs_inode *p)
{
	struct v7fs_inode root_inode;
	struct v7fs_fileattr attr;
	v7fs_ino_t ino;
	int error = 0;

	if ((error = v7fs_inode_load(fs, &root_inode, V7FS_ROOT_INODE))) {
		errno = error;
		warn("(%s) / missing.", __func__);
		return error;
	}

	memset(&attr, 0, sizeof(attr));
	attr.uid = 0;
	attr.gid = 0;
	attr.mode = V7FS_IFDIR | 0755;
	attr.device = 0;
	attr.ctime = attr.mtime = attr.atime = (v7fs_time_t)time(NULL);

	/* If lost+found already exists, returns EEXIST */
	if (!(error = v7fs_file_allocate(fs, &root_inode,
	    "lost+found", strlen("lost+found"), &attr, &ino)))
		v7fs_superblock_writeback(fs);

	if (error == EEXIST)
		error = 0;

	if (error) {
		errno = error;
		warn("(%s) Couldn't create lost+found", __func__);
		return error;
	}

	if ((error = v7fs_inode_load(fs, p, ino))) {
		errno = error;
		warn("(%s:)lost+found is created, but missing.", __func__);
	}

	return error;
}
