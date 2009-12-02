/*	$NetBSD: pool-fast.c,v 1.1.1.2 2009/12/02 00:26:09 haad Exp $	*/

/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.   
 * Copyright (C) 2004-2006 Red Hat, Inc. All rights reserved.
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

struct chunk {
	char *begin, *end;
	struct chunk *prev;
};

struct dm_pool {
	struct dm_list list;
	struct chunk *chunk, *spare_chunk;	/* spare_chunk is a one entry free
						   list to stop 'bobbling' */
	size_t chunk_size;
	size_t object_len;
	unsigned object_alignment;
};

void _align_chunk(struct chunk *c, unsigned alignment);
struct chunk *_new_chunk(struct dm_pool *p, size_t s);

/* by default things come out aligned for doubles */
#define DEFAULT_ALIGNMENT __alignof__ (double)

struct dm_pool *dm_pool_create(const char *name, size_t chunk_hint)
{
	size_t new_size = 1024;
	struct dm_pool *p = dm_malloc(sizeof(*p));

	if (!p) {
		log_error("Couldn't create memory pool %s (size %"
			  PRIsize_t ")", name, sizeof(*p));
		return 0;
	}
	memset(p, 0, sizeof(*p));

	/* round chunk_hint up to the next power of 2 */
	p->chunk_size = chunk_hint + sizeof(struct chunk);
	while (new_size < p->chunk_size)
		new_size <<= 1;
	p->chunk_size = new_size;
	dm_list_add(&_dm_pools, &p->list);
	return p;
}

void dm_pool_destroy(struct dm_pool *p)
{
	struct chunk *c, *pr;
	dm_free(p->spare_chunk);
	c = p->chunk;
	while (c) {
		pr = c->prev;
		dm_free(c);
		c = pr;
	}

	dm_list_del(&p->list);
	dm_free(p);
}

void *dm_pool_alloc(struct dm_pool *p, size_t s)
{
	return dm_pool_alloc_aligned(p, s, DEFAULT_ALIGNMENT);
}

void *dm_pool_alloc_aligned(struct dm_pool *p, size_t s, unsigned alignment)
{
	struct chunk *c = p->chunk;
	void *r;

	/* realign begin */
	if (c)
		_align_chunk(c, alignment);

	/* have we got room ? */
	if (!c || (c->begin > c->end) || (c->end - c->begin < s)) {
		/* allocate new chunk */
		size_t needed = s + alignment + sizeof(struct chunk);
		c = _new_chunk(p, (needed > p->chunk_size) ?
			       needed : p->chunk_size);

		if (!c)
			return NULL;

		_align_chunk(c, alignment);
	}

	r = c->begin;
	c->begin += s;
	return r;
}

void dm_pool_empty(struct dm_pool *p)
{
	struct chunk *c;

	for (c = p->chunk; c && c->prev; c = c->prev)
		;

	if (c)
		dm_pool_free(p, (char *) (c + 1));
}

void dm_pool_free(struct dm_pool *p, void *ptr)
{
	struct chunk *c = p->chunk;

	while (c) {
		if (((char *) c < (char *) ptr) &&
		    ((char *) c->end > (char *) ptr)) {
			c->begin = ptr;
			break;
		}

		if (p->spare_chunk)
			dm_free(p->spare_chunk);
		p->spare_chunk = c;
		c = c->prev;
	}

	if (!c)
		log_error("Internal error: pool_free asked to free pointer "
			  "not in pool");
	else
		p->chunk = c;
}

int dm_pool_begin_object(struct dm_pool *p, size_t hint)
{
	struct chunk *c = p->chunk;
	const size_t align = DEFAULT_ALIGNMENT;

	p->object_len = 0;
	p->object_alignment = align;

	if (c)
		_align_chunk(c, align);

	if (!c || (c->begin > c->end) || (c->end - c->begin < hint)) {
		/* allocate a new chunk */
		c = _new_chunk(p,
			       hint > (p->chunk_size - sizeof(struct chunk)) ?
			       hint + sizeof(struct chunk) + align :
			       p->chunk_size);

		if (!c)
			return 0;

		_align_chunk(c, align);
	}

	return 1;
}

int dm_pool_grow_object(struct dm_pool *p, const void *extra, size_t delta)
{
	struct chunk *c = p->chunk, *nc;

	if (!delta)
		delta = strlen(extra);

	if (c->end - (c->begin + p->object_len) < delta) {
		/* move into a new chunk */
		if (p->object_len + delta > (p->chunk_size / 2))
			nc = _new_chunk(p, (p->object_len + delta) * 2);
		else
			nc = _new_chunk(p, p->chunk_size);

		if (!nc)
			return 0;

		_align_chunk(p->chunk, p->object_alignment);
		memcpy(p->chunk->begin, c->begin, p->object_len);
		c = p->chunk;
	}

	memcpy(c->begin + p->object_len, extra, delta);
	p->object_len += delta;
	return 1;
}

void *dm_pool_end_object(struct dm_pool *p)
{
	struct chunk *c = p->chunk;
	void *r = c->begin;
	c->begin += p->object_len;
	p->object_len = 0u;
	p->object_alignment = DEFAULT_ALIGNMENT;
	return r;
}

void dm_pool_abandon_object(struct dm_pool *p)
{
	p->object_len = 0;
	p->object_alignment = DEFAULT_ALIGNMENT;
}

void _align_chunk(struct chunk *c, unsigned alignment)
{
	c->begin += alignment - ((unsigned long) c->begin & (alignment - 1));
}

struct chunk *_new_chunk(struct dm_pool *p, size_t s)
{
	struct chunk *c;

	if (p->spare_chunk &&
	    ((p->spare_chunk->end - (char *) p->spare_chunk) >= s)) {
		/* reuse old chunk */
		c = p->spare_chunk;
		p->spare_chunk = 0;
	} else {
		if (!(c = dm_malloc(s))) {
			log_error("Out of memory.  Requested %" PRIsize_t
				  " bytes.", s);
			return NULL;
		}

		c->end = (char *) c + s;
	}

	c->prev = p->chunk;
	c->begin = (char *) (c + 1);
	p->chunk = c;

	return c;
}
