/*	$NetBSD: ttm_page_alloc.h,v 1.1.6.3 2017/12/03 11:37:59 jdolecek Exp $	*/

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
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

#ifndef _DRM_TTM_TTM_PAGE_ALLOC_H_
#define _DRM_TTM_TTM_PAGE_ALLOC_H_

struct ttm_dma_tt;
struct ttm_mem_global;

int	ttm_bus_dma_populate(struct ttm_dma_tt *);
void	ttm_bus_dma_unpopulate(struct ttm_dma_tt *);
void	ttm_bus_dma_swapout(struct ttm_dma_tt *);

static inline int
ttm_page_alloc_init(struct ttm_mem_global *glob __unused,
    unsigned max_pages __unused)
{
	return 0;
}

static inline void
ttm_page_alloc_fini(void)
{
}

static inline int
ttm_dma_page_alloc_init(struct ttm_mem_global *glob __unused,
    unsigned max_pages __unused)
{
	return 0;
}

static inline void
ttm_dma_page_alloc_fini(void)
{
}

#endif	/* _DRM_TTM_TTM_PAGE_ALLOC_H_ */
