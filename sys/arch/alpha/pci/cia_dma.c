/* $NetBSD: cia_dma.c,v 1.37 2021/07/19 01:06:14 thorpej Exp $ */

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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: cia_dma.c,v 1.37 2021/07/19 01:06:14 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>

#define _ALPHA_BUS_DMA_PRIVATE
#include <sys/bus.h>

#include <uvm/uvm_extern.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <alpha/pci/ciareg.h>
#include <alpha/pci/ciavar.h>

static bus_dma_tag_t cia_dma_get_tag(bus_dma_tag_t, alpha_bus_t);

static int	cia_bus_dmamap_create_direct(bus_dma_tag_t, bus_size_t, int,
		    bus_size_t, bus_size_t, int, bus_dmamap_t *);

static int	cia_bus_dmamap_load_sgmap(bus_dma_tag_t, bus_dmamap_t, void *,
		    bus_size_t, struct proc *, int);

static int	cia_bus_dmamap_load_mbuf_sgmap(bus_dma_tag_t, bus_dmamap_t,
		    struct mbuf *, int);

static int	cia_bus_dmamap_load_uio_sgmap(bus_dma_tag_t, bus_dmamap_t,
		    struct uio *, int);

static int	cia_bus_dmamap_load_raw_sgmap(bus_dma_tag_t, bus_dmamap_t,
		    bus_dma_segment_t *, int, bus_size_t, int);

static void	cia_bus_dmamap_unload_sgmap(bus_dma_tag_t, bus_dmamap_t);

/*
 * Direct-mapped window: 1G at 1G
 */
#define	CIA_DIRECT_MAPPED_BASE	(1UL*1024*1024*1024)
#define	CIA_DIRECT_MAPPED_SIZE	(1UL*1024*1024*1024)

/*
 * SGMAP window for ISA: 8M at 8M
 */
#define	CIA_SGMAP_MAPPED_LO_BASE (8UL*1024*1024)
#define	CIA_SGMAP_MAPPED_LO_SIZE (8UL*1024*1024)

/*
 * SGMAP window for PCI: 1G at 3G
 */
#define	CIA_SGMAP_MAPPED_HI_BASE (3UL*1024*1024*1024)
#define	CIA_SGMAP_MAPPED_HI_SIZE (1UL*1024*1024*1024)

/* ALCOR/ALGOR2/PYXIS have a 256-byte out-bound DMA prefetch threshold. */
#define	CIA_SGMAP_PFTHRESH	256

static void	cia_tlb_invalidate(void);
static void	cia_broken_pyxis_tlb_invalidate(void);

static void	(*cia_tlb_invalidate_fn)(void);

#define	CIA_TLB_INVALIDATE()	(*cia_tlb_invalidate_fn)()

struct alpha_sgmap cia_pyxis_bug_sgmap;
#define	CIA_PYXIS_BUG_BASE	(128UL*1024*1024)
#define	CIA_PYXIS_BUG_SIZE	(2UL*1024*1024)

static void
cia_dma_shutdown(void *arg)
{
	struct cia_config *ccp = arg;
	int i;

	/*
	 * Restore the original values, to make the firmware happy.
	 */
	for (i = 0; i < 4; i++) {
		REGVAL(CIA_PCI_W0BASE + (i * 0x100)) =
		    ccp->cc_saved_windows.wbase[i];
		alpha_mb();
		REGVAL(CIA_PCI_W0MASK + (i * 0x100)) =
		    ccp->cc_saved_windows.wmask[i];
		alpha_mb();
		REGVAL(CIA_PCI_T0BASE + (i * 0x100)) =
		    ccp->cc_saved_windows.tbase[i];
		alpha_mb();
	}
}

