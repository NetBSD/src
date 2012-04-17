/*	$NetBSD: dvma.c,v 1.40.12.1 2012/04/17 00:06:58 yamt Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Gordon W. Ross and Jeremy Cooper.
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
 * DVMA (Direct Virtual Memory Access - like DMA)
 *
 * In the Sun3 architecture, memory cycles initiated by secondary bus
 * masters (DVMA devices) passed through the same MMU that governed CPU
 * accesses.  All DVMA devices were wired in such a way so that an offset
 * was added to the addresses they issued, causing them to access virtual
 * memory starting at address 0x0FF00000 - the offset.  The task of
 * enabling a DVMA device to access main memory only involved creating
 * valid mapping in the MMU that translated these high addresses into the
 * appropriate physical addresses.
 *
 * The Sun3x presents a challenge to programming DVMA because the MMU is no
 * longer shared by both secondary bus masters and the CPU.  The MC68030's
 * built-in MMU serves only to manage virtual memory accesses initiated by
 * the CPU.  Secondary bus master bus accesses pass through a different MMU,
 * aptly named the 'I/O Mapper'.  To enable every device driver that uses
 * DVMA to understand that these two address spaces are disconnected would
 * require a tremendous amount of code re-writing. To avoid this, we will
 * ensure that the I/O Mapper and the MC68030 MMU are programmed together,
 * so that DVMA mappings are consistent in both the CPU virtual address
 * space and secondary bus master address space - creating an environment
 * just like the Sun3 system.
 *
 * The maximum address space that any DVMA device in the Sun3x architecture
 * is capable of addressing is 24 bits wide (16 Megabytes.)  We can alias
 * all of the mappings that exist in the I/O mapper by duplicating them in
 * a specially reserved section of the CPU's virtual address space, 16
 * Megabytes in size.  Whenever a DVMA buffer is allocated, the allocation
 * code will enter in a mapping both in the MC68030 MMU page tables and the
 * I/O mapper.
 *
 * The address returned by the allocation routine is a virtual address that
 * the requesting driver must use to access the buffer.  It is up to the
 * device driver to convert this virtual address into the appropriate slave
 * address that its device should issue to access the buffer.  (There will be
 * routines that assist the driver in doing so.)
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: dvma.c,v 1.40.12.1 2012/04/17 00:06:58 yamt Exp $");

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

#include <uvm/uvm_extern.h>

#define _SUN68K_BUS_DMA_PRIVATE
#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/cpu.h>
#include <machine/dvma.h>
#include <machine/pmap.h>

#include <sun3/sun3/machdep.h>

#include <sun3/sun3x/enable.h>
#include <sun3/sun3x/iommu.h>

/*
 * Use an extent map to manage DVMA scratch-memory pages.
 * Note: SunOS says last three pages are reserved (PROM?)
 * Note: need a separate map (sub-map?) for last 1MB for
 *       use by VME slave interface.
 */

/* Number of slots in dvmamap. */
struct extent *dvma_extent;

