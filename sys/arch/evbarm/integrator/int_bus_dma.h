/*	$NetBSD: int_bus_dma.h,v 1.1 2001/10/27 16:17:51 rearnsha Exp $ */

/*
 * Copyright (c) 2001 ARM Ltd
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
 * 3. The name of the company may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _INT_BUS_DMA_H
#define _INT_BUS_DMA_H

#include <machine/bus.h>

#ifdef _ARM32_BUS_DMA_PRIVATE

#define CM_ALIAS_TO_LOCAL(addr) (addr & 0x0fffffff)
#define LOCAL_TO_CM_ALIAS(addr)	(addr | 0x80000000)

int     integrator_bus_dmamap_load __P((bus_dma_tag_t, bus_dmamap_t, void *,
            bus_size_t, struct proc *, int));
int     integrator_bus_dmamap_load_mbuf __P((bus_dma_tag_t, bus_dmamap_t,
            struct mbuf *, int));
int     integrator_bus_dmamap_load_uio __P((bus_dma_tag_t, bus_dmamap_t,
            struct uio *, int));

int     integrator_bus_dmamem_alloc __P((bus_dma_tag_t tag, bus_size_t size,
            bus_size_t alignment, bus_size_t boundary,
            bus_dma_segment_t *segs, int nsegs, int *rsegs, int flags));
void    integrator_bus_dmamem_free __P((bus_dma_tag_t tag, 
	    bus_dma_segment_t *segs, int nsegs));
int     integrator_bus_dmamem_map __P((bus_dma_tag_t tag,
	    bus_dma_segment_t *segs, int nsegs, size_t size, caddr_t *kvap,
	    int flags));
paddr_t integrator_bus_dmamem_mmap __P((bus_dma_tag_t tag,
	    bus_dma_segment_t *segs, int nsegs, off_t off, int prot,
	    int flags));

int     integrator_bus_dmamem_alloc_range __P((bus_dma_tag_t tag,
	    bus_size_t size, bus_size_t alignment, bus_size_t boundary,
            bus_dma_segment_t *segs, int nsegs, int *rsegs, int flags,
            vaddr_t low, vaddr_t high));
#endif /* _ARM32_BUS_DMA_PRIVATE */
#endif /* _INT_BUS_DMA_H */
