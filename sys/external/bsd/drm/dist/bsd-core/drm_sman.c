/**************************************************************************
 *
 * Copyright 2006 Tungsten Graphics, Inc., Bismarck., ND., USA.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 *
 **************************************************************************/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: drm_sman.c,v 1.1.8.2 2011/06/06 09:09:10 jruoho Exp $");

/*
 * Simple memory manager interface that keeps track on allocate regions on a
 * per "owner" basis. All regions associated with an "owner" can be released
 * with a simple call. Typically if the "owner" exists. The owner is any
 * "unsigned long" identifier. Can typically be a pointer to a file private
 * struct or a context identifier.
 *
 * Authors:
 * Thomas Hellström <thomas-at-tungstengraphics-dot-com>
 */

#include "drmP.h"
#include "drm_sman.h"

struct drm_owner_item {
	struct drm_hash_item owner_hash;
	struct list_head sman_list;
	struct list_head mem_blocks;
};

void drm_sman_takedown(struct drm_sman * sman)
{
	drm_ht_remove(&sman->user_hash_tab);
	drm_ht_remove(&sman->owner_hash_tab);
	if (sman->mm)
		drm_free(sman->mm, sman->num_managers * sizeof(*sman->mm),
		    DRM_MEM_MM);
}

int
drm_sman_init(struct drm_sman * sman, unsigned int num_managers,
	      unsigned int user_order, unsigned int owner_order)
{
	int ret = 0;

	sman->mm = (struct drm_sman_mm *) drm_calloc(num_managers,
	    sizeof(*sman->mm), DRM_MEM_MM);
	if (!sman->mm) {
		ret = -ENOMEM;
		goto out;
	}
	sman->num_managers = num_managers;
	INIT_LIST_HEAD(&sman->owner_items);
	ret = drm_ht_create(&sman->owner_hash_tab, owner_order);
	if (ret)
		goto out1;
	ret = drm_ht_create(&sman->user_hash_tab, user_order);
	if (!ret)
		goto out;

	drm_ht_remove(&sman->owner_hash_tab);
out1:
	drm_free(sman->mm, num_managers * sizeof(*sman->mm), DRM_MEM_MM);
out:
	return ret;
}

static void *drm_sman_mm_allocate(void *private, unsigned long size,
				  unsigned alignment)
{
	struct drm_mm *mm = (struct drm_mm *) private;
	struct drm_mm_node *tmp;

	tmp = drm_mm_search_free(mm, size, alignment, 1);
	if (!tmp) {
		return NULL;
	}
	/* This could be non-atomic, but we are called from a locked path */
	tmp = drm_mm_get_block_atomic(tmp, size, alignment);
	return tmp;
}

static void drm_sman_mm_free(void *private, void *ref)
{
	struct drm_mm_node *node = (struct drm_mm_node *) ref;

	drm_mm_put_block(node);
}

static void drm_sman_mm_destroy(void *private)
{
	struct drm_mm *mm = (struct drm_mm *) private;
	drm_mm_takedown(mm);
	drm_free(mm, sizeof(*mm), DRM_MEM_MM);
}

static unsigned long drm_sman_mm_offset(void *private, void *ref)
{
	struct drm_mm_node *node = (struct drm_mm_node *) ref;
	return node->start;
}

int
drm_sman_set_range(struct drm_sman * sman, unsigned int manager,
		   unsigned long start, unsigned long size)
{
	struct drm_sman_mm *sman_mm;
	struct drm_mm *mm;
	int ret;

	KASSERT(manager < sman->num_managers);

	sman_mm = &sman->mm[manager];
	mm = malloc(sizeof(*mm), DRM_MEM_MM, M_NOWAIT | M_ZERO);
	if (!mm) {
		return -ENOMEM;
	}
	sman_mm->private = mm;
	ret = drm_mm_init(mm, start, size);

	if (ret) {
		drm_free(mm, sizeof(*mm), DRM_MEM_MM);
		return ret;
	}

	sman_mm->allocate = drm_sman_mm_allocate;
	sman_mm->freem = drm_sman_mm_free;
	sman_mm->destroy = drm_sman_mm_destroy;
	sman_mm->offset = drm_sman_mm_offset;

	return 0;
}

int
drm_sman_set_manager(struct drm_sman * sman, unsigned int manager,
		     struct drm_sman_mm * allocator)
{
	KASSERT(manager < sman->num_managers);
	sman->mm[manager] = *allocator;

	return 0;
}

static struct drm_owner_item *drm_sman_get_owner_item(struct drm_sman * sman,
						 unsigned long owner)
{
	int ret;
	struct drm_hash_item *owner_hash_item;
	struct drm_owner_item *owner_item;

	ret = drm_ht_find_item(&sman->owner_hash_tab, owner, &owner_hash_item);
	if (!ret) {
		return drm_hash_entry(owner_hash_item, struct drm_owner_item,
				      owner_hash);
	}

	owner_item = malloc(sizeof(*owner_item), DRM_MEM_MM, M_NOWAIT | M_ZERO);
	if (!owner_item)
		goto out;

	INIT_LIST_HEAD(&owner_item->mem_blocks);
	owner_item->owner_hash.key = owner;
	DRM_DEBUG("owner_item = %p, mem_blocks = %p\n", owner_item, &owner_item->mem_blocks);
	if (drm_ht_insert_item(&sman->owner_hash_tab, &owner_item->owner_hash))
		goto out1;

	list_add_tail(&owner_item->sman_list, &sman->owner_items);
	return owner_item;

out1:
	drm_free(owner_item, sizeof(*owner_item), DRM_MEM_MM);
out:
	return NULL;
}

