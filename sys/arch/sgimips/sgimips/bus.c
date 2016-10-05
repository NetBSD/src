/*	$NetBSD: bus.c,v 1.65.14.2 2016/10/05 20:55:34 skrll Exp $	*/

/*
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: bus.c,v 1.65.14.2 2016/10/05 20:55:34 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/endian.h>
#include <sys/bswap.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/mbuf.h>

#define _MIPS_BUS_DMA_PRIVATE

#include <sys/bus.h>
#include <machine/cpu.h>
#include <machine/machtype.h>

#include <uvm/uvm_extern.h>

#include <mips/cpuregs.h>
#include <mips/locore.h>
#include <mips/cache.h>

#include <sgimips/mace/macereg.h>

#include "opt_sgimace.h"

struct mips_bus_dma_tag sgimips_default_bus_dma_tag = {
	._dmamap_ops = _BUS_DMAMAP_OPS_INITIALIZER,
	._dmamem_ops = _BUS_DMAMEM_OPS_INITIALIZER,
	._dmatag_ops = _BUS_DMATAG_OPS_INITIALIZER,
};

static void normal_bus_mem_init(bus_space_tag_t, void *);

static struct mips_bus_space	normal_mbst;
bus_space_tag_t	normal_memt = NULL;

/*
 * XXX
 * I'm not sure how well the common MIPS bus_dma.c handles MIPS-I and I don't
 * have any IP1x hardware, so I'll leave this in just in case it needs to be
 * put back
 */

void
sgimips_bus_dma_init(void)
{
	printf("%s\n", __func__);
	normal_bus_mem_init(&normal_mbst, NULL);
	normal_memt = &normal_mbst;

#if 0
	switch (mach_type) {
	/* R2000/R3000 */
	case MACH_SGI_IP6 | MACH_SGI_IP10:
	case MACH_SGI_IP12:
		sgimips_default_bus_dma_tag._dmamap_ops.dmamap_sync =
		    _bus_dmamap_sync_mips1;
		break;

	default:
		panic("sgimips_bus_dma_init: unsupported mach type IP%d\n",
		    mach_type);
	}
#endif
}

/*
 * XXX
 * left in for illustrative purposes
 */

#if 0
u_int8_t
bus_space_read_1(bus_space_tag_t t, bus_space_handle_t h, bus_size_t o)
{
	wbflush(); /* XXX ? */

	switch (t) {
	case SGIMIPS_BUS_SPACE_NORMAL:
		return *(volatile u_int8_t *)(vaddr_t)(h + o);
	case SGIMIPS_BUS_SPACE_IP6_DPCLOCK:
		return *(volatile u_int8_t *)(vaddr_t)(h + (o << 2));
	case SGIMIPS_BUS_SPACE_HPC:
		return *(volatile u_int8_t *)(vaddr_t)(h + (o << 2) + 3);
	case SGIMIPS_BUS_SPACE_MEM:
	case SGIMIPS_BUS_SPACE_IO:
		return *(volatile u_int8_t *)(vaddr_t)(h + (o | 3) - (o & 3));
	case SGIMIPS_BUS_SPACE_MACE:
		return *(volatile u_int8_t *)(vaddr_t)(h + (o << 8) + 7);
	default:
		panic("no bus tag");
	}
}
#endif

/* Common function from DMA map synchronization. May be called
 * by chipset-specific DMA map synchronization functions.
 *
 * This is the R3000 version.
 */
void
_bus_dmamap_sync_mips1(bus_dma_tag_t t, bus_dmamap_t map, bus_addr_t offset,
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
		 panic("_bus_dmamap_sync_mips1: mix PRE and POST");

#ifdef DIAGNOSTIC
	if (offset >= map->dm_mapsize)
		panic("_bus_dmamap_sync_mips1: bad offset %"PRIxPSIZE
		" (map size is %"PRIxPSIZE")"
		    , offset, map->dm_mapsize);
	if (len == 0 || (offset + len) > map->dm_mapsize)
		panic("_bus_dmamap_sync_mips1: bad length");
#endif

	/*
	 * The R3000 cache is write-through. Therefore, we only need
	 * to drain the write buffer on PREWRITE. The cache is not
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
	 * No cache invalidation is necessary if the DMA map covers
	 * COHERENT DMA-safe memory (which is mapped un-cached).
	 */
	if (map->_dm_flags & SGIMIPS_DMAMAP_COHERENT)
		return;

	/*
	 * If we are going to hit something as large or larger
	 * than the entire data cache, just nail the whole thing.
	 *
	 * NOTE: Even though this is `wbinv_all', since the cache is
	 * write-through, it just invalidates it.
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
		printf("bus_dmamap_sync_mips1: flushing segment %d "
		    "(0x%"PRIxBUSADDR"..0x%"PRIxBUSADDR") ...", i,
		    addr + offset, addr + offset + minlen - 1);
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

#if 0
paddr_t
bus_space_mmap(bus_space_tag_t space, bus_addr_t addr, off_t off,
         int prot, int flags)
{

	if (flags & BUS_SPACE_MAP_CACHEABLE) {

		return mips_btop(MIPS_KSEG0_TO_PHYS(addr) + off);
	} else
#if defined(_MIPS_PADDR_T_64BIT) || defined(_LP64)
		return mips_btop((MIPS_KSEG1_TO_PHYS(addr) + off)
		    | PGC_NOCACHE);
#else
		return mips_btop((MIPS_KSEG1_TO_PHYS(addr) + off));
#endif
}
#endif

#define CHIP	   		normal
#define	CHIP_MEM		/* defined */
#define	CHIP_W1_BUS_START(v)	0x00000000UL
#define CHIP_W1_BUS_END(v)	0xffffffffUL
#define	CHIP_W1_SYS_START(v)	0x00000000UL
#define	CHIP_W1_SYS_END(v)	0xffffffffUL

#include <mips/mips/bus_space_alignstride_chipdep.c>
