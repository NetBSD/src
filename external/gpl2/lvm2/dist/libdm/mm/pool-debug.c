/*	$NetBSD: pool-debug.c,v 1.1.1.2 2009/12/02 00:26:09 haad Exp $	*/

/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.  
 * Copyright (C) 2004-2005 Red Hat, Inc. All rights reserved.
 *
 * This file is part of the device-mapper userspace tools.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU Lesser General Public License v.2.1.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "dmlib.h"
#include <assert.h>

struct block {
	struct block *next;
	size_t size;
	void *data;
};

typedef struct {
	unsigned block_serialno;	/* Non-decreasing serialno of block */
	unsigned blocks_allocated;	/* Current number of blocks allocated */
	unsigned blocks_max;	/* Max no of concurrently-allocated blocks */
	unsigned int bytes, maxbytes;
} pool_stats;

struct dm_pool {
	struct dm_list list;
	const char *name;
	void *orig_pool;	/* to pair it with first allocation call */

	int begun;
	struct block *object;

	struct block *blocks;
	struct block *tail;

	pool_stats stats;
};

/* by default things come out aligned for doubles */
#define DEFAULT_ALIGNMENT __alignof__ (double)

struct dm_pool *dm_pool_create(const char *name, size_t chunk_hint)
{
	struct dm_pool *mem = dm_malloc(sizeof(*mem));

	if (!mem) {
		log_error("Couldn't create memory pool %s (size %"
			  PRIsize_t ")", name, sizeof(*mem));
		return NULL;
	}

	mem->name = name;
	mem->begun = 0;
	mem->object = 0;
	mem->blocks = mem->tail = NULL;

	mem->stats.block_serialno = 0;
	mem->stats.blocks_allocated = 0;
	mem->stats.blocks_max = 0;
	mem->stats.bytes = 0;
	mem->stats.maxbytes = 0;

	mem->orig_pool = mem;

#ifdef DEBUG_POOL
	log_debug("Created mempool %s", name);
#endif

	dm_list_add(&_dm_pools, &mem->list);
	return mem;
}

static void _free_blocks(struct dm_pool *p, struct block *b)
{
	struct block *n;

	while (b) {
		p->stats.bytes -= b->size;
		p->stats.blocks_allocated--;

		n = b->next;
		dm_free(b->data);
		dm_free(b);
		b = n;
	}
}

static void _pool_stats(struct dm_pool *p, const char *action)
{
#ifdef DEBUG_POOL
	log_debug("%s mempool %s: %u/%u bytes, %u/%u blocks, "
		  "%u allocations)", action, p->name, p->stats.bytes,
		  p->stats.maxbytes, p->stats.blocks_allocated,
		  p->stats.blocks_max, p->stats.block_serialno);
#else
	;
#endif
}

void dm_pool_destroy(struct dm_pool *p)
{
	_pool_stats(p, "Destroying");
	_free_blocks(p, p->blocks);
	dm_list_del(&p->list);
	dm_free(p);
}

void *dm_pool_alloc(struct dm_pool *p, size_t s)
{
	return dm_pool_alloc_aligned(p, s, DEFAULT_ALIGNMENT);
}

static void _append_block(struct dm_pool *p, struct block *b)
{
	if (p->tail) {
		p->tail->next = b;
		p->tail = b;
	} else
		p->blocks = p->tail = b;

	p->stats.block_serialno++;
	p->stats.blocks_allocated++;
	if (p->stats.blocks_allocated > p->stats.blocks_max)
		p->stats.blocks_max = p->stats.blocks_allocated;

	p->stats.bytes += b->size;
	if (p->stats.bytes > p->stats.maxbytes)
		p->stats.maxbytes = p->stats.bytes;
}

static struct block *_new_block(size_t s, unsigned alignment)
{
	/* FIXME: I'm currently ignoring the alignment arg. */
	size_t len = sizeof(struct block) + s;
	struct block *b = dm_malloc(len);

	/*
	 * Too lazy to implement alignment for debug version, and
	 * I don't think LVM will use anything but default
	 * align.
	 */
	assert(alignment == DEFAULT_ALIGNMENT);

	if (!b) {
		log_error("Out of memory");
		return NULL;
	}

	if (!(b->data = dm_malloc(s))) {
		log_error("Out of memory");
		dm_free(b);
		return NULL;
	}

	b->next = NULL;
	b->size = s;

	return b;
}

void *dm_pool_alloc_aligned(struct dm_pool *p, size_t s, unsigned alignment)
{
	struct block *b = _new_block(s, alignment);

	if (!b)
		return NULL;

	_append_block(p, b);

	return b->data;
}

void dm_pool_empty(struct dm_pool *p)
{
	_pool_stats(p, "Emptying");
	_free_blocks(p, p->blocks);
	p->blocks = p->tail = NULL;
}

void dm_pool_free(struct dm_pool *p, void *ptr)
{
	struct block *b, *prev = NULL;

	_pool_stats(p, "Freeing (before)");

	for (b = p->blocks; b; b = b->next) {
		if (b->data == ptr)
			break;
		prev = b;
	}

	/*
	 * If this fires then you tried to free a
	 * pointer that either wasn't from this
	 * pool, or isn't the start of a block.
	 */
	assert(b);

	_free_blocks(p, b);

	if (prev) {
		p->tail = prev;
		prev->next = NULL;
	} else
		p->blocks = p->tail = NULL;

	_pool_stats(p, "Freeing (after)");
}

int dm_pool_begin_object(struct dm_pool *p, size_t init_size)
{
	assert(!p->begun);
	p->begun = 1;
	return 1;
}

int dm_pool_grow_object(struct dm_pool *p, const void *extra, size_t delta)
{
	struct block *new;
	size_t new_size;

	if (!delta)
		delta = strlen(extra);

	assert(p->begun);

	if (p->object)
		new_size = delta + p->object->size;
	else
		new_size = delta;

	if (!(new = _new_block(new_size, DEFAULT_ALIGNMENT))) {
		log_error("Couldn't extend object.");
		return 0;
	}

	if (p->object) {
		memcpy(new->data, p->object->data, p->object->size);
		dm_free(p->object->data);
		dm_free(p->object);
	}
	p->object = new;

	memcpy(new->data + new_size - delta, extra, delta);

	return 1;
}

void *dm_pool_end_object(struct dm_pool *p)
{
	assert(p->begun);
	_append_block(p, p->object);

	p->begun = 0;
	p->object = NULL;
	return p->tail->data;
}

void dm_pool_abandon_object(struct dm_pool *p)
{
	assert(p->begun);
	dm_free(p->object);
	p->begun = 0;
	p->object = NULL;
}
