/* $NetBSD: exfatfs_trie_basic.h,v 1.1.2.2 2024/07/01 22:15:21 perseant Exp $ */

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

#ifndef EXFATFS_TRIE_BASIC_H_
#define EXFATFS_TRIE_BASIC_H_

#define BN_CHILDREN_SHIFT 8
#define BN_CHILDREN_COUNT (1 << BN_CHILDREN_SHIFT)
#define BOTTOM_LEVEL      2 /* level at which 1 block * 8 is full count */
#define BN_BLOCK_SIZE     (BN_CHILDREN_SHIFT << BOTTOM_LEVEL)
#define INVALID           (~(uint32_t)0)

struct xf_bitmap_node {
	struct xf_bitmap_node *children[BN_CHILDREN_COUNT];
	uint32_t count;
};

int exfatfs_bitmap_init(struct exfatfs *, int);
void exfatfs_bitmap_destroy(struct exfatfs *);

int exfatfs_bitmap_alloc(struct exfatfs *, uint32_t, uint32_t *);
int exfatfs_bitmap_dealloc(struct exfatfs *, uint32_t);

#endif /* EXFATFS_TRIE_BASIC_H_ */