void
cia_dma_init(struct cia_config *ccp)
{
	bus_addr_t tbase;
	bus_dma_tag_t t;
	bus_dma_tag_t t_sg_hi = NULL;
	int i;

	/*
	 * Save our configuration to restore at shutdown, just
	 * in case the firmware would get cranky with us.
	 */
	for (i = 0; i < 4; i++) {
		ccp->cc_saved_windows.wbase[i] =
		    REGVAL(CIA_PCI_W0BASE + (i * 0x100));
		ccp->cc_saved_windows.wmask[i] =
		    REGVAL(CIA_PCI_W0MASK + (i * 0x100));
		ccp->cc_saved_windows.tbase[i] =
		    REGVAL(CIA_PCI_T0BASE + (i * 0x100));
	}
	shutdownhook_establish(cia_dma_shutdown, ccp);

	/*
	 * If we have more than 1GB of RAM, then set up an sgmap-mapped
	 * DMA window for PCI.  This is better than using the ISA window,
	 * which is pretty small and PCI devices could starve it.
	 *
	 * N.B. avail_end is "last-usable PFN + 1".
	 */
	if (uvm_physseg_get_avail_end(uvm_physseg_get_last()) >
	    atop(CIA_DIRECT_MAPPED_SIZE)) {
		t = t_sg_hi = &ccp->cc_dmat_sgmap_hi;
		t->_cookie = ccp;
		t->_wbase = CIA_SGMAP_MAPPED_HI_BASE;
		t->_wsize = CIA_SGMAP_MAPPED_HI_SIZE;
		t->_next_window = NULL;
		t->_boundary = 0;
		t->_sgmap = &ccp->cc_sgmap_hi;
		t->_pfthresh = CIA_SGMAP_PFTHRESH;
		t->_get_tag = cia_dma_get_tag;
		t->_dmamap_create = alpha_sgmap_dmamap_create;
		t->_dmamap_destroy = alpha_sgmap_dmamap_destroy;
		t->_dmamap_load = cia_bus_dmamap_load_sgmap;
		t->_dmamap_load_mbuf = cia_bus_dmamap_load_mbuf_sgmap;
		t->_dmamap_load_uio = cia_bus_dmamap_load_uio_sgmap;
		t->_dmamap_load_raw = cia_bus_dmamap_load_raw_sgmap;
		t->_dmamap_unload = cia_bus_dmamap_unload_sgmap;
		t->_dmamap_sync = _bus_dmamap_sync;

		t->_dmamem_alloc = _bus_dmamem_alloc;
		t->_dmamem_free = _bus_dmamem_free;
		t->_dmamem_map = _bus_dmamem_map;
		t->_dmamem_unmap = _bus_dmamem_unmap;
		t->_dmamem_mmap = _bus_dmamem_mmap;
	}

	/*
	 * Initialize the DMA tag used for direct-mapped DMA.
	 */
	t = &ccp->cc_dmat_direct;
	t->_cookie = ccp;
	t->_wbase = CIA_DIRECT_MAPPED_BASE;
	t->_wsize = CIA_DIRECT_MAPPED_SIZE;
	t->_next_window = t_sg_hi;
	t->_boundary = 0;
	t->_sgmap = NULL;
	t->_get_tag = cia_dma_get_tag;
	t->_dmamap_create = cia_bus_dmamap_create_direct;
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
	 * Initialize the DMA tag used for sgmap-mapped ISA DMA.
	 */
	t = &ccp->cc_dmat_sgmap_lo;
	t->_cookie = ccp;
	t->_wbase = CIA_SGMAP_MAPPED_LO_BASE;
	t->_wsize = CIA_SGMAP_MAPPED_LO_SIZE;
	t->_next_window = NULL;
	t->_boundary = 0;
	t->_sgmap = &ccp->cc_sgmap_lo;
	t->_pfthresh = CIA_SGMAP_PFTHRESH;
	t->_get_tag = cia_dma_get_tag;
	t->_dmamap_create = alpha_sgmap_dmamap_create;
	t->_dmamap_destroy = alpha_sgmap_dmamap_destroy;
	t->_dmamap_load = cia_bus_dmamap_load_sgmap;
	t->_dmamap_load_mbuf = cia_bus_dmamap_load_mbuf_sgmap;
	t->_dmamap_load_uio = cia_bus_dmamap_load_uio_sgmap;
	t->_dmamap_load_raw = cia_bus_dmamap_load_raw_sgmap;
	t->_dmamap_unload = cia_bus_dmamap_unload_sgmap;
	t->_dmamap_sync = _bus_dmamap_sync;

	t->_dmamem_alloc = _bus_dmamem_alloc;
	t->_dmamem_free = _bus_dmamem_free;
	t->_dmamem_map = _bus_dmamem_map;
	t->_dmamem_unmap = _bus_dmamem_unmap;
	t->_dmamem_mmap = _bus_dmamem_mmap;

	/*
	 * The firmware will have set up window 1 as a 1G dirct-mapped
	 * DMA window beginning at 1G.  While it's pretty safe to assume
	 * this is the case, we'll go ahead and program the registers
	 * as we expect as a belt-and-suspenders measure.
	 */
	REGVAL(CIA_PCI_W1BASE) = CIA_DIRECT_MAPPED_BASE | CIA_PCI_WnBASE_W_EN;
	alpha_mb();
	REGVAL(CIA_PCI_W1MASK) = CIA_PCI_WnMASK_1G;
	alpha_mb();
	REGVAL(CIA_PCI_T1BASE) = 0;
	alpha_mb();

	/*
	 * Initialize the SGMAP(s).  Must align page table to at least 32k
	 * (hardware bug?).
	 */
	alpha_sgmap_init(t, &ccp->cc_sgmap_lo, "cia_sgmap_lo",
	    CIA_SGMAP_MAPPED_LO_BASE, 0, CIA_SGMAP_MAPPED_LO_SIZE,
	    sizeof(uint64_t), NULL, (32*1024));
	if (t_sg_hi != NULL) {
		alpha_sgmap_init(t_sg_hi, &ccp->cc_sgmap_hi, "cia_sgmap_hi",
		    CIA_SGMAP_MAPPED_HI_BASE, 0, CIA_SGMAP_MAPPED_HI_SIZE,
		    sizeof(uint64_t), NULL, (32*1024));
	}

	/*
	 * Set up window 0 as an 8MB SGMAP-mapped window
	 * starting at 8MB.
	 */
	REGVAL(CIA_PCI_W0BASE) = CIA_SGMAP_MAPPED_LO_BASE |
	    CIA_PCI_WnBASE_SG_EN | CIA_PCI_WnBASE_W_EN;
	alpha_mb();

	REGVAL(CIA_PCI_W0MASK) = CIA_PCI_WnMASK_8M;
	alpha_mb();

	tbase = ccp->cc_sgmap_lo.aps_ptpa >> CIA_PCI_TnBASE_SHIFT;
	if ((tbase & CIA_PCI_TnBASE_MASK) != tbase)
		panic("cia_dma_init: bad page table address");
	REGVAL(CIA_PCI_T0BASE) = tbase;
	alpha_mb();

	/*
	 * (Maybe) set up window 3 as a 1G SGMAP-mapped window starting
	 * at 3G.
	 */
	if (t_sg_hi != NULL) {
		REGVAL(CIA_PCI_W3BASE) = CIA_SGMAP_MAPPED_HI_BASE |
		    CIA_PCI_WnBASE_SG_EN | CIA_PCI_WnBASE_W_EN;
		alpha_mb();

		REGVAL(CIA_PCI_W3MASK) = CIA_PCI_WnMASK_1G;
		alpha_mb();

		tbase = ccp->cc_sgmap_hi.aps_ptpa >> CIA_PCI_TnBASE_SHIFT;
		if ((tbase & CIA_PCI_TnBASE_MASK) != tbase)
			panic("cia_dma_init: bad page table address");
		REGVAL(CIA_PCI_T3BASE) = tbase;
		alpha_mb();
	} else {
		REGVAL(CIA_PCI_W3BASE) = 0;
		alpha_mb();
	}

	/*
	 * Pass 1 and 2 (i.e. revision <= 1) of the Pyxis have a
	 * broken scatter/gather TLB; it cannot be invalidated.  To
	 * work around this problem, we configure window 2 as an SG
	 * 2M window at 128M, which we use in DMA loopback mode to
	 * read a spill page.  This works by causing TLB misses,
	 * causing the old entries to be purged to make room for
	 * the new entries coming in for the spill page.
	 */
	if ((ccp->cc_flags & CCF_ISPYXIS) != 0 && ccp->cc_rev <= 1) {
		uint64_t *page_table;

		cia_tlb_invalidate_fn =
		    cia_broken_pyxis_tlb_invalidate;

		alpha_sgmap_init(t, &cia_pyxis_bug_sgmap,
		    "pyxis_bug_sgmap", CIA_PYXIS_BUG_BASE, 0,
		    CIA_PYXIS_BUG_SIZE, sizeof(uint64_t), NULL,
		    (32*1024));

		REGVAL(CIA_PCI_W2BASE) = CIA_PYXIS_BUG_BASE |
		    CIA_PCI_WnBASE_SG_EN | CIA_PCI_WnBASE_W_EN;
		alpha_mb();

		REGVAL(CIA_PCI_W2MASK) = CIA_PCI_WnMASK_2M;
		alpha_mb();

		tbase = cia_pyxis_bug_sgmap.aps_ptpa >>
		    CIA_PCI_TnBASE_SHIFT;
		if ((tbase & CIA_PCI_TnBASE_MASK) != tbase)
			panic("cia_dma_init: bad page table address");
		REGVAL(CIA_PCI_T2BASE) = tbase;
		alpha_mb();

		/*
		 * Initialize the page table to point at the spill
		 * page.  Leave the last entry invalid.
		 */
		pci_sgmap_pte64_init_spill_page_pte();
		for (i = 0, page_table = cia_pyxis_bug_sgmap.aps_pt;
		     i < (CIA_PYXIS_BUG_SIZE / PAGE_SIZE) - 1; i++) {
			page_table[i] =
			    pci_sgmap_pte64_prefetch_spill_page_pte;
		}
		alpha_mb();
	} else {
		REGVAL(CIA_PCI_W2BASE) = 0;
		alpha_mb();

		cia_tlb_invalidate_fn = cia_tlb_invalidate;
	}

	CIA_TLB_INVALIDATE();
}

