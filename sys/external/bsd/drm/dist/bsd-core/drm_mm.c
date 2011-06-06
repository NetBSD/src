/**************************************************************************
 *
 * Copyright 2006 Tungsten Graphics, Inc., Bismarck, ND., USA.
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
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *
 **************************************************************************/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: drm_mm.c,v 1.1.8.2 2011/06/06 09:09:10 jruoho Exp $");

/*
 * Generic simple memory manager implementation. Intended to be used as a base
 * class implementation for more advanced memory managers.
 *
 * Note that the algorithm used is quite simple and there might be substantial
 * performance gains if a smarter free list is implemented. Currently it is just an
 * unordered stack of free regions. This could easily be improved if an RB-tree
 * is used instead. At least if we expect heavy fragmentation.
 *
 * Aligned allocations can also see improvement.
 *
 * Authors:
 * Thomas Hellström <thomas-at-tungstengraphics-dot-com>
 */

#include "drmP.h"
#include "drm_mm.h"

#define MM_UNUSED_TARGET 4

unsigned long drm_mm_tail_space(struct drm_mm *mm)
{
	struct list_head *tail_node;
	struct drm_mm_node *entry;

	tail_node = mm->ml_entry.prev;
	entry = list_entry(tail_node, struct drm_mm_node, ml_entry);
	if (!entry->free)
		return 0;

	return entry->size;
}

int drm_mm_remove_space_from_tail(struct drm_mm *mm, unsigned long size)
{
	struct list_head *tail_node;
	struct drm_mm_node *entry;

	tail_node = mm->ml_entry.prev;
	entry = list_entry(tail_node, struct drm_mm_node, ml_entry);
	if (!entry->free)
		return -ENOMEM;

	if (entry->size <= size)
		return -ENOMEM;

	entry->size -= size;
	return 0;
}

static struct drm_mm_node *drm_mm_kmalloc(struct drm_mm *mm, int atomic)
{
	struct drm_mm_node *child;

	if (atomic)
		child = malloc(sizeof(*child), DRM_MEM_MM, M_NOWAIT);
	else
		child = malloc(sizeof(*child), DRM_MEM_MM, M_WAITOK);

	if (__predict_false(child == NULL)) {
		mutex_enter(&mm->unused_lock);
		if (list_empty(&mm->unused_nodes))
			child = NULL;
		else {
			child =
			    list_entry(mm->unused_nodes.next,
				       struct drm_mm_node, fl_entry);
			list_del(&child->fl_entry);
			--mm->num_unused;
		}
		mutex_exit(&mm->unused_lock);
	}
	return child;
}

int drm_mm_pre_get(struct drm_mm *mm)
{
	struct drm_mm_node *node;

	mutex_enter(&mm->unused_lock);
	while (mm->num_unused < MM_UNUSED_TARGET) {
		mutex_exit(&mm->unused_lock);
		node = malloc(sizeof(*node), DRM_MEM_MM, M_WAITOK);
		mutex_enter(&mm->unused_lock);

		if (__predict_false(node == NULL)) {
			int ret = (mm->num_unused < 2) ? -ENOMEM : 0;
			mutex_exit(&mm->unused_lock);
			return ret;
		}
		++mm->num_unused;
		list_add_tail(&node->fl_entry, &mm->unused_nodes);
	}
	mutex_exit(&mm->unused_lock);
	return 0;
}

static int drm_mm_create_tail_node(struct drm_mm *mm,
				   unsigned long start,
				   unsigned long size, int atomic)
{
	struct drm_mm_node *child;

	child = drm_mm_kmalloc(mm, atomic);
	if (__predict_false(child == NULL))
		return -ENOMEM;

	child->free = 1;
	child->size = size;
	child->start = start;
	child->mm = mm;

	list_add_tail(&child->ml_entry, &mm->ml_entry);
	list_add_tail(&child->fl_entry, &mm->fl_entry);

	return 0;
}

int drm_mm_add_space_to_tail(struct drm_mm *mm, unsigned long size, int atomic)
{
	struct list_head *tail_node;
	struct drm_mm_node *entry;

	tail_node = mm->ml_entry.prev;
	entry = list_entry(tail_node, struct drm_mm_node, ml_entry);
	if (!entry->free) {
		return drm_mm_create_tail_node(mm, entry->start + entry->size,
					       size, atomic);
	}
	entry->size += size;
	return 0;
}

static struct drm_mm_node *drm_mm_split_at_start(struct drm_mm_node *parent,
						 unsigned long size,
						 int atomic)
{
	struct drm_mm_node *child;

	child = drm_mm_kmalloc(parent->mm, atomic);
	if (__predict_false(child == NULL))
		return NULL;

	INIT_LIST_HEAD(&child->fl_entry);

	child->free = 0;
	child->size = size;
	child->start = parent->start;
	child->mm = parent->mm;

	list_add_tail(&child->ml_entry, &parent->ml_entry);
	INIT_LIST_HEAD(&child->fl_entry);

	parent->size -= size;
	parent->start += size;
	return child;
}


