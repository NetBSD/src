/*	$NetBSD: zfs_znode.h,v 1.1 2022/06/19 11:31:19 simonb Exp $	*/

/*
 * XXX From: external/cddl/osnet/dist/uts/common/fs/zfs/sys/zfs_znode.h
 *
 * Grotty hack to define a "simple" znode_t so we don't need to
 * try to include the real zfs_znode.h which isn't easily possible
 * from userland due to all sorts of kernel only defines and structs
 * being required.
 */

/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright (c) 2005, 2010, Oracle and/or its affiliates. All rights reserved.
 * Copyright (c) 2012, 2015 by Delphix. All rights reserved.
 * Copyright (c) 2014 Integros [integros.com]
 */


#include <miscfs/genfs/genfs_node.h>

#define	VTOZ(VP)	((znode_t *)(VP)->v_data)

#define	ulong_t		unsigned long
#define	uint_t		unsigned int
#define	zfs_acl_t	void

struct avl_node;
typedef struct sa_handle sa_handle_t;
typedef int boolean_t;

typedef struct avl_tree {
	struct avl_node	*avl_root;
	int		(*avl_compar)(const void *, const void *);
	size_t		avl_offset;
	ulong_t		avl_numnodes;
	size_t		avl_size;
} avl_tree_t;

typedef struct list_node {
	struct list_node *list_next;
	struct list_node *list_prev;
} list_node_t;

typedef struct znode {
#ifdef __NetBSD__
	struct genfs_node z_gnode;
#endif
	struct zfsvfs	*z_zfsvfs;
	vnode_t		*z_vnode;
	uint64_t	z_id;		/* object ID for this znode */
#ifdef illumos
	kmutex_t	z_lock;		/* znode modification lock */
	krwlock_t	z_parent_lock;	/* parent lock for directories */
	krwlock_t	z_name_lock;	/* "master" lock for dirent locks */
	zfs_dirlock_t	*z_dirlocks;	/* directory entry lock list */
#endif
	kmutex_t	z_range_lock;	/* protects changes to z_range_avl */
	avl_tree_t	z_range_avl;	/* avl tree of file range locks */
	uint8_t		z_unlinked;	/* file has been unlinked */
	uint8_t		z_atime_dirty;	/* atime needs to be synced */
	uint8_t		z_zn_prefetch;	/* Prefetch znodes? */
	uint8_t		z_moved;	/* Has this znode been moved? */
	uint_t		z_blksz;	/* block size in bytes */
	uint_t		z_seq;		/* modification sequence number */
	uint64_t	z_mapcnt;	/* number of pages mapped to file */
	uint64_t	z_gen;		/* generation (cached) */
	uint64_t	z_size;		/* file size (cached) */
	uint64_t	z_atime[2];	/* atime (cached) */
	uint64_t	z_links;	/* file links (cached) */
	uint64_t	z_pflags;	/* pflags (cached) */
	uint64_t	z_uid;		/* uid fuid (cached) */
	uint64_t	z_gid;		/* gid fuid (cached) */
	mode_t		z_mode;		/* mode (cached) */
	uint32_t	z_sync_cnt;	/* synchronous open count */
	kmutex_t	z_acl_lock;	/* acl data lock */
	zfs_acl_t	*z_acl_cached;	/* cached acl */
	list_node_t	z_link_node;	/* all znodes in fs link */
	sa_handle_t	*z_sa_hdl;	/* handle to sa data */
	boolean_t	z_is_sa;	/* are we native sa? */
#ifdef __NetBSD__
	struct lockf	*z_lockf;	/* head of byte-level lock list */
#endif
} znode_t;
