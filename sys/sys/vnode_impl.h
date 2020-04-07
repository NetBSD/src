/*	$NetBSD: vnode_impl.h,v 1.21 2020/02/23 22:14:04 ad Exp $	*/

/*-
 * Copyright (c) 2016, 2019, 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
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

#ifndef _SYS_VNODE_IMPL_H_
#define	_SYS_VNODE_IMPL_H_

#include <sys/vnode.h>

struct namecache;
struct nchnode;

enum vnode_state {
	VS_ACTIVE,	/* Assert only, fs node attached and usecount > 0. */
	VS_MARKER,	/* Stable, used as marker. Will not change. */
	VS_LOADING,	/* Intermediate, initialising the fs node. */
	VS_LOADED,	/* Stable, valid fs node attached. */
	VS_BLOCKED,	/* Intermediate, active, no new references allowed. */
	VS_RECLAIMING,	/* Intermediate, detaching the fs node. */
	VS_RECLAIMED	/* Stable, no fs node attached. */
};

TAILQ_HEAD(vnodelst, vnode_impl);
typedef struct vnodelst vnodelst_t;

struct vcache_key {
	struct mount *vk_mount;
	const void *vk_key;
	size_t vk_key_len;
};

/*
 * Reading or writing any of these items requires holding the appropriate
 * lock.  Field markings and the corresponding locks:
 *
 *	-	stable throughout the life of the vnode
 *	c	vcache_lock
 *	d	vdrain_lock
 *	i	v_interlock
 *	l	vi_nc_listlock
 *	m	mnt_vnodelock
 *	n	namecache_lock
 *	s	syncer_data_lock
 */
struct vnode_impl {
	struct vnode vi_vnode;

	/*
	 * Largely stable data.
	 */
	struct vcache_key vi_key;		/* c   vnode cache key */

	/*
	 * Namecache.  Give it a separate line so activity doesn't impinge
	 * on the stable stuff (pending merge of ad-namecache branch).
	 */
	LIST_HEAD(, namecache) vi_dnclist	/* n: namecaches (children) */
	    __aligned(COHERENCY_UNIT);
	TAILQ_HEAD(, namecache) vi_nclist;	/* n: namecaches (parent) */

	/*
	 * vnode cache, LRU and syncer.  This all changes with some
	 * regularity so keep it together.
	 */
	struct vnodelst	*vi_lrulisthd		/* d   current lru list head */
	    __aligned(COHERENCY_UNIT);
	TAILQ_ENTRY(vnode_impl) vi_lrulist;	/* d   lru list */
	int 		vi_synclist_slot;	/* s   synclist slot index */
	int 		vi_lrulisttm;		/* i   time of lru enqueue */
	TAILQ_ENTRY(vnode_impl) vi_synclist;	/* s   vnodes with dirty bufs */
	SLIST_ENTRY(vnode_impl) vi_hash;	/* c   vnode cache list */
	enum vnode_state vi_state;		/* i   current state */

	/*
	 * Locks and expensive to access items which can be expected to
	 * generate a cache miss.
	 */
	krwlock_t	vi_lock			/* -   lock for this vnode */
	    __aligned(COHERENCY_UNIT);
	krwlock_t	vi_nc_lock;		/* -   lock on node */
	krwlock_t	vi_nc_listlock;		/* -   lock on nn_list */
	TAILQ_ENTRY(vnode_impl) vi_mntvnodes;	/* m   vnodes for mount point */
};
typedef struct vnode_impl vnode_impl_t;

#define VIMPL_TO_VNODE(vip)	(&(vip)->vi_vnode)
#define VNODE_TO_VIMPL(vp)	container_of((vp), struct vnode_impl, vi_vnode)

/*
 * Vnode state assertion.
 */
void _vstate_assert(vnode_t *, enum vnode_state, const char *, int, bool);

#if defined(DIAGNOSTIC) 

#define VSTATE_ASSERT(vp, state) \
	_vstate_assert((vp), (state), __func__, __LINE__, true)
#define VSTATE_ASSERT_UNLOCKED(vp, state) \
	_vstate_assert((vp), (state), __func__, __LINE__, false)

#else /* defined(DIAGNOSTIC) */

#define VSTATE_ASSERT(vp, state)
#define VSTATE_ASSERT_UNLOCKED(vp, state)

#endif /* defined(DIAGNOSTIC) */

/*
 * Vnode manipulation functions.
 */
const char *
	vstate_name(enum vnode_state);
vnode_t *
	vnalloc_marker(struct mount *);
void	vnfree_marker(vnode_t *);
bool	vnis_marker(vnode_t *);
void	vcache_make_anon(vnode_t *);
int	vcache_vget(vnode_t *);
int	vcache_tryvget(vnode_t *);
int	vfs_drainvnodes(void);

#endif /* !_SYS_VNODE_IMPL_H_ */