struct drm_mm_node *drm_mm_get_block_generic(struct drm_mm_node *node,
					     unsigned long size,
					     unsigned alignment,
					     int atomic)
{

	struct drm_mm_node *align_splitoff = NULL;
	unsigned tmp = 0;

	if (alignment)
		tmp = node->start % alignment;

	if (tmp) {
		align_splitoff =
		    drm_mm_split_at_start(node, alignment - tmp, atomic);
		if (__predict_false(align_splitoff == NULL))
			return NULL;
	}

	if (node->size == size) {
		list_del_init(&node->fl_entry);
		node->free = 0;
	} else {
		node = drm_mm_split_at_start(node, size, atomic);
	}

	if (align_splitoff)
		drm_mm_put_block(align_splitoff);

	return node;
}

/*
 * Put a block. Merge with the previous and / or next block if they are free.
 * Otherwise add to the free stack.
 */

void drm_mm_put_block(struct drm_mm_node *cur)
{

	struct drm_mm *mm = cur->mm;
	struct list_head *cur_head = &cur->ml_entry;
	struct list_head *root_head = &mm->ml_entry;
	struct drm_mm_node *prev_node = NULL;
	struct drm_mm_node *next_node;

	int merged = 0;

	if (cur_head->prev != root_head) {
		prev_node =
		    list_entry(cur_head->prev, struct drm_mm_node, ml_entry);
		if (prev_node->free) {
			prev_node->size += cur->size;
			merged = 1;
		}
	}
	if (cur_head->next != root_head) {
		next_node =
		    list_entry(cur_head->next, struct drm_mm_node, ml_entry);
		if (next_node->free) {
			if (merged) {
				prev_node->size += next_node->size;
				list_del(&next_node->ml_entry);
				list_del(&next_node->fl_entry);
				if (mm->num_unused < MM_UNUSED_TARGET) {
					list_add(&next_node->fl_entry,
						 &mm->unused_nodes);
					++mm->num_unused;
				} else
					free(next_node, DRM_MEM_MM);
			} else {
				next_node->size += cur->size;
				next_node->start = cur->start;
				merged = 1;
			}
		}
	}
	if (!merged) {
		cur->free = 1;
		list_add(&cur->fl_entry, &mm->fl_entry);
	} else {
		list_del(&cur->ml_entry);
		if (mm->num_unused < MM_UNUSED_TARGET) {
			list_add(&cur->fl_entry, &mm->unused_nodes);
			++mm->num_unused;
		} else
			free(cur, DRM_MEM_MM);
	}
}

struct drm_mm_node *drm_mm_search_free(const struct drm_mm *mm,
				       unsigned long size,
				       unsigned alignment, int best_match)
{
	struct list_head *list;
	const struct list_head *free_stack = &mm->fl_entry;
	struct drm_mm_node *entry;
	struct drm_mm_node *best;
	unsigned long best_size;
	unsigned wasted;

	best = NULL;
	best_size = ~0UL;

	list_for_each(list, free_stack) {
		entry = list_entry(list, struct drm_mm_node, fl_entry);
		wasted = 0;

		if (entry->size < size)
			continue;

		if (alignment) {
			register unsigned tmp = entry->start % alignment;
			if (tmp)
				wasted += alignment - tmp;
		}

		if (entry->size >= size + wasted) {
			if (!best_match)
				return entry;
			if (size < best_size) {
				best = entry;
				best_size = entry->size;
			}
		}
	}

	return best;
}

int drm_mm_clean(struct drm_mm * mm)
{
	struct list_head *head = &mm->ml_entry;

	return (head->next->next == head);
}

int drm_mm_init(struct drm_mm * mm, unsigned long start, unsigned long size)
{
	INIT_LIST_HEAD(&mm->ml_entry);
	INIT_LIST_HEAD(&mm->fl_entry);
	INIT_LIST_HEAD(&mm->unused_nodes);
	mm->num_unused = 0;
	mutex_init(&mm->unused_lock, MUTEX_DEFAULT, IPL_NONE);

	/* XXX This could be non-atomic but gets called from a locked path */
	return drm_mm_create_tail_node(mm, start, size, 1);
}

void drm_mm_takedown(struct drm_mm * mm)
{
	struct list_head *bnode = mm->fl_entry.next;
	struct drm_mm_node *entry;
	struct drm_mm_node *next;

	entry = list_entry(bnode, struct drm_mm_node, fl_entry);

	if (entry->ml_entry.next != &mm->ml_entry ||
	    entry->fl_entry.next != &mm->fl_entry) {
		DRM_ERROR("Memory manager not clean. Delaying takedown\n");
		return;
	}

	list_del(&entry->fl_entry);
	list_del(&entry->ml_entry);
	free(entry, DRM_MEM_MM);

	mutex_enter(&mm->unused_lock);
	list_for_each_entry_safe(entry, next, &mm->unused_nodes, fl_entry) {
		list_del(&entry->fl_entry);
		free(entry, DRM_MEM_MM);
		--mm->num_unused;
	}
	mutex_exit(&mm->unused_lock);

	mutex_destroy(&mm->unused_lock);

	KASSERT(mm->num_unused == 0);
}
