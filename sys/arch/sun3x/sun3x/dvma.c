/*	$NetBSD: dvma.c,v 1.2 1997/01/23 22:44:45 gwr Exp $	*/

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
 * address that its device should issue to access the buffer.  (The will be
 * routines that will assist the driver in doing so.)
 *
 * XXX - This needs work.  The address from dvma_malloc() faults!
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/map.h>
#include <sys/buf.h>
#include <sys/vnode.h>
#include <sys/user.h>
#include <sys/core.h>
#include <sys/exec.h>

#include <vm/vm.h>
#include <vm/vm_kern.h>
#include <vm/vm_map.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <machine/enable.h>
#include <machine/reg.h>
#include <machine/pmap.h>
#include <machine/dvma.h>
#include <machine/machdep.h>

#include "iommu.h"

/*
 * Use a resource map to manage DVMA scratch-memory pages.
 */

/* Number of slots in dvmamap. */
int dvma_max_segs = 256;
struct map *dvmamap;

void
dvma_init()
{

	/*
	 * Create the resource map for DVMA pages.
	 */
	dvmamap = malloc((sizeof(struct map) * dvma_max_segs),
					 M_DEVBUF, M_WAITOK);

	rminit(dvmamap, btoc(DVMA_SPACE_LENGTH), btoc(0xFF000000),
		   "dvmamap", dvma_max_segs);

	/*
	 * Enable DVMA in the System Enable register.
	 * Note:  This is only necessary for VME slave accesses.
	 *        On-board devices are always capable of DVMA.
	 * *enable_reg |= ENA_SDVMA;
	 */
}


/*
 * Given a DVMA address, return the physical address that
 * would be used by some OTHER bus-master besides the CPU.
 * (Examples: on-board ie/le, VME xy board).
 */
u_long
dvma_kvtopa(kva, bustype)
	void * kva;
	int bustype;
{
	u_long addr, mask;

	addr = (u_long)kva;
	if ((addr & DVMA_SPACE_START) != DVMA_SPACE_START)
		panic("dvma_kvtopa: bad dmva addr=0x%x\n", addr);

	/* Everything has just 24 bits. */
	mask = DVMA_SLAVE_MASK;

	return(addr & mask);
}


/*
 * Map a range [va, va+len] of wired virtual addresses in the given map
 * to a kernel address in DVMA space.
 */
void *
dvma_mapin(kmem_va, len, canwait)
	void *	kmem_va;
	int		len, canwait;
{
	void * dvma_addr;
	vm_offset_t kva, tva;
	register int npf, s;
	register vm_offset_t pa;
	long off, pn;

	kva = (u_long)kmem_va;
	if (kva < VM_MIN_KERNEL_ADDRESS)
		panic("dvma_mapin: bad kva");

	off = (int)kva & PGOFSET;
	kva -= off;
	len = round_page(len + off);
	npf = btoc(len);

	s = splimp();
	for (;;) {

		pn = rmalloc(dvmamap, npf);

		if (pn != 0)
			break;
		if (canwait) {
			(void)tsleep(dvmamap, PRIBIO+1, "physio", 0);
			continue;
		}
		splx(s);
		return NULL;
	}
	splx(s);

	tva = ctob(pn);
	dvma_addr = (void *) (tva + off);

	while (npf--) {
		pa = pmap_extract(pmap_kernel(), kva);
		if (pa == 0)
			panic("dvma_mapin: null page frame");
		pa = trunc_page(pa);

		iommu_enter((tva & DVMA_SLAVE_MASK), pa);

		pmap_enter(pmap_kernel(), tva, pa | PMAP_NC,
			VM_PROT_READ|VM_PROT_WRITE, 1);

		kva += NBPG;
		tva += NBPG;
	}

	return (dvma_addr);
}

/*
 * Remove double map of `va' in DVMA space at `kva'.
 */
void
dvma_mapout(dvma_addr, len)
	void *	dvma_addr;
	int		len;
{
	u_long kva;
	int s, off;

	kva = (u_long)dvma_addr;
	off = (int)kva & PGOFSET;
	kva -= off;
	len = round_page(len + off);

	iommu_remove((kva & DVMA_SLAVE_MASK), len);

	pmap_remove(pmap_kernel(), kva, kva + len);

	s = splimp();
	rmfree(dvmamap, btoc(len), btoc(kva));
	wakeup(dvmamap);
	splx(s);
}
