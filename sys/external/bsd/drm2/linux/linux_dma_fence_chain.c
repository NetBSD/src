/*	$NetBSD: linux_dma_fence_chain.c,v 1.4 2022/04/09 23:44:44 riastradh Exp $	*/

/*-
 * Copyright (c) 2021 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: linux_dma_fence_chain.c,v 1.4 2022/04/09 23:44:44 riastradh Exp $");

#include <sys/types.h>

#include <linux/dma-fence.h>
#include <linux/dma-fence-chain.h>
#include <linux/spinlock.h>

static void dma_fence_chain_irq_work(struct irq_work *);
static bool dma_fence_chain_enable_signaling(struct dma_fence *);

static const struct dma_fence_ops dma_fence_chain_ops;

/*
 * dma_fence_chain_init(chain, prev, fence, seqno)
 *
 *	Initialize a fence chain node.  If prev was already a chain,
 *	extend it; otherwise; create a new chain context.
 */
void
dma_fence_chain_init(struct dma_fence_chain *chain, struct dma_fence *prev,
    struct dma_fence *fence, uint64_t seqno)
{
	struct dma_fence_chain *prev_chain = to_dma_fence_chain(prev);
	uint64_t context;

	spin_lock_init(&chain->dfc_lock);
	chain->dfc_prev = prev;	  /* consume caller's reference */
	chain->dfc_fence = fence; /* consume caller's reference */
	init_irq_work(&chain->dfc_irq_work, &dma_fence_chain_irq_work);

	if (prev_chain == NULL ||
	    !__dma_fence_is_later(seqno, prev->seqno, prev->ops)) {
		context = dma_fence_context_alloc(1);
		if (prev_chain)
			seqno = MAX(prev->seqno, seqno);
		chain->prev_seqno = 0;
	} else {
		context = prev->context;
		chain->prev_seqno = prev->seqno;
	}

	dma_fence_init(&chain->base, &dma_fence_chain_ops, &chain->dfc_lock,
	    context, seqno);
}

static const char *
dma_fence_chain_driver_name(struct dma_fence *fence)
{

	return "dma_fence_chain";
}

static const char *
dma_fence_chain_timeline_name(struct dma_fence *fence)
{

	return "unbound";
}

static void
dma_fence_chain_irq_work(struct irq_work *work)
{
	struct dma_fence_chain *chain = container_of(work,
	    struct dma_fence_chain, dfc_irq_work);

	if (!dma_fence_chain_enable_signaling(&chain->base))
		dma_fence_signal(&chain->base);
	dma_fence_put(&chain->base);
}

static void
dma_fence_chain_callback(struct dma_fence *fence, struct dma_fence_cb *cb)
{
	struct dma_fence_chain *chain = container_of(cb,
	    struct dma_fence_chain, dfc_callback);

	irq_work_queue(&chain->dfc_irq_work);
	dma_fence_put(fence);
}

static bool
dma_fence_chain_enable_signaling(struct dma_fence *fence)
{
	struct dma_fence_chain *chain = to_dma_fence_chain(fence);
	struct dma_fence_chain *chain1;
	struct dma_fence *f, *f1;

	KASSERT(chain);

	dma_fence_get(&chain->base);
	dma_fence_chain_for_each(f, &chain->base) {
		f1 = (chain1 = to_dma_fence_chain(f)) ? chain1->dfc_fence : f;

		dma_fence_get(f1);
		if (dma_fence_add_callback(f, &chain->dfc_callback,
			dma_fence_chain_callback) != 0) {
			dma_fence_put(f);
			return true;
		}
		dma_fence_put(f1);
	}
	dma_fence_put(&chain->base);

	return false;
}

static bool
dma_fence_chain_signaled(struct dma_fence *fence)
{
	struct dma_fence_chain *chain1;
	struct dma_fence *f, *f1;

	dma_fence_chain_for_each(f, fence) {
		f1 = (chain1 = to_dma_fence_chain(f)) ? chain1->dfc_fence : f;

		if (!dma_fence_is_signaled(f1)) {
			dma_fence_put(f);
			return false;
		}
	}

	return true;
}

static void
dma_fence_chain_release(struct dma_fence *fence)
{
	struct dma_fence_chain *chain = to_dma_fence_chain(fence);
	struct dma_fence_chain *prev_chain;
	struct dma_fence *prev;

	KASSERT(chain);

	/*
	 * Release the previous pointer, carefully.  Caller has
	 * exclusive access to chain, so no need for atomics here.
	 */
	while ((prev = chain->dfc_prev) != NULL) {
		/*
		 * If anyone else still holds a reference to the
		 * previous fence, or if it's not a chain, stop here.
		 */
		if (kref_read(&prev->refcount) > 1)
			break;
		if ((prev_chain = to_dma_fence_chain(prev)) == NULL)
			break;

		/*
		 * Cut it out and free it.  We have exclusive access to
		 * prev so this is safe.  This dma_fence_put triggers
		 * recursion into dma_fence_chain_release, but the
		 * recursion is bounded to one level.
		 */
		chain->dfc_prev = prev_chain->dfc_prev;
		prev_chain->dfc_prev = NULL;
		dma_fence_put(prev);
	}
	dma_fence_put(prev);

	dma_fence_put(chain->dfc_fence);
	spin_lock_destroy(&chain->dfc_lock);
	dma_fence_free(&chain->base);
}

static const struct dma_fence_ops dma_fence_chain_ops = {
	.use_64bit_seqno = true,
	.get_driver_name = dma_fence_chain_driver_name,
	.get_timeline_name = dma_fence_chain_timeline_name,
	.enable_signaling = dma_fence_chain_enable_signaling,
	.signaled = dma_fence_chain_signaled,
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
 * get_prev(chain)
 *
 *	Get the previous fence of the chain and add a reference, if
 *	possible; return NULL otherwise.
 */
static struct dma_fence *
get_prev(struct dma_fence_chain *chain)
{
	struct dma_fence *prev;

	rcu_read_lock();
	prev = dma_fence_get_rcu_safe(&chain->dfc_prev);
	rcu_read_unlock();

	return prev;
}

/*
 * dma_fence_chain_walk(fence)
 *
 *	Find the first unsignalled fence in the chain, or NULL if fence
 *	is not a chain node or the chain's fences are all signalled.
 *	While searching, cull signalled fences.
 */
struct dma_fence *
dma_fence_chain_walk(struct dma_fence *fence)
{
	struct dma_fence_chain *chain, *prev_chain;
	struct dma_fence *prev, *splice;

	if ((chain = to_dma_fence_chain(fence)) == NULL) {
		dma_fence_put(fence);
		return NULL;
	}

	while ((prev = get_prev(chain)) != NULL) {
		if ((prev_chain = to_dma_fence_chain(prev)) != NULL) {
			if (!dma_fence_is_signaled(prev_chain->dfc_fence))
				break;
			splice = get_prev(prev_chain);
		} else {
			if (!dma_fence_is_signaled(prev))
				break;
			splice = NULL;
		}
		membar_release();	/* pairs with dma_fence_get_rcu_safe */
		if (atomic_cas_ptr(&chain->dfc_prev, prev, splice) == prev)
			dma_fence_put(prev); /* transferred to splice */
		else
			dma_fence_put(splice);
		dma_fence_put(prev);
	}

	dma_fence_put(fence);
	return prev;
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
