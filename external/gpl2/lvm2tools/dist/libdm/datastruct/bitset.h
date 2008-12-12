/*	$NetBSD: bitset.h,v 1.1.1.1 2008/12/12 11:42:53 haad Exp $	*/

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

#ifndef _DM_BITSET_H
#define _DM_BITSET_H

#include "pool.h"

#include <limits.h>

typedef uint32_t *bitset_t;

bitset_t bitset_create(struct pool *mem, unsigned num_bits);
void bitset_destroy(bitset_t bs);

void bit_union(bitset_t out, bitset_t in1, bitset_t in2);
int bit_get_first(bitset_t bs);
int bit_get_next(bitset_t bs, int last_bit);

#define BITS_PER_INT (sizeof(int) * CHAR_BIT)

#define bit(bs, i) \
   (bs[(i / BITS_PER_INT) + 1] & (0x1 << (i & (BITS_PER_INT - 1))))

#define bit_set(bs, i) \
   (bs[(i / BITS_PER_INT) + 1] |= (0x1 << (i & (BITS_PER_INT - 1))))

#define bit_clear(bs, i) \
   (bs[(i / BITS_PER_INT) + 1] &= ~(0x1 << (i & (BITS_PER_INT - 1))))

#define bit_set_all(bs) \
   memset(bs + 1, -1, ((*bs / BITS_PER_INT) + 1) * sizeof(int))

#define bit_clear_all(bs) \
   memset(bs + 1, 0, ((*bs / BITS_PER_INT) + 1) * sizeof(int))

#define bit_copy(bs1, bs2) \
   memcpy(bs1 + 1, bs2 + 1, ((*bs1 / BITS_PER_INT) + 1) * sizeof(int))

#endif
