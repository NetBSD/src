/*	$NetBSD: exynos_dma.c,v 1.1.6.2 2014/08/10 06:53:52 tls Exp $	*/

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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

#include "opt_exynos.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: exynos_dma.c,v 1.1.6.2 2014/08/10 06:53:52 tls Exp $");

#define _ARM32_BUS_DMA_PRIVATE

#include <sys/param.h>
#include <sys/bus.h>

#include <arm/samsung/exynos_reg.h>
#include <arm/samsung/exynos_var.h>

struct arm32_bus_dma_tag exynos_bus_dma_tag = {
	_BUS_DMAMAP_FUNCS,
	_BUS_DMAMEM_FUNCS,
	_BUS_DMATAG_FUNCS,
};

struct arm32_dma_range exynos_coherent_dma_ranges[1] = {
	[0] = {
		.dr_sysbase = 0,	/* filled in */
		.dr_busbase = 0,	/* filled in */
		.dr_flags = _BUS_DMAMAP_COHERENT,
	},
};

struct arm32_bus_dma_tag exynos_coherent_bus_dma_tag = {
	._ranges  = exynos_coherent_dma_ranges,
	._nranges = __arraycount(exynos_coherent_dma_ranges),
	_BUS_DMAMAP_FUNCS,
	_BUS_DMAMEM_FUNCS,
	_BUS_DMATAG_FUNCS,
};


#ifndef EXYNOS4
#	define EXYNOS4_SDRAM_PBASE 0
#endif
#ifndef EXYNOS5
#	define EXYNOS5_SDRAM_PBASE 0
#endif
void
exynos_dma_bootstrap(psize_t memsize)
{
	bus_addr_t dram_base = IS_EXYNOS4_P() ?
		EXYNOS4_SDRAM_PBASE : EXYNOS5_SDRAM_PBASE;

	KASSERT(dram_base);
	exynos_coherent_dma_ranges[0].dr_sysbase = dram_base;
	exynos_coherent_dma_ranges[0].dr_busbase = dram_base;
	exynos_coherent_dma_ranges[0].dr_len = memsize;
}

