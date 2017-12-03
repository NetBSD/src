/*	$NetBSD: bus.c,v 1.3.4.2 2017/12/03 11:36:35 jdolecek Exp $	*/

/*-
 * Copyright (c) 1997, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
__KERNEL_RCSID(0, "$NetBSD: bus.c,v 1.3.4.2 2017/12/03 11:36:35 jdolecek Exp $");

#include "opt_cputype.h"

#define _MIPS_BUS_DMA_PRIVATE

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/mbuf.h>
#include <sys/proc.h>
#include <sys/systm.h>

#include <sys/bus.h>
#include <machine/cpu.h>

#include <uvm/uvm_extern.h>

#include <mips/cpuregs.h>
#include <mips/locore.h>
#include <mips/cache.h>

/*
 * The default DMA tag for all busses on the DECstation.
 */
struct mips_bus_dma_tag pmax_default_bus_dma_tag = {
	._dmamap_ops = _BUS_DMAMAP_OPS_INITIALIZER,
	._dmamem_ops = _BUS_DMAMEM_OPS_INITIALIZER,
	._dmatag_ops = _BUS_DMATAG_OPS_INITIALIZER,
};

static void normal_bus_mem_init(bus_space_tag_t, void *);
static void _bus_dmamap_sync_r3k(bus_dma_tag_t, bus_dmamap_t, bus_addr_t,
				 bus_size_t, int);
#if 0
static void _bus_dmamap_sync_r4k(bus_dma_tag_t, bus_dmamap_t, bus_addr_t,
				 bus_size_t, int);
#endif

static struct mips_bus_space	normal_mbst;
bus_space_tag_t	normal_memt = NULL;


void
pmax_bus_dma_init(void)
{

	normal_bus_mem_init(&normal_mbst, NULL);
	normal_memt = &normal_mbst;

#ifdef MIPS1
	if (CPUISMIPS3 == 0)
		pmax_default_bus_dma_tag._dmamap_ops.dmamap_sync = _bus_dmamap_sync_r3k;
#endif
#if 0
	/* the common dmamap_sync() method should work on those */
#ifdef MIPS3
	if (CPUISMIPS3)
		pmax_default_bus_dma_tag._dmamap_ops.dmamap_sync = _bus_dmamap_sync_r4k;
#endif
#endif
}

#ifdef MIPS1
/*
 * Common function for DMA map synchronization.  May be called
 * by chipset-specific DMA map synchronization functions.
 *
 * This is the R3000 version.
 */
static void
_bus_dmamap_sync_r3k(bus_dma_tag_t t, bus_dmamap_t map, bus_addr_t offset,
    bus_size_t len, int ops)
{
	bus_size_t minlen;
	bus_addr_t addr;
	int i;

	/*
	 * Mixing PRE and POST operations is not allowed.
	 */
	if ((ops & (BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE)) != 0 &&
	    (ops & (BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE)) != 0)
		panic("_bus_dmamap_sync_r3k: mix PRE and POST");

#ifdef DIAGNOSTIC
	if (offset >= map->dm_mapsize)
		panic("%s: bad offset %ju (map size is %ju)", __func__,
		      (uintmax_t)offset, (uintmax_t)map->dm_mapsize);
	if (len == 0 || (offset + len) > map->dm_mapsize)
		panic("%s: bad length", __func__);
#endif

	/*
	 * The R3000 cache is write-though.  Therefore, we only need
	 * to drain the write buffer on PREWRITE.  The cache is not
	 * coherent, however, so we need to invalidate the data cache
	 * on PREREAD (should we do it POSTREAD instead?).
	 *
	 * POSTWRITE (and POSTREAD, currently) are noops.
	 */

	if (ops & BUS_DMASYNC_PREWRITE) {
		/*
		 * Flush the write buffer.
		 */
		wbflush();
	}

	/*
	 * If we're not doing PREREAD, nothing more to do.
	 */
	if ((ops & BUS_DMASYNC_PREREAD) == 0)
		return;

	/*
	 * No cache invlidation is necessary if the DMA map covers
	 * COHERENT DMA-safe memory (which is mapped un-cached).
	 */
	if (map->_dm_flags & _BUS_DMAMAP_COHERENT)
		return;

	/*
	 * If we are going to hit something as large or larger
	 * than the entire data cache, just nail the whole thing.
	 *
	 * NOTE: Even though this is `wbinv_all', since the cache is
	 * write-though, it just invalidates it.
	 */
	if (len >= mips_cache_info.mci_pdcache_size) {
		mips_dcache_wbinv_all();
		return;
	}

	for (i = 0; i < map->dm_nsegs && len != 0; i++) {
		/* Find the beginning segment. */
		if (offset >= map->dm_segs[i].ds_len) {
			offset -= map->dm_segs[i].ds_len;
			continue;
		}

		/*
		 * Now at the first segment to sync; nail 
		 * each segment until we have exhausted the
		 * length.
		 */
		minlen = len < map->dm_segs[i].ds_len - offset ?  
		    len : map->dm_segs[i].ds_len - offset;

		addr = map->dm_segs[i].ds_addr;

#ifdef BUS_DMA_DEBUG
		printf("%s: flushing segment %d (%#jx..%#jx) ...", __func__, i,
		    (intmax_t)addr + offset,
		    (intmax_t)addr + offset + minlen - 1);
#endif
		mips_dcache_inv_range(
		    MIPS_PHYS_TO_KSEG0(addr + offset), minlen);
#ifdef BUS_DMA_DEBUG
		printf("\n");
#endif
		offset = 0;
		len -= minlen;
	}
}
#endif /* MIPS1 */

