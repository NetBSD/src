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

#ifndef __NOUVEAU_PRIVATE_H__
#define __NOUVEAU_PRIVATE_H__

#include <stdint.h>
#include <xf86drm.h>
#include <nouveau_drm.h>

#include "nouveau_drmif.h"
#include "nouveau_device.h"
#include "nouveau_channel.h"
#include "nouveau_grobj.h"
#include "nouveau_notifier.h"
#include "nouveau_bo.h"
#include "nouveau_resource.h"
#include "nouveau_pushbuf.h"

#define NOUVEAU_PUSHBUF_MAX_BUFFERS 1024
#define NOUVEAU_PUSHBUF_MAX_RELOCS 1024
struct nouveau_pushbuf_priv {
	struct nouveau_pushbuf base;

	int use_cal;
	struct nouveau_bo *buffer;

	unsigned *pushbuf;
	unsigned  size;

	struct drm_nouveau_gem_pushbuf_bo *buffers;
	unsigned nr_buffers;
	struct drm_nouveau_gem_pushbuf_reloc *relocs;
	unsigned nr_relocs;

	/*XXX: nomm */
	struct nouveau_fence *fence;
};
#define nouveau_pushbuf(n) ((struct nouveau_pushbuf_priv *)(n))

#define pbbo_to_ptr(o) ((uint64_t)(unsigned long)(o))
#define ptr_to_pbbo(h) ((struct nouveau_pushbuf_bo *)(unsigned long)(h))
#define pbrel_to_ptr(o) ((uint64_t)(unsigned long)(o))
#define ptr_to_pbrel(h) ((struct nouveau_pushbuf_reloc *)(unsigned long)(h))
#define bo_to_ptr(o) ((uint64_t)(unsigned long)(o))
#define ptr_to_bo(h) ((struct nouveau_bo_priv *)(unsigned long)(h))

int
nouveau_pushbuf_init(struct nouveau_channel *);

struct nouveau_dma_priv {
	uint32_t base;
	uint32_t max;
	uint32_t cur;
	uint32_t put;
	uint32_t free;

	int push_free;
} dma;

struct nouveau_channel_priv {
	struct nouveau_channel base;

	struct drm_nouveau_channel_alloc drm;

	void     *notifier_block;

	struct nouveau_pushbuf_priv pb;

	/*XXX: nomm */
	volatile uint32_t *user, *put, *get, *ref_cnt;
	uint32_t *pushbuf;
	struct nouveau_dma_priv struct_dma;
	struct nouveau_dma_priv *dma;
	struct nouveau_fence *fence_head;
	struct nouveau_fence *fence_tail;
	uint32_t fence_sequence;
	struct nouveau_grobj *fence_grobj;
	struct nouveau_notifier *fence_ntfy;
};
#define nouveau_channel(n) ((struct nouveau_channel_priv *)(n))

struct nouveau_fence {
	struct nouveau_channel *channel;
};

struct nouveau_fence_cb {
	struct nouveau_fence_cb *next;
	void (*func)(void *);
	void *priv;
};

struct nouveau_fence_priv {
	struct nouveau_fence base;
	int refcount;

	struct nouveau_fence *next;
	struct nouveau_fence_cb *signal_cb;

	uint32_t sequence;
	int emitted;
	int signalled;
};
#define nouveau_fence(n) ((struct nouveau_fence_priv *)(n))

int
nouveau_fence_new(struct nouveau_channel *, struct nouveau_fence **);

int
nouveau_fence_ref(struct nouveau_fence *, struct nouveau_fence **);

int
nouveau_fence_signal_cb(struct nouveau_fence *, void (*)(void *), void *);

void
nouveau_fence_emit(struct nouveau_fence *);

int
nouveau_fence_wait(struct nouveau_fence **);

void
nouveau_fence_flush(struct nouveau_channel *);

struct nouveau_grobj_priv {
	struct nouveau_grobj base;
};
#define nouveau_grobj(n) ((struct nouveau_grobj_priv *)(n))

struct nouveau_notifier_priv {
	struct nouveau_notifier base;

	struct drm_nouveau_notifierobj_alloc drm;
	volatile void *map;
};
#define nouveau_notifier(n) ((struct nouveau_notifier_priv *)(n))

struct nouveau_bo_priv {
	struct nouveau_bo base;
	int refcount;

	/* Buffer configuration + usage hints */
	unsigned flags;
	unsigned size;
	unsigned align;
	int user;

	/* Tracking */
	struct drm_nouveau_gem_pushbuf_bo *pending;
	struct nouveau_channel *pending_channel;
	int write_marker;

	/* Userspace object */
	void *sysmem;

	/* Kernel object */
	uint32_t global_handle;
	drm_handle_t handle;
	void *map;

	/* Last known information from kernel on buffer status */
	int pinned;
	uint64_t offset;
	uint32_t domain;

	/*XXX: nomm stuff */
	struct nouveau_fence *fence;
	struct nouveau_fence *wr_fence;
};
#define nouveau_bo(n) ((struct nouveau_bo_priv *)(n))

int
nouveau_bo_init(struct nouveau_device *);

void
nouveau_bo_takedown(struct nouveau_device *);

struct drm_nouveau_gem_pushbuf_bo *
nouveau_bo_emit_buffer(struct nouveau_channel *, struct nouveau_bo *);

int
nouveau_bo_validate_nomm(struct nouveau_bo_priv *, uint32_t);

#include "nouveau_dma.h"
#endif
