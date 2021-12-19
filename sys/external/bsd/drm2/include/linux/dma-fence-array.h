/*	$NetBSD: dma-fence-array.h,v 1.6 2021/12/19 12:23:50 riastradh Exp $	*/

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

#ifndef	_LINUX_DMA_FENCE_ARRAY_H_
#define	_LINUX_DMA_FENCE_ARRAY_H_

#include <sys/stdbool.h>

#include <linux/dma-fence.h>
#include <linux/irq_work.h>

#define	dma_fence_array_create		linux_dma_fence_array_create
#define	dma_fence_is_array		linux_dma_fence_is_array
#define	to_dma_fence_array		linux_to_dma_fence_array

struct dma_fence_array_cb {
	struct dma_fence_array	*dfac_array;
	struct dma_fence_cb	dfac_cb;
};

struct dma_fence_array {
	struct dma_fence		base;
	struct dma_fence		**fences;
	unsigned			num_fences;

	spinlock_t			dfa_lock;
	int				dfa_npending;
	struct irq_work			dfa_work;
	struct dma_fence_array_cb	dfa_cb[];
};

struct dma_fence_array *
	dma_fence_array_create(int, struct dma_fence **, unsigned, unsigned,
	    bool);

bool	dma_fence_is_array(struct dma_fence *);
struct dma_fence_array *
	to_dma_fence_array(struct dma_fence *);

#endif	/* _LINUX_DMA_FENCE_ARRAY_H_ */