void 
dvma_init(void)
{

	/*
	 * Create the extent map for DVMA pages.
	 */
	dvma_extent = extent_create("dvma", DVMA_MAP_BASE,
	    DVMA_MAP_BASE + (DVMA_MAP_AVAIL - 1),
	    NULL, 0, EX_NOCOALESCE|EX_NOWAIT);

	/*
	 * Enable DVMA in the System Enable register.
	 * Note:  This is only necessary for VME slave accesses.
	 *        On-board devices are always capable of DVMA.
	 */
	*enable_reg |= ENA_SDVMA;
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
 * Map a range [va, va+len] of wired virtual addresses in the given map
 * to a kernel address in DVMA space.
 */
void *
dvma_mapin(void *kmem_va, int len, int canwait)
{
	void *dvma_addr;
	vaddr_t kva, tva;
	int npf, s, error;
	paddr_t pa;
	long off;
	bool rv;

	kva = (vaddr_t)kmem_va;
#ifdef	DIAGNOSTIC
	/*
	 * Addresses below VM_MIN_KERNEL_ADDRESS are not part of the kernel
	 * map and should not participate in DVMA.
	 */
	if (kva < VM_MIN_KERNEL_ADDRESS)
		panic("dvma_mapin: bad kva");
#endif

	/*
	 * Calculate the offset of the data buffer from a page boundary.
	 */
	off = kva & PGOFSET;
	kva -= off;	/* Truncate starting address to nearest page. */
	len = round_page(len + off); /* Round the buffer length to pages. */
	npf = btoc(len); /* Determine the number of pages to be mapped. */

	/*
	 * Try to allocate DVMA space of the appropriate size
	 * in which to do a transfer.
	 */
	s = splvm();
	error = extent_alloc(dvma_extent, len, PAGE_SIZE, 0,
	    EX_FAST | EX_NOWAIT | (canwait ? EX_WAITSPACE : 0), &tva);
	splx(s);
	if (error)
		return NULL;
	
	/* 
	 * Tva is the starting page to which the data buffer will be double
	 * mapped.  Dvma_addr is the starting address of the buffer within
	 * that page and is the return value of the function.
	 */
	dvma_addr = (void *)(tva + off);

	for (; npf--; kva += PAGE_SIZE, tva += PAGE_SIZE) {
		/*
		 * Retrieve the physical address of each page in the buffer
		 * and enter mappings into the I/O MMU so they may be seen
		 * by external bus masters and into the special DVMA space
		 * in the MC68030 MMU so they may be seen by the CPU.
		 */
		rv = pmap_extract(pmap_kernel(), kva, &pa);
#ifdef	DEBUG
		if (rv == false)
			panic("dvma_mapin: null page frame");
#endif	/* DEBUG */

		iommu_enter((tva & IOMMU_VA_MASK), pa);
		pmap_kenter_pa(tva,
		    pa | PMAP_NC, VM_PROT_READ | VM_PROT_WRITE, 0);
	}
	pmap_update(pmap_kernel());

	return dvma_addr;
}

/*
 * Remove double map of `va' in DVMA space at `kva'.
 *
 * TODO - This function might be the perfect place to handle the
 *       synchronization between the DVMA cache and central RAM
 *       on the 3/470.
 */
void 
dvma_mapout(void *dvma_addr, int len)
{
	u_long kva;
	int s, off;

	kva = (u_long)dvma_addr;
	off = (int)kva & PGOFSET;
	kva -= off;
	len = round_page(len + off);

	iommu_remove((kva & IOMMU_VA_MASK), len);
	pmap_kremove(kva, len);
	pmap_update(pmap_kernel());

	s = splvm();
	if (extent_free(dvma_extent, kva, len, EX_NOWAIT | EX_MALLOCOK))
		panic("dvma_mapout: unable to free region: 0x%lx,0x%x",
		    kva, len);
	splx(s);
}

/*
 * Allocate actual memory pages in DVMA space.
 * (For sun3 compatibility - the ie driver.)
 */
void *
dvma_malloc(size_t bytes)
{
	void *new_mem, *dvma_mem;
	vsize_t new_size;

	if (bytes == 0)
		return NULL;
	new_size = m68k_round_page(bytes);
	new_mem = (void *)uvm_km_alloc(kernel_map, new_size, 0, UVM_KMF_WIRED);
	if (new_mem == 0)
		return NULL;
	dvma_mem = dvma_mapin(new_mem, new_size, 1);
	return dvma_mem;
}

/*
 * Free pages from dvma_malloc()
 */
void 
dvma_free(void *addr, size_t size)
{
	vsize_t sz = m68k_round_page(size);

	dvma_mapout(addr, sz);
	/* XXX: need kmem address to free it...
	   Oh well, we never call this anyway. */
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
	int error, rv, s;

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
		iommu_enter((dva & IOMMU_VA_MASK), pa);
		pmap_kenter_pa(dva,
		    pa | PMAP_NC, VM_PROT_READ | VM_PROT_WRITE, 0);
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
	int error, s;

#ifdef DIAGNOSTIC
	if (map->dm_nsegs != 1)
		panic("%s: invalid nsegs = %d", __func__, map->dm_nsegs);
#endif

	segs = map->dm_segs;
	dva = segs[0]._ds_va & ~PGOFSET;
	sgsize = segs[0]._ds_sgsize;

	/* Unmap the DVMA addresses. */
	iommu_remove((dva & IOMMU_VA_MASK), sgsize);
	pmap_kremove(dva, sgsize);
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
