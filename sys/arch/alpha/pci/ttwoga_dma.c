/* $NetBSD: ttwoga_dma.c,v 1.1 2000/12/21 20:51:55 thorpej Exp $ */

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: ttwoga_dma.c,v 1.1 2000/12/21 20:51:55 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h> 

#include <uvm/uvm_extern.h>

#define	_ALPHA_BUS_DMA_PRIVATE
#include <machine/bus.h>

#include <dev/pci/pcireg.h> 
#include <dev/pci/pcivar.h>

#include <alpha/pci/ttwogareg.h>
#include <alpha/pci/ttwogavar.h>

bus_dma_tag_t ttwoga_dma_get_tag(bus_dma_tag_t, alpha_bus_t);

int	ttwoga_bus_dmamap_create_sgmap(bus_dma_tag_t, bus_size_t, int,
	    bus_size_t, bus_size_t, int, bus_dmamap_t *);

void	ttwoga_bus_dmamap_destroy_sgmap(bus_dma_tag_t, bus_dmamap_t);

int	ttwoga_bus_dmamap_load_sgmap(bus_dma_tag_t, bus_dmamap_t, void *,
	    bus_size_t, struct proc *, int);

int	ttwoga_bus_dmamap_load_mbuf_sgmap(bus_dma_tag_t, bus_dmamap_t,
	    struct mbuf *, int);

int	ttwoga_bus_dmamap_load_uio_sgmap(bus_dma_tag_t, bus_dmamap_t,
	    struct uio *, int);

int	ttwoga_bus_dmamap_load_raw_sgmap(bus_dma_tag_t, bus_dmamap_t,
	    bus_dma_segment_t *, int, bus_size_t, int);

void	ttwoga_bus_dmamap_unload_sgmap(bus_dma_tag_t, bus_dmamap_t);

/*
 * Direct-mapped window: 1G at 1G
 */
#define	TTWOGA_DIRECT_MAPPED_BASE	(1UL*1024UL*1024UL*1024UL)
#define	TTWOGA_DIRECT_MAPPED_SIZE	(1UL*1024UL*1024UL*1024UL)

/*
 * SGMAP window: 8M at 8M
 */
#define	TTWOGA_SGMAP_MAPPED_BASE	(8UL*1024UL*1024UL)
#define	TTWOGA_SGMAP_MAPPED_SIZE	(8UL*1024UL*1024UL)

/*
 * Macro to flush the T2 Gate Array scatter/gather TLB.
 */
#define	TTWOGA_TLB_INVALIDATE(tcp)					\
do {									\
	u_int64_t temp;							\
									\
	alpha_mb();							\
	temp = T2GA((tcp), T2_IOCSR);					\
	T2GA((tcp), T2_IOCSR) = temp | IOCSR_FTLB;			\
	alpha_mb();							\
	alpha_mb();	/* MAGIC */					\
	T2GA((tcp), T2_IOCSR) = temp;					\
	alpha_mb();							\
	alpha_mb();	/* MAGIC */					\
} while (0)

