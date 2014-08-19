/*	$NetBSD: ugenhc_dma.c,v 1.1.10.2 2014/08/20 00:04:38 tls Exp $	*/

/*
 * Copyright (c) 2009 Antti Kantee.  All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/kmem.h>

#include <sys/bus.h>

/*
 * bus_dma(9) that works for USB drivers
 */

int
bus_dmamap_create(bus_dma_tag_t tag, bus_size_t sz, int flag, bus_size_t bsz,
	bus_size_t bsz2, int i, bus_dmamap_t *ptr)
{

	return 0;
}

void
bus_dmamap_destroy(bus_dma_tag_t tag, bus_dmamap_t map)
{

	panic("unimplemented %s", __func__);
}

int
bus_dmamap_load(bus_dma_tag_t t, bus_dmamap_t a, void *b, bus_size_t c,
	struct proc *d, int e)
{

	return 0;
}

void
bus_dmamap_unload(bus_dma_tag_t a, bus_dmamap_t b)
{

	panic("unimplemented %s", __func__);
}

void
bus_dmamap_sync(bus_dma_tag_t a, bus_dmamap_t b, bus_addr_t c,
	bus_size_t d, int e)
{

	panic("unimplemented %s", __func__);
}

int
bus_dmamem_alloc(bus_dma_tag_t tag, bus_size_t size, bus_size_t align,
	bus_size_t boundary, bus_dma_segment_t *segs, int nsegs,
	int *rsegs, int flags)
{

	*rsegs = nsegs;
	return 0;
}

void
bus_dmamem_free(bus_dma_tag_t a, bus_dma_segment_t *b, int c)
{

	panic("unimplemented %s", __func__);
}

int
bus_dmamem_map(bus_dma_tag_t tag, bus_dma_segment_t *segs, int nsegs,
                       size_t size, void **kvap, int flags)
{

	KASSERT(nsegs == 1);
	*kvap = kmem_alloc(size, KM_SLEEP);
	return 0;
}

void
bus_dmamem_unmap(bus_dma_tag_t a, void *kva, size_t b)
{

	kmem_free(kva, b);
}
