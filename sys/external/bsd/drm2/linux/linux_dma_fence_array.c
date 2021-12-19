/*	$NetBSD: linux_dma_fence_array.c,v 1.2 2021/12/19 12:23:50 riastradh Exp $	*/

/*-
 * Copyright (c) 2021 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: linux_dma_fence_array.c,v 1.2 2021/12/19 12:23:50 riastradh Exp $");

#include <sys/systm.h>

#include <linux/dma-fence-array.h>

static const char *
dma_fence_array_driver_name(struct dma_fence *fence)
{
	return "dma-fence-array";
}

static const char *
dma_fence_array_timeline_name(struct dma_fence *fence)
{
	return "dma-fence-array-timeline";
}

static void
dma_fence_array_done1(struct dma_fence *fence, struct dma_fence_cb *cb)
{
	struct dma_fence_array_cb *C =
	    container_of(cb, struct dma_fence_array_cb, dfac_cb);
	struct dma_fence_array *A = C->dfac_array;

	KASSERT(spin_is_locked(&A->dfa_lock));

	if (fence->error && A->base.error == 1) {
		KASSERT(fence->error != 1);
		A->base.error = fence->error;
	}
	if (--A->dfa_npending) {
		dma_fence_put(&A->base);
		return;
	}

	/* Last one out, hit the lights -- dma_fence_array_done.  */
	irq_work_queue(&A->dfa_work);
}

static void
dma_fence_array_done(struct irq_work *W)
{
	struct dma_fence_array *A = container_of(W, struct dma_fence_array,
	    dfa_work);

	spin_lock(&A->dfa_lock);
	if (A->base.error == 1)
		A->base.error = 0;
	dma_fence_signal_locked(&A->base);
	spin_unlock(&A->dfa_lock);

	dma_fence_put(&A->base);
}

static bool
dma_fence_array_enable_signaling(struct dma_fence *fence)
{
	struct dma_fence_array *A = to_dma_fence_array(fence);
	struct dma_fence_array_cb *C;
	unsigned i;
	int error;

	KASSERT(spin_is_locked(&A->dfa_lock));

	for (i = 0; i < A->num_fences; i++) {
		C = &A->dfa_cb[i];
		C->dfac_array = A;
		dma_fence_get(&A->base);
		if (dma_fence_add_callback(A->fences[i], &C->dfac_cb,
			dma_fence_array_done1)) {
			error = A->fences[i]->error;
			if (error) {
				KASSERT(error != 1);
				if (A->base.error == 1)
					A->base.error = error;
			}
			dma_fence_put(&A->base);
			if (--A->dfa_npending == 0) {
				if (A->base.error == 1)
					A->base.error = 0;
				return false;
			}
		}
	}

	return true;
}

static bool
dma_fence_array_signaled(struct dma_fence *fence)
{
	struct dma_fence_array *A = to_dma_fence_array(fence);

	KASSERT(spin_is_locked(&A->dfa_lock));

	return A->dfa_npending == 0;
}

static void
dma_fence_array_release(struct dma_fence *fence)
{
	struct dma_fence_array *A = to_dma_fence_array(fence);
	unsigned i;

	for (i = 0; i < A->num_fences; i++)
		dma_fence_put(A->fences[i]);

	kfree(A->fences);
	dma_fence_free(fence);
}

static const struct dma_fence_ops dma_fence_array_ops = {
	.get_driver_name = dma_fence_array_driver_name,
	.get_timeline_name = dma_fence_array_timeline_name,
	.enable_signaling = dma_fence_array_enable_signaling,
	.signaled = dma_fence_array_signaled,
	.release = dma_fence_array_release,
};

struct dma_fence_array *
dma_fence_array_create(int num_fences, struct dma_fence **fences,
    unsigned context, unsigned seqno, bool signal_on_any)
{
	struct dma_fence_array *A;

	/*
	 * Must be allocated with kmalloc or equivalent because
	 * dma-fence will free it with kfree.
	 */
	A = kzalloc(struct_size(A, dfa_cb, num_fences), GFP_KERNEL);
	if (A == NULL)
		return NULL;

	A->fences = fences;
	A->num_fences = num_fences;
	A->dfa_npending = signal_on_any ? 1 : num_fences;

	spin_lock_init(&A->dfa_lock);
	dma_fence_init(&A->base, &dma_fence_array_ops, &A->dfa_lock,
	    context, seqno);
	init_irq_work(&A->dfa_work, dma_fence_array_done);

	return A;
}

bool
dma_fence_is_array(struct dma_fence *fence)
{

	return fence->ops == &dma_fence_array_ops;
}

struct dma_fence_array *
to_dma_fence_array(struct dma_fence *fence)
{

	KASSERT(dma_fence_is_array(fence));
	return container_of(fence, struct dma_fence_array, base);
}
