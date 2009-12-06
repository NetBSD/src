/*	$NetBSD: pmap_bootstrap.c,v 1.86 2009/12/06 06:41:30 tsutsui Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: pmap_bootstrap.c,v 1.86 2009/12/06 06:41:30 tsutsui Exp $");

#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "zsc.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/reboot.h>

#include <uvm/uvm_extern.h>

#include <machine/pte.h>
#include <machine/vmparam.h>
#include <machine/cpu.h>
#include <machine/pmap.h>
#include <machine/autoconf.h>
#include <machine/video.h>

#include <mac68k/mac68k/macrom.h>

#define PA2VA(v, t)	(t)((u_int)(v) - firstpa)

extern char *etext;
extern char *extiobase;

extern paddr_t avail_start;
extern paddr_t avail_end;

#if NZSC > 0
extern	int	zsinited;
#endif

/*
 * These are used to map the RAM:
 */
int	numranges;	/* = 0 == don't use the ranges */
u_long	low[8];
u_long	high[8];
u_long	maxaddr;	/* PA of the last physical page */
int	vidlen;
#define VIDMAPSIZE	btoc(vidlen)
static vaddr_t	newvideoaddr;

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

void	pmap_bootstrap(paddr_t, paddr_t);
void	bootstrap_mac68k(int);

/*
 * Bootstrap the VM system.
 *
 * This is called with the MMU either on or off.  If it's on, we assume
 * that it's mapped with the same PA <=> LA mapping that we eventually
 * want.  The page sizes and the protections will be wrong, anyway.
 *
 * nextpa is the first address following the loaded kernel.  On a IIsi
 * on 12 May 1996, that was 0xf9000 beyond firstpa.
 */
void
pmap_bootstrap(paddr_t nextpa, paddr_t firstpa)
{
	paddr_t kstpa, kptpa, kptmpa, lwp0upa;
	u_int nptpages, kstsize;
	paddr_t avail_next;
	int avail_remaining;
	int avail_range;
	int i;
	st_entry_t protoste, *ste, *este;
	pt_entry_t protopte, *pte, *epte;
	u_int stfree = 0;	/* XXX: gcc -Wuninitialized */
	extern char start[];

	vidlen = m68k_round_page(mac68k_video.mv_height *
	    mac68k_video.mv_stride + m68k_page_offset(mac68k_video.mv_phys));

	/*
	 * Calculate important physical addresses:
	 *
	 *	lwp0upa		lwp 0 u-area		UPAGES pages
	 *
	 *	kstpa		kernel segment table	1 page (!040)
	 *						N pages (040)
	 *
	 *	kptmpa		kernel PT map		1 page
	 *
	 *	kptpa		statically allocated
	 *			kernel PT pages		Sysptsize+ pages
	 *
	 * [ Sysptsize is the number of pages of PT, IIOMAPSIZE and
	 *   NBMAPSIZE are the number of PTEs, hence we need to round
	 *   the total to a page boundary with IO maps at the end. ]
	 *
	 */
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
		(IIOMAPSIZE + ROMMAPSIZE + VIDMAPSIZE + NPTEPG - 1) / NPTEPG;
	nextpa += nptpages * PAGE_SIZE;
	
	for (i = 0; i < numranges; i++)
		if (low[i] <= firstpa && firstpa < high[i])
			break;
	if (i >= numranges || nextpa > high[i]) {
		if (mac68k_machine.do_graybars) {
			printf("Failure in NetBSD boot; ");
			if (i < numranges)
				printf("nextpa=0x%lx, high[%d]=0x%lx.\n",
				    nextpa, i, high[i]);
			else
				printf("can't find kernel RAM segment.\n");
			printf("You're hosed!  Try booting with 32-bit ");
			printf("addressing enabled in the memory control ");
			printf("panel.\n");
			printf("Older machines may need Mode32 to get that ");
			printf("option.\n");
		}
		panic("Cannot work with the current memory mappings.");
	}

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
		epte = &epte[NPTEPG];		/* XXX: should be TIB_SIZE */
		while (pte < epte) {
			*pte++ = PG_NV;
		}
		/*
		 * Initialize the last one to point to Sysptmap.
		 */
		pte = PA2VA(kptmpa, pt_entry_t *);
		pte = &pte[NPTEPG - 1];		/* XXX: should be TIA_SIZE */
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
		este = &este[NPTEPG];		/* XXX: should be TIA_SIZE */
		while (ste < este)
			*ste++ = SG_NV;
		epte = PA2VA(kptmpa, pt_entry_t *);
		epte = &epte[NPTEPG];		/* XXX: should be TIB_SIZE */
		while (pte < epte)
			*pte++ = PG_NV;
		/*
		 * Initialize the last one to point to Sysptmap.
		 */
		ste = PA2VA(kstpa, st_entry_t *);
		ste = &ste[NPTEPG - 1];		/* XXX: should be TIA_SIZE */
		*ste = kptmpa | SG_RW | SG_V;
		pte = PA2VA(kptmpa, pt_entry_t *);
		pte = &pte[NPTEPG - 1];		/* XXX: should be TIA_SIZE */
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
	 * Pages up to "start" must be writable for the ROM.
	 */
	pte = PA2VA(kptpa, pt_entry_t *);
	pte = &pte[m68k_btop(KERNBASE)];
	/* XXX why KERNBASE relative? */
	epte = &pte[m68k_btop(m68k_round_page(start))];
	protopte = firstpa | PG_RW | PG_V;
	while (pte < epte) {
		*pte++ = protopte;
		protopte += PAGE_SIZE;
	}
	/* XXX why KERNBASE relative? */
	epte = &pte[m68k_btop(m68k_trunc_page(&etext))];
	protopte = (protopte & ~PG_PROT) | PG_RO;
	while (pte < epte) {
		*pte++ = protopte;
		protopte += PAGE_SIZE;
	}
	/*
	 * Validate PTEs for kernel data/bss, dynamic data allocated
	 * by us so far (nextpa - firstpa bytes), and pages for lwp0
	 * u-area and page table allocated below (RW).
	 */
	epte = PA2VA(kptpa, pt_entry_t *);
	epte = &epte[m68k_btop(nextpa - firstpa)];
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
		epte = &pte[VIDMAPSIZE];
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
	Sysseg_pa = PA2VA(kstpa, paddr_t);
	if (mmutype == MMU_68040)
		protostfree = stfree;
	/*
	 * Sysptmap: base of kernel page table map
	 */
	Sysptmap = PA2VA(kptmpa, pt_entry_t *);
	/*
	 * Sysmap: kernel page table (as mapped through Sysptmap)
	 * Allocated at the end of KVA space.
	 */
	Sysmap = (pt_entry_t *)m68k_ptob((NPTEPG - 1) * NPTEPG);

	/*
	 * Remember the u-area address so it can be loaded in the lwp0
	 * via uvm_lwp_setuarea() later in pmap_bootstrap_finalize().
	 */
	lwp0uarea = PA2VA(lwp0upa, vaddr_t);

	/*
	 * VM data structures are now initialized, set up data for
	 * the pmap module.
	 *
	 * Note about avail_end: msgbuf is initialized just after
	 * avail_end in machdep.c.  Since the last page is used
	 * for rebooting the system (code is copied there and
	 * excution continues from copied code before the MMU
	 * is disabled), the msgbuf will get trounced between
	 * reboots if it's placed in the last physical page.
	 * To work around this, we move avail_end back one more
	 * page so the msgbuf can be preserved.
	 */
	avail_next = avail_start = m68k_round_page(nextpa);
	avail_remaining = 0;
	avail_range = -1;
	for (i = 0; i < numranges; i++) {
		if (low[i] <= avail_next && avail_next < high[i]) {
			avail_range = i;
			avail_remaining = high[i] - avail_next;
		} else if (avail_range != -1) {
			avail_remaining += (high[i] - low[i]);
		}
	}
	physmem = m68k_btop(avail_remaining + nextpa - firstpa);

	maxaddr = high[numranges - 1] - m68k_ptob(1);
	high[numranges - 1] -= (m68k_round_page(MSGBUFSIZE) + m68k_ptob(1));
	avail_end = high[numranges - 1];
	mem_size = m68k_ptob(physmem);
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
}

