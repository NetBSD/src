/*	$NetBSD: ibm4xx_460ex_l2.c,v 1.1 2026/06/19 18:55:23 rkujawa Exp $	*/

/*
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa.
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
 * AMCC PPC460EX on-chip 256KB L2 cache support
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ibm4xx_460ex_l2.c,v 1.1 2026/06/19 18:55:23 rkujawa Exp $");

#include "opt_ppc4xx.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/intr.h>

#define	_POWERPC_BUS_DMA_PRIVATE
#include <sys/bus.h>

#include <powerpc/ibm4xx/cpu.h>
#include <powerpc/ibm4xx/amcc460ex.h>
#include <powerpc/ibm4xx/ibm4xx_460ex_l2.h>

#define	L2_LINE_SIZE	32
#define	L2_WAYS		4
#define	L2_SIZE		(256 * 1024)
#define	L2_INDEX_SPAN	(L2_SIZE / L2_WAYS)	/* 64KB */

bool ibm4xx_460ex_l2_enabled = false;
uint32_t ibm4xx_460ex_l2_cfg;

/*
 * Build the L2C0_ADDR val for an invalidate command 
 */
static inline uint32_t
ibm4xx_460ex_l2_addr(bus_addr_t pa)
{
	return (uint32_t)pa & 0xfffffff0;
}

/*
 * Invalidate the L2 lines backing the (offset, offset+len) window 
 */
static void
ibm4xx_460ex_l2_invalidate(bus_dma_tag_t t, bus_dmamap_t map,
    bus_addr_t offset, bus_size_t len)
{
	const bus_dma_segment_t *ds = map->dm_segs;
	int s;

	if (len == 0)
		return;

	s = splhigh();

	if (len >= L2_INDEX_SPAN) {
		/* Window sweeps every index: one whole-cache invalidate. */
		mtdcr(DCR_L2C0_ADDR, 0);
		mtdcr(DCR_L2C0_CMD, L2C_CMD_HCC);
		while ((mfdcr(DCR_L2C0_SR) & L2C_SR_CC) == 0)
			;
		__asm volatile ("msync" ::: "memory");
		splx(s);
		return;
	}

	/* Skip leading amount. */
	while (offset >= ds->ds_len) {
		offset -= ds->ds_len;
		ds++;
	}
	for (; len > 0; ds++, offset = 0) {
		bus_size_t seglen = ds->ds_len - offset;
		bus_addr_t addr = BUS_MEM_TO_PHYS(t, ds->ds_addr) + offset;
		bus_addr_t lineoff, epa;

		if (seglen > len)
			seglen = len;
		len -= seglen;
		KASSERT(ds < &map->dm_segs[map->dm_nsegs]);

		/* Realign to cache-line boundaries. */
		lineoff = addr & (L2_LINE_SIZE - 1);
		seglen += lineoff;
		addr -= lineoff;

		for (epa = addr + seglen; addr < epa; addr += L2_LINE_SIZE) {
			mtdcr(DCR_L2C0_ADDR, ibm4xx_460ex_l2_addr(addr));
			mtdcr(DCR_L2C0_CMD, L2C_CMD_INV);
			while ((mfdcr(DCR_L2C0_SR) & L2C_SR_CC) == 0)
				;
		}
	}
	__asm volatile ("msync" ::: "memory");
	splx(s);
}

/*
 * Private bus_dma sync for the USB controllers. 
 */
static void
ibm4xx_460ex_l2_dmamap_sync(bus_dma_tag_t t, bus_dmamap_t map,
    bus_addr_t offset, bus_size_t len, int ops)
{
	if (ibm4xx_460ex_l2_enabled && (ops & BUS_DMASYNC_POSTREAD) != 0)
		ibm4xx_460ex_l2_invalidate(t, map, offset, len);
	_bus_dmamap_sync(t, map, offset, len, ops);
}

/*
 * A clone of ibm4xx_default_bus_dma_tag sync overriden
 */
struct powerpc_bus_dma_tag ibm4xx_460ex_l2_bus_dma_tag = {
	0, 0,
	_bus_dmamap_create,
	_bus_dmamap_destroy,
	_bus_dmamap_load,
	_bus_dmamap_load_mbuf,
	_bus_dmamap_load_uio,
	_bus_dmamap_load_raw,
	_bus_dmamap_unload,
	ibm4xx_460ex_l2_dmamap_sync,
	_bus_dmamem_alloc,
	_bus_dmamem_free,
	_bus_dmamem_map,
	_bus_dmamem_unmap,
	_bus_dmamem_mmap,
	_bus_dma_phys_to_bus_mem_generic,
	_bus_dma_bus_mem_to_phys_generic,
};

bus_dma_tag_t
ibm4xx_460ex_l2_dmatag(void)
{
	return &ibm4xx_460ex_l2_bus_dma_tag;
}

void
ibm4xx_460ex_l2cache_enable(u_int memsize)
{
	uint32_t snpsz;
	u_int code;

	/* Hand the SRAM0 data arrays back from the SRAM controller. */
	mtdcr(DCR_SRAM0_SB0CR, 0);
	mtdcr(DCR_SRAM0_SB1CR, 0);
	mtdcr(DCR_SRAM0_SB2CR, 0);
	mtdcr(DCR_SRAM0_SB3CR, 0);

	/* RDBW is required; switch the array into L2 mode. */
	mtdcr(DCR_L2C0_CFG, L2C_CFG_RDBW | L2C_CFG_L2M | L2C_CFG_SS_256KB);

	/* Reset the tag array with the hardware clear command. */
	mtdcr(DCR_L2C0_ADDR, 0);
	mtdcr(DCR_L2C0_CMD, L2C_CMD_HCC);
	while ((mfdcr(DCR_L2C0_SR) & L2C_SR_CC) == 0)
		;
	/* Clear any latched cache- and tag-parity errors. */
	mtdcr(DCR_L2C0_CMD, L2C_CMD_CCP);
	while ((mfdcr(DCR_L2C0_SR) & L2C_SR_CC) == 0)
		;
	mtdcr(DCR_L2C0_CMD, L2C_CMD_CTE);
	while ((mfdcr(DCR_L2C0_SR) & L2C_SR_CC) == 0)
		;
	__asm volatile ("msync" ::: "memory");

	/*
	 * Program snoop region 0 to cover all of DRAM on the LL segment!
	 */
	snpsz = 0x100000;		/* 1MB, the smallest snoop region */
	code = 0;
	while (snpsz < memsize) {
		snpsz <<= 1;
		code++;
	}
	mtdcr(DCR_L2C0_SNP0, (code << L2C_SNP_SSR_SHIFT) | L2C_SNP_ESR);
	mtdcr(DCR_L2C0_SNP1, 0);

	/* Enable instruction- and data-side L2 caching. */
	mtdcr(DCR_L2C0_CFG, L2C_CFG_RDBW | L2C_CFG_L2M | L2C_CFG_SS_256KB |
	    L2C_CFG_FRAN | L2C_CFG_SNPCI | L2C_CFG_ICU | L2C_CFG_DCU);
	__asm volatile ("msync" ::: "memory");

	/* Read back so the caller can confirm what actually stuck. */
	ibm4xx_460ex_l2_cfg = mfdcr(DCR_L2C0_CFG);
	if (ibm4xx_460ex_l2_cfg & L2C_CFG_L2M)
		ibm4xx_460ex_l2_enabled = true;
}

