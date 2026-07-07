/*	$NetBSD: bus_dma_stub.c,v 1.1 2026/07/07 15:24:51 thorpej Exp $	*/

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

/*
 * Stub implementation of bus_dma() that does nothing, returns errors.
 * This is for platforms that don't support DMA, but where providing
 * the symbols is still useful.
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */
__KERNEL_RCSID(0, "$NetBSD: bus_dma_stub.c,v 1.1 2026/07/07 15:24:51 thorpej Exp $");

#define _M68K_BUS_DMA_PRIVATE

#include <sys/param.h>
#include <sys/errno.h>
#include <machine/bus.h>

void	_bus_dma_nullop(void);
int	_bus_dma_enotsup(void);
int	_bus_dma_neg_one(void);

void
_bus_dma_nullop(void)
{
	return;
}

int
_bus_dma_enotsup(void)
{
	return ENOTSUP;
}

int
_bus_dma_neg_one(void)
{
	return -1;
}

__strong_alias(_bus_dmamap_create,_bus_dma_enotsup);
__strong_alias(_bus_dmamap_destroy,_bus_dma_nullop);

__strong_alias(_bus_dmamap_load_direct,_bus_dma_enotsup);
__strong_alias(_bus_dmamap_load_mbuf_direct,_bus_dma_enotsup);
__strong_alias(_bus_dmamap_load_uio_direct,_bus_dma_enotsup);
__strong_alias(_bus_dmamap_load_raw_direct,_bus_dma_enotsup);
__strong_alias(_bus_dmamap_unload,_bus_dma_nullop);

__strong_alias(_bus_dmamap_sync,_bus_dma_nullop);

__strong_alias(_bus_dmamem_alloc_common,_bus_dma_enotsup);
__strong_alias(_bus_dmamem_alloc,_bus_dma_enotsup);
__strong_alias(_bus_dmamem_free,_bus_dma_enotsup);
__strong_alias(_bus_dmamem_map,_bus_dma_enotsup);
__strong_alias(_bus_dmamem_unmap,_bus_dma_nullop);
__strong_alias(_bus_dmamem_mmap,_bus_dma_neg_one);
