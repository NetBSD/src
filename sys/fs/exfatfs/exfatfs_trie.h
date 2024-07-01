/* $NetBSD: exfatfs_trie.h,v 1.1.2.2 2024/07/01 22:15:21 perseant Exp $ */

/*-
 * Copyright (c) 2022, 2024 The NetBSD Foundation, Inc.
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

#ifndef EXFATFS_TRIE_H_
#define EXFATFS_TRIE_H_

#define TBN_CHILDREN_SHIFT 8
#define TBN_CHILDREN_COUNT (1U << TBN_CHILDREN_SHIFT)
#define BOTTOM_LEVEL      2 /* level at which 1 block * 8 is full count */
#define TBN_BLOCK_SIZE     (TBN_CHILDREN_SHIFT << BOTTOM_LEVEL)
#define INVALID           (~(uint32_t)0)

#define TBN_SIZE(level) (1U << (level * TBN_CHILDREN_SHIFT))
#define TBN_EMPTY(bp, level) ((bp)->count == 0)
#define TBN_FULL(bp, level) ((bp)->count == TBN_SIZE(level))

/* Convert cluster number to disk address and offset */
#define NBBYSHIFT 3 /* 1 << NBBYSHIFT == NBBY == 8 */
#define BITMAPSHIFT(fs)  ((fs)->xf_BytesPerSectorShift + NBBYSHIFT)
#define BITMAPLBN(fs, cn) (((cn) - 2) >> BITMAPSHIFT(fs))
#define BITMAPOFF(fs, cn) (((cn) - 2) & ((1 << BITMAPSHIFT(fs)) - 1))
#define LBNOFF2CLUSTER(fs, lbn, off) ((lbn << BITMAPSHIFT(fs)) + (off) + 2)

/*
 * The data structure for the bitmap radix tree.
 *
 * children[i] can be empty under the following conditions:
 * 1) There is no allocation in children[i], i.e., if it existed,
 *    children[i]->count == 0.
 * 2) We are full.  If we ever stop being full, we must reallocate
 *    all the children and set them full before proceeding.
 * 3) The child represents only invalid cluster numbers.
 */
struct xf_bitmap_node {
	struct xf_bitmap_node *children[TBN_CHILDREN_COUNT];
	uint32_t count;
};

int exfatfs_bitmap_init(struct exfatfs *fs, int);
void exfatfs_bitmap_destroy(struct exfatfs *fs);
int exfatfs_bitmap_alloc(struct exfatfs *fs, uint32_t start, uint32_t *cp);
int exfatfs_bitmap_alloc0(struct exfatfs *fs, uint32_t start, uint32_t *cp);
int exfatfs_bitmap_dealloc(struct exfatfs *fs, uint32_t cno);

#endif /* EXFATFS_TRIE_H_ */
