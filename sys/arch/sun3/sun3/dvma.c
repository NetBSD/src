/*	$NetBSD: dvma.c,v 1.38.6.1 2014/08/20 00:03:26 tls Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Gordon W. Ross.
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
__KERNEL_RCSID(0, "$NetBSD: dvma.c,v 1.38.6.1 2014/08/20 00:03:26 tls Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/extent.h>
#include <sys/buf.h>
#include <sys/vnode.h>
#include <sys/core.h>
#include <sys/exec.h>

#include <uvm/uvm.h> /* XXX: not _extern ... need uvm_map_create */

#define _SUN68K_BUS_DMA_PRIVATE
#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/cpu.h>
#include <machine/dvma.h>
#include <machine/pmap.h>
#include <machine/pte.h>

#include <sun3/sun3/control.h>
#include <sun3/sun3/machdep.h>

/* DVMA is the last 1MB, but the PROM owns the last page. */
#define DVMA_MAP_END	(DVMA_MAP_BASE + DVMA_MAP_AVAIL)

/* Extent map used by dvma_mapin/dvma_mapout */
struct extent *dvma_extent;

/* XXX: Might need to tune this... */
vsize_t dvma_segmap_size = 6 * NBSG;

/* Using phys_map to manage DVMA scratch-memory pages. */
/* Note: Could use separate pagemap for obio if needed. */

void
dvma_init(void)
{
	vaddr_t segmap_addr;

	/*
	 * Create phys_map covering the entire DVMA space,
	 * then allocate the segment pool from that.  The
	 * remainder will be used as the DVMA page pool.
	 *
	 * Note that no INTRSAFE is needed here because the
	 * dvma_extent manages things handled in interrupt
	 * context.
	 */
	phys_map = kmem_alloc(sizeof(struct vm_map), KM_SLEEP);
	if (phys_map == NULL)
		panic("unable to create DVMA map");

	uvm_map_setup(phys_map, DVMA_MAP_BASE, DVMA_MAP_END, 0);
	phys_map->pmap = pmap_kernel();

	/*
	 * Reserve the DVMA space used for segment remapping.
	 * The remainder of phys_map is used for DVMA scratch
	 * memory pages (i.e. driver control blocks, etc.)
	 */
	segmap_addr = uvm_km_alloc(phys_map, dvma_segmap_size, 0,
	    UVM_KMF_VAONLY | UVM_KMF_WAITVA);
	if (segmap_addr != DVMA_MAP_BASE)
		panic("dvma_init: unable to allocate DVMA segments");

	/*
	 * Create the VM pool used for mapping whole segments
	 * into DVMA space for the purpose of data transfer.
	 */
	dvma_extent = extent_create("dvma", segmap_addr,
	    segmap_addr + (dvma_segmap_size - 1),
	    NULL, 0, EX_NOCOALESCE|EX_NOWAIT);
}

/*
 * Allocate actual memory pages in DVMA space.
 * (idea for implementation borrowed from Chris Torek.)
 */
void *
dvma_malloc(size_t bytes)
{
	void *new_mem;
	vsize_t new_size;

	if (bytes == 0)
		return NULL;
	new_size = m68k_round_page(bytes);
	new_mem = (void *)uvm_km_alloc(phys_map, new_size, 0, UVM_KMF_WIRED);
	if (new_mem == 0)
		panic("dvma_malloc: no space in phys_map");
	/* The pmap code always makes DVMA pages non-cached. */
	return new_mem;
}

/*
 * Free pages from dvma_malloc()
 */
void
dvma_free(void *addr, size_t size)
{
	vsize_t sz = m68k_round_page(size);

	uvm_km_free(phys_map, (vaddr_t)addr, sz, UVM_KMF_WIRED);
}

/*
 * Given a DVMA address, return the physical address that
 * would be used by some OTHER bus-master besides the CPU.
 * (Examples: on-board ie/le, VME xy board).
 */
u_long
dvma_kvtopa(void *kva, int bustype)
{
	u_long addr, mask;

	addr = (u_long)kva;
	if ((addr & DVMA_MAP_BASE) != DVMA_MAP_BASE)
		panic("dvma_kvtopa: bad dmva addr=0x%lx", addr);

	switch (bustype) {
	case BUS_OBIO:
	case BUS_OBMEM:
		mask = DVMA_OBIO_SLAVE_MASK;
		break;
	default:	/* VME bus device. */
		mask = DVMA_VME_SLAVE_MASK;
		break;
	}

	return addr & mask;
}

/*
 * Given a range of kernel virtual space, remap all the
 * pages found there into the DVMA space (dup mappings).
 * This IS safe to call at interrupt time.
 * (Typically called at SPLBIO)
 */
void *
dvma_mapin(void *kva, int len, int canwait /* ignored */)
{
	vaddr_t seg_kva, seg_dma;
	vsize_t seg_len, seg_off;
	vaddr_t v, x;
	int s, sme, error;

	/* Get seg-aligned address and length. */
	seg_kva = (vaddr_t)kva;
	seg_len = (vsize_t)len;
	seg_off = seg_kva & SEGOFSET;
	seg_kva -= seg_off;
	seg_len = sun3_round_seg(seg_len + seg_off);

	s = splvm();

	/* Allocate the DVMA segment(s) */

	error = extent_alloc(dvma_extent, seg_len, NBSG, 0,
	    EX_FAST | EX_NOWAIT | EX_MALLOCOK, &seg_dma);
	if (error) {
		splx(s);
		return NULL;
	}

#ifdef	DIAGNOSTIC
	if (seg_dma & SEGOFSET)
		panic("dvma_mapin: seg not aligned");
#endif

	/* Duplicate the mappings into DMA space. */
	v = seg_kva;
	x = seg_dma;
	while (seg_len > 0) {
		sme = get_segmap(v);
#ifdef	DIAGNOSTIC
		if (sme == SEGINV)
			panic("dvma_mapin: seg not mapped");
#endif
#ifdef	HAVECACHE
		/* flush write-back on old mappings */
		if (cache_size)
			cache_flush_segment(v);
#endif
		set_segmap_allctx(x, sme);
		v += NBSG;
		x += NBSG;
		seg_len -= NBSG;
	}
	seg_dma += seg_off;

	splx(s);
	return (void *)seg_dma;
}

