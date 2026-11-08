/*	$NetBSD: pmap_bootstrap.c,v 1.14 2026/11/08 08:39:10 tsutsui Exp $	*/
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
 * Copyright (c) 1991 Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)pmap.c	7.5 (Berkeley) 5/10/91
 */

/*
 * Copyright (c) 1995 Leo Weppelman
 * Copyright (c) 1994 Michael L. Hitch
 * Copyright (c) 1993 Markus Wild
 * All rights reserved.
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
 *      This product includes software developed by Markus Wild.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>

#include "opt_m68k_arch.h"
#include "opt_mbtype.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>

#include <uvm/uvm.h>

#include <machine/pte.h>
#include <machine/cpu.h>
#include <machine/vmparam.h>

#include <m68k/cacheops.h>
#include <machine/iomap.h>
#include <sys/bus.h>

static vaddr_t bootstrap_vstart;

/*
 * Keep these declarations here during preparation for NEW_PMAP.
 * They may become unnecessary after the transition, so defer moving
 * them to a header.
 */
extern psize_t st_pool_size;
extern paddr_t st_pool_phys;
extern vaddr_t st_pool_virt;
extern vaddr_t page_zero;

#define	RELOC_PA(base, pa)	((base) + (pa))	/* used to set up PTE etc. */
int kernel_copyback = 1;

static void mmu030_setup(paddr_t, u_int, paddr_t, psize_t, paddr_t, paddr_t);
static void map_io_areas(paddr_t, psize_t, u_int);
#if defined(M68040) || defined(M68060)
static void mmu040_setup(paddr_t, u_int, paddr_t, psize_t, paddr_t, paddr_t);
#endif

extern paddr_t	avail_start;
extern paddr_t	avail_end;

extern paddr_t	msgbufpa;

/*
 * All those kernel PT submaps that BSD is so fond of
 */
void		*CADDR1, *CADDR2;
char		*vmmap;

