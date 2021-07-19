/* $NetBSD: tsp_dma.c,v 1.22 2021/07/19 01:06:14 thorpej Exp $ */

/*-
 * Copyright (c) 1997, 1998, 2021 The NetBSD Foundation, Inc.
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

/*-
 * Copyright (c) 1999 by Ross Harvey.  All rights reserved.
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
 *	This product includes software developed by Ross Harvey.
 * 4. The name of Ross Harvey may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ROSS HARVEY ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURP0SE
 * ARE DISCLAIMED.  IN NO EVENT SHALL ROSS HARVEY BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tsp_dma.c,v 1.22 2021/07/19 01:06:14 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <machine/autoconf.h>
#define _ALPHA_BUS_DMA_PRIVATE
#include <sys/bus.h>
#include <machine/rpb.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <alpha/pci/tsreg.h>
#include <alpha/pci/tsvar.h>

#define tsp_dma() { Generate ctags(1) key. }

static bus_dma_tag_t tsp_dma_get_tag(bus_dma_tag_t, alpha_bus_t);

static int	tsp_bus_dmamap_load_sgmap(bus_dma_tag_t, bus_dmamap_t, void *,
		    bus_size_t, struct proc *, int);

static int	tsp_bus_dmamap_load_mbuf_sgmap(bus_dma_tag_t, bus_dmamap_t,
		    struct mbuf *, int);

static int	tsp_bus_dmamap_load_uio_sgmap(bus_dma_tag_t, bus_dmamap_t,
		    struct uio *, int);

static int	tsp_bus_dmamap_load_raw_sgmap(bus_dma_tag_t, bus_dmamap_t,
		    bus_dma_segment_t *, int, bus_size_t, int);

static void	tsp_bus_dmamap_unload_sgmap(bus_dma_tag_t, bus_dmamap_t);

static void	tsp_tlb_invalidate(struct tsp_config *);

/*
 * XXX Need to figure out what this is, if any.  Initialize it to
 * XXX something that should be safe.
 */
#define	TSP_SGMAP_PFTHRESH	256

/*
 * Quoting the 21272 programmer's reference manual:
 *
 * <quote>
 * 10.1.4.4 Monster Window DMA Address Translation
 *
 * In case of a PCI dual-address cycle command, the high-order PCI address
 * bits <63:40> are compared to the constant value 0x0000_01 (that is, bit
 * <40> = 1; all other bits = 0). If these bits match, a monster window hit
 * has occurred and the low-order PCI address bits <34:0> are used unchanged
 * as the system address bits <34:0>. PCI address bits <39:35> are ignored.
 * The high-order 32 PCI address bits are available on b_ad<31:0> in the
 * second cycle of a DAC, and also on b_ad<63:32> in the first cycle of a
 * DAC if b_req64_l is asserted.
 * </quote>
 *
 * This means that we can address up to 32GB of RAM using a direct-mapped
 * 64-bit DMA tag.  This leaves us possibly having to fall back on SGMAP
 * DMA on a Titan system (those support up to 64GB of RAM), and we may have
 * to address that with an additional large SGMAP DAC window at another
 * time.  XXX Does the Titan Monster Window support the extra bit?
 */
#define	TSP_MONSTER_DMA_WINDOW_BASE	0x100##00000000UL
#define	TSP_MONSTER_DMA_WINDOW_SIZE	0x008##00000000UL

/*
 * Basic 24-bit ISA DMA window is 8MB @ 8MB.  The firmware will
 * have set this up in Window 0.
 */
#define	TSP_SGMAP_MAPPED_LO_BASE	(8UL * 1024 * 1024)
#define	TSP_SGMAP_MAPPED_LO_SIZE	(8UL * 1024 * 1024)

/*
 * Basic 32-bit PCI DMA window is 1GB @ 2GB.  The firmware will
 * have set this up in Window 1.
 */
#define	TSP_DIRECT_MAPPED_BASE		(2UL * 1024 * 1024 * 1024)
#define	TSP_DIRECT_MAPPED_SIZE		(1UL * 1024 * 1024 * 1024)

/*
 * For systems that have > 1GB of RAM, but PCI devices that don't
 * support dual-address cycle, we will also set up an additional
 * SGMAP DMA window 1GB @ 3GB.  We will use Window 2 for this purpose.
 */
#define	TSP_SGMAP_MAPPED_HI_BASE	(3UL * 1024 * 1024 * 1024)
#define	TSP_SGMAP_MAPPED_HI_SIZE	(1UL * 1024 * 1024 * 1024)

/*
 * Window 3 is still available for use in the future.  Window 3 supports
 * dual address cycle.
 */