/*
 * Return the bus dma tag to be used for the specified bus type.
 * INTERNAL USE ONLY!
 */
static bus_dma_tag_t
cia_dma_get_tag(bus_dma_tag_t t, alpha_bus_t bustype)
{
	struct cia_config *ccp = t->_cookie;

	switch (bustype) {
	case ALPHA_BUS_PCI:
	case ALPHA_BUS_EISA:
		/*
		 * Regardless if how much memory is installed,
		 * start with the direct-mapped window.  It will
		 * fall back to the SGMAP window if we encounter a
		 * page that is out of range.
		 */
		return (&ccp->cc_dmat_direct);

	case ALPHA_BUS_ISA:
		/*
		 * ISA doesn't have enough address bits to use
		 * the direct-mapped DMA window, so we must use
		 * SGMAPs.
		 */
		return (&ccp->cc_dmat_sgmap_lo);

	default:
		panic("cia_dma_get_tag: shouldn't be here, really...");
	}
}

/*
 * Create a CIA direct-mapped DMA map.
 */
static int
cia_bus_dmamap_create_direct(
	bus_dma_tag_t t,
	bus_size_t size,
	int nsegments,
	bus_size_t maxsegsz,
	bus_size_t boundary,
	int flags,
	bus_dmamap_t *dmamp)
{
	struct cia_config *ccp = t->_cookie;
	bus_dmamap_t map;
	int error;

	error = _bus_dmamap_create(t, size, nsegments, maxsegsz,
	    boundary, flags, dmamp);
	if (error)
		return (error);

	map = *dmamp;

	if ((ccp->cc_flags & CCF_PYXISBUG) != 0 &&
	    map->_dm_segcnt > 1) {
		/*
		 * We have a Pyxis with the DMA page crossing bug, make
		 * sure we don't coalesce adjacent DMA segments.
		 *
		 * NOTE: We can only do this if the max segment count
		 * is greater than 1.  This is because many network
		 * drivers allocate large contiguous blocks of memory
		 * for control data structures, even though they won't
		 * do any single DMA that crosses a page coundary.
		 *	-- thorpej@NetBSD.org, 2/5/2000
		 */
		map->_dm_flags |= DMAMAP_NO_COALESCE;
	}

	return (0);
}

