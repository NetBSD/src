// SPDX-FileCopyrightText: 2011 Lai Jiangshan <laijs@cn.fujitsu.com>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

/*
 * Chunk based memory management for Lock-Free RCU Hash Table
 */

#include <stddef.h>
#include <urcu/assert.h>
#include "rculfhash-internal.h"

static
void cds_lfht_alloc_bucket_table(struct cds_lfht *ht, unsigned long order)
{
	if (order == 0) {
		ht->tbl_chunk[0] = ht->alloc->calloc(ht->alloc->state,
			ht->min_nr_alloc_buckets, sizeof(struct cds_lfht_node));
		urcu_posix_assert(ht->tbl_chunk[0]);
	} else if (order > ht->min_alloc_buckets_order) {
		unsigned long i, len = 1UL << (order - 1 - ht->min_alloc_buckets_order);

		for (i = len; i < 2 * len; i++) {
			ht->tbl_chunk[i] = ht->alloc->calloc(ht->alloc->state,
				ht->min_nr_alloc_buckets, sizeof(struct cds_lfht_node));
			urcu_posix_assert(ht->tbl_chunk[i]);
		}
	}
	/* Nothing to do for 0 < order && order <= ht->min_alloc_buckets_order */
}

/*
 * cds_lfht_free_bucket_table() should be called with decreasing order.
 * When cds_lfht_free_bucket_table(0) is called, it means the whole
 * lfht is destroyed.
 */
static
void cds_lfht_free_bucket_table(struct cds_lfht *ht, unsigned long order)
{
	if (order == 0)
		poison_free(ht->alloc, ht->tbl_chunk[0]);
	else if (order > ht->min_alloc_buckets_order) {
		unsigned long i, len = 1UL << (order - 1 - ht->min_alloc_buckets_order);

		for (i = len; i < 2 * len; i++)
			poison_free(ht->alloc, ht->tbl_chunk[i]);
	}
	/* Nothing to do for 0 < order && order <= ht->min_alloc_buckets_order */
}

static
struct cds_lfht_node *bucket_at(struct cds_lfht *ht, unsigned long index)
{
	unsigned long chunk, offset;

	chunk = index >> ht->min_alloc_buckets_order;
	offset = index & (ht->min_nr_alloc_buckets - 1);
	return &ht->tbl_chunk[chunk][offset];
}

static
struct cds_lfht *alloc_cds_lfht(unsigned long min_nr_alloc_buckets,
		unsigned long max_nr_buckets, const struct cds_lfht_alloc *alloc)
{
	unsigned long nr_chunks, cds_lfht_size;

	min_nr_alloc_buckets = max(min_nr_alloc_buckets,
				max_nr_buckets / MAX_CHUNK_TABLE);
	nr_chunks = max_nr_buckets / min_nr_alloc_buckets;
	cds_lfht_size = offsetof(struct cds_lfht, tbl_chunk) +
			sizeof(struct cds_lfht_node *) * nr_chunks;
	cds_lfht_size = max(cds_lfht_size, sizeof(struct cds_lfht));

	return __default_alloc_cds_lfht(
			&cds_lfht_mm_chunk, alloc, cds_lfht_size,
			min_nr_alloc_buckets, max_nr_buckets);
}

const struct cds_lfht_mm_type cds_lfht_mm_chunk = {
	.alloc_cds_lfht = alloc_cds_lfht,
	.alloc_bucket_table = cds_lfht_alloc_bucket_table,
	.free_bucket_table = cds_lfht_free_bucket_table,
	.bucket_at = bucket_at,
};
