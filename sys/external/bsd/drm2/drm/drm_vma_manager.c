/*	$NetBSD: drm_vma_manager.c,v 1.1 2014/07/16 20:56:25 riastradh Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: drm_vma_manager.c,v 1.1 2014/07/16 20:56:25 riastradh Exp $");

#include <sys/kmem.h>
#include <sys/rbtree.h>
#include <sys/vmem.h>

#include <drm/drm_vma_manager.h>

static int
drm_vma_node_compare(void *cookie __unused, const void *va, const void *vb)
{
	const struct drm_vma_offset_node *const na = va;
	const struct drm_vma_offset_node *const nb = vb;

	if (na->von_startpage < nb->von_startpage)
		return -1;
	if (na->von_startpage > nb->von_startpage)
		return +1;
	return 0;
}

static int
drm_vma_node_compare_key(void *cookie __unused, const void *vn, const void *vk)
{
	const struct drm_vma_offset_node *const n = vn;
	const vmem_addr_t *const k = vk;

	if (n->von_startpage < *k)
		return -1;
	if (n->von_startpage > *k)
		return +1;
	return 0;
}

static const rb_tree_ops_t drm_vma_node_rb_ops = {
	.rbto_compare_nodes = &drm_vma_node_compare,
	.rbto_compare_key = &drm_vma_node_compare_key,
	.rbto_node_offset = offsetof(struct drm_vma_offset_node, von_rb_node),
	.rbto_context = NULL,
};

static int
drm_vma_file_compare(void *cookie __unused, const void *va, const void *vb)
{
	const struct drm_vma_offset_file *const fa = va;
	const struct drm_vma_offset_file *const fb = vb;

	if (fa->vof_file < fb->vof_file)
		return -1;
	if (fa->vof_file > fb->vof_file)
		return +1;
	return 0;
}

static int
drm_vma_file_compare_key(void *cookie __unused, const void *vf, const void *vk)
{
	const struct drm_vma_offset_file *const f = vf;
	const struct file *const k = vk;

	if (f->vof_file < k)
		return -1;
	if (f->vof_file > k)
		return +1;
	return 0;
}

static const rb_tree_ops_t drm_vma_file_rb_ops = {
	.rbto_compare_nodes = &drm_vma_file_compare,
	.rbto_compare_key = &drm_vma_file_compare_key,
	.rbto_node_offset = offsetof(struct drm_vma_offset_file, vof_rb_node),
	.rbto_context = NULL,
};

void
drm_vma_offset_manager_init(struct drm_vma_offset_manager *mgr,
    unsigned long startpage, unsigned long npages)
{

	rw_init(&mgr->vom_lock);
	rb_tree_init(&mgr->vom_nodes, &drm_vma_node_rb_ops);
	mgr->vom_vmem = vmem_create("drm_vma", startpage, npages, 1,
	    NULL, NULL, NULL, 0, VM_SLEEP, IPL_NONE);
}

void
drm_vma_offset_manager_destroy(struct drm_vma_offset_manager *mgr)
{

	vmem_destroy(mgr->vom_vmem);
#if 0
	rb_tree_destroy(&mgr->vom_nodes);
#endif
	rw_destroy(&mgr->vom_lock);
}

void
drm_vma_node_init(struct drm_vma_offset_node *node)
{
	static const struct drm_vma_offset_node zero_node;

	*node = zero_node;

	rw_init(&node->von_lock);
	node->von_startpage = 0;
	node->von_npages = 0;
	rb_tree_init(&node->von_files, &drm_vma_file_rb_ops);
}

void
drm_vma_node_destroy(struct drm_vma_offset_node *node)
{

#if 0
	rb_tree_destroy(&node->von_files);
#endif
	KASSERT(node->von_startpage == 0);
	KASSERT(node->von_npages == 0);
	rw_destroy(&node->von_lock);
}

int
drm_vma_offset_add(struct drm_vma_offset_manager *mgr,
    struct drm_vma_offset_node *node, unsigned long npages)
{
	vmem_size_t startpage;
	struct drm_vma_offset_node *collision __diagused;
	int error;

	KASSERT(npages != 0);

	if (0 < node->von_npages)
		return 0;

	error = vmem_alloc(mgr->vom_vmem, npages, VM_SLEEP|VM_BESTFIT,
	    &startpage);
	if (error)
		/* XXX errno NetBSD->Linux */
		return -error;

	node->von_startpage = startpage;
	node->von_npages = npages;

	rw_enter(&node->von_lock, RW_WRITER);
	collision = rb_tree_insert_node(&mgr->vom_nodes, node);
	KASSERT(collision == node);
	rw_exit(&node->von_lock);

	return 0;
}