static void
tsp_dma_shutdown(void *arg)
{
	struct tsp_config *pcp = arg;
	struct ts_pchip *pccsr = pcp->pc_csr;
	int i;

	/*
	 * Restore the initial values, to make the firmware happy.
	 */
	for (i = 0; i < 4; i++) {
		pccsr->tsp_wsba[i].tsg_r = pcp->pc_saved_windows.wsba[i];
		pccsr->tsp_wsm[i].tsg_r = pcp->pc_saved_windows.wsm[i];
		pccsr->tsp_tba[i].tsg_r = pcp->pc_saved_windows.tba[i];
		alpha_mb();
	}
	pccsr->tsp_pctl.tsg_r = pcp->pc_saved_pctl;
	alpha_mb();
}

void
tsp_dma_init(struct tsp_config *pcp)
{
	bus_dma_tag_t t;
	bus_dma_tag_t t_sg_hi = NULL;
	struct ts_pchip *pccsr = pcp->pc_csr;
	bus_addr_t tbase;
	int i;

	/*
	 * Save our configuration to restore at shutdown, just
	 * in case the firmware would get cranky with us.
	 */
	for (i = 0; i < 4; i++) {
		pcp->pc_saved_windows.wsba[i] = pccsr->tsp_wsba[i].tsg_r;
		pcp->pc_saved_windows.wsm[i] = pccsr->tsp_wsm[i].tsg_r;
		pcp->pc_saved_windows.tba[i] = pccsr->tsp_tba[i].tsg_r;
	}
	pcp->pc_saved_pctl = pccsr->tsp_pctl.tsg_r;
	shutdownhook_establish(tsp_dma_shutdown, pcp);

	/* Ensure the Monster Window is enabled. */
	alpha_mb();
	pccsr->tsp_pctl.tsg_r |= PCTL_MWIN;
	alpha_mb();

	/*
	 * If we have more than 1GB of RAM, then set up an sgmap-mapped
	 * DMA window for non-DAC PCI.  This is better than using the ISA
	 * window, which is pretty small and PCI devices could starve it.
	 *
	 * N.B. avail_end is "last-usable PFN + 1".
	 */
	if (uvm_physseg_get_avail_end(uvm_physseg_get_last()) >
	    atop(TSP_DIRECT_MAPPED_SIZE)) {
		t = t_sg_hi = &pcp->pc_dmat_sgmap_hi;
		t->_cookie = pcp;
		t->_wbase = TSP_SGMAP_MAPPED_HI_BASE;
		t->_wsize = TSP_SGMAP_MAPPED_HI_SIZE;
		t->_next_window = NULL;
		t->_boundary = 0;
		t->_sgmap = &pcp->pc_sgmap_hi;
		/*
		 * According to section 8.1.2.2 of the Tsunami/Typhoon
		 * hardware reference manual (DS-0025A-TE), the SGMAP
		 * TLB is arranged as 168 locations of 4 consecutive
		 * quadwords.  It seems that on some revisions of the
		 * Pchip, SGMAP translation is not perfectly reliable
		 * unless we align the DMA segments to the TLBs natural
		 * boundaries (observed on the API CS20).
		 *
		 * N.B. the Titan (as observed on a Compaq DS25) does not
		 * seem to have this problem, but we'll play it safe and
		 * run this way on both variants.
		 */
		t->_sgmap_minalign = PAGE_SIZE * 4;
		t->_pfthresh = TSP_SGMAP_PFTHRESH;
		t->_get_tag = tsp_dma_get_tag;
		t->_dmamap_create = alpha_sgmap_dmamap_create;
		t->_dmamap_destroy = alpha_sgmap_dmamap_destroy;
		t->_dmamap_load = tsp_bus_dmamap_load_sgmap;
		t->_dmamap_load_mbuf = tsp_bus_dmamap_load_mbuf_sgmap;
		t->_dmamap_load_uio = tsp_bus_dmamap_load_uio_sgmap;
		t->_dmamap_load_raw = tsp_bus_dmamap_load_raw_sgmap;
		t->_dmamap_unload = tsp_bus_dmamap_unload_sgmap;
		t->_dmamap_sync = _bus_dmamap_sync;

		t->_dmamem_alloc = _bus_dmamem_alloc;
		t->_dmamem_free = _bus_dmamem_free;
		t->_dmamem_map = _bus_dmamem_map;
		t->_dmamem_unmap = _bus_dmamem_unmap;
		t->_dmamem_mmap = _bus_dmamem_mmap;
	}

	/*
	 * Initialize the DMA tag used for direct-mapped 64-bit DMA.
	 */
	t = &pcp->pc_dmat64_direct;
	t->_cookie = pcp;
	t->_wbase = TSP_MONSTER_DMA_WINDOW_BASE;
	t->_wsize = TSP_MONSTER_DMA_WINDOW_SIZE;
	t->_next_window = t_sg_hi;
	t->_boundary = 0;
	t->_sgmap = NULL;
	t->_get_tag = tsp_dma_get_tag;
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
	 * Initialize the DMA tag used for direct-mapped DMA.
	 */
	t = &pcp->pc_dmat_direct;
	t->_cookie = pcp;
	t->_wbase = TSP_DIRECT_MAPPED_BASE;
	t->_wsize = TSP_DIRECT_MAPPED_SIZE;
	t->_next_window = t_sg_hi;
	t->_boundary = 0;
	t->_sgmap = NULL;
	t->_get_tag = tsp_dma_get_tag;
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
	 * Initialize the DMA tag used for ISA sgmap-mapped DMA.
	 */
	t = &pcp->pc_dmat_sgmap_lo;
	t->_cookie = pcp;
	t->_wbase = TSP_SGMAP_MAPPED_LO_BASE;
	t->_wsize = TSP_SGMAP_MAPPED_LO_SIZE;
	t->_next_window = NULL;
	t->_boundary = 0;
	t->_sgmap = &pcp->pc_sgmap_lo;
	/*
	 * This appears to be needed to make DMA on the ALI southbridge
	 * that's present in some systems happy.  ???
	 */
	t->_sgmap_minalign = (64UL * 1024);
	t->_pfthresh = TSP_SGMAP_PFTHRESH;
	t->_get_tag = tsp_dma_get_tag;
	t->_dmamap_create = alpha_sgmap_dmamap_create;
	t->_dmamap_destroy = alpha_sgmap_dmamap_destroy;
	t->_dmamap_load = tsp_bus_dmamap_load_sgmap;
	t->_dmamap_load_mbuf = tsp_bus_dmamap_load_mbuf_sgmap;
	t->_dmamap_load_uio = tsp_bus_dmamap_load_uio_sgmap;
	t->_dmamap_load_raw = tsp_bus_dmamap_load_raw_sgmap;
	t->_dmamap_unload = tsp_bus_dmamap_unload_sgmap;
	t->_dmamap_sync = _bus_dmamap_sync;

	t->_dmamem_alloc = _bus_dmamem_alloc;
	t->_dmamem_free = _bus_dmamem_free;
	t->_dmamem_map = _bus_dmamem_map;
	t->_dmamem_unmap = _bus_dmamem_unmap;
	t->_dmamem_mmap = _bus_dmamem_mmap;

	/*
	 * Initialize the SGMAPs.  Align page tables to 32k in case
	 * window is somewhat larger than expected.
	 */
	alpha_sgmap_init(t, &pcp->pc_sgmap_lo, "tsp_sgmap_lo",
	    TSP_SGMAP_MAPPED_LO_BASE, 0, TSP_SGMAP_MAPPED_LO_SIZE,
	    sizeof(uint64_t), NULL, (32*1024));
	if (t_sg_hi != NULL) {
		alpha_sgmap_init(t_sg_hi, &pcp->pc_sgmap_hi, "tsp_sgmap_hi",
		    TSP_SGMAP_MAPPED_HI_BASE, 0, TSP_SGMAP_MAPPED_HI_SIZE,
		    sizeof(uint64_t), NULL, (32*1024));
	}

	/*
	 * Enable window 0 and enable SG PTE mapping.
	 */
	alpha_mb();
	pccsr->tsp_wsba[0].tsg_r =
	    TSP_SGMAP_MAPPED_LO_BASE | WSBA_SG | WSBA_ENA;
	alpha_mb();
	pccsr->tsp_wsm[0].tsg_r = WSM_8MB;
	alpha_mb();
	tbase = pcp->pc_sgmap_lo.aps_ptpa;
	if (tbase & ~0x7fffffc00UL)
		panic("tsp_dma_init: bad page table address");
	pccsr->tsp_tba[0].tsg_r = tbase;
	alpha_mb();

	/*
	 * Enable window 1 in direct mode.
	 */
	alpha_mb();
	pccsr->tsp_wsba[1].tsg_r = TSP_DIRECT_MAPPED_BASE | WSBA_ENA;
	alpha_mb();
	pccsr->tsp_wsm[1].tsg_r = WSM_1GB;
	alpha_mb();
	pccsr->tsp_tba[1].tsg_r = 0;
	alpha_mb();

	if (t_sg_hi != NULL) {
		/*
		 * Enable window 2 and enable SG PTE mapping.
		 */
		alpha_mb();
		pccsr->tsp_wsba[2].tsg_r =
		    TSP_SGMAP_MAPPED_HI_BASE | WSBA_SG | WSBA_ENA;
		alpha_mb();
		pccsr->tsp_wsm[2].tsg_r = WSM_1GB;
		alpha_mb();
		tbase = pcp->pc_sgmap_hi.aps_ptpa;
		if (tbase & ~0x7fffffc00UL)
			panic("tsp_dma_init: bad page table address");
		pccsr->tsp_tba[2].tsg_r = tbase;
		alpha_mb();
	}

	/*
	 * Disable window 3.
	 */
	alpha_mb();
	pccsr->tsp_wsba[3].tsg_r = 0;
	alpha_mb();
	pccsr->tsp_wsm[3].tsg_r = 0;
	alpha_mb();
	pccsr->tsp_tba[3].tsg_r = 0;
	alpha_mb();

	tsp_tlb_invalidate(pcp);
	alpha_mb();
}