paddr_t __attribute__((no_instrument_function))
pmap_bootstrap1(paddr_t nextpa, paddr_t reloff)
{
	extern void etext(void);
	paddr_t pstart, ptpa, Sysptmap_pa;
	vaddr_t vstart;		/* Next available virtual address */
	vaddr_t kva;
	vsize_t avail;
	psize_t ptsize;
	u_int ptextra, kstsize;
	pt_entry_t *pg, *epg, pg_proto;

	/*
	 * We run the kernel from ST memory at the moment.
	 * The kernel segment table is put just behind the loaded image.
	 * pstart: start of usable ST memory
	 * avail : size of ST memory available.
	 */
	vstart = (vaddr_t)nextpa;
	vstart = m68k_round_page(vstart);
	pstart = (paddr_t)vstart;	/* pre-reloc PA == kernel VA here */
	avail  = st_pool_phys - pstart;

	/*
	 * Save KVA of lwp0 uarea and allocate it.
	 */
	lwp0uarea  = vstart;
	pstart    += USPACE;
	vstart    += USPACE;
	avail     -= USPACE;

	/*
	 * Calculate the number of pages needed for Sysseg.
	 * For the 68030, we need 256 descriptors (segment-table-entries).
	 * This easily fits into one page.
	 * For the 68040, both the level-1 and level-2 descriptors are
	 * stored into Sysseg. We currently handle a maximum sum of MAXKL2SIZE
	 * level-1 & level-2 tables.
	 */
#if defined(M68040) || defined(M68060)
	if (mmutype == MMU_68040)
		kstsize = MAXKL2SIZE / (NPTEPG/SG4_LEV2SIZE);
	else
#endif
		kstsize = 1;
	/*
	 * allocate the kernel segment table
	 */
	Sysseg_pa  = pstart;			/* pre-reloc PA to init STEs */
	Sysseg     = (st_entry_t *)vstart;
	pstart    += kstsize * PAGE_SIZE;
	vstart    += kstsize * PAGE_SIZE;
	avail     -= kstsize * PAGE_SIZE;

	/*
	 * allocate kernel page table map
	 */
	Sysptmap_pa = pstart;			/* pre-reloc PA to init PTEs */
	Sysptmap = (pt_entry_t *)vstart;
	pstart  += PAGE_SIZE;
	vstart  += PAGE_SIZE;
	avail   -= PAGE_SIZE;

	/*
	 * Determine the number of pte's we need for extra's like
	 * ST I/O map's.
	 */
	ptextra = btoc(STIO_SIZE);

	/*
	 * If present, add pci areas
	 */
	if (machineid & ATARI_HADES)
		ptextra += btoc(PCI_CONFIG_SIZE + PCI_IO_SIZE + PCI_MEM_SIZE);
	if (machineid & ATARI_MILAN)
		ptextra += btoc(PCI_IO_SIZE + PCI_MEM_SIZE);
	ptextra += btoc(BOOTM_VA_POOL);
	/*
	 * now need to account for the kmem area, which is allocated
	 * before pmap_init() is called.  It is roughly the size of physical
	 * memory.
	 */
	ptextra += physmem;

	/*
	 * The 'pt' (the initial kernel pagetable) has to map the kernel and
	 * the I/O areas. The various I/O areas are mapped (virtually) at
	 * the top of the address space mapped by 'pt' (ie. just below Sysmap).
	 */
	ptpa	= pstart;			/* pre-reloc PA to init PTEs */
	ptsize  = (Sysptsize + howmany(ptextra, NPTEPG)) << PGSHIFT;
	pstart += ptsize;
	vstart += ptsize;
	avail  -= ptsize;

	/*
	 * Sysmap is now placed at the end of Supervisor virtual address space.
	 */
	Sysmap = (pt_entry_t *)SYSMAP_VA;

	/*
	 * Initialize segment tables
	 */
#if defined(M68040) || defined(M68060)
	if (mmutype == MMU_68040)
		mmu040_setup(Sysseg_pa, kstsize, ptpa, ptsize, Sysptmap_pa,
		    reloff);
	else
#endif /* defined(M68040) || defined(M68060) */
		mmu030_setup(Sysseg_pa, kstsize, ptpa, ptsize, Sysptmap_pa,
		    reloff);

	/*
	 * initialize kernel page table page(s).
	 * Assume load at VA 0.
	 * - Text pages are RO
	 * - Page zero is invalid
	 */
	pg_proto = RELOC_PA(reloff, 0) | PG_RO | PG_V;
	pg       = (pt_entry_t *)ptpa;
	*pg++    = PG_NV;

	pg_proto += PAGE_SIZE;
	for (kva = PAGE_SIZE; kva < (vaddr_t)etext; kva += PAGE_SIZE) {
		*pg++ = pg_proto;
		pg_proto += PAGE_SIZE;
	}

	/*
	 * data, bss and dynamic tables are read/write
	 */
	pg_proto = (pg_proto & PG_FRAME) | PG_RW | PG_V;

#if defined(M68040) || defined(M68060)
	/*
	 * Map the kernel segment table cache invalidated for 68040/68060.
	 * (for the 68040 not strictly necessary, but recommended by Motorola;
	 *  for the 68060 mandatory)
	 */
	if (mmutype == MMU_68040) {

		if (kernel_copyback)
			pg_proto |= PG_CCB;

		for (; kva < (vaddr_t)Sysseg; kva += PAGE_SIZE) {
			*pg++ = pg_proto;
			pg_proto += PAGE_SIZE;
		}

		pg_proto = (pg_proto & ~PG_CCB) | PG_CI;
		for (; kva < (vaddr_t)Sysptmap; kva += PAGE_SIZE) {
			*pg++ = pg_proto;
			pg_proto += PAGE_SIZE;
		}

		pg_proto = (pg_proto & ~PG_CI);
		if (kernel_copyback)
			pg_proto |= PG_CCB;
	}
#endif /* defined(M68040) || defined(M68060) */

	/*
	 * go till end of data allocated so far
	 * plus lwp0 u-area (to be allocated)
	 */
	for (; kva < vstart; kva += PAGE_SIZE) {
		*pg++ = pg_proto;
		pg_proto += PAGE_SIZE;
	}

	/*
	 * invalidate remainder of kernel PT
	 */
	epg = (pt_entry_t *)ptpa;
	epg = &epg[ptsize / sizeof(pt_entry_t)];
	while (pg < epg)
		*pg++ = PG_NV;

	/*
	 * Map various I/O areas
	 */
	map_io_areas(ptpa, ptsize, ptextra);

	/*
	 * Map the allocated space in ST-ram now. In the contig-case, there
	 * is no need to make a distinction between virtual and physical
	 * addresses. But I make it anyway to be prepared.
	 * Physical space is already reserved!
	 */
	st_pool_virt = vstart;
	pg           = (pt_entry_t *)ptpa;
	pg           = &pg[vstart / PAGE_SIZE];
	pg_proto     = st_pool_phys | PG_RW | PG_CI | PG_V;
	vstart      += st_pool_size;
	while (pg_proto < (st_pool_phys + st_pool_size)) {
		*pg++     = pg_proto;
		pg_proto += PAGE_SIZE;
	}

	/*
	 * Map physical page_zero and page-zero+1 (First ST-ram page). We need
	 * to reference it in the reboot code. Two pages are mapped, because
	 * we must make sure 'doboot()' is contained in it (see the tricky
	 * copying there....).
	 */
	page_zero  = vstart;
	pg         = (pt_entry_t *)ptpa;
	pg         = &pg[vstart / PAGE_SIZE];
	*pg++      = PG_RW | PG_CI | PG_V;
	vstart    += PAGE_SIZE;
	*pg        = PG_RW | PG_CI | PG_V | PAGE_SIZE;
	vstart    += PAGE_SIZE;

	/*
	 * All necessary STEs and PTEs have been initialized.
	 * Update Sysseg_pa and Sysptmap_pa to point relocated PA.
	 */
	if (reloff) {
		Sysseg_pa   += reloff;
		Sysptmap_pa += reloff;
	}

	bootstrap_vstart = vstart;
	return pstart;
}

