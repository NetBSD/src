/*	$NetBSD: i80321_pci_dma.c,v 1.2.2.3 2002/06/20 03:38:15 nathanw Exp $	*/

/*
 * Copyright (c) 2001, 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * PCI DMA support for i80321 I/O Processor chip.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>

#include <uvm/uvm_extern.h>

#define	_ARM32_BUS_DMA_PRIVATE
#include <machine/bus.h>

#include <arm/xscale/i80321reg.h>
#include <arm/xscale/i80321var.h>

int	i80321_pci_dmamap_load(bus_dma_tag_t, bus_dmamap_t, void *,
	    bus_size_t, struct proc *, int);
int	i80321_pci_dmamap_load_mbuf(bus_dma_tag_t, bus_dmamap_t,
	    struct mbuf *, int);
int	i80321_pci_dmamap_load_uio(bus_dma_tag_t, bus_dmamap_t,
	    struct uio *, int);
int	i80321_pci_dmamap_load_raw(bus_dma_tag_t, bus_dmamap_t,
	    bus_dma_segment_t *, int, bus_size_t, int);

int	i80321_pci_dmamem_alloc(bus_dma_tag_t, bus_size_t,
	    bus_size_t, bus_size_t, bus_dma_segment_t *, int, int *, int);

void
i80321_pci_dma_init(bus_dma_tag_t dmat, void *cookie)
{

	/*
	 * XXX XXX XXX The arm32_bus_dma_tag really needs to
	 * XXX XXX XXX be rototilled.
	 */
	
	dmat->_ranges = cookie;
	dmat->_nranges = 0;

	dmat->_dmamap_create = _bus_dmamap_create;
	dmat->_dmamap_destroy = _bus_dmamap_destroy;
	dmat->_dmamap_load = i80321_pci_dmamap_load;
	dmat->_dmamap_load_mbuf = i80321_pci_dmamap_load_mbuf;
	dmat->_dmamap_load_uio = i80321_pci_dmamap_load_uio;
	dmat->_dmamap_load_raw = i80321_pci_dmamap_load_raw;
	dmat->_dmamap_unload = _bus_dmamap_unload;
	dmat->_dmamap_sync = _bus_dmamap_sync;

	dmat->_dmamem_alloc = i80321_pci_dmamem_alloc;
	dmat->_dmamem_free = _bus_dmamem_free;
	dmat->_dmamem_map = _bus_dmamem_map;
	dmat->_dmamem_unmap = _bus_dmamem_unmap;
	dmat->_dmamem_mmap = _bus_dmamem_mmap;
}

/*
 * i80321_pci_dmamap_load_buffer:
 *
 *	Utility function to load a linear buffer.  lastaddrp holds state
 *	between invocations (for multiple-buffer loads).  segp contains
 *	the starting segment on entry, and the ending segment on exit.
 *	first indicates if this is the first invocation of this function.
 */
static int
i80321_pci_dmamap_load_buffer(bus_dma_tag_t t, bus_dmamap_t map,
    void *buf, bus_size_t buflen, struct proc *p, int flags,
    vaddr_t *lastaddrp, int *segp, int first)
{
	struct i80321_softc *sc = (void *) t->_ranges;	/* XXX XXX XXX */
	bus_size_t sgsize;
	bus_addr_t curaddr, lastaddr, baddr, bmask;
	vaddr_t vaddr = (vaddr_t) buf;
	int seg;
	struct pmap *pmap;

	if (p != NULL)
		pmap = p->p_vmspace->vm_map.pmap;
	else
		pmap = pmap_kernel();

	lastaddr = *lastaddrp;
	bmask = ~(map->_dm_boundary - 1);

	for (seg = *segp; buflen > 0; ) {
		/* Get the physical address for this segment. */
		(void) pmap_extract(pmap, (vaddr_t) vaddr, &curaddr);

		/*
		 * Make sure we're in an allowed DMA range.
		 */
		if (curaddr < sc->sc_iwin[2].iwin_xlate ||
		    curaddr >= (sc->sc_iwin[2].iwin_xlate +
				sc->sc_iwin[2].iwin_size))
			return (EINVAL);

		/*
		 * Translate the physical memory address to an address
		 * in the SDRAM Inbound window.
		 */
		curaddr = (curaddr - sc->sc_iwin[2].iwin_xlate) +
		    PCI_MAPREG_MEM_ADDR(sc->sc_iwin[2].iwin_base_lo);

		/* Compute the segment size, and adjust counts. */
		sgsize = PAGE_SIZE - (vaddr & PAGE_MASK);
		if (buflen < sgsize)
			sgsize = buflen;

		/* Make sure we don't cross any boundaries. */
		if (map->_dm_boundary > 0) {
			baddr = (curaddr + map->_dm_boundary) & bmask;
			if (sgsize > (baddr - curaddr))
				sgsize = (baddr - curaddr);
		}

		/*
		 * Insert chunk into a segment, coalescing with
		 * the previous segment if possible.
		 */
		if (first) {
			map->dm_segs[seg].ds_addr = curaddr;
			map->dm_segs[seg].ds_len = sgsize;
			map->dm_segs[seg]._ds_vaddr = vaddr;
			first = 0;
		} else {
			if (curaddr == lastaddr &&
			    (map->dm_segs[seg].ds_len + sgsize) <=
			     map->_dm_maxsegsz &&
			    (map->_dm_boundary == 0 ||
			     (map->dm_segs[seg].ds_addr & bmask) ==
			     (curaddr & bmask)))
				map->dm_segs[seg].ds_len += sgsize;
			else {
				if (++seg >= map->_dm_segcnt)
					break;
				map->dm_segs[seg].ds_addr = curaddr;
				map->dm_segs[seg].ds_len = sgsize;
				map->dm_segs[seg]._ds_vaddr = vaddr;
			}
		}

		lastaddr = curaddr + sgsize;
		vaddr += sgsize;
		buflen -= sgsize;
	}

	*segp = seg;
	*lastaddrp = lastaddr;

	/* Did we fit? */
	if (buflen != 0)
		return (EFBIG);		/* XXX better return value here? */
	return (0);
}

