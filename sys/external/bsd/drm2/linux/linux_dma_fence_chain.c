/*	$NetBSD: linux_dma_fence_chain.c,v 1.1 2021/12/19 10:47:06 riastradh Exp $	*/

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: linux_dma_fence_chain.c,v 1.1 2021/12/19 10:47:06 riastradh Exp $");

#include <sys/types.h>

#include <linux/dma-fence.h>
#include <linux/dma-fence-chain.h>
#include <linux/spinlock.h>

static const struct dma_fence_ops dma_fence_chain_ops;

/*
 * dma_fence_chain_init(chain, prev, fence, seqno)
 *
 *	Initialize a new fence chain, and either start 
 */
void
dma_fence_chain_init(struct dma_fence_chain *chain, struct dma_fence *prev,
    struct dma_fence *fence, uint64_t seqno)
{
	uint64_t context = -1;	/* XXX */

	chain->prev_seqno = 0;
	spin_lock_init(&chain->dfc_lock);

	/* XXX we don't use these yet */
	dma_fence_put(prev);
	dma_fence_put(fence);

	dma_fence_init(&chain->base, &dma_fence_chain_ops, &chain->dfc_lock,
	    context, seqno);
}

static void
dma_fence_chain_release(struct dma_fence *fence)
{
	struct dma_fence_chain *chain = to_dma_fence_chain(fence);

	KASSERT(chain);

	spin_lock_destroy(&chain->dfc_lock);
	dma_fence_free(&chain->base);
}

static const struct dma_fence_ops dma_fence_chain_ops = {
	.release = dma_fence_chain_release,
};

/*
 * to_dma_fence_chain(fence)
 *
 *	If fence is nonnull and in a chain, return the chain.
 *	Otherwise return NULL.
 */
struct dma_fence_chain *
to_dma_fence_chain(struct dma_fence *fence)
{

	if (fence == NULL || fence->ops != &dma_fence_chain_ops)
		return NULL;
	return container_of(fence, struct dma_fence_chain, base);
}

/*
 * dma_fence_chain_walk(fence)
 *
 *	Return the next fence in the chain, or NULL if end of chain,
 *	after releasing any fences that have already been signalled.
 */
struct dma_fence *
dma_fence_chain_walk(struct dma_fence *fence)
{

	/* XXX */
	return NULL;
}

/*
 * dma_fence_chain_find_seqno(&fence, seqno)
 *
 *	If seqno is zero, do nothing and succeed.
 *
 *	Otherwise, if fence is not on a chain or if its sequence
 *	number has not yet reached seqno, fail with EINVAL.
 *
 *	Otherwise, set fence to the first fence in the chain which
 *	will signal this sequence number.
 */
int
dma_fence_chain_find_seqno(struct dma_fence **fencep, uint64_t seqno)
{
	struct dma_fence_chain *chain;

	if (seqno == 0)
		return 0;

	chain = to_dma_fence_chain(*fencep);
	if (chain == NULL || chain->base.seqno < seqno)
		return -EINVAL;

	dma_fence_chain_for_each(*fencep, &chain->base) {
		if ((*fencep)->context != chain->base.context ||
		    to_dma_fence_chain(*fencep)->prev_seqno < seqno)
			break;
	}

	/* Release reference acquired by dma_fence_chain_for_each.  */
	dma_fence_put(&chain->base);

	return 0;
}