/*
 * Do the dull work of mapping the various I/O areas. They MUST be Cache
 * inhibited!
 * All I/O areas are virtually mapped at the end of the pt-table.
 */
static void __attribute__((no_instrument_function))
map_io_areas(paddr_t ptpa, psize_t ptsize, u_int ptextra)
	/* ptsize:	 Size of 'pt' in bytes		*/
	/* ptextra:	 #of additional I/O pte's	*/
{
	vaddr_t		ioaddr;
	pt_entry_t	*pt, *pg, *epg;
	pt_entry_t	pg_proto;
	u_long		mask;

	pt = (pt_entry_t *)ptpa;
	ioaddr = ((ptsize / sizeof(pt_entry_t)) - ptextra) * PAGE_SIZE;

	/*
	 * Map ST-IO area
	 */
	stio_addr = ioaddr;
	ioaddr   += STIO_SIZE;
	pg        = &pt[stio_addr / PAGE_SIZE];
	epg       = &pg[btoc(STIO_SIZE)];
#ifdef _MILANHW_
	/*
	 * Turn on byte swaps in the ST I/O area. On the Milan, the
	 * U0 signal of the MMU controls the BigEndian signal
	 * of the PLX9080. We use this setting so we can read/write the
	 * PLX registers (and PCI-config space) in big-endian mode.
	 */
	pg_proto  = STIO_PHYS | PG_RW | PG_CI | PG_V | PTE40_U0;
#else
	pg_proto  = STIO_PHYS | PG_RW | PG_CI | PG_V;
#endif
	while (pg < epg) {
		*pg++     = pg_proto;
		pg_proto += PAGE_SIZE;
	}

	/*
	 * Map PCI areas
	 */
	if (machineid & ATARI_HADES) {
		/*
		 * Only Hades maps the PCI-config space!
		 */
		pci_conf_addr = ioaddr;
		ioaddr       += PCI_CONFIG_SIZE;
		pg            = &pt[pci_conf_addr / PAGE_SIZE];
		epg           = &pg[btoc(PCI_CONFIG_SIZE)];
		mask          = PCI_CONFM_PHYS;
		pg_proto      = PCI_CONFB_PHYS | PG_RW | PG_CI | PG_V;
		for (; pg < epg; mask <<= 1)
			*pg++ = pg_proto | mask;
	} else
		pci_conf_addr = 0; /* XXX: should crash */

	if (machineid & (ATARI_HADES|ATARI_MILAN)) {
		pci_io_addr   = ioaddr;
		ioaddr       += PCI_IO_SIZE;
		pg	      = &pt[pci_io_addr / PAGE_SIZE];
		epg           = &pg[btoc(PCI_IO_SIZE)];
		pg_proto      = PCI_IO_PHYS | PG_RW | PG_CI | PG_V;
		while (pg < epg) {
			*pg++     = pg_proto;
			pg_proto += PAGE_SIZE;
		}

		pci_mem_addr  = ioaddr;
		/* Provide an uncached PCI address for the MILAN */
		pci_mem_uncached = ioaddr;
		ioaddr       += PCI_MEM_SIZE;
		epg           = &pg[btoc(PCI_MEM_SIZE)];
		pg_proto      = PCI_VGA_PHYS | PG_RW | PG_CI | PG_V;
		while (pg < epg) {
			*pg++     = pg_proto;
			pg_proto += PAGE_SIZE;
		}
	}

	bootm_init(ioaddr, pg, BOOTM_VA_POOL);
	/*
	 * ioaddr += BOOTM_VA_POOL;
	 * pg = &pg[btoc(BOOTM_VA_POOL)];
	 */
}

