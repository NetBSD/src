/*	$NetBSD: pmap_bootstrap.c,v 1.57.6.1 2014/08/20 00:03:00 tls Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: pmap_bootstrap.c,v 1.57.6.1 2014/08/20 00:03:00 tls Exp $");

#include <sys/param.h>
#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/hp300spu.h>
#include <machine/pte.h>
#include <machine/vmparam.h>

#include <hp300/hp300/clockreg.h>

#define RELOC(v, t)	*((t*)((uintptr_t)&(v) + firstpa))

extern char *etext;
extern vaddr_t CLKbase, MMUbase;

extern int maxmem, physmem;
extern paddr_t avail_start, avail_end;

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

void pmap_bootstrap(paddr_t, paddr_t);

/*
 * Bootstrap the VM system.
 *
 * Called with MMU off so we must relocate all global references by `firstpa'
 * (don't call any functions here!)  `nextpa' is the first available physical
 * memory address.  Returns an updated first PA reflecting the memory we
 * have allocated.  MMU is still off when we return.
 *
 * XXX assumes sizeof(u_int) == sizeof(pt_entry_t)
 * XXX a PIC compiler would make this much easier.
 */
void
pmap_bootstrap(paddr_t nextpa, paddr_t firstpa)
{
	paddr_t lwp0upa, kstpa, kptmpa, kptpa;
	paddr_t lkptpa;
	u_int nptpages, kstsize;
	st_entry_t protoste, *ste, *este;
	pt_entry_t protopte, *pte, *epte;
	u_int stfree = 0;	/* XXX: gcc -Wuninitialized */

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
	 *	lkptpa		last kernel PT page	1 page
	 *
	 *	kptpa		statically allocated
	 *			kernel PT pages		Sysptsize+ pages
	 *
	 * [ Sysptsize is the number of pages of PT, and IIOMAPSIZE and
	 *   EIOMAPSIZE are the number of PTEs, hence we need to round
	 *   the total to a page boundary with IO maps at the end. ]
	 *
	 * The KVA corresponding to any of these PAs is:
	 *	(PA - firstpa + KERNBASE).
	 */
	lwp0upa = nextpa;
	nextpa += USPACE;
	if (RELOC(mmutype, int) == MMU_68040)
		kstsize = MAXKL2SIZE / (NPTEPG/SG4_LEV2SIZE);
	else
		kstsize = 1;
	kstpa = nextpa;
	nextpa += kstsize * PAGE_SIZE;
	kptmpa = nextpa;
	nextpa += PAGE_SIZE;
	lkptpa = nextpa;
	nextpa += PAGE_SIZE;
	kptpa = nextpa;
	nptpages = RELOC(Sysptsize, int) + howmany(RELOC(physmem, int), NPTEPG) +
		(IIOMAPSIZE + EIOMAPSIZE + NPTEPG - 1) / NPTEPG;
	nextpa += nptpages * PAGE_SIZE;

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
	 * Portions of the last two segment of KVA space (0xFF800000 -
	 * 0xFFFFFFFF) are mapped for a couple of purposes.
	 * The first segment (0xFF800000 - 0xFFBFFFFF) is mapped
	 * for the kernel page tables.
	 * The very last page (0xFFFFF000) in the second segment is mapped
	 * to the last physical page of RAM to give us a region in which
	 * PA == VA.  We use the first part of this page for enabling
	 * and disabling mapping.  The last part of this page also contains
	 * info left by the boot ROM.
	 *
	 * XXX cramming two levels of mapping into the single "segment"
	 * table on the 68040 is intended as a temporary hack to get things
	 * working.  The 224mb of address space that this allows will most
	 * likely be insufficient in the future (at least for the kernel).
	 */
	if (RELOC(mmutype, int) == MMU_68040) {
		int nl1desc, nl2desc, i;

		/*
		 * First invalidate the entire "segment table" pages
		 * (levels 1 and 2 have the same "invalid" value).
		 */
		ste = (st_entry_t *)kstpa;
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
		ste = (st_entry_t *)kstpa;
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
		ste = (st_entry_t *)kstpa;
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
		ste = (st_entry_t *)kstpa;
		ste = &ste[SG4_LEV1SIZE - 1];
		*ste = protoste;
		/*
		 * Now initialize the final portion of that block of
		 * descriptors to map Sysmap and the "last PT page".
		 */
		i = SG4_LEV1SIZE + (nl1desc * SG4_LEV2SIZE);
		ste = (st_entry_t *)kstpa;
		ste = &ste[i + SG4_LEV2SIZE - (NPTEPG / SG4_LEV3SIZE) * 2];
		este = &ste[NPTEPG / SG4_LEV3SIZE];
		protoste = kptmpa | SG_U | SG_RW | SG_V;
		while (ste < este) {
			*ste++ = protoste;
			protoste += (SG4_LEV3SIZE * sizeof(st_entry_t));
		}
		este = &ste[NPTEPG / SG4_LEV3SIZE];
		protoste = lkptpa | SG_U | SG_RW | SG_V;
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
		pte = (pt_entry_t *)kptmpa;
		epte = &pte[nptpages];
		protopte = kptpa | PG_RW | PG_CI | PG_V;
		while (pte < epte) {
			*pte++ = protopte;
			protopte += PAGE_SIZE;
		}
		/*
		 * Invalidate all remaining entries.
		 */
		epte = (pt_entry_t *)kptmpa;
		epte = &epte[TIB_SIZE];
		while (pte < epte) {
			*pte++ = PG_NV;
		}
		/*
		 * Initialize the last ones to point to Sysptmap and the page
		 * table page allocated earlier.
		 */
		pte = (pt_entry_t *)kptmpa;
		pte = &pte[SYSMAP_VA >> SEGSHIFT];
		*pte = kptmpa | PG_RW | PG_CI | PG_V;
		pte = (pt_entry_t *)kptmpa;
		pte = &pte[MAXADDR >> SEGSHIFT];
		*pte = lkptpa | PG_RW | PG_CI | PG_V;
	} else {
		/*
		 * Map the page table pages in both the HW segment table
		 * and the software Sysptmap.
		 */
		ste = (st_entry_t *)kstpa;
		pte = (pt_entry_t *)kptmpa;
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
		este = (st_entry_t *)kstpa;
		este = &este[TIA_SIZE];
		while (ste < este)
			*ste++ = SG_NV;
		epte = (pt_entry_t *)kptmpa;
		epte = &epte[TIB_SIZE];
		while (pte < epte)
			*pte++ = PG_NV;
		/*
		 * Initialize the last ones to point to Sysptmap and the page
		 * table page allocated earlier.
		 */
		ste = (st_entry_t *)kstpa;
		ste = &ste[SYSMAP_VA >> SEGSHIFT];
		pte = (pt_entry_t *)kptmpa;
		pte = &pte[SYSMAP_VA >> SEGSHIFT];
		*ste = kptmpa | SG_RW | SG_V;
		*pte = kptmpa | PG_RW | PG_CI | PG_V;
		ste = (st_entry_t *)kstpa;
		ste = &ste[MAXADDR >> SEGSHIFT];
		pte = (pt_entry_t *)kptmpa;
		pte = &pte[MAXADDR >> SEGSHIFT];
		*ste = lkptpa | SG_RW | SG_V;
		*pte = lkptpa | PG_RW | PG_CI | PG_V;
	}

	/*
	 * Invalidate all but the final entry in the last kernel PT page.
	 * The final entry maps the last page of physical memory to
	 * prepare a page that is PA == VA to turn on the MMU.
	 */
	pte = (pt_entry_t *)lkptpa;
	epte = &pte[NPTEPG - 1];
	while (pte < epte)
		*pte++ = PG_NV;
	*pte = MAXADDR | PG_RW | PG_CI | PG_V;
	/*
	 * Initialize kernel page table.
	 * Start by invalidating the `nptpages' that we have allocated.
	 */
	pte = (pt_entry_t *)kptpa;
	epte = &pte[nptpages * NPTEPG];
	while (pte < epte)
		*pte++ = PG_NV;
	/*
	 * Validate PTEs for kernel text (RO).
	 * The first page of kernel text remains invalid; see locore.s
	 */
	pte = (pt_entry_t *)kptpa;
	pte = &pte[m68k_btop(KERNBASE + PAGE_SIZE)];
	epte = &pte[m68k_btop(m68k_trunc_page(&etext))];
	protopte = (firstpa + PAGE_SIZE) | PG_RO | PG_V;
	while (pte < epte) {
		*pte++ = protopte;
		protopte += PAGE_SIZE;
	}
	/*
	 * Validate PTEs for kernel data/bss, dynamic data allocated
	 * by us so far (kstpa - firstpa bytes), and pages for lwp0
	 * u-area and page table allocated below (RW).
	 */
	epte = (pt_entry_t *)kptpa;
	epte = &epte[m68k_btop(kstpa - firstpa)];
	protopte = (protopte & ~PG_PROT) | PG_RW;
	/*
	 * Enable copy-back caching of data pages
	 */
	if (RELOC(mmutype, int) == MMU_68040)
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
	epte = (pt_entry_t *)kptpa;
	epte = &epte[m68k_btop(nextpa - firstpa)];
	protopte = (protopte & ~PG_PROT) | PG_RW;
	if (RELOC(mmutype, int) == MMU_68040) {
		protopte &= ~PG_CCB;
		protopte |= PG_CIN;
	}
	while (pte < epte) {
		*pte++ = protopte;
		protopte += PAGE_SIZE;
	}

	/*
	 * Finally, validate the internal IO space PTEs (RW+CI).
	 * We do this here since the 320/350 MMU registers (also
	 * used, but to a lesser extent, on other models) are mapped
	 * in this range and it would be nice to be able to access
	 * them after the MMU is turned on.
	 */

#define	PTE2VA(pte)	m68k_ptob(pte - ((pt_entry_t *)kptpa))

	protopte = INTIOBASE | PG_RW | PG_CI | PG_V;
	epte = &pte[IIOMAPSIZE];
	RELOC(intiobase, uint8_t *) = (uint8_t *)PTE2VA(pte);
	RELOC(intiolimit, uint8_t *) = (uint8_t *)PTE2VA(epte);
	while (pte < epte) {
		*pte++ = protopte;
		protopte += PAGE_SIZE;
	}
	RELOC(extiobase, uint8_t *) = (uint8_t *)PTE2VA(pte);
	pte += EIOMAPSIZE;
	RELOC(virtual_avail, vaddr_t) = PTE2VA(pte);

	/*
	 * Calculate important exported kernel addresses and related values.
	 */
	/*
	 * Sysseg: base of kernel segment table
	 */
	RELOC(Sysseg, st_entry_t *) = (st_entry_t *)(kstpa - firstpa);
	RELOC(Sysseg_pa, paddr_t) = kstpa;
#if defined(M68040)
	if (RELOC(mmutype, int) == MMU_68040)
		RELOC(protostfree, u_int) = stfree;
#endif
	/*
	 * Sysptmap: base of kernel page table map
	 */
	RELOC(Sysptmap, pt_entry_t *) = (pt_entry_t *)(kptmpa - firstpa);
	/*
	 * Sysmap: kernel page table (as mapped through Sysptmap)
	 * Allocated at the end of KVA space.
	 */
	RELOC(Sysmap, pt_entry_t *) = (pt_entry_t *)SYSMAP_VA;
	/*
	 * CLKbase, MMUbase: important registers in internal IO space
	 * accessed from assembly language.
	 */
	RELOC(CLKbase, vaddr_t) =
		(vaddr_t)RELOC(intiobase, char *) + CLKBASE;
	RELOC(MMUbase, vaddr_t) =
		(vaddr_t)RELOC(intiobase, char *) + MMUBASE;

	/*
	 * Remember the u-area address so it can be loaded in the lwp0
	 * via uvm_lwp_setuarea() later in pmap_bootstrap_finalize().
	 */
	RELOC(lwp0uarea, vaddr_t) = lwp0upa - firstpa;

	/*
	 * VM data structures are now initialized, set up data for
	 * the pmap module.
	 *
	 * Note about avail_end: msgbuf is initialized just after
	 * avail_end in machdep.c.
	 * Since the last page is used for rebooting the system
	 * (code is copied there and excution continues from copied code
	 * before the MMU is disabled), the msgbuf will get trounced
	 * between reboots if it's placed in the last physical page.
	 * To work around this, we move avail_end back one more
	 * page so the msgbuf can be preserved.
	 */
	RELOC(avail_start, paddr_t) = nextpa;
	RELOC(avail_end, paddr_t) = m68k_ptob(RELOC(maxmem, int)) -
	    (m68k_round_page(MSGBUFSIZE) + m68k_ptob(1));
	RELOC(mem_size, vsize_t) = m68k_ptob(RELOC(physmem, int));

	RELOC(virtual_end, vaddr_t) = VM_MAX_KERNEL_ADDRESS;

	/*
	 * Allocate some fixed, special purpose kernel virtual addresses
	 */
	{
		vaddr_t va = RELOC(virtual_avail, vaddr_t);

		RELOC(CADDR1, void *) = (void *)va;
		va += PAGE_SIZE;
		RELOC(CADDR2, void *) = (void *)va;
		va += PAGE_SIZE;
		RELOC(vmmap, void *) = (void *)va;
		va += PAGE_SIZE;
		RELOC(msgbufaddr, void *) = (void *)va;
		va += m68k_round_page(MSGBUFSIZE);
		RELOC(virtual_avail, vaddr_t) = va;
	}
}
