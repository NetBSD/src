/*	$NetBSD: dma-fence-chain.h,v 1.3 2021/12/19 10:47:06 riastradh Exp $	*/

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
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

#ifndef	_LINUX_DMA_FENCE_CHAIN_H_
#define	_LINUX_DMA_FENCE_CHAIN_H_

#include <sys/types.h>

#include <linux/dma-fence.h>

struct dma_fence_chain {
	struct dma_fence	base;
	uint64_t		prev_seqno;

	spinlock_t		dfc_lock;
};

#define	dma_fence_chain_find_seqno	linux_dma_fence_chain_find_seqno
#define	dma_fence_chain_init		linux_dma_fence_chain_init
#define	dma_fence_chain_walk		linux_dma_fence_chain_walk
#define	to_dma_fence_chain		linux_to_dma_fence_chain

void	dma_fence_chain_init(struct dma_fence_chain *, struct dma_fence *,
	    struct dma_fence *, uint64_t);
int	dma_fence_chain_find_seqno(struct dma_fence **, uint64_t);
struct dma_fence_chain *
	to_dma_fence_chain(struct dma_fence *);
struct dma_fence *
	dma_fence_chain_walk(struct dma_fence *);

#define	dma_fence_chain_for_each(VAR, FENCE)				      \
	for ((VAR) = dma_fence_get(FENCE);				      \
		(VAR) != NULL;						      \
		(VAR) = dma_fence_chain_walk(VAR))

#endif	/* _LINUX_DMA_FENCE_CHAIN_H_ */