void
ttwoga_dma_init(struct ttwoga_config *tcp)
{
	bus_dma_tag_t t;

	/*
	 * Initialize the DMA tag used for direct-mapped DMA.
	 */
	t = &tcp->tc_dmat_direct;
	t->_cookie = tcp;
	t->_wbase = TTWOGA_DIRECT_MAPPED_BASE;
	t->_wsize = TTWOGA_DIRECT_MAPPED_SIZE;
	t->_next_window = NULL;
	t->_boundary = 0;
	t->_sgmap = NULL;
	t->_get_tag = ttwoga_dma_get_tag;
	t->_dmamap_create = _bus_dmamap_create;
	t->_dmamap_destroy = _bus_dmamap_destroy;
	t->_dmamap_load = _bus_dmamap_load_direct;
	t->_dmamap_load_mbuf = _bus_dmamap_load_mbuf_direct;
	t->_dmamap_load_uio = _bus_dmamap_load_uio_direct;
	t->_dmamap_load_raw = _bus_dmamap_load_raw_direct;
	t->_dmamap_unload = _bus_dmamap_unload;
	t->_dmamap_sync = _bus_dmamap_sync;

	t->_dmamem_alloc = _bus_dmamem_alloc;
	t->_dmamem_free = _bus_dmamem_free;
	t->_dmamem_map = _bus_dmamem_map;
	t->_dmamem_unmap = _bus_dmamem_unmap;
	t->_dmamem_mmap = _bus_dmamem_mmap;

	/*
	 * Initialize the DMA tag used for sgmap-mapped DMA.
	 */
	t = &tcp->tc_dmat_sgmap;
	t->_cookie = tcp;
	t->_wbase = TTWOGA_SGMAP_MAPPED_BASE;
	t->_wsize = TTWOGA_SGMAP_MAPPED_SIZE;
	t->_next_window = NULL;
	t->_boundary = 0;
	t->_sgmap = &tcp->tc_sgmap;
	t->_get_tag = ttwoga_dma_get_tag;
	t->_dmamap_create = ttwoga_bus_dmamap_create_sgmap;
	t->_dmamap_destroy = ttwoga_bus_dmamap_destroy_sgmap;
	t->_dmamap_load = ttwoga_bus_dmamap_load_sgmap;
	t->_dmamap_load_mbuf = ttwoga_bus_dmamap_load_mbuf_sgmap;
	t->_dmamap_load_uio = ttwoga_bus_dmamap_load_uio_sgmap;
	t->_dmamap_load_raw = ttwoga_bus_dmamap_load_raw_sgmap;
	t->_dmamap_unload = ttwoga_bus_dmamap_unload_sgmap;
	t->_dmamap_sync = _bus_dmamap_sync;

	t->_dmamem_alloc = _bus_dmamem_alloc;
	t->_dmamem_free = _bus_dmamem_free;
	t->_dmamem_map = _bus_dmamem_map;
	t->_dmamem_unmap = _bus_dmamem_unmap;
	t->_dmamem_mmap = _bus_dmamem_mmap;

	/*
	 * Disable the SGMAP TLB, and flush it.  We reenable it if
	 * we have a Sable or a Gamma with T3 or T4; Gamma with T2
	 * has a TLB bug apparently severe enough to require disabling
	 * it.
	 */
	alpha_mb();
	T2GA(tcp, T2_IOCSR) &= ~IOCSR_ETLB;
	alpha_mb();
	alpha_mb();	/* MAGIC */

	TTWOGA_TLB_INVALIDATE(tcp);

	/*
	 * XXX We might want to make sure our DMA windows don't
	 * XXX overlap with PCI memory space!
	 */

	/*
	 * Set up window 1 as a 1G direct-mapped window
	 * starting at 1G.
	 */
	T2GA(tcp, T2_WBASE1) = 0;
	alpha_mb();

	T2GA(tcp, T2_WMASK1) = (TTWOGA_DIRECT_MAPPED_SIZE - 1) & WMASKx_PWM;
	alpha_mb();

	T2GA(tcp, T2_TBASE1) = 0;
	alpha_mb();

	T2GA(tcp, T2_WBASE1) = TTWOGA_DIRECT_MAPPED_BASE |
	    ((TTWOGA_DIRECT_MAPPED_BASE + (TTWOGA_DIRECT_MAPPED_SIZE - 1)) >>
	     WBASEx_PWxA_SHIFT) | WBASEx_PWE;
	alpha_mb();

	/*
	 * Initialize the SGMAP.
	 */
	alpha_sgmap_init(t, &tcp->tc_sgmap, "ttwoga_sgmap",
	    TTWOGA_SGMAP_MAPPED_BASE, 0, TTWOGA_SGMAP_MAPPED_SIZE,
	    sizeof(u_int64_t), NULL, 0);

	/*
	 * Set up window 2 as an 8MB SGMAP-mapped window
	 * starting at 8MB.
	 */
#ifdef DIAGNOSTIC
	if ((TTWOGA_SGMAP_MAPPED_BASE & WBASEx_PWSA) !=
	    TTWOGA_SGMAP_MAPPED_BASE)
		panic("ttwoga_dma_init: SGMAP base inconsistency");
#endif
	T2GA(tcp, T2_WBASE2) = 0;
	alpha_mb();

	T2GA(tcp, T2_WMASK2) = (TTWOGA_SGMAP_MAPPED_SIZE - 1) & WMASKx_PWM;
	alpha_mb();

	T2GA(tcp, T2_TBASE2) = tcp->tc_sgmap.aps_ptpa >> 1;
	alpha_mb();

	T2GA(tcp, T2_WBASE2) = TTWOGA_SGMAP_MAPPED_BASE |
	    ((TTWOGA_SGMAP_MAPPED_BASE + (TTWOGA_SGMAP_MAPPED_SIZE - 1)) >>
	     WBASEx_PWxA_SHIFT) | WBASEx_SGE | WBASEx_PWE;
	alpha_mb();

	/*
	 * Enable SGMAP TLB on Sable or Gamma with T3 or T4; see above.
	 */
	if (alpha_implver() == ALPHA_IMPLVER_EV4 ||
	    tcp->tc_rev >= TRN_T3) {
		alpha_mb();
		T2GA(tcp, T2_IOCSR) |= IOCSR_ETLB;
		alpha_mb();
		alpha_mb();	/* MAGIC */
		tcp->tc_use_tlb = 1;
	}

	/* XXX XXX BEGIN XXX XXX */
	{							/* XXX */
		extern paddr_t alpha_XXX_dmamap_or;		/* XXX */
		alpha_XXX_dmamap_or = TTWOGA_DIRECT_MAPPED_BASE;/* XXX */
	}							/* XXX */
	/* XXX XXX END XXX XXX */
}

/*
 * Return the bus dma tag to be used for the specified bus type.
 * INTERNAL USE ONLY!
 */