#if 0
#ifdef MIPS3
/*
 * Common function for DMA map synchronization.  May be called
 * by chipset-specific DMA map synchronization functions.
 *
 * This is the R4000 version.
 */
static void
_bus_dmamap_sync_r4k(bus_dma_tag_t t, bus_dmamap_t map, bus_addr_t offset,
    bus_size_t len, int ops)
{
	bus_size_t minlen;
	bus_addr_t addr;
	int i, useindex;

	/*
	 * Mixing PRE and POST operations is not allowed.
	 */
	if ((ops & (BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE)) != 0 &&
	    (ops & (BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE)) != 0)
		panic("_bus_dmamap_sync_r4k: mix PRE and POST");

#ifdef DIAGNOSTIC
	if (offset >= map->dm_mapsize)
		panic("%s: bad offset %ju (map size is %ju)", __func__,
		      (uintmax_t)offset, (uintmax_t)map->dm_mapsize);
	if (len == 0 || (offset + len) > map->dm_mapsize)
		panic("%s: bad length", __func__);
#endif

	/*
	 * The R4000 cache is virtually-indexed, write-back.  This means
	 * we need to do the following things:
	 *
	 *	PREREAD -- Invalidate D-cache.  Note we might have
	 *	to also write-back here if we have to use an Index
	 *	op, or if the buffer start/end is not cache-line aligned.
	 *
	 *	PREWRITE -- Write-back the D-cache.  If we have to use
	 *	an Index op, we also have to invalidate.  Note that if
	 *	we are doing PREREAD|PREWRITE, we can collapse everything
	 *	into a single op.
	 *
	 *	POSTREAD -- Nothing.
	 *
	 *	POSTWRITE -- Nothing.
	 */

	/*
	 * Flush the write buffer.
	 * XXX Is this always necessary?
	 */
	wbflush();

	ops &= (BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);
	if (ops == 0)
		return;

	/*
	 * If the mapping is of COHERENT DMA-safe memory, no cache
	 * flush is necessary.
	 */
	if (map->_dm_flags & _BUS_DMAMAP_COHERENT)
		return;

	/*
	 * If the mapping belongs to the kernel, or if it belongs
	 * to the currently-running process (XXX actually, vmspace),
	 * then we can use Hit ops.  Otherwise, Index ops.
	 *
	 * This should be true the vast majority of the time.
	 */
	if (__predict_true(VMSPACE_IS_KERNEL_P(map->_dm_vmspace) ||
	    map->_dm_vmspace == curproc->p_vmspace))
		useindex = 0;
	else
		useindex = 1;

	for (i = 0; i < map->dm_nsegs && len != 0; i++) {
		/* Find the beginning segment. */
		if (offset >= map->dm_segs[i].ds_len) {
			offset -= map->dm_segs[i].ds_len;
			continue;
		}

		/*
		 * Now at the first segment to sync; nail
		 * each segment until we have exhausted the
		 * length.
		 */
		minlen = len < map->dm_segs[i].ds_len - offset ?
		    len : map->dm_segs[i].ds_len - offset;

		addr = map->dm_segs[i]._ds_vaddr;

#ifdef BUS_DMA_DEBUG
		printf("%s: flushing segment %d (%#jx..%#jx) ...",
		    __func__, i, (intmax_t)addr + offset,
		    (intmax_t)ddr + offset + minlen - 1);
#endif

		/*
		 * If we are forced to use Index ops, it's always a
		 * Write-back,Invalidate, so just do one test.
		 */
		if (__predict_false(useindex)) {
			mips_dcache_wbinv_range_index(addr + offset, minlen);
#ifdef BUS_DMA_DEBUG
			printf("\n");
#endif
			offset = 0;
			len -= minlen;
			continue;
		}

		switch (ops) {
		case BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE:
			mips_dcache_wbinv_range(addr + offset, minlen);
			break;

		case BUS_DMASYNC_PREREAD:
#if 1
			mips_dcache_wbinv_range(addr + offset, minlen);
#else
			mips_dcache_inv_range(addr + offset, minlen);
#endif
			break;

		case BUS_DMASYNC_PREWRITE:
			mips_dcache_wb_range(addr + offset, minlen);
			break;
		}
#ifdef BUS_DMA_DEBUG
		printf("\n");
#endif
		offset = 0;
		len -= minlen;
	}
}
#endif /* MIPS3 */
#endif

/*
 * XXX
 * I have no idea what kind of limits to apply here, so for now allow
 * everything
 */

#define CHIP	   		normal
#define	CHIP_MEM		/* defined */
#define	CHIP_W1_BUS_START(v)	0x00000000UL
#define CHIP_W1_BUS_END(v)	0xffffffffUL
#define	CHIP_W1_SYS_START(v)	0x00000000UL
#define	CHIP_W1_SYS_END(v)	0xffffffffUL

#include <mips/mips/bus_space_alignstride_chipdep.c>