void __attribute__((no_instrument_function))
mmu030_setup(paddr_t sysseg_pa, u_int kstsize, paddr_t ptpa, psize_t ptsize,
    paddr_t sysptmap_pa, paddr_t kbase)
	/* sysseg_pa:	 System segment table		*/
	/* kstsize:	 size of 'sysseg' in pages	*/
	/* ptpa:	 Kernel page table		*/
	/* ptsize:	 size	of 'pt' in bytes	*/
	/* sysptmap_pa:	 System page table		*/
{
	st_entry_t	sg_proto, *sg, *esg;
	pt_entry_t	pg_proto, *pg, *epg;

	/*
	 * Map the page table pages in both the HW segment table
	 * and the software Sysptmap.
	 */
	sg  = (st_entry_t *)sysseg_pa;
	pg  = (pt_entry_t *)sysptmap_pa;
	epg = &pg[ptsize >> PGSHIFT];
	sg_proto = RELOC_PA(kbase, ptpa) | SG_RW | SG_V;
	pg_proto = RELOC_PA(kbase, ptpa) | PG_RW | PG_CI | PG_V;
	while (pg < epg) {
		*sg++ = sg_proto;
		*pg++ = pg_proto;
		sg_proto += PAGE_SIZE;
		pg_proto += PAGE_SIZE;
	}

	/*
	 * Invalidate the remainder of the tables.
	 */
	esg = (st_entry_t *)sysseg_pa;
	esg = &esg[TIA_SIZE];
	while (sg < esg)
		*sg++ = SG_NV;
	epg = (pt_entry_t *)sysptmap_pa;
	epg = &epg[TIB_SIZE];
	while (pg < epg)
		*pg++ = PG_NV;

	/*
	 * Initialize the PTE for the last one to point Sysptmap.
	 */
	sg = (st_entry_t *)sysseg_pa;
	sg = &sg[SYSMAP_VA >> SEGSHIFT];
	pg = (pt_entry_t *)sysptmap_pa;
	pg = &pg[SYSMAP_VA >> SEGSHIFT];
	*sg = RELOC_PA(kbase, sysptmap_pa) | SG_RW | SG_V;
	*pg = RELOC_PA(kbase, sysptmap_pa) | PG_RW | PG_CI | PG_V;
}

