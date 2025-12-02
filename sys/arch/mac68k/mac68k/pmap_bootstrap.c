/*	$NetBSD: pmap_bootstrap.c,v 1.113 2025/12/02 02:26:18 thorpej Exp $	*/

/* 
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	@(#)pmap_bootstrap.c	8.1 (Berkeley) 6/10/93
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pmap_bootstrap.c,v 1.113 2025/12/02 02:26:18 thorpej Exp $");

#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_m68k_arch.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/reboot.h>

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/pte.h>
#include <machine/vmparam.h>
#include <machine/pmap.h>
#include <machine/autoconf.h>
#include <machine/video.h>

#include <mac68k/mac68k/macrom.h>

#define PA2VA(v, t)	(t)((u_int)(v) - firstpa)

extern char *etext;
extern char *extiobase;

extern vaddr_t kernel_reloc_offset;

extern int vidlen;
extern vaddr_t newvideoaddr;

extern void *	ROMBase;

/*
 * Special purpose kernel virtual addresses, used for mapping
 * physical pages for a variety of temporary or permanent purposes:
 *
 *	CADDR1, CADDR2:	pmap zero/copy operations
 *	vmmap:		/dev/mem, crash dumps, parity error checking
 *	msgbufaddr:	kernel message buffer
 */
void *CADDR1, *CADDR2;
char *vmmap;
void *msgbufaddr;

/*
 * Bootstrap the VM system.
 *
 * This is called with the MMU either on or off.  If it's on, we assume
 * that it's mapped with the same PA <=> LA mapping that we eventually
 * want.  The page sizes and the protections will be wrong, anyway.
 *
 * nextpa is the first address following the loaded kernel.
 */