/*
 * Return the bus dma tag to be used for the specified bus type.
 * INTERNAL USE ONLY!
 */
static bus_dma_tag_t
tsp_dma_get_tag(bus_dma_tag_t t, alpha_bus_t bustype)
{
	struct tsp_config *pcp = t->_cookie;

	switch (bustype) {
	case ALPHA_BUS_PCI:
	case ALPHA_BUS_EISA:
		/*
		 * The direct mapped window will work for most systems,
		 * most of the time. When it doesn't, we chain to the sgmap
		 * window automatically.
		 */
		return (&pcp->pc_dmat_direct);

	case ALPHA_BUS_ISA:
		/*
		 * ISA doesn't have enough address bits to use
		 * the direct-mapped DMA window, so we must use
		 * SGMAPs.
		 */
		return (&pcp->pc_dmat_sgmap_lo);

	default:
		panic("tsp_dma_get_tag: shouldn't be here, really...");
	}
}

/*
 * Load a TSP SGMAP-mapped DMA map with a linear buffer.
 */
static int
tsp_bus_dmamap_load_sgmap(bus_dma_tag_t t, bus_dmamap_t map, void *buf, bus_size_t buflen, struct proc *p, int flags)
{
	int error;

	error = pci_sgmap_pte64_load(t, map, buf, buflen, p, flags,
	    t->_sgmap);
	if (error == 0)
		tsp_tlb_invalidate(t->_cookie);

	return (error);
}