/*
 * bus_dmamap_load:
 *
 *	Load a DMA map with a linear buffer.
 */
int
i80321_pci_dmamap_load(bus_dma_tag_t t, bus_dmamap_t map, void *buf,
    bus_size_t buflen, struct proc *p, int flags)
{
	vaddr_t lastaddr;
	int seg, error;

	/*
	 * Make sure that on error condition we return "no valid mappings".
	 */
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;

	if (buflen > map->_dm_size)
		return (EINVAL);

	seg = 0;
	error = i80321_pci_dmamap_load_buffer(t, map, buf, buflen, p,
	    flags, &lastaddr, &seg, 1);
	if (error == 0) {
		map->dm_mapsize = buflen;
		map->dm_nsegs = seg + 1;
		map->_dm_proc = p;
	}

	return (error);
}

/*
 * bus_dmamap_load_mbuf:
 *
 *	Load a DMA map with an mbuf chain.
 */
int
i80321_pci_dmamap_load_mbuf(bus_dma_tag_t t, bus_dmamap_t map,
    struct mbuf *m0, int flags)
{
	vaddr_t lastaddr;
	int seg, error, first;
	struct mbuf *m;

	/*
	 * Make sure that on error condition we return "no valid mappings".
	 */
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;

#ifdef DIAGNOSTIC
	if ((m0->m_flags & M_PKTHDR) == 0)
		panic("i80321_pci_bus_dmamap_load_mbuf: no packet header");
#endif

	if (m0->m_pkthdr.len > map->_dm_size)
		return (EINVAL);

	first = 1;
	seg = 0;
	error = 0;
	for (m = m0; m != NULL && error == 0; m = m->m_next) {
		error = i80321_pci_dmamap_load_buffer(t, map, m->m_data,
		    m->m_len, NULL, flags, &lastaddr, &seg, first);
		first = 0;
	}
	if (error == 0) {
		map->dm_mapsize = m0->m_pkthdr.len;
		map->dm_nsegs = seg + 1;
		map->_dm_proc = NULL;	/* always kernel */
	}

	return (error);
}

/*
 * bus_dmamap_load_uio:
 *
 *	Load a DMA map with a uio.
 */
int
i80321_pci_dmamap_load_uio(bus_dma_tag_t t, bus_dmamap_t map,
    struct uio *uio, int flags)
{
	vaddr_t lastaddr;
	int seg, i, error, first;
	bus_size_t minlen, resid;
	struct proc *p = NULL;
	struct iovec *iov;
	caddr_t addr;

	/*
	 * Make sure that on error condition we return "no valid mappings".
	 */
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;

	resid = uio->uio_resid;
	iov = uio->uio_iov;

	if (uio->uio_segflg == UIO_USERSPACE) {
		p = uio->uio_procp;
#ifdef DIAGNOSTIC
		if (p == NULL)
			panic("i80321_pci_dmamap_load_uio: USERSPACE but "
			    "no proc");
#endif
	}

	first = 1;
	seg = 0;
	error = 0;
	for (i = 0; i < uio->uio_iovcnt && resid != 0 && error == 0; i++) {
		/*
		 * Now at the first iovec to load.  Load each iovec
		 * until we have exhausted the residual count.
		 */
		minlen = resid < iov[i].iov_len ? resid : iov[i].iov_len;
		addr = (caddr_t)iov[i].iov_base;

		error = i80321_pci_dmamap_load_buffer(t, map, addr, minlen,
		    p, flags, &lastaddr, &seg, first);
		first = 0;

		resid -= minlen;
	}
	if (error == 0) {
		map->dm_mapsize = uio->uio_resid;
		map->dm_nsegs = seg + 1;
		map->_dm_proc = p;
	}

	return (error);
}

/*
 * bus_dmamap_load_raw:
 *
 *	Load a DMA map with raw memory allocated with
 *	bus_dmamem_alloc().
 */
int
i80321_pci_dmamap_load_raw(bus_dma_tag_t t, bus_dmamap_t map,
    bus_dma_segment_t *segs, int nsegs, bus_size_t size, int flags)
{

	panic("i80321_pci_dmamap_load_raw: not implemented");
}

/*
 * bus_dmamem_alloc:
 *
 *	Allocate DMA-safe memory.
 */
int
i80321_pci_dmamem_alloc(bus_dma_tag_t t, bus_size_t size,
    bus_size_t alignment, bus_size_t boundary,
    bus_dma_segment_t *segs, int nsegs, int *rsegs, int flags)
{
	struct i80321_softc *sc = (void *) t->_ranges;
	int error;

	if (sc->sc_iwin[2].iwin_size == 0)
		return (ENOMEM);

	error = _bus_dmamem_alloc_range(t, size, alignment, boundary,
	    segs, nsegs, rsegs, flags, trunc_page(sc->sc_iwin[2].iwin_xlate),
	    trunc_page(sc->sc_iwin[2].iwin_xlate +
		       sc->sc_iwin[2].iwin_size - 1));

	return (error);
}