bus_dma_tag_t
ttwoga_dma_get_tag(bus_dma_tag_t t, alpha_bus_t bustype)
{
	struct ttwoga_config *tcp = t->_cookie;

	switch (bustype) {
	case ALPHA_BUS_PCI:
	case ALPHA_BUS_EISA:
		/*
		 * Systems with a T2 Gate Array can have 2G
		 * of memory, but we only get a direct-mapped
		 * window of 1G!
		 *
		 * XXX FIX THIS SOMEDAY!
		 */
		return (&tcp->tc_dmat_direct);

	case ALPHA_BUS_ISA:
		/*
		 * ISA doesn't have enough address bits to use
		 * the direct-mapped DMA window, so we must use
		 * SGMAPs.
		 */
		return (&tcp->tc_dmat_sgmap);

	default:
		panic("ttwoga_dma_get_tag: shouldn't be here, really...");
	}
}

/*
 * Create a T2 SGMAP-mapped DMA map.
 */
int
ttwoga_bus_dmamap_create_sgmap(bus_dma_tag_t t, bus_size_t size, int nsegments,
    bus_size_t maxsegsz, bus_size_t boundary, int flags, bus_dmamap_t *dmamp)
{
	bus_dmamap_t map;
	int error;

	error = _bus_dmamap_create(t, size, nsegments, maxsegsz,
	    boundary, flags, dmamp);
	if (error)
		return (error);

	map = *dmamp;

	if (flags & BUS_DMA_ALLOCNOW) {
		error = alpha_sgmap_alloc(map, round_page(size),
		    t->_sgmap, flags);
		if (error)
			ttwoga_bus_dmamap_destroy_sgmap(t, map);
	}

	return (error);
}

/*
 * Destroy a T2 SGMAP-mapped DMA map.
 */
void
ttwoga_bus_dmamap_destroy_sgmap(bus_dma_tag_t t, bus_dmamap_t map)
{

	if (map->_dm_flags & DMAMAP_HAS_SGMAP)
		alpha_sgmap_free(map, t->_sgmap);

	_bus_dmamap_destroy(t, map);
}

/*
 * Load a T2 SGMAP-mapped DMA map with a liner buffer.
 */
int
ttwoga_bus_dmamap_load_sgmap(bus_dma_tag_t t, bus_dmamap_t map, void *buf,
    bus_size_t buflen, struct proc *p, int flags)
{
	struct ttwoga_config *tcp = t->_cookie;
	int error;

	error = pci_sgmap_pte64_load(t, map, buf, buflen, p, flags,
	    t->_sgmap);
	if (error == 0 && tcp->tc_use_tlb)
		TTWOGA_TLB_INVALIDATE(tcp);

	return (error);
}

/*
 * Load a T2 SGMAP-mapped DMA map with an mbuf chain.
 */
int
ttwoga_bus_dmamap_load_mbuf_sgmap(bus_dma_tag_t t, bus_dmamap_t map,
    struct mbuf *m, int flags)
{
	struct ttwoga_config *tcp = t->_cookie;
	int error;

	error = pci_sgmap_pte64_load_mbuf(t, map, m, flags, t->_sgmap);
	if (error == 0 && tcp->tc_use_tlb)
		TTWOGA_TLB_INVALIDATE(tcp);

	return (error);
}

/*
 * Load a T2 SGMAP-mapped DMA map with a uio.
 */
int
ttwoga_bus_dmamap_load_uio_sgmap(bus_dma_tag_t t, bus_dmamap_t map,
    struct uio *uio, int flags)
{
	struct ttwoga_config *tcp = t->_cookie;
	int error;

	error = pci_sgmap_pte64_load_uio(t, map, uio, flags, t->_sgmap);
	if (error == 0 && tcp->tc_use_tlb)
		TTWOGA_TLB_INVALIDATE(tcp);

	return (error);
}

/*
 * Load a T2 SGMAP-mapped DMA map with raw memory.
 */
int
ttwoga_bus_dmamap_load_raw_sgmap(bus_dma_tag_t t, bus_dmamap_t map,
    bus_dma_segment_t *segs, int nsegs, bus_size_t size, int flags)
{
	struct ttwoga_config *tcp = t->_cookie;
	int error;

	error = pci_sgmap_pte64_load_raw(t, map, segs, nsegs, size, flags,
	    t->_sgmap);
	if (error == 0 && tcp->tc_use_tlb)
		TTWOGA_TLB_INVALIDATE(tcp);

	return (error);
}

/*
 * Unload an T2 DMA map.
 */
void
ttwoga_bus_dmamap_unload_sgmap(bus_dma_tag_t t, bus_dmamap_t map)
{
	struct ttwoga_config *tcp = t->_cookie;

	/*
	 * Invalidate any SGMAP page table entries used by this
	 * mapping.
	 */
	pci_sgmap_pte64_unload(t, map, t->_sgmap);
	if (tcp->tc_use_tlb)
		TTWOGA_TLB_INVALIDATE(tcp);

	/*
	 * Do the generic bits of the unload.
	 */
	_bus_dmamap_unload(t, map);
}