/*
 * Load a TSP SGMAP-mapped DMA map with an mbuf chain.
 */
static int
tsp_bus_dmamap_load_mbuf_sgmap(bus_dma_tag_t t, bus_dmamap_t map, struct mbuf *m, int flags)
{
	int error;

	error = pci_sgmap_pte64_load_mbuf(t, map, m, flags, t->_sgmap);
	if (error == 0)
		tsp_tlb_invalidate(t->_cookie);

	return (error);
}

/*
 * Load a TSP SGMAP-mapped DMA map with a uio.
 */
static int
tsp_bus_dmamap_load_uio_sgmap(bus_dma_tag_t t, bus_dmamap_t map, struct uio *uio, int flags)
{
	int error;

	error = pci_sgmap_pte64_load_uio(t, map, uio, flags, t->_sgmap);
	if (error == 0)
		tsp_tlb_invalidate(t->_cookie);

	return (error);
}

/*
 * Load a TSP SGMAP-mapped DMA map with raw memory.
 */
static int
tsp_bus_dmamap_load_raw_sgmap(bus_dma_tag_t t, bus_dmamap_t map, bus_dma_segment_t *segs, int nsegs, bus_size_t size, int flags)
{
	int error;

	error = pci_sgmap_pte64_load_raw(t, map, segs, nsegs, size, flags,
	    t->_sgmap);
	if (error == 0)
		tsp_tlb_invalidate(t->_cookie);

	return (error);
}

/*
 * Unload a TSP DMA map.
 */
static void
tsp_bus_dmamap_unload_sgmap(bus_dma_tag_t t, bus_dmamap_t map)
{

	/*
	 * Invalidate any SGMAP page table entries used by this
	 * mapping.
	 */
	pci_sgmap_pte64_unload(t, map, t->_sgmap);
	tsp_tlb_invalidate(t->_cookie);

	/*
	 * Do the generic bits of the unload.
	 */
	_bus_dmamap_unload_common(t, map);
}

/*
 * Flush the TSP scatter/gather TLB.
 */
static void
tsp_tlb_invalidate(struct tsp_config *pcp)
{

	alpha_mb();
	*pcp->pc_tlbia = 0;
	alpha_mb();
}