struct drm_memblock_item *drm_sman_alloc(struct drm_sman *sman, unsigned int manager,
				    unsigned long size, unsigned alignment,
				    unsigned long owner)
{
	void *tmp;
	struct drm_sman_mm *sman_mm;
	struct drm_owner_item *owner_item;
	struct drm_memblock_item *memblock;

	KASSERT(manager < sman->num_managers);

	sman_mm = &sman->mm[manager];
	tmp = sman_mm->allocate(sman_mm->private, size, alignment);
	if (!tmp) {
		return NULL;
	}

	memblock = malloc(sizeof(*memblock), DRM_MEM_MM, M_NOWAIT | M_ZERO);
	DRM_DEBUG("allocated mem_block %p\n", memblock);
	if (!memblock)
		goto out;

	memblock->mm_info = tmp;
	memblock->mm = sman_mm;
	memblock->sman = sman;
	INIT_LIST_HEAD(&memblock->owner_list);

	if (drm_ht_just_insert_please
	    (&sman->user_hash_tab, &memblock->user_hash,
	     (unsigned long)memblock, 32, 0, 0))
		goto out1;

	owner_item = drm_sman_get_owner_item(sman, owner);
	if (!owner_item)
		goto out2;

	DRM_DEBUG("owner_item = %p, mem_blocks = %p\n", owner_item, &owner_item->mem_blocks);
	DRM_DEBUG("owner_list.prev = %p, mem_blocks.prev = %p\n", memblock->owner_list.prev, owner_item->mem_blocks.prev);
	DRM_DEBUG("owner_list.next = %p, mem_blocks.next = %p\n", memblock->owner_list.next, owner_item->mem_blocks.next);
	list_add_tail(&memblock->owner_list, &owner_item->mem_blocks);

	DRM_DEBUG("Complete\n");
	return memblock;

out2:
	drm_ht_remove_item(&sman->user_hash_tab, &memblock->user_hash);
out1:
	drm_free(memblock, sizeof(*memblock), DRM_MEM_MM);
out:
	sman_mm->freem(sman_mm->private, tmp);

	return NULL;
}

static void drm_sman_free(struct drm_memblock_item *item)
{
	struct drm_sman *sman = item->sman;

	list_del(&item->owner_list);
	drm_ht_remove_item(&sman->user_hash_tab, &item->user_hash);
	item->mm->freem(item->mm->private, item->mm_info);
	drm_free(item, sizeof(*item), DRM_MEM_MM);
}

int drm_sman_free_key(struct drm_sman *sman, unsigned int key)
{
	struct drm_hash_item *hash_item;
	struct drm_memblock_item *memblock_item;

	if (drm_ht_find_item(&sman->user_hash_tab, key, &hash_item))
		return -EINVAL;

	memblock_item = drm_hash_entry(hash_item, struct drm_memblock_item,
				       user_hash);
	drm_sman_free(memblock_item);
	return 0;
}

static void drm_sman_remove_owner(struct drm_sman *sman,
				  struct drm_owner_item *owner_item)
{
	list_del(&owner_item->sman_list);
	drm_ht_remove_item(&sman->owner_hash_tab, &owner_item->owner_hash);
	drm_free(owner_item, sizeof(*owner_item), DRM_MEM_MM);
}

int drm_sman_owner_clean(struct drm_sman *sman, unsigned long owner)
{

	struct drm_hash_item *hash_item;
	struct drm_owner_item *owner_item;

	if (drm_ht_find_item(&sman->owner_hash_tab, owner, &hash_item)) {
		return -1;
	}

	owner_item = drm_hash_entry(hash_item, struct drm_owner_item, owner_hash);
	DRM_DEBUG("cleaning owner_item %p\n", owner_item);
	if (owner_item->mem_blocks.next == &owner_item->mem_blocks) {
		drm_sman_remove_owner(sman, owner_item);
		return -1;
	}

	return 0;
}

static void drm_sman_do_owner_cleanup(struct drm_sman *sman,
				      struct drm_owner_item *owner_item)
{
	struct drm_memblock_item *entry, *next;

	list_for_each_entry_safe(entry, next, &owner_item->mem_blocks,
				 owner_list) {
		DRM_DEBUG("freeing mem_block %p\n", entry);
		drm_sman_free(entry);
	}
	drm_sman_remove_owner(sman, owner_item);
}

void drm_sman_owner_cleanup(struct drm_sman *sman, unsigned long owner)
{

	struct drm_hash_item *hash_item;
	struct drm_owner_item *owner_item;

	if (drm_ht_find_item(&sman->owner_hash_tab, owner, &hash_item)) {

		return;
	}

	owner_item = drm_hash_entry(hash_item, struct drm_owner_item, owner_hash);
	drm_sman_do_owner_cleanup(sman, owner_item);
}

void drm_sman_cleanup(struct drm_sman *sman)
{
	struct drm_owner_item *entry, *next;
	unsigned int i;
	struct drm_sman_mm *sman_mm;

	DRM_DEBUG("sman = %p, owner_items = %p\n",
	    sman, &sman->owner_items);
	list_for_each_entry_safe(entry, next, &sman->owner_items, sman_list) {
		DRM_DEBUG("cleaning owner_item = %p\n", entry);
		drm_sman_do_owner_cleanup(sman, entry);
	}
	if (sman->mm) {
		for (i = 0; i < sman->num_managers; ++i) {
			sman_mm = &sman->mm[i];
			if (sman_mm->private) {
				sman_mm->destroy(sman_mm->private);
				sman_mm->private = NULL;
			}
		}
	}
}
