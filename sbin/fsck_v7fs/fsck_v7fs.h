/*	$NetBSD: fsck_v7fs.h,v 1.1 2011/06/27 11:52:58 uch Exp $	*/

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

#ifndef _SBIN_FSCK_V7FS_FSCK_V7FS_H_
#define	_SBIN_FSCK_V7FS_FSCK_V7FS_H_

enum fsck_operate {
	PREEN,
	ASK,
	ALWAYS_YES,
	ALWAYS_NO,
};

#define	V7FS_FSCK_DATABLOCK_DUP	0x01
#define	V7FS_FSCK_FREEBLOCK_DUP	0x02

#define	PROGRESS_BAR_GRANULE	100
struct progress_arg {
	const char *cdev;
	const char *label;
	off_t tick;
	off_t cnt;
	off_t total;
};

__BEGIN_DECLS
char *filename(struct v7fs_self *, v7fs_ino_t);
void progress(const struct progress_arg *);

int v7fs_fsck(const struct v7fs_mount_device *, int);
int reply(const char *);
int reply_trivial(const char *);

int pathname_check(struct v7fs_self *);
int ilist_check(struct v7fs_self *);
int freeinode_check(struct v7fs_self *);
int freeblock_check(struct v7fs_self *);
int datablock_vs_datablock_check(struct v7fs_self *);
int datablock_vs_freeblock_check(struct v7fs_self *);
int freeblock_vs_freeblock_check(struct v7fs_self *);

void freeblock_dup_remove(struct v7fs_self *, v7fs_daddr_t);

int v7fs_freeblock_foreach(struct v7fs_self *,
    int (*)(struct v7fs_self *, void *, v7fs_daddr_t), void *);
__END_DECLS

extern enum fsck_operate fsck_operate;
extern struct v7fs_inode lost_and_found;

#include "fsutil.h"
#include "exitvalues.h"

#endif /*!_SBIN_FSCK_V7FS_FSCK_V7FS_H_ */