void
drm_vma_offset_remove(struct drm_vma_offset_manager *mgr,
    struct drm_vma_offset_node *node)
{

	if (node->von_npages == 0)
		return;

	rw_enter(&node->von_lock, RW_WRITER);
	rb_tree_remove_node(&mgr->vom_nodes, node);
	rw_exit(&node->von_lock);

	vmem_free(mgr->vom_vmem, node->von_startpage, node->von_npages);

	node->von_npages = 0;
	node->von_startpage = 0;
}

void
drm_vma_offset_lock_lookup(struct drm_vma_offset_manager *mgr)
{

	rw_enter(&mgr->vom_lock, RW_READER);
}

void
drm_vma_offset_unlock_lookup(struct drm_vma_offset_manager *mgr)
{

	rw_exit(&mgr->vom_lock);
}

struct drm_vma_offset_node *
drm_vma_offset_lookup_locked(struct drm_vma_offset_manager *mgr,
    unsigned long startpage, unsigned long npages)
{
	const vmem_addr_t key = startpage;
	struct drm_vma_offset_node *node;

	KASSERT(rw_lock_held(&mgr->vom_lock));

	node = rb_tree_find_node_leq(&mgr->vom_nodes, &key);
	if (node == NULL)
		return NULL;
	KASSERT(node->von_startpage <= startpage);
	if (npages < node->von_npages)
		return NULL;
	if (node->von_npages - npages < startpage - node->von_startpage)
		return NULL;

	return node;
}

struct drm_vma_offset_node *
drm_vma_offset_exact_lookup(struct drm_vma_offset_manager *mgr,
    unsigned long startpage, unsigned long npages)
{
	const vmem_addr_t key = startpage;
	struct drm_vma_offset_node *node;

	rw_enter(&mgr->vom_lock, RW_READER);

	node = rb_tree_find_node(&mgr->vom_nodes, &key);
	if (node == NULL)
		goto out;
	KASSERT(node->von_startpage == startpage);
	if (node->von_npages != npages) {
		node = NULL;
		goto out;
	}

out:	rw_exit(&mgr->vom_lock);
	return node;
}

int
drm_vma_node_allow(struct drm_vma_offset_node *node, struct file *file)
{
	struct drm_vma_offset_file *new, *old;

	new = kmem_alloc(sizeof(*new), KM_NOSLEEP);
	if (new == NULL)
		return -ENOMEM;
	new->vof_file = file;

	rw_enter(&node->von_lock, RW_WRITER);
	old = rb_tree_insert_node(&node->von_files, new);
	rw_exit(&node->von_lock);

	if (old != new)		/* collision */
		kmem_free(new, sizeof(*new));

	return 0;
}

void
drm_vma_node_revoke(struct drm_vma_offset_node *node, struct file *file)
{

	rw_enter(&node->von_lock, RW_WRITER);
	struct drm_vma_offset_file *const found =
	    rb_tree_find_node(&node->von_files, file);
	if (found != NULL)
		rb_tree_remove_node(&node->von_files, found);
	rw_exit(&node->von_lock);
}

bool
drm_vma_node_is_allowed(struct drm_vma_offset_node *node, struct file *file)
{

	rw_enter(&node->von_lock, RW_READER);
	const bool allowed =
	    (rb_tree_find_node(&node->von_files, file) != NULL);
	rw_exit(&node->von_lock);

	return allowed;
}

int
drm_vma_node_verify_access(struct drm_vma_offset_node *node, struct file *file)
{

	if (!drm_vma_node_is_allowed(node, file))
		return -EACCES;

	return 0;
}