/*
 * Free some DVMA space allocated by the above.
 * This IS safe to call at interrupt time.
 * (Typically called at SPLBIO)
 */
void
dvma_mapout(void *dma, int len)
{
	vaddr_t seg_dma;
	vsize_t seg_len, seg_off;
	vaddr_t v, x;
	int sme __diagused;
	int s;

	/* Get seg-aligned address and length. */
	seg_dma = (vaddr_t)dma;
	seg_len = (vsize_t)len;
	seg_off = seg_dma & SEGOFSET;
	seg_dma -= seg_off;
	seg_len = sun3_round_seg(seg_len + seg_off);

	s = splvm();

	/* Flush cache and remove DVMA mappings. */
	v = seg_dma;
	x = v + seg_len;
	while (v < x) {
		sme = get_segmap(v);
#ifdef	DIAGNOSTIC
		if (sme == SEGINV)
			panic("dvma_mapout: seg not mapped");
#endif
#ifdef	HAVECACHE
		/* flush write-back on the DVMA mappings */
		if (cache_size)
			cache_flush_segment(v);
#endif
		set_segmap_allctx(v, SEGINV);
		v += NBSG;
	}

	if (extent_free(dvma_extent, seg_dma, seg_len,
	    EX_NOWAIT | EX_MALLOCOK))
		panic("dvma_mapout: unable to free 0x%lx,0x%lx",
		    seg_dma, seg_len);
	splx(s);
}

int
_bus_dmamap_load_raw(bus_dma_tag_t t, bus_dmamap_t map, bus_dma_segment_t *segs,
    int nsegs, bus_size_t size, int flags)
{

	panic("_bus_dmamap_load_raw(): not implemented yet.");
}

int
_bus_dmamap_load(bus_dma_tag_t t, bus_dmamap_t map, void *buf,
    bus_size_t buflen, struct proc *p, int flags)
{
	vaddr_t kva, dva;
	vsize_t off, sgsize;
	paddr_t pa;
	pmap_t pmap;
	int error, rv __diagused, s;

	/*
	 * Make sure that on error condition we return "no valid mappings".
	 */
	map->dm_nsegs = 0;
	map->dm_mapsize = 0;

	if (buflen > map->_dm_size)
		return EINVAL;

	kva = (vaddr_t)buf;
	off = kva & PGOFSET;
	sgsize = round_page(off + buflen);

	/* Try to allocate DVMA space. */
	s = splvm();
	error = extent_alloc(dvma_extent, sgsize, PAGE_SIZE, 0,
	    EX_FAST | ((flags & BUS_DMA_NOWAIT) == 0 ? EX_WAITOK : EX_NOWAIT),
	    &dva);
	splx(s);
	if (error)
		return ENOMEM;

	/* Fill in the segment. */
	map->dm_segs[0].ds_addr = dva + off;
	map->dm_segs[0].ds_len = buflen;
	map->dm_segs[0]._ds_va = dva;
	map->dm_segs[0]._ds_sgsize = sgsize;

	/*
	 * Now map the DVMA addresses we allocated to point to the
	 * pages of the caller's buffer.
	 */
	if (p != NULL)
		pmap = p->p_vmspace->vm_map.pmap;
	else
		pmap = pmap_kernel();

	while (sgsize > 0) {
		rv = pmap_extract(pmap, kva, &pa);
#ifdef DIAGNOSTIC
		if (rv == false)
			panic("%s: unmapped VA", __func__);
#endif
		pmap_enter(pmap_kernel(), dva, pa | PMAP_NC,
		    VM_PROT_READ | VM_PROT_WRITE, PMAP_WIRED);
		kva += PAGE_SIZE;
		dva += PAGE_SIZE;
		sgsize -= PAGE_SIZE;
	}

	map->dm_nsegs = 1;
	map->dm_mapsize = map->dm_segs[0].ds_len;

	return 0;
}

void
_bus_dmamap_unload(bus_dma_tag_t t, bus_dmamap_t map)
{
	bus_dma_segment_t *segs;
	vaddr_t dva;
	vsize_t sgsize;
	int error __diagused, s;

#ifdef DIAGNOSTIC
	if (map->dm_nsegs != 1)
		panic("%s: invalid nsegs = %d", __func__, map->dm_nsegs);
#endif

	segs = map->dm_segs;
	dva = segs[0]._ds_va & ~PGOFSET;
	sgsize = segs[0]._ds_sgsize;

	/* Unmap the DVMA addresses. */
	pmap_remove(pmap_kernel(), dva, dva + sgsize);
	pmap_update(pmap_kernel());

	/* Free the DVMA addresses. */
	s = splvm();
	error = extent_free(dvma_extent, dva, sgsize, EX_NOWAIT);
	splx(s);
#ifdef DIAGNOSTIC
	if (error)
		panic("%s: unable to free DVMA region", __func__);
#endif

	/* Mark the mappings as invalid. */
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;
}
