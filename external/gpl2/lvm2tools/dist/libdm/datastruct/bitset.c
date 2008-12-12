/*	$NetBSD: bitset.c,v 1.1.1.1 2008/12/12 11:42:53 haad Exp $	*/

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

/* FIXME: calculate this. */
#define INT_SHIFT 5

dm_bitset_t dm_bitset_create(struct dm_pool *mem, unsigned num_bits)
{
	unsigned n = (num_bits / DM_BITS_PER_INT) + 2;
	size_t size = sizeof(int) * n;
	dm_bitset_t bs;
	
	if (mem)
		bs = dm_pool_zalloc(mem, size);
	else
		bs = dm_malloc(size);

	if (!bs)
		return NULL;

	*bs = num_bits;

	if (!mem)
		dm_bit_clear_all(bs);

	return bs;
}

void dm_bitset_destroy(dm_bitset_t bs)
{
	dm_free(bs);
}

void dm_bit_union(dm_bitset_t out, dm_bitset_t in1, dm_bitset_t in2)
{
	int i;
	for (i = (in1[0] / DM_BITS_PER_INT) + 1; i; i--)
		out[i] = in1[i] | in2[i];
}

/*
 * FIXME: slow
 */
static inline int _test_word(uint32_t test, int bit)
{
	while (bit < (int) DM_BITS_PER_INT) {
		if (test & (0x1 << bit))
			return bit;
		bit++;
	}

	return -1;
}

int dm_bit_get_next(dm_bitset_t bs, int last_bit)
{
	int bit, word;
	uint32_t test;

	last_bit++;		/* otherwise we'll return the same bit again */

	/*
	 * bs[0] holds number of bits
	 */
	while (last_bit < (int) bs[0]) {
		word = last_bit >> INT_SHIFT;
		test = bs[word + 1];
		bit = last_bit & (DM_BITS_PER_INT - 1);

		if ((bit = _test_word(test, bit)) >= 0)
			return (word * DM_BITS_PER_INT) + bit;

		last_bit = last_bit - (last_bit & (DM_BITS_PER_INT - 1)) +
		    DM_BITS_PER_INT;
	}

	return -1;
}

int dm_bit_get_first(dm_bitset_t bs)
{
	return dm_bit_get_next(bs, -1);
}
