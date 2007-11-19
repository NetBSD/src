/*	$NetBSD: devfs.h,v 1.1.2.1 2007/11/19 00:34:33 mjf Exp $	*/

/*
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Fleming.
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

#ifndef _FS_DEVFS_DEVFS_H_
#define _FS_DEVFS_DEVFS_H_

#include <sys/dirent.h>
#include <sys/mount.h>
#include <sys/queue.h>
#include <sys/vnode.h>

MALLOC_DECLARE(M_DEVFSMNT);	/* declared in devfs_vfsops.c */
MALLOC_DECLARE(M_DEVFS);	/* declared in devfs_subr.c */

#define DE_WHITEOUT     0x1
#define DE_DOT          0x2
#define DE_DOTDOT       0x4
#define DE_DOOMED       0x8

#if defined(_KERNEL)

/*
 * Internal representation of a devfs mount point. The beginning of this 
 * structure must be kept in-sync with struct tmpfs_mount because we make 
 * use of a lot of tmpfs functions and pass this argument as a pretend 
 * struct tmpfs_mount.
 */
struct devfs_mount {
	size_t                  tm_pages_max;
	size_t                  tm_pages_used;
	struct tmpfs_node *     tm_root;
	ino_t                   tm_nodes_max;
	ino_t                   tm_nodes_last;
	struct tmpfs_node_list  tm_nodes_used;
	struct tmpfs_node_list  tm_nodes_avail;
	struct tmpfs_pool       tm_dirent_pool;
	struct tmpfs_pool       tm_node_pool;
	struct tmpfs_str_pool   tm_str_pool;

	struct mount *		dm_mount;
};

static __inline
struct devfs_mount *
VFSTODEVFS(struct mount *mp)
{
	struct devfs_mount *dmp;

#ifdef KASSERT
	KASSERT((mp) != NULL && (mp)->mnt_data != NULL);
#endif
	dmp = (struct devfs_mount *)(mp)->mnt_data;
	return dmp;
}

#endif /* defined(_KERNEL) */

/*
 * This structure is used to communicate mount parameters between userland
 * and kernel space.
 */
#define DEVFS_ARGS_VERSION	1
struct devfs_args {
	int			ta_version;

	/* Size counters. */
	ino_t			ta_nodes_max;
	off_t			ta_size_max;

	/* Root node attributes. */
	uid_t			ta_root_uid;
	gid_t			ta_root_gid;
	mode_t			ta_root_mode;
};
#endif /* _FS_DEVFS_DEVFS_H_ */