/*
 * Load a CIA SGMAP-mapped DMA map with a linear buffer.
 */
static int
cia_bus_dmamap_load_sgmap(bus_dma_tag_t t, bus_dmamap_t map, void *buf,
    bus_size_t buflen, struct proc *p, int flags)
{
	int error;

	error = pci_sgmap_pte64_load(t, map, buf, buflen, p, flags,
	    t->_sgmap);
	if (error == 0)
		CIA_TLB_INVALIDATE();

	return (error);
}

/*
 * Load a CIA SGMAP-mapped DMA map with an mbuf chain.
 */
static int
cia_bus_dmamap_load_mbuf_sgmap(bus_dma_tag_t t, bus_dmamap_t map,
    struct mbuf *m, int flags)
{
	int error;

	error = pci_sgmap_pte64_load_mbuf(t, map, m, flags, t->_sgmap);
	if (error == 0)
		CIA_TLB_INVALIDATE();

	return (error);
}

/*
 * Load a CIA SGMAP-mapped DMA map with a uio.
 */
static int
cia_bus_dmamap_load_uio_sgmap(bus_dma_tag_t t, bus_dmamap_t map,
    struct uio *uio, int flags)
{
	int error;

	error = pci_sgmap_pte64_load_uio(t, map, uio, flags, t->_sgmap);
	if (error == 0)
		CIA_TLB_INVALIDATE();