paddr_t
pmap_bootstrap1(paddr_t nextpa, paddr_t firstpa)
{
	paddr_t lwp0upa, kstpa, kptmpa, kptpa;
	u_int nptpages, kstsize;
	int i;
	st_entry_t protoste, *ste, *este;
	pt_entry_t protopte, *pte, *epte;
	u_int stfree = 0;	/* XXX: gcc -Wuninitialized */
	extern char start[];

	nextpa = m68k_round_page(nextpa);

	/*
	 * Calculate important physical addresses:
	 *
	 *	lwp0upa		lwp0 u-area		UPAGES pages
	 *
	 *	kstpa		kernel segment table	1 page (!040)
	 *						N pages (040)
	 *
	 *	kptmpa		kernel PT map		1 page
	 *
	 *	kptpa		statically allocated
	 *			kernel PT pages		Sysptsize+ pages
	 *
	 * [ Sysptsize is the number of pages of PT, and IIOMAPSIZE and
	 *   NBMAPSIZE are the number of PTEs, hence we need to round
	 *   the total to a page boundary with IO maps at the end. ]
	 *
	 */
	kernel_reloc_offset = firstpa;

	lwp0upa = nextpa;
	nextpa += USPACE;
	if (mmutype == MMU_68040)
		kstsize = MAXKL2SIZE / (NPTEPG/SG4_LEV2SIZE);
	else
		kstsize = 1;
	kstpa = nextpa;
	nextpa += kstsize * PAGE_SIZE;
	kptmpa = nextpa;
	nextpa += PAGE_SIZE;
	kptpa = nextpa;
	nptpages = Sysptsize +
		(IIOMAPSIZE + ROMMAPSIZE + btoc(vidlen) + NPTEPG - 1) / NPTEPG;
	/*
	 * New kmem arena is allocated prior to pmap_init(), so we need
	 * additiona PT pages to account for that allocation, which is based
	 * on physical memory size.  Just sum up memory and add enough PT
	 * pages for that size.
	 */
	nptpages += howmany(physmem, NPTEPG);
	nptpages++;
	nextpa += nptpages * PAGE_SIZE;

	pmap_machine_check_bootstrap_allocations(nextpa, firstpa);

	/*
	 * Initialize segment table and kernel page table map.
	 *
	 * On 68030s and earlier MMUs the two are identical except for
	 * the valid bits so both are initialized with essentially the
	 * same values.  On the 68040, which has a mandatory 3-level
	 * structure, the segment table holds the level 1 table and part
	 * (or all) of the level 2 table and hence is considerably
	 * different.  Here the first level consists of 128 descriptors
	 * (512 bytes) each mapping 32mb of address space.  Each of these
	 * points to blocks of 128 second level descriptors (512 bytes)
	 * each mapping 256kb.  Note that there may be additional "segment
	 * table" pages depending on how large MAXKL2SIZE is.
	 *
	 * Portions of the last segment of KVA space (0xFFC00000 -
	 * 0xFFFFFFFF) are mapped for the kernel page tables.
	 *
	 * XXX cramming two levels of mapping into the single "segment"
	 * table on the 68040 is intended as a temporary hack to get things
	 * working.  The 224mb of address space that this allows will most
	 * likely be insufficient in the future (at least for the kernel).
	 */
	if (mmutype == MMU_68040) {
		int nl1desc, nl2desc;

		/*
		 * First invalidate the entire "segment table" pages
		 * (levels 1 and 2 have the same "invalid" value).
		 */
		ste = PA2VA(kstpa, st_entry_t *);
		este = &ste[kstsize * NPTEPG];
		while (ste < este)
			*ste++ = SG_NV;
		/*
		 * Initialize level 2 descriptors (which immediately
		 * follow the level 1 table).  We need:
		 *	NPTEPG / SG4_LEV3SIZE
		 * level 2 descriptors to map each of the nptpages
		 * pages of PTEs.  Note that we set the "used" bit
		 * now to save the HW the expense of doing it.
		 */
		nl2desc = nptpages * (NPTEPG / SG4_LEV3SIZE);
		ste = PA2VA(kstpa, st_entry_t *);
		ste = &ste[SG4_LEV1SIZE];
		este = &ste[nl2desc];
		protoste = kptpa | SG_U | SG_RW | SG_V;
		while (ste < este) {
			*ste++ = protoste;
			protoste += (SG4_LEV3SIZE * sizeof(st_entry_t));
		}
		/*
		 * Initialize level 1 descriptors.  We need:
		 *	howmany(nl2desc, SG4_LEV2SIZE)
		 * level 1 descriptors to map the `nl2desc' level 2's.
		 */
		nl1desc = howmany(nl2desc, SG4_LEV2SIZE);
		ste = PA2VA(kstpa, u_int *);
		este = &ste[nl1desc];
		protoste = (paddr_t)&ste[SG4_LEV1SIZE] | SG_U | SG_RW | SG_V;
		while (ste < este) {
			*ste++ = protoste;
			protoste += (SG4_LEV2SIZE * sizeof(st_entry_t));
		}
		/*
		 * Initialize the final level 1 descriptor to map the next
		 * block of level 2 descriptors for Sysptmap.
		 */
		ste = PA2VA(kstpa, st_entry_t *);
		ste = &ste[SG4_LEV1SIZE - 1];
		*ste = protoste;
		/*
		 * Now initialize the final portion of that block of
		 * descriptors to map Sysmap.
		 */
		i = SG4_LEV1SIZE + (nl1desc * SG4_LEV2SIZE);
		ste = PA2VA(kstpa, st_entry_t *);
		ste = &ste[i + SG4_LEV2SIZE - (NPTEPG / SG4_LEV3SIZE)];
		este = &ste[NPTEPG / SG4_LEV3SIZE];
		protoste = kptmpa | SG_U | SG_RW | SG_V;
		while (ste < este) {
			*ste++ = protoste;
			protoste += (SG4_LEV3SIZE * sizeof(st_entry_t));
		}
		/*
		 * Calculate the free level 2 descriptor mask
		 * noting that we have used:
		 *	0:		level 1 table
		 *	1 to nl1desc:	map page tables
		 *	nl1desc + 1:	maps kptmpa and last-page page table
		 */
		/* mark an entry for level 1 table */
		stfree = ~l2tobm(0);
		/* mark entries for map page tables */
		for (i = 1; i <= nl1desc; i++)
			stfree &= ~l2tobm(i);
		/* mark an entry for kptmpa and lkptpa */
		stfree &= ~l2tobm(i);
		/* mark entries not available */
		for (i = MAXKL2SIZE; i < sizeof(stfree) * NBBY; i++)
			stfree &= ~l2tobm(i);

		/*
		 * Initialize Sysptmap
		 */
		pte = PA2VA(kptmpa, pt_entry_t *);
		epte = &pte[nptpages];
		protopte = kptpa | PG_RW | PG_CI | PG_V;
		while (pte < epte) {
			*pte++ = protopte;
			protopte += PAGE_SIZE;
		}
		/*
		 * Invalidate all remaining entries.
		 */
		epte = PA2VA(kptmpa, pt_entry_t *);
		epte = &epte[TIB_SIZE];
		while (pte < epte) {
			*pte++ = PG_NV;
		}
		/*
		 * Initialize the last one to point to Sysptmap.
		 */
		pte = PA2VA(kptmpa, pt_entry_t *);
		pte = &pte[SYSMAP_VA >> SEGSHIFT];
		*pte = kptmpa | PG_RW | PG_CI | PG_V;
	} else {
		/*
		 * Map the page table pages in both the HW segment table
		 * and the software Sysptmap.
		 */
		ste = PA2VA(kstpa, st_entry_t *);
		pte = PA2VA(kptmpa, pt_entry_t *);
		epte = &pte[nptpages];
		protoste = kptpa | SG_RW | SG_V;
		protopte = kptpa | PG_RW | PG_CI | PG_V;
		while (pte < epte) {
			*ste++ = protoste;
			*pte++ = protopte;
			protoste += PAGE_SIZE;
			protopte += PAGE_SIZE;
		}
		/*
		 * Invalidate all remaining entries in both.
		 */
		este = PA2VA(kstpa, st_entry_t *);
		este = &este[TIA_SIZE];
		while (ste < este)
			*ste++ = SG_NV;
		epte = PA2VA(kptmpa, pt_entry_t *);
		epte = &epte[TIB_SIZE];
		while (pte < epte)
			*pte++ = PG_NV;
		/*
		 * Initialize the last one to point to Sysptmap.
		 */
		ste = PA2VA(kstpa, st_entry_t *);
		ste = &ste[SYSMAP_VA >> SEGSHIFT];
		pte = PA2VA(kptmpa, pt_entry_t *);
		pte = &pte[SYSMAP_VA >> SEGSHIFT];
		*ste = kptmpa | SG_RW | SG_V;
		*pte = kptmpa | PG_RW | PG_CI | PG_V;
	}

	/*
	 * Initialize kernel page table.
	 * Start by invalidating the `nptpages' that we have allocated.
	 */
	pte = PA2VA(kptpa, pt_entry_t *);
	epte = &pte[nptpages * NPTEPG];
	while (pte < epte)
		*pte++ = PG_NV;
	/*
	 * Validate PTEs for kernel text (RO).
	 * Pages up to "start" (vectors and Mac OS global variable space)
	 * must be writable for the ROM.
	 */
	pte = PA2VA(kptpa, pt_entry_t *);
	pte = &pte[m68k_btop(KERNBASE)];
	epte = &pte[m68k_btop(m68k_round_page(start))];
	protopte = firstpa | PG_RW | PG_V;
	while (pte < epte) {
		*pte++ = protopte;
		protopte += PAGE_SIZE;
	}
	epte = &pte[m68k_btop(m68k_trunc_page(&etext))];
	protopte = (protopte & ~PG_PROT) | PG_RO;
	while (pte < epte) {
		*pte++ = protopte;
		protopte += PAGE_SIZE;
	}
	/*
	 * Validate PTEs for kernel data/bss, dynamic data allocated
	 * by us so far (kstpa - firstpa bytes), and pages for lwp0
	 * u-area and page table allocated below (RW).
	 */
	epte = PA2VA(kptpa, pt_entry_t *);
	epte = &epte[m68k_btop(kstpa - firstpa)];
	protopte = (protopte & ~PG_PROT) | PG_RW;
	/*
	 * Enable copy-back caching of data pages
	 */
	if (mmutype == MMU_68040)
		protopte |= PG_CCB;
	while (pte < epte) {
		*pte++ = protopte;
		protopte += PAGE_SIZE;
	}
	/*
	 * Map the kernel segment table cache invalidated for 68040/68060.
	 * (for the 68040 not strictly necessary, but recommended by Motorola;
	 *  for the 68060 mandatory)
	 */
	epte = PA2VA(kptpa, pt_entry_t *);
	epte = &epte[m68k_btop(nextpa - firstpa)];
	protopte = (protopte & ~PG_PROT) | PG_RW;
	if (mmutype == MMU_68040) {
		protopte &= ~PG_CCB;
		protopte |= PG_CIN;
	}
	while (pte < epte) {
		*pte++ = protopte;
		protopte += PAGE_SIZE;
	}

	/*
	 * Finally, validate the internal IO space PTEs (RW+CI).
	 */

#define	PTE2VA(pte)	m68k_ptob(pte - PA2VA(kptpa, pt_entry_t *))

	protopte = IOBase | PG_RW | PG_CI | PG_V;
	IOBase = PTE2VA(pte);
	epte = &pte[IIOMAPSIZE];
	while (pte < epte) {
		*pte++ = protopte;
		protopte += PAGE_SIZE;
	}

	protopte = (pt_entry_t)ROMBase | PG_RO | PG_V;
	ROMBase = (void *)PTE2VA(pte);
	epte = &pte[ROMMAPSIZE];
	while (pte < epte) {
		*pte++ = protopte;
		protopte += PAGE_SIZE;
	}

	if (vidlen) {
		protopte = m68k_trunc_page(mac68k_video.mv_phys) |
		    PG_RW | PG_V | PG_CI;
		newvideoaddr = PTE2VA(pte)
		    + m68k_page_offset(mac68k_video.mv_phys);
		epte = &pte[btoc(vidlen)];
		while (pte < epte) {
			*pte++ = protopte;
			protopte += PAGE_SIZE;
		}
	}
	virtual_avail = PTE2VA(pte);

	/*
	 * Calculate important exported kernel addresses and related values.
	 */
	/*
	 * Sysseg: base of kernel segment table
	 */
	Sysseg = PA2VA(kstpa, st_entry_t *);
	Sysseg_pa = kstpa;
#if defined(M68040)
	if (mmutype == MMU_68040)
		protostfree = stfree;
#endif
	/*
	 * Sysptmap: base of kernel page table map
	 */
	Sysptmap = PA2VA(kptmpa, pt_entry_t *);
	/*
	 * Sysmap: kernel page table (as mapped through Sysptmap)
	 * Allocated at the end of KVA space.
	 */
	Sysmap = (pt_entry_t *)SYSMAP_VA;

	/*
	 * Remember the u-area address so it can be loaded in the lwp0
	 * via uvm_lwp_setuarea() later in pmap_bootstrap2().
	 */
	lwp0uarea = PA2VA(lwp0upa, vaddr_t);

	virtual_end = VM_MAX_KERNEL_ADDRESS;

	/*
	 * Allocate some fixed, special purpose kernel virtual addresses
	 */
	{
		vaddr_t va = virtual_avail;

		CADDR1 = (void *)va;
		va += PAGE_SIZE;
		CADDR2 = (void *)va;
		va += PAGE_SIZE;
		vmmap = (void *)va;
		va += PAGE_SIZE;
		msgbufaddr = (void *)va;
		va += m68k_round_page(MSGBUFSIZE);
		virtual_avail = va;
	}

	return nextpa;
}
