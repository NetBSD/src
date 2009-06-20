/*
 * Copyright 2007 Nouveau Project
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

#include "nouveau_private.h"
#include "nouveau_dma.h"

static void
nouveau_fence_del_unsignalled(struct nouveau_fence *fence)
{
	struct nouveau_channel_priv *nvchan = nouveau_channel(fence->channel);
	struct nouveau_fence *le;

	if (nvchan->fence_head == fence) {
		nvchan->fence_head = nouveau_fence(fence)->next;
		if (nvchan->fence_head == NULL)
			nvchan->fence_tail = NULL;
		return;
	}

	le = nvchan->fence_head;
	while (le && nouveau_fence(le)->next != fence)
		le = nouveau_fence(le)->next;
	assert(le && nouveau_fence(le)->next == fence);
	nouveau_fence(le)->next = nouveau_fence(fence)->next;
	if (nvchan->fence_tail == fence)
		nvchan->fence_tail = le;
}

static void
nouveau_fence_del(struct nouveau_fence **fence)
{
	struct nouveau_fence_priv *nvfence;

	if (!fence || !*fence)
		return;
	nvfence = nouveau_fence(*fence);
	*fence = NULL;

	if (--nvfence->refcount)
		return;

	if (nvfence->emitted && !nvfence->signalled) {
		if (nvfence->signal_cb) {
			nvfence->refcount++;
			nouveau_fence_wait((void *)&nvfence);
			return;
		}

		nouveau_fence_del_unsignalled(&nvfence->base);
	}
	free(nvfence);
}

int
nouveau_fence_new(struct nouveau_channel *chan, struct nouveau_fence **fence)
{
	struct nouveau_fence_priv *nvfence;

	if (!chan || !fence || *fence)
		return -EINVAL;
	
	nvfence = calloc(1, sizeof(struct nouveau_fence_priv));
	if (!nvfence)
		return -ENOMEM;
	nvfence->base.channel = chan;
	nvfence->refcount = 1;

	*fence = &nvfence->base;
	return 0;
}

int
nouveau_fence_ref(struct nouveau_fence *ref, struct nouveau_fence **fence)
{
	if (!fence)
		return -EINVAL;

	if (ref)
		nouveau_fence(ref)->refcount++;

	if (*fence)
		nouveau_fence_del(fence);

	*fence = ref;
	return 0;
}

int
nouveau_fence_signal_cb(struct nouveau_fence *fence, void (*func)(void *),
			void *priv)
{
	struct nouveau_fence_priv *nvfence = nouveau_fence(fence);
	struct nouveau_fence_cb *cb;

	if (!nvfence || !func)
		return -EINVAL;

	cb = malloc(sizeof(struct nouveau_fence_cb));
	if (!cb)
		return -ENOMEM;

	cb->func = func;
	cb->priv = priv;
	cb->next = nvfence->signal_cb;
	nvfence->signal_cb = cb;
	return 0;
}

void
nouveau_fence_emit(struct nouveau_fence *fence)
{
	struct nouveau_channel_priv *nvchan = nouveau_channel(fence->channel);
	struct nouveau_fence_priv *nvfence = nouveau_fence(fence);

	nvfence->emitted = 1;
	nvfence->sequence = ++nvchan->fence_sequence;
	if (nvfence->sequence == 0xffffffff)
		printf("AII wrap unhandled\n");

	if (!nvchan->fence_ntfy) {
		/*XXX: assumes subc 0 is populated */
		nouveau_dma_space(fence->channel, 2);
		nouveau_dma_out  (fence->channel, 0x00040050);
		nouveau_dma_out  (fence->channel, nvfence->sequence);
	}
	nouveau_dma_kickoff(fence->channel);

	if (nvchan->fence_tail) {
		nouveau_fence(nvchan->fence_tail)->next = fence;
	} else {
		nvchan->fence_head = fence;
	}
	nvchan->fence_tail = fence;
}

static void
nouveau_fence_flush_seq(struct nouveau_channel *chan, uint32_t sequence)
{
	struct nouveau_channel_priv *nvchan = nouveau_channel(chan);

	while (nvchan->fence_head) {
		struct nouveau_fence_priv *nvfence;
	
		nvfence = nouveau_fence(nvchan->fence_head);
		if (nvfence->sequence > sequence)
			break;
		nouveau_fence_del_unsignalled(&nvfence->base);
		nvfence->signalled = 1;

		if (nvfence->signal_cb) {
			struct nouveau_fence *fence = NULL;

			nouveau_fence_ref(&nvfence->base, &fence);

			while (nvfence->signal_cb) {
				struct nouveau_fence_cb *cb;
				
				cb = nvfence->signal_cb;
				nvfence->signal_cb = cb->next;
				cb->func(cb->priv);
				free(cb);
			}

			nouveau_fence_ref(NULL, &fence);
		}
	}
}

void
nouveau_fence_flush(struct nouveau_channel *chan)
{
	struct nouveau_channel_priv *nvchan = nouveau_channel(chan);

	if (!nvchan->fence_ntfy)
		nouveau_fence_flush_seq(chan, *nvchan->ref_cnt);
}

int
nouveau_fence_wait(struct nouveau_fence **fence)
{
	struct nouveau_fence_priv *nvfence;
	struct nouveau_channel_priv *nvchan;

	if (!fence)
		return -EINVAL;

	nvfence = nouveau_fence(*fence);
	if (!nvfence)
		return 0;
	nvchan = nouveau_channel(nvfence->base.channel);

	if (nvfence->emitted) {
		if (!nvfence->signalled && nvchan->fence_ntfy) {
			struct nouveau_channel *chan = &nvchan->base;
			int ret;
 
			/*XXX: NV04/NV05: Full sync + flush all fences */
			nouveau_notifier_reset(nvchan->fence_ntfy, 0);
			BEGIN_RING(chan, nvchan->fence_grobj, 0x0104, 1);
			OUT_RING  (chan, 0);
			BEGIN_RING(chan, nvchan->fence_grobj, 0x0100, 1);
			OUT_RING  (chan, 0);
			FIRE_RING (chan);
			ret = nouveau_notifier_wait_status(nvchan->fence_ntfy,
							   0, 0, 2.0);
			if (ret)
				return ret;
 
			nouveau_fence_flush_seq(chan, nvchan->fence_sequence);
		}

		while (!nvfence->signalled)
			nouveau_fence_flush(nvfence->base.channel);
	}

	nouveau_fence_ref(NULL, fence);
	return 0;
}