	return (error);
}

/*
 * Load a CIA SGMAP-mapped DMA map with raw memory.
 */
static int
cia_bus_dmamap_load_raw_sgmap(bus_dma_tag_t t, bus_dmamap_t map,
    bus_dma_segment_t *segs, int nsegs, bus_size_t size, int flags)
{
	int error;

	error = pci_sgmap_pte64_load_raw(t, map, segs, nsegs, size, flags,
	    t->_sgmap);
	if (error == 0)
		CIA_TLB_INVALIDATE();

	return (error);
}

/*
 * Unload a CIA DMA map.
 */
static void
cia_bus_dmamap_unload_sgmap(bus_dma_tag_t t, bus_dmamap_t map)
{

	/*
	 * Invalidate any SGMAP page table entries used by this
	 * mapping.
	 */
	pci_sgmap_pte64_unload(t, map, t->_sgmap);
	CIA_TLB_INVALIDATE();

	/*
	 * Do the generic bits of the unload.
	 */
	_bus_dmamap_unload_common(t, map);
}

/*
 * Flush the CIA scatter/gather TLB.
 */
static void
cia_tlb_invalidate(void)
{

	alpha_mb();
	REGVAL(CIA_PCI_TBIA) = CIA_PCI_TBIA_ALL;
	alpha_mb();
}

/*
 * Flush the scatter/gather TLB on broken Pyxis chips.
 */
static void
cia_broken_pyxis_tlb_invalidate(void)
{
	uint32_t ctrl;
	int i, s;

	s = splhigh();

	/*
	 * Put the Pyxis into PCI loopback mode.
	 */
	alpha_mb();
	ctrl = REGVAL(CIA_CSR_CTRL);
	REGVAL(CIA_CSR_CTRL) = ctrl | CTRL_PCI_LOOP_EN;
	alpha_mb();

	/*
	 * Now, read from PCI dense memory space at offset 128M (our
	 * target window base), skipping 64k on each read.  This forces
	 * S/G TLB misses.
	 *
	 * XXX Looks like the TLB entries are `not quite LRU'.  We need
	 * XXX to read more times than there are actual tags!
	 */
	for (i = 0; i < CIA_TLB_NTAGS + 4; i++) {
		volatile uint64_t dummy;
		dummy = *((volatile uint64_t *)
		    ALPHA_PHYS_TO_K0SEG(CIA_PCI_DENSE + CIA_PYXIS_BUG_BASE +
		    (i * 65536)));
		__USE(dummy);
	}

	/*
	 * Restore normal PCI operation.
	 */
	alpha_mb();
	REGVAL(CIA_CSR_CTRL) = ctrl;
	alpha_mb();

	splx(s);
}
