/*	$NetBSD: bus_stubs.c,v 1.1 2023/05/07 12:41:48 skrll Exp $	*/

/*
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Maxime Villard.
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
__KERNEL_RCSID(0, "$NetBSD: bus_stubs.c,v 1.1 2023/05/07 12:41:48 skrll Exp $");

#include <sys/systm.h>
#include <sys/asan.h>

#include <machine/bus_defs.h>
#include <machine/bus_funcs.h>

int
bus_dmamap_create(bus_dma_tag_t t, bus_size_t size, int nsegments,
    bus_size_t maxsegsz, bus_size_t boundary, int flags, bus_dmamap_t *dmamp)
{
	return (*t->_dmamap_create)(t, size, nsegments, maxsegsz, boundary,
	    flags, dmamp);
}

void
bus_dmamap_destroy(bus_dma_tag_t t, bus_dmamap_t dmam)
{
	(*t->_dmamap_destroy)(t, dmam);
}

int
bus_dmamap_load(bus_dma_tag_t t, bus_dmamap_t dmam, void *buf,
    bus_size_t buflen, struct proc *p, int flags)
{
	kasan_dma_load(dmam, buf, buflen, KASAN_DMA_LINEAR);
	return (*t->_dmamap_load)(t, dmam, buf, buflen, p, flags);
}

int
bus_dmamap_load_mbuf(bus_dma_tag_t t, bus_dmamap_t dmam, struct mbuf *chain,
    int flags)
{
	kasan_dma_load(dmam, chain, 0, KASAN_DMA_MBUF);
	return (*t->_dmamap_load_mbuf)(t, dmam, chain, flags);
}

int
bus_dmamap_load_uio(bus_dma_tag_t t, bus_dmamap_t dmam, struct uio *uio,
    int flags)
{
	kasan_dma_load(dmam, uio, 0, KASAN_DMA_UIO);
	return (*t->_dmamap_load_uio)(t, dmam, uio, flags);
}

int
bus_dmamap_load_raw(bus_dma_tag_t t, bus_dmamap_t dmam, bus_dma_segment_t *segs,
    int nsegs, bus_size_t size, int flags)
{
	return (*t->_dmamap_load_raw)(t, dmam, segs, nsegs, size, flags);
}

void
bus_dmamap_unload(bus_dma_tag_t t, bus_dmamap_t dmam)
{
	(*t->_dmamap_unload)(t, dmam);
}

void
bus_dmamap_sync(bus_dma_tag_t t, bus_dmamap_t p, bus_addr_t o, bus_size_t l,
    int ops)
{
	kasan_dma_sync(p, o, l, ops);

	(*t->_dmamap_sync)(t, p, o, l, ops);
}
