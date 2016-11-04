/*	$NetBSD: vnode_impl.h,v 1.2.2.2 2016/11/04 14:49:22 pgoyette Exp $	*/

/*-
 * Copyright (c) 2016 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by 
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

enum vnode_state {
	VS_MARKER,	/* Stable, used as marker. Will not change. */
	VS_LOADING,	/* Intermediate, initialising the fs node. */
	VS_ACTIVE,	/* Stable, valid fs node attached. */
	VS_BLOCKED,	/* Intermediate, active, no new references allowed. */
	VS_RECLAIMING,	/* Intermediate, detaching the fs node. */
	VS_RECLAIMED	/* Stable, no fs node attached. */
};
struct vcache_key {
	struct mount *vk_mount;
	const void *vk_key;
	size_t vk_key_len;
};
struct vnode_impl {
	struct vnode vi_vnode;
	enum vnode_state vi_state;
	SLIST_ENTRY(vnode_impl) vi_hash;
	struct vcache_key vi_key;
};
typedef struct vnode_impl vnode_impl_t;

#define VIMPL_TO_VNODE(node)	((vnode_t *)(node))
#define VNODE_TO_VIMPL(vp)	((vnode_impl_t *)(vp))

/*
 * Vnode manipulation functions.
 */
const char *
	vstate_name(enum vnode_state);
vnode_t *
	vnalloc_marker(struct mount *);
void	vnfree_marker(vnode_t *);
bool	vnis_marker(vnode_t *);

#endif /* !_SYS_VNODE_IMPL_H_ */