#if defined(M68040) || defined(M68060)
void __attribute__((no_instrument_function))
mmu040_setup(paddr_t sysseg_pa, u_int kstsize, paddr_t ptpa, psize_t ptsize,
    paddr_t sysptmap_pa, paddr_t kbase)
	/* sysseg_pa:	 System segment table		*/
	/* kstsize:	 size of 'sysseg' in pages	*/
	/* ptpa:	 Kernel page table		*/
	/* ptsize:	 size	of 'pt' in bytes	*/
	/* sysptmap_pa:	 System page table		*/
{
	int		nl1desc, nl2desc, i;
	st_entry_t	sg_proto, *sg, *esg;
	pt_entry_t	pg_proto, *pg, *epg;

	/*
	 * First invalidate the entire "segment table" pages
	 * (levels 1 and 2 have the same "invalid" values).
	 */
	sg  = (st_entry_t *)sysseg_pa;
	esg = &sg[kstsize * NPTEPG];
	while (sg < esg)
		*sg++ = SG_NV;

	/*
	 * Initialize level 2 descriptors (which immediately
	 * follow the level 1 table).
	 * We need:
	 *	NPTEPG / SG4_LEV3SIZE
	 * level 2 descriptors to map each of the nptpages
	 * pages of PTEs.  Note that we set the "used" bit
	 * now to save the HW the expense of doing it.
	 */
	nl2desc = (ptsize >> PGSHIFT) * (NPTEPG / SG4_LEV3SIZE);
	sg  = (st_entry_t *)sysseg_pa;
	sg  = &sg[SG4_LEV1SIZE];
	esg = &sg[nl2desc];
	sg_proto = RELOC_PA(kbase, ptpa) | SG_U | SG_RW | SG_V;
	while (sg < esg) {
		*sg++     = sg_proto;
		sg_proto += (SG4_LEV3SIZE * sizeof(st_entry_t));
	}

	/*
	 * Initialize level 1 descriptors.  We need:
	 *	howmany(nl2desc, SG4_LEV2SIZE)
	 * level 1 descriptors to map the 'nl2desc' level 2's.
	 */
	nl1desc = howmany(nl2desc, SG4_LEV2SIZE);
	sg  = (st_entry_t *)sysseg_pa;
	esg = &sg[nl1desc];
	sg_proto = RELOC_PA(kbase, (paddr_t)&sg[SG4_LEV1SIZE])
	    | SG_U | SG_RW | SG_V;
	while (sg < esg) {
		*sg++     = sg_proto;
		sg_proto += (SG4_LEV2SIZE * sizeof(st_entry_t));
	}

	/* Sysmap is last entry in level 1 */
	sg  = (st_entry_t *)sysseg_pa;
	sg  = &sg[SG4_LEV1SIZE - 1];
	*sg = sg_proto;

	/*
	 * Kernel segment table at end of next level 2 table
	 */
	i = SG4_LEV1SIZE + (nl1desc * SG4_LEV2SIZE);
	sg  = (st_entry_t *)sysseg_pa;
	sg  = &sg[i + SG4_LEV2SIZE - (NPTEPG / SG4_LEV3SIZE)];
	esg = &sg[NPTEPG / SG4_LEV3SIZE];
	sg_proto = RELOC_PA(kbase, sysptmap_pa) | SG_U | SG_RW | SG_V;
	while (sg < esg) {
		*sg++ = sg_proto;
		sg_proto += (SG4_LEV3SIZE * sizeof(st_entry_t));
	}

	/* Include additional level 2 table for Sysmap in protostfree */
	protostfree = (~0U << (1 + nl1desc + 1)) /* & ~(~0U << MAXKL2SIZE) */;

	/*
	 * Initialize Sysptmap
	 */
	pg  = (pt_entry_t *)sysptmap_pa;
	epg = &pg[ptsize >> PGSHIFT];
	pg_proto = RELOC_PA(kbase, ptpa) | PG_RW | PG_CI | PG_V;
	while (pg < epg) {
		*pg++ = pg_proto;
		pg_proto += PAGE_SIZE;
	}

	/*
	 * Invalidate rest of Sysptmap page.
	 */
	epg = (pt_entry_t *)sysptmap_pa;
	epg = &epg[TIB_SIZE];
	while (pg < epg)
		*pg++ = PG_NV;

	/*
	 * Initialize the PTE for the last one to point Sysptmap.
	 */
	pg = (pt_entry_t *)sysptmap_pa;
	pg = &pg[SYSMAP_VA >> SEGSHIFT];
	*pg = RELOC_PA(kbase, sysptmap_pa) | PG_RW | PG_CI | PG_V;
}
#endif /* M68040 */

