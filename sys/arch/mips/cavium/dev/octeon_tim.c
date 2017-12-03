/*	$NetBSD: octeon_tim.c,v 1.1.18.2 2017/12/03 11:36:27 jdolecek Exp $	*/

/*
 * Copyright (c) 2007 Internet Initiative Japan, Inc.
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
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: octeon_tim.c,v 1.1.18.2 2017/12/03 11:36:27 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/types.h>

#include <sys/bus.h>

#include <mips/cavium/include/iobusvar.h>
#include <mips/cavium/dev/octeon_timreg.h>

/* ---- definitions */

struct _ring {
	struct _bucket	*r_buckets;
	size_t		r_nbuckets;

	/* per-ring parameters */
	size_t		r_chunksize;
};

struct _bucket {
	struct _ring	*b_ring;	/* back pointer */

	int		b_id;
	struct _bucket	*b_first;
	uint32_t	b_nentries;
	uint32_t	b_remainder;
	struct _bucket	*b_current;
	/* ... */
};

struct _chunk {
	struct _bucket	*c_bucket;	/* back pointer */

	struct _entry	c_entries[];
	/* ... */
};

struct _entry {
	bus_addr_t	e_wqe;
};

struct _context {
	struct _ring	*x_ring;
	struct _bucket	*x_bucket;
};

/* ---- global functions */

void
octeon_tim_init(void)
{
	/* .... */
}

/* ---- local functions */

/* ring (an array of buckets) */

int
_ring_add_wqe(struct _ring *ring, int bid, bus_addr_t wqe, ...)
{
	int result = 0;
	struct _bucket *bucket;

	bucket = &ring->r_buckets[bid];
	result = _bucket_add_wqe(ring, bucket, ...);
	return result;
}

/* bucket (an entry with a linked list of chunks) */

int
_bucket_add_wqe(struct _ring *ring, struct _bucket *bucket, bus_addr_t wqe/* pointer */)
{
	int result = 0;
	struct _entry *entry;

	if (bucket->b_remainder == 0) {
		status = _bucket_alloc_chunk(ring, bucket);
		if (status != 0) {
			result = status;
			goto out;
		}
	}
	entry = _bucket_get_current_entry(ring, bucket);
	entry->e_wqe = wqe;

out:
	return result;
}

int
_bucket_alloc_chunk(struct _ring *ring, struct _bucket *bucket)
{
	int result = 0;
	struct _chunk *chunk;

	status = _chunk_alloc(ring, &chunk);
	if (status != 0) {
		result = status;
		goto out;
	}
	_bucket_append_chunk(...);

out:
	return result;
}

void
_bucket_append_chunk(...)
{
	if (bucket->b_current == NULL)
		bucket->b_first = chunk;
	else
		bucket->b_current->b_next;
	bucket->b_current = chunk;
	bucket->b_remainder += ring->r_chunksize;
}

struct _entry *
_bucket_get_current_entry(...)
{
	return &bucket->b_current->c_entries
	    [ring->r_chunksize - bucket->b_remainder];
}

/* chunk (an array of pointers to WQE) */

int
_chunk_alloc(struct _ring *ring, struct _chunk **rchunk)
{
	int result = 0;
	struct _chunk *chunk;

	chunk = fpa_alloc(
		/* array of entires (pointer to WQE) */
		sizeof(struct _entry) * ring->r_chunksize +
		/* pointer to next chunk */
		sizeof(uint64_t));
	if (chunk == NULL) {
		result = ENOMEM;
		goto out;
	}

out:
	return result;
}
