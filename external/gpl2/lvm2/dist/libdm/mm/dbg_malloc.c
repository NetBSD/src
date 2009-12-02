/*	$NetBSD: dbg_malloc.c,v 1.1.1.2 2009/12/02 00:26:09 haad Exp $	*/

/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.  
 * Copyright (C) 2004-2007 Red Hat, Inc. All rights reserved.
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
#include <stdarg.h>

char *dm_strdup_aux(const char *str, const char *file, int line)
{
	char *ret;

	if (!str) {
		log_error("Internal error: dm_strdup called with NULL pointer");
		return NULL;
	}

	if ((ret = dm_malloc_aux_debug(strlen(str) + 1, file, line)))
		strcpy(ret, str);

	return ret;
}

struct memblock {
	struct memblock *prev, *next;	/* All allocated blocks are linked */
	size_t length;		/* Size of the requested block */
	int id;			/* Index of the block */
	const char *file;	/* File that allocated */
	int line;		/* Line that allocated */
	void *magic;		/* Address of this block */
} __attribute__((aligned(8)));

static struct {
	unsigned block_serialno;/* Non-decreasing serialno of block */
	unsigned blocks_allocated; /* Current number of blocks allocated */
	unsigned blocks_max;	/* Max no of concurrently-allocated blocks */
	unsigned int bytes, mbytes;

} _mem_stats = {
0, 0, 0, 0, 0};

static struct memblock *_head = 0;
static struct memblock *_tail = 0;

void *dm_malloc_aux_debug(size_t s, const char *file, int line)
{
	struct memblock *nb;
	size_t tsize = s + sizeof(*nb) + sizeof(unsigned long);

	if (s > 50000000) {
		log_error("Huge memory allocation (size %" PRIsize_t
			  ") rejected - metadata corruption?", s);
		return 0;
	}

	if (!(nb = malloc(tsize))) {
		log_error("couldn't allocate any memory, size = %" PRIsize_t,
			  s);
		return 0;
	}

	/* set up the file and line info */
	nb->file = file;
	nb->line = line;

	dm_bounds_check();

	/* setup fields */
	nb->magic = nb + 1;
	nb->length = s;
	nb->id = ++_mem_stats.block_serialno;
	nb->next = 0;

	/* stomp a pretty pattern across the new memory
	   and fill in the boundary bytes */
	{
		char *ptr = (char *) (nb + 1);
		size_t i;
		for (i = 0; i < s; i++)
			*ptr++ = i & 0x1 ? (char) 0xba : (char) 0xbe;

		for (i = 0; i < sizeof(unsigned long); i++)
			*ptr++ = (char) nb->id;
	}

	nb->prev = _tail;

	/* link to tail of the list */
	if (!_head)
		_head = _tail = nb;
	else {
		_tail->next = nb;
		_tail = nb;
	}

	_mem_stats.blocks_allocated++;
	if (_mem_stats.blocks_allocated > _mem_stats.blocks_max)
		_mem_stats.blocks_max = _mem_stats.blocks_allocated;

	_mem_stats.bytes += s;
	if (_mem_stats.bytes > _mem_stats.mbytes)
		_mem_stats.mbytes = _mem_stats.bytes;

	/* log_debug("Allocated: %u %u %u", nb->id, _mem_stats.blocks_allocated,
		  _mem_stats.bytes); */

	return nb + 1;
}

void dm_free_aux(void *p)
{
	char *ptr;
	size_t i;
	struct memblock *mb = ((struct memblock *) p) - 1;
	if (!p)
		return;

	dm_bounds_check();

	/* sanity check */
	assert(mb->magic == p);

	/* check data at the far boundary */
	ptr = ((char *) mb) + sizeof(struct memblock) + mb->length;
	for (i = 0; i < sizeof(unsigned long); i++)
		if (*ptr++ != (char) mb->id)
			assert(!"Damage at far end of block");

	/* have we freed this before ? */
	assert(mb->id != 0);

	/* unlink */
	if (mb->prev)
		mb->prev->next = mb->next;
	else
		_head = mb->next;

	if (mb->next)
		mb->next->prev = mb->prev;
	else
		_tail = mb->prev;

	mb->id = 0;

	/* stomp a different pattern across the memory */
	ptr = ((char *) mb) + sizeof(struct memblock);
	for (i = 0; i < mb->length; i++)
		*ptr++ = i & 1 ? (char) 0xde : (char) 0xad;

	assert(_mem_stats.blocks_allocated);
	_mem_stats.blocks_allocated--;
	_mem_stats.bytes -= mb->length;

	/* free the memory */
	free(mb);
}

void *dm_realloc_aux(void *p, unsigned int s, const char *file, int line)
{
	void *r;
	struct memblock *mb = ((struct memblock *) p) - 1;

	r = dm_malloc_aux_debug(s, file, line);

	if (p) {
		memcpy(r, p, mb->length);
		dm_free_aux(p);
	}

	return r;
}

int dm_dump_memory_debug(void)
{
	unsigned long tot = 0;
	struct memblock *mb;
	char str[32];
	size_t c;

	if (_head)
		log_very_verbose("You have a memory leak:");

	for (mb = _head; mb; mb = mb->next) {
		for (c = 0; c < sizeof(str) - 1; c++) {
			if (c >= mb->length)
				str[c] = ' ';
			else if (*(char *)(mb->magic + c) == '\0')
				str[c] = '\0';
			else if (*(char *)(mb->magic + c) < ' ')
				str[c] = '?';
			else
				str[c] = *(char *)(mb->magic + c);
		}
		str[sizeof(str) - 1] = '\0';

		LOG_MESG(_LOG_INFO, mb->file, mb->line, 0,
			 "block %d at %p, size %" PRIsize_t "\t [%s]",
			 mb->id, mb->magic, mb->length, str);
		tot += mb->length;
	}

	if (_head)
		log_very_verbose("%ld bytes leaked in total", tot);

	return 1;
}

void dm_bounds_check_debug(void)
{
	struct memblock *mb = _head;
	while (mb) {
		size_t i;
		char *ptr = ((char *) (mb + 1)) + mb->length;
		for (i = 0; i < sizeof(unsigned long); i++)
			if (*ptr++ != (char) mb->id)
				assert(!"Memory smash");

		mb = mb->next;
	}
}

void *dm_malloc_aux(size_t s, const char *file __attribute((unused)),
		    int line __attribute((unused)))
{
	if (s > 50000000) {
		log_error("Huge memory allocation (size %" PRIsize_t
			  ") rejected - metadata corruption?", s);
		return 0;
	}

	return malloc(s);
}