#ifdef DEBUG
void dump_segtable(u_int *);
void dump_pagetable(u_int *, u_int, u_int);

void
dump_segtable(u_int *stp)
{
	u_int *s, *es;
	int shift, i;

	s = stp;
	{
		es = s + (M68K_STSIZE >> 2);
		shift = SG_ISHIFT;
	}

	/*
	 * XXX need changes for 68040
	 */
	for (i = 0; s < es; s++, i++)
		if (*s & SG_V)
			printf("$%08x: $%08x\t", i << shift, *s & SG_FRAME);
	printf("\n");
}

void
dump_pagetable(u_int *ptp, u_int i, u_int n)
{
	u_int *p, *ep;

	p = ptp + i;
	ep = p + n;
	for (; p < ep; p++, i++)
		if (*p & PG_V)
			printf("$%08x -> $%08x\t", i, *p & PG_FRAME);
	printf("\n");
}

#endif

/*
 *	Bootstrap the system enough to run with virtual memory.
 *
 *	This is called after mapping has already been enabled
 *	and just syncs the pmap module with what has already been done.
 */
void __attribute__((no_instrument_function))
pmap_bootstrap(void)
{
	vaddr_t	va;
	vaddr_t vstart = bootstrap_vstart;
	int	i;

	/*
	 * Announce page-size to the VM-system
	 */
	uvmexp.pagesize = NBPG;
	uvm_md_init();

	/*
	 * Setup physical address ranges
	 */
	for (i = 0; i < NMEM_SEGS && usable_segs[i].start; i++)
		continue;
	/* XXX: allow for msgbuf */
	usable_segs[i - 1].end -= m68k_round_page(MSGBUFSIZE);
	msgbufpa = usable_segs[i - 1].end;

	/*
	 * Count physical memory
	 */
	extern vsize_t mem_size;
	mem_size = 0;
	for (i = 0; i < NMEM_SEGS; i++) {
		if (boot_segs[i].start == boot_segs[i].end)
			break;
		mem_size += boot_segs[i].end - boot_segs[i].start;
	}

	/*
	 * Announce available memory to the VM-system
	 */
	for (i = 0; i < NMEM_SEGS && usable_segs[i].start; i++)
		uvm_page_physload(atop(usable_segs[i].start),
				 atop(usable_segs[i].end),
				 atop(usable_segs[i].start),
				 atop(usable_segs[i].end),
				 usable_segs[i].free_list);

	avail_start = usable_segs[0].start;
	avail_end   = usable_segs[i - 1].end;

	virtual_avail = vstart;
	virtual_end   = VM_MAX_KERNEL_ADDRESS;

	/*
	 * Allocate all the submaps we need
	 */
#define	SYSMAP(c, v, n)	\
	v = (c)va; va += ((n)*PAGE_SIZE);

	va = virtual_avail;

	SYSMAP(void *   ,CADDR1	,1			)
	SYSMAP(void *   ,CADDR2	,1			)
	SYSMAP(void *   ,vmmap	,1			)
	SYSMAP(void *	,msgbufaddr ,btoc(MSGBUFSIZE)	)

	DCIS();

	virtual_avail = reserve_dumppages(va);
}
