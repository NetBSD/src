/* $NetBSD: mcadma_machdep.c,v 1.1.8.2 2008/01/21 09:38:56 yamt Exp $ */

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tim Rightnour
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: mcadma_machdep.c,v 1.1.8.2 2008/01/21 09:38:56 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/mbuf.h>

#define _POWERPC_BUS_DMA_PRIVATE
#include <machine/bus.h>

#include <machine/pio.h>

#include <dev/mca/mcavar.h>
#include <dev/mca/mcareg.h>

struct rs6000_dma_cookie {
	int	id_flags;
	uint32_t	bid;		/* the BID of this IOCC */
	uint32_t	tce_off;	/* offset of the TCE */
	uint32_t	nroftce;
	uint32_t	nrofslaves;	/* nrof slave channels */
	uint32_t	slave_regs;	/* allocation bitmask */
	uint32_t	dma_regs;	/* allocation bitmask */
};

/*
 * Entry points for MCA DMA.  These are mostly wrappers around
 * the generic functions that understand how to deal with bounce
 * buffers, if necessary.
 */

struct powerpc_bus_dma_tag mca_bus_dma_tag = {
	0,			/* _bounce_thresh */
	_mca_bus_dmamap_create,
	_bus_dmamap_destroy,
	_bus_dmamap_load,
	_bus_dmamap_load_mbuf,
	_bus_dmamap_load_uio,
	_bus_dmamap_load_raw,
	_bus_dmamap_unload,
	NULL,			/* _dmamap_sync */
	_bus_dmamem_alloc,
	_bus_dmamem_free,
	_bus_dmamem_map,
	_bus_dmamem_unmap,
	_bus_dmamem_mmap,
};

/*      
 * Allocate a DMA map, and set up DMA channel.
 */     
static int
_mca_bus_dmamap_create(bus_dma_tag_t t, bus_size_t size, int flags,
    bus_dmamap_t *dmamp, int dmach)
{
	int error;
	struct rs6000_dma_cookie *cookie;

#ifdef DEBUG
	/* Sanity check */
	if (dmach < 0 || dmach >= MAX_DMA_CHANNELS) {
		printf("mcadma_create: invalid DMA channel %d\n", dmach);
		return EINVAL;
	}

	if (size > 65536) {
		panic("mca_dmamap_create: dmamap sz %ld > 65536", (long) size);
	}
#endif
        
	/*
	 * MCA DMA transfer can be maximum 65536 bytes long and must
 	 * be in one chunk. No specific boundary constraints are present.
 	 */
	if ((error = _bus_dmamap_create(t, size, 1, 65536, 0, flags, dmamp)))
		return error;

	cookie = (struct rs6000_dma_cookie *) (*dmamp)->_dm_cookie;

	if (cookie == NULL) {
		/*
 		 * Allocate our cookie if not yet done.
		 */
		cookie = malloc(sizeof(struct rs6000_dma_cookie), M_DMAMAP,
		    ((flags & BUS_DMA_NOWAIT) ? M_NOWAIT : M_WAITOK) | M_ZERO);
		if (cookie == NULL) {

			return ENOMEM;
		}
		(*dmamp)->_dm_cookie = cookie;
	}

	/* Encode DMA channel */
	cookie->id_flags &= 0x0f;
	cookie->id_flags |= dmach << 4;

	/* Mark the dmamap as using DMA controller. Some devices
	 * drive DMA themselves, and don't need the MCA DMA controller.
	 * To distinguish the two, use a flag for dmamaps which use the DMA
	 * controller.
	 */     
	(*dmamp)->_dm_flags |= _MCABUS_DMA_USEDMACTRL; 
	return 0;
}

static void
_mca_bus_dmamap_destroy(bus_dma_tag_t t, bus_dmamap_t map)
{
	struct rs6000_dma_cookie *cookie = map->_dm_cookie;

	free(cookie, M_DMAMAP);
	_bus_dmamap_destroy(t, map);
}

/*
 * Load an MCA DMA map with a linear buffer.
 */
static int
_mca_bus_dmamap_load(bus_dma_tag_t t, bus_dmamap_t map, void *buf,
    bus_size_t buflen, struct proc *p, int flags)
{
	struct rs6000_dma_cookie *cookie = map->_dm_cookie;
	int error;

	/* Make sure that on error condition we return "no valid mappings." */
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;

	error = _bus_dmamap_load(t, map, buf, buflen, p, flags);
	return error;
}

/*
 * Like _mca_bus_dmamap_load(), but for mbufs.
 */
static int
_mca_bus_dmamap_load_mbuf(bus_dma_tag_t t, bus_dmamap_t map, struct mbuf *m0,
    int flags)
{
	struct rs6000_dma_cookie *cookie = map->_dm_cookie;
	int error;

	/* Make sure that on error condition we return "no valid mappings." */
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;

#ifdef DIAGNOSTIC
	if ((m0->m_flags & M_PKTHDR) == 0)
		panic("_mca_bus_dmamap_load_mbuf: no packet header");
#endif

	if (m0->m_pkthdr.len > map->_dm_size)
		return EINVAL;

	error = _bus_dmamap_load_mbuf(t, map, m0, flags);
	return error;
}

/*
 * Like _mca_bus_dmamap_load(), but for uios.
 */
static int
_mca_bus_dmamap_load_uio(bus_dma_tag_t t, bus_dmamap_t map, struct uio *uio,
    int flags)
{

	panic("_mca_bus_dmamap_load_uio: not implemented");
}

/*
 * Like _mca_bus_dmamap_load(), but for raw memory allocated with
 * bus_dmamem_alloc().
 */
static int
_mca_bus_dmamap_load_raw(bus_dma_tag_t t, bus_dmamap_t map,
    bus_dma_segment_t *segs, int nsegs, bus_size_t size, int flags)
{

	panic("_mca_bus_dmamap_load_raw: not implemented");
}

/*
 * Unload an MCA DMA map.
 */
static void
_mca_bus_dmamap_unload(bus_dma_tag_t t, bus_dmamap_t map)
{

	_bus_dmamap_unload(t, map);
}

/*
 * Allocate memory safe for MCA DMA.
 */
static int
_mca_bus_dmamem_alloc(bus_dma_tag_t t, bus_size_t size, bus_size_t alignment,
    bus_size_t boundary, bus_dma_segment_t *segs, int nsegs, int *rsegs,
    int flags)
{
	paddr_t high;

	high = trunc_page(avail_end);

	return _bus_dmamem_alloc_range(t, size, alignment, boundary,
	    segs, nsegs, rsegs, flags, 0, high);
}
