/*	$NetBSD: drm_vma_manager.h,v 1.1.2.2 2014/08/10 06:55:39 tls Exp $	*/

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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

#ifndef	_DRM_DRM_VMA_MANAGER_H_
#define	_DRM_DRM_VMA_MANAGER_H_

#include <sys/types.h>
#include <sys/param.h>
#include <sys/rbtree.h>
#include <sys/rwlock.h>
#include <sys/vmem.h>

struct drm_vma_offset_manager {
	krwlock_t	vom_lock;
	struct rb_tree	vom_nodes;
	struct vmem	*vom_vmem;
};

struct drm_vma_offset_node {
	krwlock_t	von_lock;
	vmem_addr_t	von_startpage;
	vmem_size_t	von_npages;
	struct rb_tree	von_files;
	struct rb_node	von_rb_node;
};

static inline unsigned long
drm_vma_node_start(struct drm_vma_offset_node *node)
{
	return node->von_startpage;
}

static inline unsigned long
drm_vma_node_size(struct drm_vma_offset_node *node)
{
	return node->von_npages;
}

static inline bool
drm_vma_node_has_offset(struct drm_vma_offset_node *node)
{
	return (node->von_npages != 0);
}

static inline uint64_t
drm_vma_node_offset_addr(struct drm_vma_offset_node *node)
{
	return (uint64_t)node->von_startpage << PAGE_SHIFT;
}

struct drm_vma_offset_file {
	struct file	*vof_file;
	struct rb_node	vof_rb_node;
};

void	drm_vma_offset_manager_init(struct drm_vma_offset_manager *,
	    unsigned long, unsigned long);
void	drm_vma_offset_manager_destroy(struct drm_vma_offset_manager *);

/*
 * This is called drm_vma_node_reset upstream.  Name is changed so you
 * have to find calls and match them with drm_vma_node_destroy.
 */
void	drm_vma_node_init(struct drm_vma_offset_node *);

/* This does not exist upstream.  */
void	drm_vma_node_destroy(struct drm_vma_offset_node *);

int	drm_vma_offset_add(struct drm_vma_offset_manager *,
	    struct drm_vma_offset_node *, unsigned long);
void	drm_vma_offset_remove(struct drm_vma_offset_manager *,
	    struct drm_vma_offset_node *);

void	drm_vma_offset_lock_lookup(struct drm_vma_offset_manager *);
void	drm_vma_offset_unlock_lookup(struct drm_vma_offset_manager *);
struct drm_vma_offset_node *
	drm_vma_offset_lookup_locked(struct drm_vma_offset_manager *,
	    unsigned long, unsigned long);
struct drm_vma_offset_node *
	drm_vma_offset_exact_lookup(struct drm_vma_offset_manager *,
	    unsigned long, unsigned long);

int	drm_vma_node_allow(struct drm_vma_offset_node *, struct file *);
void	drm_vma_node_revoke(struct drm_vma_offset_node *, struct file *);
bool	drm_vma_node_is_allowed(struct drm_vma_offset_node *, struct file *);
int	drm_vma_node_verify_access(struct drm_vma_offset_node *,
	    struct file *);

#endif	/* _DRM_DRM_VMA_MANAGER_H_ */
