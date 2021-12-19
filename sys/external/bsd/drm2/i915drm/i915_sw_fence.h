/*	$NetBSD: i915_sw_fence.h,v 1.4 2021/12/19 11:19:55 riastradh Exp $	*/

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
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

#ifndef	_I915DRM_I915_SW_FENCE_H_
#define	_I915DRM_I915_SW_FENCE_H_

#include <sys/cdefs.h>
#include <sys/types.h>

#include <linux/gfp.h>

struct dma_fence_ops;
struct dma_resv;
struct i915_sw_fence;
struct i915_sw_fence_cb;

struct i915_sw_fence {
	char dummy;
};

struct i915_sw_fence_wait {
	char dummy;
};

struct i915_sw_dma_fence_cb {
	char dummy;
};

enum i915_sw_fence_notify {
	FENCE_COMPLETE,
	FENCE_FREE,
};

#define	__i915_sw_fence_call	__aligned(4)

void	i915_sw_fence_init(struct i915_sw_fence *,
	    int (*)(struct i915_sw_fence *, enum i915_sw_fence_notify));
void	i915_sw_fence_fini(struct i915_sw_fence *);

bool	i915_sw_fence_signaled(struct i915_sw_fence *);

void	i915_sw_fence_await_reservation(struct i915_sw_fence *,
	    struct dma_resv *, const struct dma_fence_ops *, bool,
	    unsigned long, gfp_t);
void	i915_sw_fence_await_sw_fence(struct i915_sw_fence *,
	    struct i915_sw_fence *, struct i915_sw_fence_wait *);
void	i915_sw_fence_await_sw_fence_gfp(struct i915_sw_fence *,
	    struct i915_sw_fence *, gfp_t);
int	i915_sw_fence_await_dma_fence(struct i915_sw_fence *,
	    struct dma_fence *, int, gfp_t);
void	i915_sw_fence_commit(struct i915_sw_fence *);

#endif	/* _I915DRM_I915_SW_FENCE_H_ */