void
bootstrap_mac68k(int tc)
{
#if NZSC > 0
	extern void zs_init(void);
#endif
	extern int *esym;
	paddr_t nextpa;
	void *oldROMBase;

	if (mac68k_machine.do_graybars)
		printf("Bootstrapping NetBSD/mac68k.\n");

	oldROMBase = ROMBase;
	mac68k_video.mv_phys = mac68k_video.mv_kvaddr;

	if (((tc & 0x80000000) && (mmutype == MMU_68030)) ||
	    ((tc & 0x8000) && (mmutype == MMU_68040))) {
		if (mac68k_machine.do_graybars)
			printf("Getting mapping from MMU.\n");
		(void) get_mapping();
		if (mac68k_machine.do_graybars)
			printf("Done.\n");
	} else {
		/* MMU not enabled.  Fake up ranges. */
		numranges = 1;
		low[0] = 0;
		high[0] = mac68k_machine.mach_memsize * (1024 * 1024);
		if (mac68k_machine.do_graybars)
			printf("Faked range to byte 0x%lx.\n", high[0]);
	}
	nextpa = load_addr + m68k_round_page(esym);

	if (mac68k_machine.do_graybars)
		printf("Bootstrapping the pmap system.\n");

	pmap_bootstrap(nextpa, load_addr);

	if (mac68k_machine.do_graybars)
		printf("Pmap bootstrapped.\n");

	if (!vidlen)
		panic("Don't know how to relocate video!");

	if (mac68k_machine.do_graybars)
		printf("Moving ROMBase from %p to %p.\n", oldROMBase, ROMBase);

	mrg_fixupROMBase(oldROMBase, ROMBase);

	if (mac68k_machine.do_graybars)
		printf("Video address 0x%p -> 0x%p.\n",
		    (void *)mac68k_video.mv_kvaddr, (void *)newvideoaddr);

	mac68k_set_io_offsets(IOBase);

	/*
	 * If the serial ports are going (for console or 'echo'), then
	 * we need to make sure the IO change gets propagated properly.
	 * This resets the base addresses for the 8530 (serial) driver.
	 *
	 * WARNING!!! No printfs() (etc) BETWEEN zs_init() and the end
	 * of this function (where we start using the MMU, so the new
	 * address is correct.
	 */
#if NZSC > 0
	if (zsinited != 0)
		zs_init();
#endif

	mac68k_video.mv_kvaddr = newvideoaddr;
}
