/*	$NetBSD: pmap.c,v 1.128 2007/05/28 22:36:15 mhitch Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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
 *	AMIGA physical map management code.
 *	For 68020/68030 machines with 68851, or 68030 MMUs
 *	Don't even pay lip service to multiprocessor support.
 *
 *	right now because of the assumed one-to-one relationship of PT
 *	pages to STEs.
 */

/*
 *	Manages physical address maps.
 *
 *	In addition to hardware address maps, this
 *	module is called upon to provide software-use-only
 *	maps which may or may not be stored in the same
 *	form as hardware maps.  These pseudo-maps are
 *	used to store intermediate results from copy
 *	operations to and from address spaces.
 *
 *	Since the information managed by this module is
 *	also stored by the logical address mapping module,
 *	this module may throw away valid virtual-to-physical
 *	mappings at almost any time.  However, invalidations
 *	of virtual-to-physical mappings must be done as
 *	requested.
 *
 *	In order to cope with hardware architectures which
 *	make virtual-to-physical map invalidates expensive,
 *	this module may delay invalidate or reduced protection
 *	operations until such time as they are actually
 *	necessary.  This module is given full information as
 *	to which processors are currently using which maps,
 *	and to when physical maps must be made correct.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pmap.c,v 1.128 2007/05/28 22:36:15 mhitch Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/user.h>

#include <uvm/uvm.h>

#include <machine/pte.h>
#include <machine/cpu.h>
#include <machine/vmparam.h>

#include <m68k/cacheops.h>

#include <amiga/amiga/memlist.h>
/*
 * Allocate various and sundry SYSMAPs used in the days of old VM
 * and not yet converted.  XXX.
 */

#ifdef DEBUG
struct kpt_stats {
	int collectscans;
	int collectpages;
	int kpttotal;
	int kptinuse;
	int kptmaxuse;
};
struct enter_stats {
	int kernel;	/* entering kernel mapping */
	int user;	/* entering user mapping */
	int ptpneeded;	/* needed to allocate a PT page */
	int pwchange;	/* no mapping change, just wiring or protection */
	int wchange;	/* no mapping change, just wiring */
	int mchange;	/* was mapped but mapping to different page */
	int managed;	/* a managed page */
	int firstpv;	/* first mapping for this PA */
	int secondpv;	/* second mapping for this PA */
	int ci;		/* cache inhibited */
	int unmanaged;	/* not a managed page */
	int flushes;	/* cache flushes */
};
struct remove_stats {
	int calls;
	int removes;
	int pvfirst;
	int pvsearch;
	int ptinvalid;
	int uflushes;
	int sflushes;
};

struct remove_stats remove_stats;
struct enter_stats enter_stats;
struct kpt_stats kpt_stats;

#define PDB_FOLLOW	0x0001
#define PDB_INIT	0x0002
#define PDB_ENTER	0x0004
#define PDB_REMOVE	0x0008
#define PDB_CREATE	0x0010
#define PDB_PTPAGE	0x0020
#define PDB_CACHE	0x0040
#define PDB_BITS	0x0080
#define PDB_COLLECT	0x0100
#define PDB_PROTECT	0x0200
#define PDB_SEGTAB	0x0400
#define PDB_PARANOIA	0x2000
#define PDB_WIRING	0x4000
#define PDB_PVDUMP	0x8000
int debugmap = 0;
int pmapdebug = PDB_PARANOIA;

#define	PMAP_DPRINTF(l, x)	if (pmapdebug & (l)) printf x

static void	pmap_check_wiring(const char *, vaddr_t);
static void	pmap_pvdump(paddr_t);
#else
#define	PMAP_DPRINTF(l, x)
#endif

/*
 * Get STEs and PTEs for user/kernel address space
 */
#if defined(M68040) || defined(M68060)
#define	pmap_ste(m, v)	(&((m)->pm_stab[(vaddr_t)(v) >> pmap_ishift]))
#define	pmap_ste1(m, v) (&((m)->pm_stab[(vaddr_t)(v) >> SG4_SHIFT1]))
/* XXX assumes physically contiguous ST pages (if more than one) */
#define	pmap_ste2(m, v) \
	(&((m)->pm_stab[(u_int *)(*(u_int *)pmap_ste1(m,v) & SG4_ADDR1) \
			- (m)->pm_stpa + (((v) & SG4_MASK2) >> SG4_SHIFT2)]))
#define	pmap_ste_v(m, v) \
	(mmutype == MMU_68040		\
	? ((*pmap_ste1(m, v) & SG_V) &&	\
	   (*pmap_ste2(m, v) & SG_V))	\
	: (*pmap_ste(m, v) & SG_V))
#else	/* defined(M68040) || defined(M68060) */
#define	pmap_ste(m, v)	(&((m)->pm_stab[(vaddr_t)(v) >> SG_ISHIFT]))
#define pmap_ste_v(m, v)	(*pmap_ste(m, v) & SG_V)
#endif	/* defined(M68040) || defined(M68060) */

#define pmap_pte(m, v)	(&((m)->pm_ptab[(vaddr_t)(v) >> PG_SHIFT]))

#define pmap_pte_pa(pte)	(*(u_int *)(pte) & PG_FRAME)

#define pmap_pte_w(pte)		(*(u_int *)(pte) & PG_W)
#define pmap_pte_ci(pte)	(*(u_int *)(pte) & PG_CI)
#define pmap_pte_m(pte)		(*(u_int *)(pte) & PG_M)
#define pmap_pte_u(pte)		(*(u_int *)(pte) & PG_U)
#define pmap_pte_prot(pte)	(*(u_int *)(pte) & PG_PROT)
#define pmap_pte_v(pte)		(*(u_int *)(pte) & PG_V)

#define pmap_pte_set_w(pte, v) \
    do { if (v) *(u_int *)(pte) |= PG_W; else *(u_int *)(pte) &= ~PG_W; \
    } while (0)
#define pmap_pte_set_prot(pte, v) \
    do { if (v) *(u_int *)(pte) |= PG_PROT; else *(u_int *)(pte) &= ~PG_PROT; \
    } while (0)
#define pmap_pte_w_chg(pte, nw)		((nw) ^ pmap_pte_w(pte))
#define pmap_pte_prot_chg(pte, np)	((np) ^ pmap_pte_prot(pte))

#define active_pmap(pm)	\
    ((pm) == pmap_kernel() || (pm) == curproc->p_vmspace->vm_map.pmap)

/*
 * Given a map and a machine independent protection code,
 * convert to a vax protection code.
 */
#define pte_prot(m, p)	(protection_codes[p])
int	protection_codes[8];

/*
 * Kernel page table page management.
 *
 * One additional page of KPT allows for 16 MB of virtual buffer cache.
 * A GENERIC kernel allocates this for 2 MB of real buffer cache,
 * which in turn is allocated for 38 MB of RAM.
 * We add one per 16 MB of RAM to allow for tuning the machine-independent
 * options.
 */
#ifndef NKPTADDSHIFT
#define NKPTADDSHIFT 24
#endif

struct kpt_page {
	struct kpt_page *kpt_next;	/* link on either used or free list */
	vaddr_t		kpt_va;		/* always valid kernel VA */
	paddr_t		kpt_pa;		/* PA of this page (for speed) */
};
struct kpt_page *kpt_free_list, *kpt_used_list;
struct kpt_page *kpt_pages;

/*
 * Kernel segment/page table and page table map.
 * The page table map gives us a level of indirection we need to dynamically
 * expand the page table.  It is essentially a copy of the segment table
 * with PTEs instead of STEs.  All are initialized in locore at boot time.
 * Sysmap will initially contain VM_KERNEL_PT_PAGES pages of PTEs.
 * Segtabzero is an empty segment table which all processes share til they
 * reference something.
 */
u_int	*Sysseg, *Sysseg_pa;
u_int	*Sysmap, *Sysptmap;
u_int	*Segtabzero, *Segtabzeropa;
vsize_t	Sysptsize = VM_KERNEL_PT_PAGES;

pv_entry_t	pv_table;	/* array of entries, one per page */

struct pmap	kernel_pmap_store;
struct vm_map	*pt_map;
struct vm_map_kernel pt_map_store;

vsize_t		mem_size;	/* memory size in bytes */
vaddr_t		virtual_avail;  /* VA of first avail page (after kernel bss)*/
vaddr_t		virtual_end;	/* VA of last avail page (end of kernel AS) */
int		page_cnt;	/* number of pages managed by the VM system */
bool		pmap_initialized = false;	/* Has pmap_init completed? */
char		*pmap_attributes;	/* reference and modify bits */
TAILQ_HEAD(pv_page_list, pv_page) pv_page_freelist;
int		pv_nfree;
#if defined(M68040) || defined(M68060)
static int	pmap_ishift;	/* segment table index shift */
int		protostfree;	/* prototype (default) free ST map */
#endif

pt_entry_t	*caddr1_pte;	/* PTE for CADDR1 */
pt_entry_t	*caddr2_pte;	/* PTE for CADDR2 */

extern void *	msgbufaddr;
extern paddr_t	msgbufpa;

u_long	noncontig_enable;
extern const vaddr_t amiga_uptbase;

extern paddr_t z2mem_start;

extern vaddr_t reserve_dumppages(vaddr_t);

bool		pmap_testbit(paddr_t, int);
int		pmap_enter_ptpage(pmap_t, vaddr_t, bool);
static void	pmap_ptpage_addref(vaddr_t);
static int	pmap_ptpage_delref(vaddr_t);
static void	pmap_changebit(vaddr_t, int, bool);
struct pv_entry * pmap_alloc_pv(void);
void		pmap_free_pv(struct pv_entry *);
void		pmap_pinit(pmap_t);
void		pmap_release(pmap_t);
static void	pmap_remove_mapping(pmap_t, vaddr_t, pt_entry_t *, int);

static void	amiga_protection_init(void);
void		pmap_collect1(pmap_t, paddr_t, paddr_t);

/* pmap_remove_mapping flags */
#define		PRM_TFLUSH	0x01
#define		PRM_CFLUSH	0x02
#define		PRM_KEEPPTPAGE	0x04





/*
 * All those kernel PT submaps that BSD is so fond of
 */
void 	*CADDR1, *CADDR2, *vmmap;

#define	PAGE_IS_MANAGED(pa)	(pmap_initialized			\
				 && vm_physseg_find(atop((pa)), NULL) != -1)

#define	pa_to_pvh(pa)							\
({									\
	int bank_, pg_ = 0;	/* gcc4 -Wunitialized */		\
									\
	bank_ = vm_physseg_find(atop((pa)), &pg_);			\
	&vm_physmem[bank_].pmseg.pvent[pg_];				\
})

#define	pa_to_attribute(pa)						\
({									\
	int bank_, pg_ = 0;	/* gcc4 -Wuninitialized */		\
									\
	bank_ = vm_physseg_find(atop((pa)), &pg_);			\
	&vm_physmem[bank_].pmseg.attrs[pg_];				\
})

/*
 *	Bootstrap the system enough to run with virtual memory.
 *	Map the kernel's code and data, and allocate the system page table.
 *
 *	On the HP this is called after mapping has already been enabled
 *	and just syncs the pmap module with what has already been done.
 *	[We can't call it easily with mapping off since the kernel is not
 *	mapped with PA == VA, hence we would have to relocate every address
 *	from the linked base (virtual) address 0 to the actual (physical)
 *	address of 0xFFxxxxxx.]
 */
void
pmap_bootstrap(firstaddr, loadaddr)
	paddr_t firstaddr;
	paddr_t loadaddr;
{
	vaddr_t va;
	int i;
	struct boot_memseg *sp, *esp;
	paddr_t fromads, toads;

	fromads = firstaddr;
	toads = maxmem << PGSHIFT;

	/* XXX: allow for msgbuf */
	toads -= m68k_round_page(MSGBUFSIZE);
	msgbufpa = toads;
	/*
	 * first segment of memory is always the one loadbsd found
	 * for loading the kernel into.
	 */

	uvmexp.pagesize = NBPG;
	uvm_setpagesize();

	/*
	 * May want to check if first segment is Zorro-II?
	 */
	uvm_page_physload(atop(fromads), atop(toads),
	    atop(fromads), atop(toads), VM_FREELIST_DEFAULT);

	sp = memlist->m_seg;
	esp = sp + memlist->m_nseg;
	i = 1;
	for (; noncontig_enable && sp < esp; sp++) {
		if ((sp->ms_attrib & MEMF_FAST) == 0)
			continue;		/* skip if not FastMem */
		if (firstaddr >= sp->ms_start &&
		    firstaddr < sp->ms_start + sp->ms_size)
			continue;		/* skip kernel segment */
		if (sp->ms_size == 0)
			continue;		/* skip zero size segments */
		fromads = sp->ms_start;
		toads = sp->ms_start + sp->ms_size;
#ifdef DEBUG_A4000
		/*
		 * My A4000 doesn't seem to like Zorro II memory - this
		 * hack is to skip the motherboard memory and use the
		 * Zorro II memory.  Only for trying to debug the problem.
		 * Michael L. Hitch
		 */
		if (toads == 0x08000000)
			continue;	/* skip A4000 motherboard mem */
#endif
		/*
		 * Deal with Zorro II memory stolen for DMA bounce buffers.
		 * This needs to be handled better.
		 *
		 * XXX is: disabled. This is handled now in amiga_init.c
		 * by removing the stolen memory from the memlist.
		 *
		 * XXX is: enabled again, but check real size and position.
		 * We check z2mem_start is in this segment, and set its end
		 * to the z2mem_start.
		 *
		 */
		if ((fromads <= z2mem_start) && (toads > z2mem_start))
			toads = z2mem_start;

		uvm_page_physload(atop(fromads), atop(toads),
		    atop(fromads), atop(toads), (fromads & 0xff000000) ?
		    VM_FREELIST_DEFAULT : VM_FREELIST_ZORROII);
		physmem += (toads - fromads) / PAGE_SIZE;
		++i;
		if (noncontig_enable == 1)
			break;		/* Only two segments enabled */
	}

	mem_size = physmem << PGSHIFT;
	virtual_end = VM_MAX_KERNEL_ADDRESS;

	/*
	 * Initialize protection array.
	 */
	amiga_protection_init();

	/*
	 * Kernel page/segment table allocated in locore,
	 * just initialize pointers.
	 */
	pmap_kernel()->pm_stpa = Sysseg_pa;
	pmap_kernel()->pm_stab = Sysseg;
	pmap_kernel()->pm_ptab = Sysmap;
#if defined(M68040) || defined(M68060)
	if (mmutype == MMU_68040) {
		pmap_ishift = SG4_SHIFT1;
		pmap_kernel()->pm_stfree = protostfree;
	} else
		pmap_ishift = SG_ISHIFT;
#endif

	simple_lock_init(&pmap_kernel()->pm_lock);
	pmap_kernel()->pm_count = 1;

	/*
	 * Allocate all the submaps we need
	 */
#define	SYSMAP(c, v, n)	\
	v = (c)va; va += ((n)*PAGE_SIZE);

	va = virtual_avail;

	SYSMAP(void *	,CADDR1	 ,1			)
	SYSMAP(void *	,CADDR2	 ,1			)
	SYSMAP(void *	,vmmap	 ,1			)
	SYSMAP(void *	,msgbufaddr ,btoc(MSGBUFSIZE)	)

	DCIS();

	virtual_avail = reserve_dumppages(va);
}

/*
 *	Initialize the pmap module.
 *	Called by vm_init, to initialize any structures that the pmap
 *	system needs to map virtual memory.
 */
void
pmap_init()
{
	vaddr_t		addr, addr2;
	vsize_t		s;
	u_int		npg;
	struct pv_entry *pv;
	char            *attr;
	int             rv, bank;
#if defined(M68060)
	struct kpt_page *kptp;
	paddr_t		paddr;
#endif

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_init()\n");
#endif

	/*
	 * Before we do anything else, initialize the PTE pointers
	 * used by pmap_zero_page() and pmap_copy_page().
	 */
	caddr1_pte = pmap_pte(pmap_kernel(), CADDR1);
	caddr2_pte = pmap_pte(pmap_kernel(), CADDR2);

#ifdef DEBUG
	if (pmapdebug & PDB_INIT) {
		printf("pmap_init: Sysseg %p, Sysmap %p, Sysptmap %p\n",
		    Sysseg, Sysmap, Sysptmap);
		printf(" vstart %lx, vend %lx\n", virtual_avail, virtual_end);
	}
#endif

	/*
	 * Allocate memory for random pmap data structures.  Includes the
	 * initial segment table, pv_head_table and pmap_attributes.
	 */
	for (page_cnt = 0, bank = 0; bank < vm_nphysseg; bank++) {
		page_cnt += vm_physmem[bank].end - vm_physmem[bank].start;
#ifdef DEBUG
		printf("pmap_init: %2d: %08lx - %08lx (%10d)\n", bank,
		    vm_physmem[bank].start << PGSHIFT,
		    vm_physmem[bank].end << PGSHIFT, page_cnt << PGSHIFT);
#endif
	}
	s = AMIGA_STSIZE;				/* Segtabzero */
	s += page_cnt * sizeof(struct pv_entry);	/* pv table */
	s += page_cnt * sizeof(char);			/* attribute table */
	s = round_page(s);

	addr = uvm_km_alloc(kernel_map, s, 0, UVM_KMF_WIRED | UVM_KMF_ZERO);
	if (addr == 0)
		panic("pmap_init: can't allocate data structures");
	Segtabzero = (u_int *) addr;
	(void) pmap_extract(pmap_kernel(), addr, (paddr_t *)(void *)&Segtabzeropa);
	addr += AMIGA_STSIZE;

	pv_table = (pv_entry_t) addr;
	addr += page_cnt * sizeof(struct pv_entry);

	pmap_attributes = (char *) addr;
#ifdef DEBUG
	if (pmapdebug & PDB_INIT)
		printf("pmap_init: %lx bytes: page_cnt %x s0 %p(%p) "
			"tbl %p atr %p\n",
			s, page_cnt, Segtabzero, Segtabzeropa,
			pv_table, pmap_attributes);
#endif

	/*
	 * Now that the pv and attribute tables have been allocated,
	 * assign them to the memory segments.
	 */
	pv = pv_table;
	attr = pmap_attributes;
	for (bank = 0; bank < vm_nphysseg; bank++) {
		npg = vm_physmem[bank].end - vm_physmem[bank].start;
		vm_physmem[bank].pmseg.pvent = pv;
		vm_physmem[bank].pmseg.attrs = attr;
		pv += npg;
		attr += npg;
	}

	/*
	 * Allocate physical memory for kernel PT pages and their management.
	 * we need enough pages to map the page tables for each process
	 * plus some slop.
	 */
	npg = howmany(((maxproc + 16) * AMIGA_UPTSIZE / NPTEPG), PAGE_SIZE);
#ifdef NKPTADD
	npg += NKPTADD;
#else
	npg += mem_size >> NKPTADDSHIFT;
#endif
#if 1/*def DEBUG*/
	printf("Maxproc %d, mem_size %ld MB: allocating %d KPT pages\n",
	    maxproc, mem_size>>20, npg);
#endif
	s = ptoa(npg) + round_page(npg * sizeof(struct kpt_page));

	/*
	 * Verify that space will be allocated in region for which
	 * we already have kernel PT pages.
	 */
	addr = 0;
	rv = uvm_map(kernel_map, &addr, s, NULL, UVM_UNKNOWN_OFFSET, 0,
		     UVM_MAPFLAG(UVM_PROT_NONE, UVM_PROT_NONE, UVM_INH_NONE,
				 UVM_ADV_RANDOM, UVM_FLAG_NOMERGE));
	if (rv != 0 || (addr + s) >= (vaddr_t)Sysmap)
		panic("pmap_init: kernel PT too small");
	uvm_unmap(kernel_map, addr, addr + s);

	/*
	 * Now allocate the space and link the pages together to
	 * form the KPT free list.
	 */
	addr = uvm_km_alloc(kernel_map, s, 0, UVM_KMF_WIRED | UVM_KMF_ZERO);
	if (addr == 0)
		panic("pmap_init: cannot allocate KPT free list");
	s = ptoa(npg);
	addr2 = addr + s;
	kpt_pages = &((struct kpt_page *)addr2)[npg];
	kpt_free_list = NULL;
	do {
		addr2 -= PAGE_SIZE;
		(--kpt_pages)->kpt_next = kpt_free_list;
		kpt_free_list = kpt_pages;
		kpt_pages->kpt_va = addr2;
		(void) pmap_extract(pmap_kernel(), addr2,
		    (paddr_t *)&kpt_pages->kpt_pa);
	} while (addr != addr2);

#ifdef DEBUG
	kpt_stats.kpttotal = atop(s);
	if (pmapdebug & PDB_INIT)
		printf("pmap_init: KPT: %ld pages from %lx to %lx\n",
		    atop(s), addr, addr + s);
#endif

	/*
	 * Allocate the segment table map and the page table map.
	 */
	addr = amiga_uptbase;
	if (AMIGA_UPTMAXSIZE / AMIGA_UPTSIZE < maxproc) {
		s = AMIGA_UPTMAXSIZE;

		/*
		 * XXX We don't want to hang when we run out of page
		 * tables, so we lower maxproc so that fork will fail
		 * instead.  Note that root could still raise this
		 * value through sysctl(3).
		 */
		maxproc = AMIGA_UPTMAXSIZE / AMIGA_UPTSIZE;
	} else
		s = maxproc * AMIGA_UPTSIZE;

	pt_map = uvm_km_suballoc(kernel_map, &addr, &addr2, s, 0,
	    true, &pt_map_store);

#if defined(M68040) || defined(M68060)
	if (mmutype == MMU_68040)
		protostfree = ~1 & ~(-1 << MAXUL2SIZE);
#endif /* defined(M68040) || defined(M68060) */

	/*
	 * Now it is safe to enable pv_table recording.
	 */
	pmap_initialized = true;

	/*
	 * Now that this is done, mark the pages shared with the
	 * hardware page table search as non-CCB (actually, as CI).
	 *
	 * XXX Hm. Given that this is in the kernel map, can't we just
	 * use the va's?
	 */
#ifdef M68060
	if (cputype == CPU_68060) {
		kptp = kpt_free_list;
		while (kptp) {
			pmap_changebit(kptp->kpt_pa, PG_CCB, 0);
			pmap_changebit(kptp->kpt_pa, PG_CI, 1);
			kptp = kptp->kpt_next;
		}

		paddr = (paddr_t)Segtabzeropa;
		while (paddr < (paddr_t)Segtabzeropa + AMIGA_STSIZE) {
			pmap_changebit(paddr, PG_CCB, 0);
			pmap_changebit(paddr, PG_CI, 1);
			paddr += PAGE_SIZE;
		}

		DCIS();
	}
#endif
}

struct pv_entry *
pmap_alloc_pv()
{
	struct pv_page *pvp;
	struct pv_entry *pv;
	int i;

	if (pv_nfree == 0) {
		pvp = (struct pv_page *)uvm_km_alloc(kernel_map, PAGE_SIZE, 0,
		    UVM_KMF_WIRED | UVM_KMF_ZERO);
		if (pvp == 0)
			panic("pmap_alloc_pv: uvm_km_zalloc() failed");
		pvp->pvp_pgi.pgi_freelist = pv = &pvp->pvp_pv[1];
		for (i = NPVPPG - 2; i; i--, pv++)
			pv->pv_next = pv + 1;
		pv->pv_next = 0;
		pv_nfree += pvp->pvp_pgi.pgi_nfree = NPVPPG - 1;
		TAILQ_INSERT_HEAD(&pv_page_freelist, pvp, pvp_pgi.pgi_list);
		pv = &pvp->pvp_pv[0];
	} else {
		--pv_nfree;
		pvp = pv_page_freelist.tqh_first;
		if (--pvp->pvp_pgi.pgi_nfree == 0) {
			TAILQ_REMOVE(&pv_page_freelist, pvp, pvp_pgi.pgi_list);
		}
		pv = pvp->pvp_pgi.pgi_freelist;
#ifdef DIAGNOSTIC
		if (pv == 0)
			panic("pmap_alloc_pv: pgi_nfree inconsistent");
#endif
		pvp->pvp_pgi.pgi_freelist = pv->pv_next;
	}
	return pv;
}

void
pmap_free_pv(pv)
	struct pv_entry *pv;
{
	struct pv_page *pvp;

	pvp = (struct pv_page *) trunc_page((vaddr_t)pv);
	switch (++pvp->pvp_pgi.pgi_nfree) {
	case 1:
		TAILQ_INSERT_TAIL(&pv_page_freelist, pvp, pvp_pgi.pgi_list);
	default:
		pv->pv_next = pvp->pvp_pgi.pgi_freelist;
		pvp->pvp_pgi.pgi_freelist = pv;
		++pv_nfree;
		break;
	case NPVPPG:
		pv_nfree -= NPVPPG - 1;
		TAILQ_REMOVE(&pv_page_freelist, pvp, pvp_pgi.pgi_list);
		uvm_km_free(kernel_map, (vaddr_t)pvp, PAGE_SIZE, UVM_KMF_WIRED);
		break;
	}
}


/*
 *	Used to map a range of physical addresses into kernel
 *	virtual address space.
 *
 *	For now, VM is already on, we only need to map the
 *	specified memory.
 */
vaddr_t
pmap_map(virt, start, end, prot)
	vaddr_t	virt;
	paddr_t	start;
	paddr_t	end;
	int		prot;
{
#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_map(%lx, %lx, %lx, %x)\n", virt, start, end, prot);
#endif
	while (start < end) {
		pmap_enter(pmap_kernel(), virt, start, prot, 0);
		virt += PAGE_SIZE;
		start += PAGE_SIZE;
	}
	pmap_update(pmap_kernel());
	return(virt);
}

/*
 *	Create and return a physical map.
 *
 *	If the size specified for the map
 *	is zero, the map is an actual physical
 *	map, and may be referenced by the
 *	hardware.
 *
 *	If the size specified is non-zero,
 *	the map will be used in software only, and
 *	is bounded by that size.
 */
pmap_t
pmap_create()
{
	pmap_t pmap;

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_CREATE))
		printf("pmap_create\n");
#endif

	pmap = malloc(sizeof *pmap, M_VMPMAP, M_WAITOK);
	bzero(pmap, sizeof(*pmap));
	pmap_pinit(pmap);
	return (pmap);
}

/*
 * Initialize a preallocated and zeroed pmap structure,
 * such as one in a vmspace structure.
 */
void
pmap_pinit(pmap)
	pmap_t pmap;
{

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_CREATE))
		printf("pmap_pinit(%p)\n", pmap);
#endif
	/*
	 * No need to allocate page table space yet but we do need a
	 * valid segment table.  Initially, we point everyone at the
	 * "null" segment table.  On the first pmap_enter, a real
	 * segment table will be allocated.
	 */
	pmap->pm_stab = Segtabzero;
	pmap->pm_stpa = Segtabzeropa;
#if defined(M68040) || defined(M68060)
	if (mmutype == MMU_68040)
		pmap->pm_stfree = protostfree;
#endif
	pmap->pm_count = 1;
	simple_lock_init(&pmap->pm_lock);
}

/*
 *	Retire the given physical map from service.
 *	Should only be called if the map contains
 *	no valid mappings.
 */
void
pmap_destroy(pmap)
	pmap_t pmap;
{
	int count;

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_destroy(%p)\n", pmap);
#endif
	simple_lock(&pmap->pm_lock);
	count = --pmap->pm_count;
	simple_unlock(&pmap->pm_lock);
	if (count == 0) {
		pmap_release(pmap);
		free((void *)pmap, M_VMPMAP);
	}
}

/*
 * Release any resources held by the given physical map.
 * Called when a pmap initialized by pmap_pinit is being released.
 * Should only be called if the map contains no valid mappings.
 */
void
pmap_release(pmap)
	pmap_t pmap;
{

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_release(%p)\n", pmap);
#endif
#ifdef notdef /* DIAGNOSTIC */
	/* count would be 0 from pmap_destroy... */
	simple_lock(&pmap->pm_lock);
	if (pmap->pm_count != 1)
		panic("pmap_release count");
#endif
	if (pmap->pm_ptab) {
		pmap_remove(pmap_kernel(), (vaddr_t)pmap->pm_ptab,
		    (vaddr_t)pmap->pm_ptab + AMIGA_UPTSIZE);
		uvm_km_pgremove((vaddr_t)pmap->pm_ptab,
		    (vaddr_t)pmap->pm_ptab + AMIGA_UPTSIZE);
		uvm_km_free(pt_map, (vaddr_t)pmap->pm_ptab,
		    AMIGA_UPTSIZE, UVM_KMF_VAONLY);
	}
	KASSERT(pmap->pm_stab == Segtabzero);
}

/*
 *	Add a reference to the specified pmap.
 */
void
pmap_reference(pmap)
	pmap_t	pmap;
{
#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_reference(%p)\n", pmap);
#endif
	if (pmap != NULL) {
		simple_lock(&pmap->pm_lock);
		pmap->pm_count++;
		simple_unlock(&pmap->pm_lock);
	}
}

/*
 *	Remove the given range of addresses from the specified map.
 *
 *	It is assumed that the start and end are properly
 *	rounded to the page size.
 */
void
pmap_remove(pmap, sva, eva)
	pmap_t pmap;
	vaddr_t sva, eva;
{
	paddr_t pa;
	vaddr_t va;
	u_int *pte;
	int flags;

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_REMOVE|PDB_PROTECT))
		printf("pmap_remove(%p, %lx, %lx)\n", pmap, sva, eva);
	remove_stats.calls++;
#endif
	flags = active_pmap(pmap) ? PRM_TFLUSH : 0;
	for (va = sva; va < eva; va += PAGE_SIZE) {
		/*
		 * Weed out invalid mappings.
		 * Note: we assume that the segment table is always allocated.
		 */
		if (!pmap_ste_v(pmap, va)) {
			/* XXX: avoid address wrap around */
			if (va >= m68k_trunc_seg((vaddr_t)-1))
				break;
			va = m68k_round_seg(va + PAGE_SIZE) - PAGE_SIZE;
			continue;
		}
		pte = pmap_pte(pmap, va);
		pa = pmap_pte_pa(pte);
		if (pa == 0)
			continue;
		pmap_remove_mapping(pmap, va, pte, flags);
	}
}

/*
 *	pmap_page_protect:
 *
 *	Lower the permission for all mappings to a given page.
 */
void
pmap_page_protect(pg, prot)
	struct vm_page *pg;
	vm_prot_t	prot;
{
	pv_entry_t pv;
	int s;
	paddr_t	pa = VM_PAGE_TO_PHYS(pg);

#ifdef DEBUG
	if ((pmapdebug & (PDB_FOLLOW|PDB_PROTECT)) ||
	    (prot == VM_PROT_NONE && (pmapdebug & PDB_REMOVE)))
		printf("pmap_page_protect(%lx, %x)\n", pa, prot);
#endif
	switch (prot) {
	case VM_PROT_ALL:
		break;
	/* copy_on_write */
	case VM_PROT_READ:
	case VM_PROT_READ|VM_PROT_EXECUTE:
		pmap_changebit(pa, PG_RO, true);
		break;
	/* remove_all */
	default:
		pv = pa_to_pvh(pa);
		s = splvm();
		while (pv->pv_pmap != NULL) {
			pt_entry_t  *pte;

			pte = pmap_pte(pv->pv_pmap, pv->pv_va);
#ifdef DEBUG
			if (!pmap_ste_v(pv->pv_pmap,pv->pv_va) ||
			    pmap_pte_pa(pte) != pa)
{
    printf ("pmap_page_protect: va %lx, pmap_ste_v %d pmap_pte_pa %08x/%lx\n",
	pv->pv_va, pmap_ste_v(pv->pv_pmap,pv->pv_va),
	pmap_pte_pa(pmap_pte(pv->pv_pmap,pv->pv_va)),pa);
    printf (" pvh %p pv %p pv_next %p\n", pa_to_pvh(pa), pv, pv->pv_next);
			panic("pmap_page_protect: bad mapping");
}
#endif
			pmap_remove_mapping(pv->pv_pmap, pv->pv_va,
			    pte, PRM_TFLUSH|PRM_CFLUSH);
		}
		splx(s);
		break;
	}
}

/*
 *	Set the physical protection on the
 *	specified range of this map as requested.
 */
void
pmap_protect(pmap, sva, eva, prot)
	pmap_t pmap;
	vaddr_t	sva, eva;
	vm_prot_t prot;
{
	u_int *pte;
	vaddr_t va;
	bool needtflush;
	int isro;

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_PROTECT))
		printf("pmap_protect(%p, %lx, %lx, %x)\n", pmap, sva, eva, prot);
#endif
	if ((prot & VM_PROT_READ) == VM_PROT_NONE) {
		pmap_remove(pmap, sva, eva);
		return;
	}
	pte = pmap_pte(pmap, sva);
	isro = pte_prot(pmap, prot) == PG_RO ? 1 : 0;
	needtflush = active_pmap(pmap);
	for (va = sva; va < eva; va += PAGE_SIZE) {
		/*
		 * Page table page is not allocated.
		 * Skip it, we don't want to force allocation
		 * of unnecessary PTE pages just to set the protection.
		 */
		if (!pmap_ste_v(pmap, va)) {
			/* XXX: avoid address wrap around */
			if (va >= m68k_trunc_seg((vaddr_t)-1))
				break;
			va = m68k_round_seg(va + PAGE_SIZE) - PAGE_SIZE;
			pte = pmap_pte(pmap, va);
			pte++;
			continue;
		}
		/*
		 * skip if page not valid or protection is same
		 */
		if (!pmap_pte_v(pte) || !pmap_pte_prot_chg(pte, isro)) {
			pte++;
			continue;
		}
#if defined(M68040) || defined(M68060)
		/*
		 * Clear caches if making RO (see section
		 * "7.3 Cache Coherency" in the manual).
		 */
		if (isro && mmutype == MMU_68040) {
			paddr_t pa = pmap_pte_pa(pte);

			DCFP(pa);
			ICPP(pa);
		}
#endif
		pmap_pte_set_prot(pte, isro);
		if (needtflush)
			TBIS(va);
		pte++;
	}
}

/*
 *	Insert the given physical page (p) at
 *	the specified virtual address (v) in the
 *	target physical map with the protection requested.
 *
 *	If specified, the page will be wired down, meaning
 *	that the related pte can not be reclaimed.
 *
 *	NB:  This is the only routine which MAY NOT lazy-evaluate
 *	or lose information.  That is, this routine must actually
 *	insert this page into the given map NOW.
 */
extern int kernel_copyback;

int
pmap_enter(pmap, va, pa, prot, flags)
	pmap_t pmap;
	vaddr_t va;
	paddr_t pa;
	vm_prot_t prot;
	int flags;
{
	u_int *pte;
	int npte;
	paddr_t opa;
	bool cacheable = true;
	bool checkpv = true;
	bool wired = (flags & PMAP_WIRED) != 0;
	bool can_fail = (flags & PMAP_CANFAIL) != 0;

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_ENTER))
		printf("pmap_enter(%p, %lx, %lx, %x, %x)\n",
		    pmap, va, pa, prot, wired);
	if (pmap == pmap_kernel())
		enter_stats.kernel++;
	else
		enter_stats.user++;
#endif
	/*
	 * For user mapping, allocate kernel VM resources if necessary.
	 */
	if (pmap->pm_ptab == NULL)
		pmap->pm_ptab = (pt_entry_t *)
		    uvm_km_alloc(pt_map, AMIGA_UPTSIZE, 0,
		    UVM_KMF_VAONLY | 
		    (can_fail ? UVM_KMF_NOWAIT : UVM_KMF_WAITVA));
		if (pmap->pm_ptab == NULL)
			return ENOMEM;

	/*
	 * Segment table entry not valid, we need a new PT page
	 */
	if (!pmap_ste_v(pmap, va)) {
		int err = pmap_enter_ptpage(pmap, va, can_fail);
		if (err)
			return err;
	}

	pte = pmap_pte(pmap, va);
	opa = pmap_pte_pa(pte);
#ifdef DEBUG
	if (pmapdebug & PDB_ENTER)
		printf("enter: pte %p, *pte %x\n", pte, *(int *)pte);
#endif

	/*
	 * Mapping has not changed, must be protection or wiring change.
	 */
	if (opa == pa) {
#ifdef DEBUG
		enter_stats.pwchange++;
#endif
		/*
		 * Wiring change, just update stats.
		 * We don't worry about wiring PT pages as they remain
		 * resident as long as there are valid mappings in them.
		 * Hence, if a user page is wired, the PT page will be also.
		 */
		if ((wired && !pmap_pte_w(pte)) || (!wired && pmap_pte_w(pte))){
#ifdef DEBUG
			if (pmapdebug & PDB_ENTER)
				printf("enter: wiring change -> %x\n", wired);
#endif
			if (wired)
				pmap->pm_stats.wired_count++;
			else
				pmap->pm_stats.wired_count--;
#ifdef DEBUG
			enter_stats.wchange++;
#endif
		}
		/*
		 * Retain cache inhibition status
		 */
		checkpv = false;
		if (pmap_pte_ci(pte))
			cacheable = false;
		goto validate;
	}

	/*
	 * Mapping has changed, invalidate old range and fall through to
	 * handle validating new mapping.
	 */
	if (opa) {
#ifdef DEBUG
		if (pmapdebug & PDB_ENTER)
			printf("enter: removing old mapping %lx\n", va);
#endif
		pmap_remove_mapping(pmap, va, pte,
			PRM_TFLUSH|PRM_CFLUSH|PRM_KEEPPTPAGE);
#ifdef DEBUG
		enter_stats.mchange++;
#endif
	}

	/*
	 * If this is a new user mapping, increment the wiring count
	 * on this PT page.  PT pages are wired down as long as there
	 * is a valid mapping in the page.
	 */
	if (pmap != pmap_kernel())
		pmap_ptpage_addref(trunc_page((vaddr_t)pte));

	/*
	 * Enter on the PV list if part of our managed memory
	 * Note that we raise IPL while manipulating pv_table
	 * since pmap_enter can be called at interrupt time.
	 */
	if (PAGE_IS_MANAGED(pa)) {
		pv_entry_t pv, npv;
		int s;

#ifdef DEBUG
		enter_stats.managed++;
#endif
		pv = pa_to_pvh(pa);
		s = splvm();
#ifdef DEBUG
		if (pmapdebug & PDB_ENTER)
			printf("enter: pv at %p: %lx/%p/%p\n",
			       pv, pv->pv_va, pv->pv_pmap, pv->pv_next);
#endif
		/*
		 * No entries yet, use header as the first entry
		 */
		if (pv->pv_pmap == NULL) {
#ifdef DEBUG
			enter_stats.firstpv++;
#endif
			pv->pv_va = va;
			pv->pv_pmap = pmap;
			pv->pv_next = NULL;
			pv->pv_ptste = NULL;
			pv->pv_ptpmap = NULL;
			pv->pv_flags = 0;
		}
		/*
		 * There is at least one other VA mapping this page.
		 * Place this entry after the header.
		 */
		else {
#ifdef DEBUG
			for (npv = pv; npv; npv = npv->pv_next)
				if (pmap == npv->pv_pmap && va == npv->pv_va)
					panic("pmap_enter: already in pv_tab");
#endif
			npv = pmap_alloc_pv();
			npv->pv_va = va;
			npv->pv_pmap = pmap;
			npv->pv_next = pv->pv_next;
			npv->pv_ptste = NULL;
			npv->pv_ptpmap = NULL;
			pv->pv_next = npv;
#ifdef DEBUG
			if (!npv->pv_next)
				enter_stats.secondpv++;
#endif
		}
		splx(s);
	}
	/*
	 * Assumption: if it is not part of our managed memory
	 * then it must be device memory which may be volitile.
	 */
	else if (pmap_initialized) {
		checkpv = cacheable = false;
#ifdef DEBUG
		enter_stats.unmanaged++;
#endif
	}

	/*
	 * Increment counters
	 */
	pmap->pm_stats.resident_count++;
	if (wired)
		pmap->pm_stats.wired_count++;

validate:
	/*
	 * Now validate mapping with desired protection/wiring.
	 * Assume uniform modified and referenced status for all
	 * AMIGA pages in a MACH page.
	 */
#if defined(M68040) || defined(M68060)
#if DEBUG
	if (pmapdebug & 0x10000 && mmutype == MMU_68040 &&
	    pmap == pmap_kernel()) {
		const char *s;
		struct proc *cp = curproc;
		if (va >= amiga_uptbase &&
		    va < (amiga_uptbase + AMIGA_UPTMAXSIZE))
			s = "UPT";
		else if (va >= (u_int)Sysmap &&
		    va < ((u_int)Sysmap + AMIGA_KPTSIZE))
			s = "KPT";
		else if (va >= (u_int)pmap->pm_stab &&
		    va < ((u_int)pmap->pm_stab + AMIGA_STSIZE))
			s = "KST";
		else if (cp &&
		    va >= (u_int)cp->p_vmspace->vm_map.pmap->pm_stab &&
		    va < ((u_int)cp->p_vmspace->vm_map.pmap->pm_stab +
		    AMIGA_STSIZE))
			s = "UST";
		else
			s = "other";
		printf("pmap_init: validating %s kernel page at %lx -> %lx\n",
		    s, va, pa);

	}
#endif
	if (mmutype == MMU_68040 && pmap == pmap_kernel() && (
	    (va >= amiga_uptbase && va < (amiga_uptbase + AMIGA_UPTMAXSIZE)) ||
	    (va >= (u_int)Sysmap && va < ((u_int)Sysmap + AMIGA_KPTSIZE))))
		cacheable = false;	/* don't cache user page tables */
#endif
	npte = (pa & PG_FRAME) | pte_prot(pmap, prot) | PG_V;
	npte |= (*(int *)pte & (PG_M|PG_U));
	if (wired)
		npte |= PG_W;
	if (!checkpv && !cacheable)
#if defined(M68060) && defined(NO_SLOW_CIRRUS)
#if defined(M68040) || defined(M68030) || defined(M68020)
		npte |= (cputype == CPU_68060 ? PG_CIN : PG_CI);
#else
		npte |= PG_CIN;
#endif
#else
		npte |= PG_CI;
#endif
#if defined(M68040) || defined(M68060)
	else if (mmutype == MMU_68040 && (npte & PG_PROT) == PG_RW &&
	    (kernel_copyback || pmap != pmap_kernel()))
		npte |= PG_CCB;		/* cache copyback */
#endif
	if (flags & VM_PROT_ALL) {
		npte |= PG_U;
		if (flags & VM_PROT_WRITE)
			npte |= PG_M;
	}

	/*
	 * Remember if this was a wiring-only change.
	 * If so, we need not flush the TLB and caches.
	 */
	wired = ((*(int *)pte ^ npte) == PG_W);
#if defined(M68040) || defined(M68060)
	if (mmutype == MMU_68040 && !wired) {
		DCFP(pa);
		ICPP(pa);
	}
#endif
#ifdef DEBUG
	if (pmapdebug & PDB_ENTER)
		printf("enter: new pte value %x\n", npte);
#endif
	*(int *)pte++ = npte;
	if (!wired && active_pmap(pmap))
		TBIS(va);
#ifdef DEBUG
	if ((pmapdebug & PDB_WIRING) && pmap != pmap_kernel()) {
		va -= PAGE_SIZE;
		pmap_check_wiring("enter", trunc_page((vaddr_t) pmap_pte(pmap, va)));
	}
#endif
	return 0;
}

void
pmap_kenter_pa(va, pa, prot)
	vaddr_t va;
	paddr_t pa;
	vm_prot_t prot;
{
	struct pmap *pmap = pmap_kernel();
	pt_entry_t *pte;
	int s, npte;

	PMAP_DPRINTF(PDB_FOLLOW|PDB_ENTER,
	    ("pmap_kenter_pa(%lx, %lx, %x)\n", va, pa, prot));

	/*
	 * Segment table entry not valid, we need a new PT page
	 */

	if (!pmap_ste_v(pmap, va)) {
		s = splvm();
		pmap_enter_ptpage(pmap, va, false);
		splx(s);
	}

	pa = m68k_trunc_page(pa);
	pte = pmap_pte(pmap, va);

	PMAP_DPRINTF(PDB_ENTER, ("enter: pte %p, *pte %x\n", pte, *pte));
	KASSERT(!pmap_pte_v(pte));

	/*
	 * Increment counters
	 */

	pmap->pm_stats.resident_count++;
	pmap->pm_stats.wired_count++;

	/*
	 * Build the new PTE.
	 */

	npte = pa | pte_prot(pmap, prot) | PG_V | PG_W;
#if defined(M68040)
	if (mmutype == MMU_68040 && (npte & (PG_PROT)) == PG_RW)
		npte |= PG_CCB;
#endif

	PMAP_DPRINTF(PDB_ENTER, ("enter: new pte value %x\n", npte));
#if defined(M68040)
	if (mmutype == MMU_68040) {
		DCFP(pa);
		ICPP(pa);
	}
#endif
	*pte = npte;
}

void
pmap_kremove(va, len)
	vaddr_t va;
	vsize_t len;
{
	struct pmap *pmap = pmap_kernel();
	vaddr_t sva, eva, nssva;
	pt_entry_t *pte;

	PMAP_DPRINTF(PDB_FOLLOW|PDB_REMOVE|PDB_PROTECT,
	    ("pmap_kremove(%lx, %lx)\n", va, len));

	sva = va;
	eva = va + len;
	while (sva < eva) {
		nssva = m68k_trunc_seg(sva) + NBSEG;
		if (nssva == 0 || nssva > eva)
			nssva = eva;

		/*
		 * If VA belongs to an unallocated segment,
		 * skip to the next segment boundary.
		 */

		if (!pmap_ste_v(pmap, sva)) {
			sva = nssva;
			continue;
		}

		/*
		 * Invalidate every valid mapping within this segment.
		 */

		pte = pmap_pte(pmap, sva);
		while (sva < nssva) {
			if (pmap_pte_v(pte)) {
#ifdef DEBUG
				struct pv_entry *pv;
				int s;

				pv = pa_to_pvh(pmap_pte_pa(pte));
				s = splvm();
				while (pv->pv_pmap != NULL) {
					KASSERT(pv->pv_pmap != pmap_kernel() ||
						pv->pv_va != sva);
					pv = pv->pv_next;
					if (pv == NULL) {
						break;
					}
				}
				splx(s);
#endif
				/*
				 * Update statistics
				 */

				pmap->pm_stats.wired_count--;
				pmap->pm_stats.resident_count--;

				/*
				 * Invalidate the PTE.
				 */

				*pte = PG_NV;
				TBIS(sva);
			}
			pte++;
			sva += PAGE_SIZE;
		}
	}
}

/*
 *	Routine:	pmap_unwire
 *	Function:	Clear the wired attribute for a map/virtual-address
 *			pair.
 *	In/out conditions:
 *			The mapping must already exist in the pmap.
 */
void
pmap_unwire(pmap, va)
	pmap_t	pmap;
	vaddr_t	va;
{
	u_int *pte;

	pte = pmap_pte(pmap, va);
#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_unwire(%p, %lx)\n", pmap, va);

	/*
	 * Page table page is not allocated.
	 * Should this ever happen?  Ignore it for now,
	 * we don't want to force allocation of unnecessary PTE pages.
	 */
	if (!pmap_ste_v(pmap, va)) {
		if (pmapdebug & PDB_PARANOIA)
			printf("pmap_unwire: invalid STE for %lx\n", va);
		return;
	}
	/*
	 * Page not valid.  Should this ever happen?
	 * Just continue and change wiring anyway.
	 */
	if (!pmap_pte_v(pte)) {
		if (pmapdebug & PDB_PARANOIA)
			printf("pmap_unwire: invalid PTE for %lx\n", va);
	}
#endif
	/*
	 * Wiring is not a hardware characteristic so there is no need
	 * to invalidate TLB.
	 */
	if (pmap_pte_w(pte)) {
		pmap->pm_stats.wired_count--;
		pmap_pte_set_w(pte, false);
	}
#ifdef DIAGNOSTIC
	else {
		printf("pmap_unwire: wiring for pmap %p va 0x%lx "
		    "didn't change!\n", pmap, va);
	}
#endif
}

/*
 *	Routine:	pmap_extract
 *	Function:
 *		Extract the physical page address associated
 *		with the given map/virtual_address pair.
 */

bool
pmap_extract(pmap, va, pap)
	pmap_t	pmap;
	vaddr_t va;
	paddr_t *pap;
{
	bool rv = false;
	paddr_t pa = 0;
	u_int pte;

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_extract(%p, %lx) -> ", pmap, va);
#endif
	if (pmap_ste_v(pmap, va)) {
		pte = *(u_int *)pmap_pte(pmap, va);
		if (pte) {
			pa = (pte & PG_FRAME) | (va & ~PG_FRAME);
			if (pap != NULL)
				*pap = pa;
			rv = true;
		}
	}
#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW) {
		if (rv)
			printf("%lx\n", pa);
		else
			printf("failed\n");
	}
#endif
	return (rv);
}

/*
 *	Copy the range specified by src_addr/len
 *	from the source map to the range dst_addr/len
 *	in the destination map.
 *
 *	This routine is only advisory and need not do anything.
 */
void pmap_copy(dst_pmap, src_pmap, dst_addr, len, src_addr)
	pmap_t	dst_pmap;
	pmap_t	src_pmap;
	vaddr_t	dst_addr;
	vsize_t	len;
	vaddr_t	src_addr;
{
#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_copy(%p, %p, %lx, %lx, %lx)\n",
		       dst_pmap, src_pmap, dst_addr, len, src_addr);
#endif
}

/*
 *	Routine:	pmap_collect
 *	Function:
 *		Garbage collects the physical map system for
 *		pages which are no longer used.
 *		Success need not be guaranteed -- that is, there
 *		may well be pages which are not referenced, but
 *		others may be collected.
 *	Usage:
 *		Called by the pageout daemon when pages are scarce.
 */
void
pmap_collect(pmap)
	pmap_t		pmap;
{
	int bank, s;

	if (pmap != pmap_kernel())
		return;

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_collect(%p)\n", pmap);
	kpt_stats.collectscans++;
#endif
	s = splvm();

	for (bank = 0; bank < vm_nphysseg; bank++)
		pmap_collect1(pmap, ptoa(vm_physmem[bank].start),
		    ptoa(vm_physmem[bank].end));

#ifdef notyet
	/* Go compact and garbage-collect the pv_table. */
	pmap_collect_pv();
#endif
	splx(s);
}

/*
 *	Routine:	pmap_collect1()
 *
 *	Function:
 *		Helper function for pmap_collect().  Do the actual
 *		garbage-collection of range of physical addresses.
 */
void
pmap_collect1(pmap, startpa, endpa)
    pmap_t	pmap;
    paddr_t	startpa, endpa;
{
	paddr_t pa;
	struct pv_entry *pv;
	pt_entry_t *pte;
	paddr_t kpa;
#ifdef DEBUG
	int *ste;
	int opmapdebug = 0;
#endif

	for (pa = startpa; pa < endpa; pa += PAGE_SIZE) {
		struct kpt_page *kpt, **pkpt;

		/*
		 * Locate physical pages which are being used as kernel
		 * page table pages.
		 */
		pv = pa_to_pvh(pa);
		if (pv->pv_pmap != pmap_kernel() || !(pv->pv_flags & PV_PTPAGE))
			continue;
		do {
			if (pv->pv_ptste && pv->pv_ptpmap == pmap_kernel())
				break;
		} while ((pv = pv->pv_next) > 0);
		if (pv == NULL)
			continue;
#ifdef DEBUG
		if (pv->pv_va < (vaddr_t)Sysmap ||
		    pv->pv_va >= (vaddr_t)Sysmap + AMIGA_KPTSIZE)
			printf("collect: kernel PT VA out of range\n");
		else
			goto ok;
		pmap_pvdump(pa);
		continue;
ok:
#endif
		pte = (int *)(pv->pv_va + PAGE_SIZE);
		while (--pte >= (pt_entry_t *)pv->pv_va && *pte == PG_NV)
			;
		if (pte >= (pt_entry_t *)pv->pv_va)
			continue;

#ifdef DEBUG
		if (pmapdebug & (PDB_PTPAGE|PDB_COLLECT)) {
			printf("collect: freeing KPT page at %lx (ste %x@%p)\n",
			       pv->pv_va, *(int *)pv->pv_ptste, pv->pv_ptste);
			opmapdebug = pmapdebug;
			pmapdebug |= PDB_PTPAGE;
		}

		ste = (int *)pv->pv_ptste;
#endif
		/*
		 * If all entries were invalid we can remove the page.
		 * We call pmap_remove to take care of invalidating ST
		 * and Sysptmap entries.
		 */
		(void) pmap_extract(pmap, pv->pv_va, (paddr_t *)&kpa);
		pmap_remove_mapping(pmap, pv->pv_va, PT_ENTRY_NULL,
				PRM_TFLUSH|PRM_CFLUSH);
		/*
		 * Use the physical address to locate the original
		 * (kmem_alloc assigned) address for the page and put
		 * that page back on the free list.
		 */
		for (pkpt = &kpt_used_list, kpt = *pkpt;
		     kpt != NULL;
		     pkpt = &kpt->kpt_next, kpt = *pkpt)
			if (kpt->kpt_pa == kpa)
				break;
#ifdef DEBUG
		if (kpt == NULL)
			panic("pmap_collect: lost a KPT page");
		if (pmapdebug & (PDB_PTPAGE|PDB_COLLECT))
			printf("collect: %lx (%lx) to free list\n",
			       kpt->kpt_va, kpa);
#endif
		*pkpt = kpt->kpt_next;
		kpt->kpt_next = kpt_free_list;
		kpt_free_list = kpt;
#ifdef DEBUG
		kpt_stats.kptinuse--;
		kpt_stats.collectpages++;
		if (pmapdebug & (PDB_PTPAGE|PDB_COLLECT))
			pmapdebug = opmapdebug;

		if (*ste)
			printf("collect: kernel STE at %p still valid (%x)\n",
			       ste, *ste);
		ste = (int *)&Sysptmap[(u_int *)ste-pmap_ste(pmap_kernel(), 0)];
		if (*ste)
			printf("collect: kernel PTmap at %p still valid (%x)\n",
			       ste, *ste);
#endif
	}
}

/*
 *	Mark that a processor is about to be used by a given pmap.
 */
void
pmap_activate(l)
	struct lwp *l;
{
	pmap_t pmap = l->l_proc->p_vmspace->vm_map.pmap;

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_SEGTAB))
		printf("pmap_activate(%p)\n", l);
#endif

	PMAP_ACTIVATE(pmap, (curlwp->l_flag & LW_IDLE) != 0 ||
	    l->l_proc == curproc);
}

/*
 *	Mark that a processor is no longer in use by a given pmap.
 */
void
pmap_deactivate(l)
	struct lwp *l;
{
}

/*
 * pmap_zero_page:		[ INTERFACE ]
 *
 *	Zero the specified (machine independent) page by mapping the page
 *	into virtual memory and using bzero to clear its contents, one
 *	machine dependent page at a time.
 *
 *	Note: WE DO NOT CURRENTLY LOCK THE TEMPORARY ADDRESSES!
 *	      (Actually, we go to splvm(), and since we don't
 *	      support multiple processors, this is sufficient.)
 */
void
pmap_zero_page(phys)
	paddr_t	phys;
{
	int	s;
	int	dst_pte = PG_RW | PG_V;

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_zero_page(%lx)\n", phys);
#endif
#if defined(M68040) || defined(M68060)
	if (mmutype == MMU_68040) {
		/*
		 * Set copyback caching on the page; this is required
		 * for cache consistency (since regular mappings are
		 * copyback as well).
		 */
		dst_pte |= PG_CCB;
	}
#endif

	s = splvm();

	*caddr1_pte = phys | dst_pte;
	TBIS((vaddr_t)CADDR1);
	zeropage(CADDR1);

#ifdef DEBUG
	/*
	 * XXX: Invalidating is not strictly necessary.... Not doing it
         * is saving us a few cycles
	 */
	*caddr1_pte = PG_NV;
	TBIS((vaddr_t)CADDR1);
#endif

	splx(s);
}

/*
 * pmap_copy_page:		[ INTERFACE ]
 *
 *	Copy the specified (machine independent) page by mapping the page
 *	into virtual memory and using bcopy to copy the page, one machine
 *	dependent page at a time.
 *
 *	Note: WE DO NOT CURRENTLY LOCK THE TEMPORARY ADDRESSES!
 *	      (Actually, we go to splvm(), and since we don't
 *	      support multiple processors, this is sufficient.)
 */
void
pmap_copy_page(src, dst)
	paddr_t	src, dst;
{
	int	s;
	int	src_pte = PG_RO | PG_V;
	int	dst_pte = PG_RW | PG_V;

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_copy_page(%lx, %lx)\n", src, dst);
#endif
#if defined(M68040) || defined(M68060)
	if (mmutype == MMU_68040) {
		/*
		 * Set copyback caching on the page; this is required
		 * for cache consistency (since regular mappings are
		 * copyback as well).
		 */
		dst_pte |= PG_CCB;
	}
#endif

	s = splvm();
	*caddr1_pte = src | src_pte;
	TBIS((vaddr_t)CADDR1);
	*caddr2_pte = dst | dst_pte;
	TBIS((vaddr_t)CADDR2);

	copypage(CADDR1, CADDR2);

#ifdef DEBUG
	/*
	 * XXX: Invalidating is not strictly necessary.... Not doing it
         * is saving us a few cycles
	 */
	*caddr1_pte = PG_NV;
	TBIS((vaddr_t)CADDR1);
	*caddr2_pte = PG_NV;
	TBIS((vaddr_t)CADDR2);
#endif

	splx(s);
}

/*
 *	Clear the modify bits on the specified physical page.
 */

bool
pmap_clear_modify(pg)
	struct vm_page *pg;
{
	bool rv;
	paddr_t	pa = VM_PAGE_TO_PHYS(pg);

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_clear_modify(%lx)\n", pa);
#endif

	rv = pmap_testbit(pa, PG_M);
	pmap_changebit(pa, PG_M, false);
	return rv;
}

/*
 *	pmap_clear_reference:
 *
 *	Clear the reference bit on the specified physical page.
 */

bool
pmap_clear_reference(pg)
	struct vm_page *pg;
{
	bool rv;
	paddr_t	pa = VM_PAGE_TO_PHYS(pg);

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_clear_reference(%lx)\n", pa);
#endif

	rv = pmap_testbit(pa, PG_U);
	pmap_changebit(pa, PG_U, false);
	return rv;
}

/*
 *	pmap_is_referenced:
 *
 *	Return whether or not the specified physical page is referenced
 *	by any physical maps.
 */

bool
pmap_is_referenced(pg)
	struct vm_page *pg;
{
	paddr_t	pa = VM_PAGE_TO_PHYS(pg);

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW) {
		bool rv = pmap_testbit(pa, PG_U);
		printf("pmap_is_referenced(%lx) -> %c\n", pa, "FT"[rv]);
		return(rv);
	}
#endif
	return(pmap_testbit(pa, PG_U));
}

/*
 *	pmap_is_modified:
 *
 *	Return whether or not the specified physical page is modified
 *	by any physical maps.
 */

bool
pmap_is_modified(pg)
	struct vm_page *pg;
{
	paddr_t	pa = VM_PAGE_TO_PHYS(pg);

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW) {
		bool rv = pmap_testbit(pa, PG_M);
		printf("pmap_is_modified(%lx) -> %c\n", pa, "FT"[rv]);
		return(rv);
	}
#endif
	return(pmap_testbit(pa, PG_M));
}

paddr_t
pmap_phys_address(ppn)
	int ppn;
{
	return(m68k_ptob(ppn));
}

/*
 * Miscellaneous support routines follow
 */

/*
 * pmap_remove_mapping:
 *
 *	Invalidate a single page denoted by pmap/va.
 *
 *	If (pte != NULL), it is the already computed PTE for the page.
 *
 *	If (flags & PRM_TFLUSH), we must invalidate any TLB information.
 *
 *	If (flags & PRM_CFLUSH), we must flush/invalidate any cache
 *	information.
 *
 *	If (flags & PRM_KEEPPTPAGE), we don't free the page table page
 *	if the reference drops to zero.
 */
static void
pmap_remove_mapping(pmap, va, pte, flags)
	pmap_t pmap;
	vaddr_t va;
	pt_entry_t *pte;
	int flags;
{
	paddr_t pa;
	struct pv_entry *pv, *npv;
	pmap_t ptpmap;
	st_entry_t *ste;
	int s, bits;
#if defined(M68040) || defined(M68060)
	int i;
#endif
#ifdef DEBUG
	pt_entry_t opte;
#endif

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_REMOVE|PDB_PROTECT))
	    printf("pmap_remove_mapping(%p, %lx, %p, %x)\n",
	    		pmap, va, pte, flags);
#endif

	/*
	 * PTE not provided, compute it from pmap and va.
	 */
	if (pte == PT_ENTRY_NULL) {
		pte = pmap_pte(pmap, va);
		if (*pte == PG_NV)
			return;
	}

	pa = pmap_pte_pa(pte);
#ifdef DEBUG
	opte = *pte;
#endif
	/*
	 * Update statistics
	 */
	if (pmap_pte_w(pte))
		pmap->pm_stats.wired_count--;
	pmap->pm_stats.resident_count--;

	/*
	 * Invalidate the PTE after saving the reference modify info.
	 */
#ifdef DEBUG
	if (pmapdebug & PDB_REMOVE)
		printf ("remove: invalidating pte at %p\n", pte);
#endif

	bits = *pte & (PG_U|PG_M);
	*pte = PG_NV;
	if ((flags & PRM_TFLUSH) && active_pmap(pmap))
		TBIS(va);
	/*
	 * For user mappings decrement the wiring count on
	 * the PT page.
	 */
	if (pmap != pmap_kernel()) {
		vaddr_t ptpva = trunc_page((vaddr_t)pte);
		int refs = pmap_ptpage_delref(ptpva);
#ifdef DEBUG
		if (pmapdebug & PDB_WIRING)
			pmap_check_wiring("remove", ptpva);
#endif
		/*
		 * If reference count drops to 0, and we're not instructed
		 * to keep it around, free the PT page.
		 */
		if (refs == 0 && (flags & PRM_KEEPPTPAGE) == 0) {
#ifdef DIAGNOSTIC
			struct pv_entry *_pv;
#endif
			paddr_t _pa;

			_pa = pmap_pte_pa(pmap_pte(pmap_kernel(), ptpva));
#ifdef DIAGNOSTIC
			if (PAGE_IS_MANAGED(_pa) == 0)
				panic("pmap_remove_mapping: unmanaged PT page");
			_pv = pa_to_pvh(_pa);
			if (_pv->pv_ptste == NULL)
				panic("pmap_remove_mapping: ptste == NULL");
			if (_pv->pv_pmap != pmap_kernel() ||
			    _pv->pv_va != ptpva ||
			    _pv->pv_next != NULL)
				panic("pmap_remove_mapping: "
				    "bad PT page pmap %p, va 0x%lx, next %p",
				    _pv->pv_pmap, _pv->pv_va, _pv->pv_next);
#endif
			pmap_remove_mapping(pmap_kernel(), ptpva,
			    NULL, PRM_TFLUSH|PRM_CFLUSH);
			simple_lock(&uvm.kernel_object->vmobjlock);
			uvm_pagefree(PHYS_TO_VM_PAGE(_pa));
			simple_unlock(&uvm.kernel_object->vmobjlock);
#ifdef DEBUG
			if (pmapdebug & (PDB_REMOVE|PDB_PTPAGE))
			    printf("remove: PT page 0x%lx (0x%lx) freed\n",
				    ptpva, _pa);
#endif
		}
	}

	/*
	 * If this isn't a managed page, we are all done.
	 */
	if (PAGE_IS_MANAGED(pa) == 0)
		return;
	/*
	 * Otherwise remove it from the PV table
	 * (raise IPL since we may be called at interrupt time).
	 */
	pv = pa_to_pvh(pa);
	ste = ST_ENTRY_NULL;
	s = splvm();
	/*
	 * If it is the first entry on the list, it is actually
	 * in the header and we must copy the following entry up
	 * to the header.  Otherwise we must search the list for
	 * the entry.  In either case we free the now unused entry.
	 */
	if (pmap == pv->pv_pmap && va == pv->pv_va) {
		ste = pv->pv_ptste;
		ptpmap = pv->pv_ptpmap;
		npv = pv->pv_next;
		if (npv) {
			npv->pv_flags = pv->pv_flags;
			*pv = *npv;
			pmap_free_pv(npv);
		} else
			pv->pv_pmap = NULL;
#ifdef DEBUG
		remove_stats.pvfirst++;
#endif
	} else {
		for (npv = pv->pv_next; npv; npv = npv->pv_next) {
#ifdef DEBUG
			remove_stats.pvsearch++;
#endif
			if (pmap == npv->pv_pmap && va == npv->pv_va)
				break;
			pv = npv;
		}
#ifdef DEBUG
		if (npv == NULL)
			panic("pmap_remove: PA not in pv_tab");
#endif
		ste = npv->pv_ptste;
		ptpmap = npv->pv_ptpmap;
		pv->pv_next = npv->pv_next;
		pmap_free_pv(npv);
		pv = pa_to_pvh(pa);
	}

	/*
	 * If this was a PT page we must also remove the
	 * mapping from the associated segment table.
	 */
	if (ste) {
#ifdef DEBUG
		remove_stats.ptinvalid++;
		if (pmapdebug & (PDB_REMOVE|PDB_PTPAGE))
		    printf("remove: ste was %x@%p pte was %x@%p\n",
			    *ste, ste, opte, pmap_pte(pmap, va));
#endif
#if defined(M68040) || defined(M68060)
		if (mmutype == MMU_68040) {
		    /*
		     * On the 68040, the PT page contains NPTEPG/SG4_LEV3SIZE
		     * page tables, so we need to remove all the associated
		     * segment table entries
		     * (This may be incorrect:  if a single page table is
		     * being removed, the whole page should not be
		     * removed.)
		     */
		    for (i = 0; i < NPTEPG / SG4_LEV3SIZE; ++i)
			*ste++ = SG_NV;
		    ste -= NPTEPG / SG4_LEV3SIZE;
#ifdef DEBUG
		    if (pmapdebug &(PDB_REMOVE|PDB_SEGTAB|0x10000))
			printf("pmap_remove:PT at %lx removed\n", va);
#endif
		} else
#endif /* defined(M68040) || defined(M68060) */
		*ste = SG_NV;
		/*
		 * If it was a user PT page, we decrement the
		 * reference count on the segment table as well,
		 * freeing it if it is now empty.
		 */
		if (ptpmap != pmap_kernel()) {
#ifdef DEBUG
			if (pmapdebug & (PDB_REMOVE|PDB_SEGTAB))
				printf("remove: stab %p, refcnt %d\n",
					ptpmap->pm_stab,
					ptpmap->pm_sref - 1);
			if ((pmapdebug & PDB_PARANOIA) &&
			    ptpmap->pm_stab != (st_entry_t *)trunc_page((vaddr_t) ste))
				panic("remove: bogus ste");
#endif
			if (--(ptpmap->pm_sref) == 0) {
#ifdef DEBUG
				if (pmapdebug&(PDB_REMOVE|PDB_SEGTAB))
				    printf("remove: free stab %p\n",
					    ptpmap->pm_stab);
#endif
				uvm_km_free(kernel_map,
				    (vaddr_t)ptpmap->pm_stab, AMIGA_STSIZE,
				    UVM_KMF_WIRED);
				ptpmap->pm_stab = Segtabzero;
				ptpmap->pm_stpa = Segtabzeropa;
#if defined(M68040) || defined(M68060)
				if (mmutype == MMU_68040)
					ptpmap->pm_stfree = protostfree;
#endif
				/*
				 * XXX may have changed segment table
				 * pointer for current process so
				 * update now to reload hardware.
				 */
				if (active_user_pmap(ptpmap))
					PMAP_ACTIVATE(ptpmap, 1);
			}
#ifdef DEBUG
			else if (ptpmap->pm_sref < 0)
				panic("remove: sref < 0");
#endif
		}
#if 0
		/*
		 * XXX this should be unnecessary as we have been
		 * flushing individual mappings as we go.
		 */
		if (ptpmap == pmap_kernel())
			TBIAS();
		else
			TBIAU();
#endif
		pv->pv_flags &= ~PV_PTPAGE;
		ptpmap->pm_ptpages--;
	}
	/*
	 * Update saved attributes for managed page
	 */
	*pa_to_attribute(pa) |= bits;
	splx(s);
}

/*
 * pmap_ptpage_addref:
 *
 *	Add a reference to the specified PT page.
 */
void
pmap_ptpage_addref(ptpva)
	vaddr_t ptpva;
{
	struct vm_page *pg;

	simple_lock(&uvm.kernel_object->vmobjlock);
	pg = uvm_pagelookup(uvm.kernel_object, ptpva - vm_map_min(kernel_map));
	pg->wire_count++;
	simple_unlock(&uvm.kernel_object->vmobjlock);
}

/*
 * pmap_ptpage_delref:
 *
 *	Delete a reference to the specified PT page.
 */
int
pmap_ptpage_delref(ptpva)
	vaddr_t ptpva;
{
	struct vm_page *pg;
	int rv;

	simple_lock(&uvm.kernel_object->vmobjlock);
	pg = uvm_pagelookup(uvm.kernel_object, ptpva - vm_map_min(kernel_map));
	rv = --pg->wire_count;
	simple_unlock(&uvm.kernel_object->vmobjlock);
	return (rv);
}

static void
amiga_protection_init()
{
	int *kp, prot;

	kp = protection_codes;
	for (prot = 0; prot < 8; prot++) {
		switch (prot) {
		case VM_PROT_NONE | VM_PROT_NONE | VM_PROT_NONE:
			*kp++ = 0;
			break;
		case VM_PROT_READ | VM_PROT_NONE | VM_PROT_NONE:
		case VM_PROT_READ | VM_PROT_NONE | VM_PROT_EXECUTE:
		case VM_PROT_NONE | VM_PROT_NONE | VM_PROT_EXECUTE:
			*kp++ = PG_RO;
			break;
		case VM_PROT_NONE | VM_PROT_WRITE | VM_PROT_NONE:
		case VM_PROT_NONE | VM_PROT_WRITE | VM_PROT_EXECUTE:
		case VM_PROT_READ | VM_PROT_WRITE | VM_PROT_NONE:
		case VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE:
			*kp++ = PG_RW;
			break;
		}
	}
}

/* static */
bool
pmap_testbit(pa, bit)
	paddr_t pa;
	int bit;
{
	pv_entry_t pv;
	int *pte;
	int s;

	pv = pa_to_pvh(pa);
	s = splvm();

	/*
	 * Check saved info first
	 */

	if (*pa_to_attribute(pa) & bit) {
		splx(s);
		return(true);
	}

	/*
	 * Not found, check current mappings returning
	 * immediately if found.
	 */

	if (pv->pv_pmap != NULL) {
		for (; pv; pv = pv->pv_next) {
			pte = (int *) pmap_pte(pv->pv_pmap, pv->pv_va);
			if (*pte & bit) {
				splx(s);
				return(true);
			}
		}
	}
	splx(s);
	return(false);
}

static void
pmap_changebit(pa, bit, setem)
	paddr_t pa;
	int bit;
	bool setem;
{
	pv_entry_t pv;
	int *pte, npte;
	vaddr_t va;
	bool firstpage;
	int s;

	firstpage = true;

#ifdef DEBUG
	if (pmapdebug & PDB_BITS)
		printf("pmap_changebit(%lx, %x, %s)\n",
		    pa, bit, setem ? "set" : "clear");
#endif

	pv = pa_to_pvh(pa);
	s = splvm();

	/*
	 * Clear saved attributes (modify, reference)
	 */

	if (!setem)
		*pa_to_attribute(pa) &= ~bit;

	/*
	 * Loop over all current mappings setting/clearing as appropos
	 * If setting RO do we need to clear the VAC?
	 */

	if (pv->pv_pmap == NULL) {
		splx(s);
		return;
	}
	for (; pv; pv = pv->pv_next) {
		va = pv->pv_va;
		pte = (int *) pmap_pte(pv->pv_pmap, va);
		if (setem)
			npte = *pte | bit;
		else
			npte = *pte & ~bit;
		if (*pte != npte) {

			/*
			 * If we are changing caching status or
			 * protection make sure the caches are
			 * flushed (but only once).
			 */

#if defined(M68040) || defined(M68060)
			if (firstpage && mmutype == MMU_68040 &&
			    ((bit == PG_RO && setem) || (bit & PG_CMASK))) {
				firstpage = false;
				DCFP(pa);
				ICPP(pa);
			}
#endif
			*pte = npte;
			if (active_pmap(pv->pv_pmap))
				TBIS(va);
		}
	}
	splx(s);
}

/* static */
int
pmap_enter_ptpage(pmap, va, can_fail)
	pmap_t pmap;
	vaddr_t va;
	bool can_fail;
{
	paddr_t ptpa;
	struct vm_page *pg;
	pv_entry_t pv;
#ifdef M68060
	u_int stpa;
#endif
	u_int *ste;
	int s;

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_ENTER|PDB_PTPAGE))
		printf("pmap_enter_ptpage: pmap %p, va %lx\n", pmap, va);
	enter_stats.ptpneeded++;
#endif
	/*
	 * Allocate a segment table if necessary.  Note that it is allocated
	 * from kernel_map and not pt_map.  This keeps user page tables
	 * aligned on segment boundaries in the kernel address space.
	 * The segment table is wired down.  It will be freed whenever the
	 * reference count drops to zero.
	 */
	if (pmap->pm_stab == Segtabzero) {
		/* XXX Atari uses kernel_map here: */
		pmap->pm_stab = (st_entry_t *)
		    uvm_km_alloc(kernel_map, AMIGA_STSIZE, 0,
			UVM_KMF_WIRED | UVM_KMF_ZERO |
		 	(can_fail ? UVM_KMF_NOWAIT : 0));
		if (pmap->pm_stab == NULL) {
			pmap->pm_stab = Segtabzero;
			return ENOMEM;
		}
		(void) pmap_extract(pmap_kernel(), (vaddr_t)pmap->pm_stab,
		    (paddr_t *)&pmap->pm_stpa);
#if defined(M68040) || defined(M68060)
		if (mmutype == MMU_68040) {
#if defined(M68060)
			stpa = (u_int)pmap->pm_stpa;
			if (cputype == CPU_68060) {
				pt_entry_t	*pte;

				pte = pmap_pte(pmap_kernel(), pmap->pm_stab);
				while (stpa < (u_int)pmap->pm_stpa +
				    AMIGA_STSIZE) {
					*pte = (*pte & ~PG_CMASK) | PG_CI;
					++pte;
					stpa += PAGE_SIZE;
				}
	 		}
#endif
			pmap->pm_stfree = protostfree;
		}
#endif
		/*
		 * XXX may have changed segment table pointer for current
		 * process so update now to reload hardware.
		 */
		if (active_user_pmap(pmap))
			PMAP_ACTIVATE(pmap, 1);
#ifdef DEBUG
		if (pmapdebug & (PDB_ENTER|PDB_PTPAGE|PDB_SEGTAB))
			printf("enter_pt: pmap %p stab %p(%p)\n",
			       pmap, pmap->pm_stab, pmap->pm_stpa);
#endif
	}

	ste = pmap_ste(pmap, va);

#if defined(M68040) || defined(M68060)
	/*
	 * Allocate level 2 descriptor block if necessary
	 */
	if (mmutype == MMU_68040) {
		if (*ste == SG_NV) {
			int ix;
			void *addr;

			ix = bmtol2(pmap->pm_stfree);
			if (ix == -1)
				panic("enter_pt: out of address space");
			pmap->pm_stfree &= ~l2tobm(ix);
			addr = (void *)&pmap->pm_stab[ix * SG4_LEV2SIZE];
			bzero(addr, SG4_LEV2SIZE * sizeof(st_entry_t));
			addr = (void *)&pmap->pm_stpa[ix * SG4_LEV2SIZE];
			*ste = (u_int) addr | SG_RW | SG_U | SG_V;
#ifdef DEBUG
			if (pmapdebug & (PDB_ENTER|PDB_PTPAGE|PDB_SEGTAB))
				printf("enter_pt: alloc ste2 %d(%p)\n", ix, addr);
#endif
		}
		ste = pmap_ste2(pmap, va);
		/*
		 * Since a level 2 descriptor maps a block of SG4_LEV3SIZE
		 * level 3 descriptors, we need a chunk of NPTEPG/SEG4_LEV3SIZE
		 * (64) such descriptors (PAGE_SIZE/SG4_LEV3SIZE bytes) to map a
		 * PT page -- the unit of allocation.  We set 'ste' to point
		 * to the first entry of that chunk which is validated in its
		 * entirety below.
		 */
		ste = (u_int *)((int)ste & ~(PAGE_SIZE / SG4_LEV3SIZE - 1));
#ifdef DEBUG
		if (pmapdebug &  (PDB_ENTER|PDB_PTPAGE|PDB_SEGTAB))
			printf("enter_pt: ste2 %p (%p)\n",
			    pmap_ste2(pmap, va), ste);
#endif
	}
#endif
	va = trunc_page((vaddr_t)pmap_pte(pmap, va));

	/*
	 * In the kernel we allocate a page from the kernel PT page
	 * free list and map it into the kernel page table map (via
	 * pmap_enter).
	 */
	if (pmap == pmap_kernel()) {
		struct kpt_page *kpt;

		s = splvm();
		if ((kpt = kpt_free_list) == NULL) {
			/*
			 * No PT pages available.
			 * Try once to free up unused ones.
			 */
#ifdef DEBUG
			if (pmapdebug & PDB_COLLECT)
				printf("enter_pt: no KPT pages, collecting...\n");
#endif
			pmap_collect(pmap_kernel());
			if ((kpt = kpt_free_list) == NULL)
				panic("pmap_enter_ptpage: can't get KPT page");
		}
#ifdef DEBUG
		if (++kpt_stats.kptinuse > kpt_stats.kptmaxuse)
			kpt_stats.kptmaxuse = kpt_stats.kptinuse;
#endif
		kpt_free_list = kpt->kpt_next;
		kpt->kpt_next = kpt_used_list;
		kpt_used_list = kpt;
		ptpa = kpt->kpt_pa;
		bzero((char *)kpt->kpt_va, PAGE_SIZE);
		pmap_enter(pmap, va, ptpa, VM_PROT_READ | VM_PROT_WRITE,
		    VM_PROT_READ | VM_PROT_WRITE | PMAP_WIRED);
		pmap_update(pmap);
#if defined(M68060)
		if (cputype == CPU_68060) {
			pmap_changebit(ptpa, PG_CCB, 0);
			pmap_changebit(ptpa, PG_CI, 1);
			DCIS();
	 	}
#endif
#ifdef DEBUG
		if (pmapdebug & (PDB_ENTER|PDB_PTPAGE))
			printf("enter_pt: add &Sysptmap[%d]: %x (KPT page %lx)\n",
			       ste - pmap_ste(pmap, 0),
			       *(int *)&Sysptmap[ste - pmap_ste(pmap, 0)],
			       kpt->kpt_va);
#endif
		splx(s);
	} else {

		/*
		 * For user processes we just allocate a page from the
		 * VM system.  Note that we set the page "wired" count to 1,
		 * which is what we use to check if the page can be freed.
		 * See pmap_remove_mapping().
		 *
		 * Count the segment table reference first so that we won't
		 * lose the segment table when low on memory.
		 */

		pmap->pm_sref++;
#ifdef DEBUG
		if (pmapdebug & (PDB_ENTER|PDB_PTPAGE))
			printf("enter_pt: about to alloc UPT pg at %lx\n", va);
#endif
		simple_lock(&uvm.kernel_object->vmobjlock);
		while ((pg = uvm_pagealloc(uvm.kernel_object,
					   va - vm_map_min(kernel_map),
					   NULL, UVM_PGA_ZERO)) == NULL) {
			simple_unlock(&uvm.kernel_object->vmobjlock);
			uvm_wait("ptpage");
			simple_lock(&uvm.kernel_object->vmobjlock);
		}
		simple_unlock(&uvm.kernel_object->vmobjlock);
		pg->flags &= ~(PG_BUSY|PG_FAKE);
		UVM_PAGE_OWN(pg, NULL);
		ptpa = VM_PAGE_TO_PHYS(pg);
		pmap_enter(pmap_kernel(), va, ptpa,
		    VM_PROT_READ | VM_PROT_WRITE,
		    VM_PROT_READ | VM_PROT_WRITE | PMAP_WIRED);
		pmap_update(pmap_kernel());
	}

#ifdef M68060
	if (cputype == CPU_68060) {
		pmap_changebit(ptpa, PG_CCB, 0);
		pmap_changebit(ptpa, PG_CI, 1);
		DCIS();
	}
#endif
	/*
	 * Locate the PV entry in the kernel for this PT page and
	 * record the STE address.  This is so that we can invalidate
	 * the STE when we remove the mapping for the page.
	 */
	pv = pa_to_pvh(ptpa);
	s = splvm();
	if (pv) {
		pv->pv_flags |= PV_PTPAGE;
		do {
			if (pv->pv_pmap == pmap_kernel() && pv->pv_va == va)
				break;
		} while ((pv = pv->pv_next) > 0);
	}
#ifdef DEBUG
	if (pv == NULL) {
		printf("enter_pt: PV entry for PT page %lx not found\n", ptpa);
		panic("pmap_enter_ptpage: PT page not entered");
	}
#endif
	pv->pv_ptste = ste;
	pv->pv_ptpmap = pmap;
#ifdef DEBUG
	if (pmapdebug & (PDB_ENTER|PDB_PTPAGE))
		printf("enter_pt: new PT page at PA %lx, ste at %p\n", ptpa, ste);
#endif

	/*
	 * Map the new PT page into the segment table.
	 * Also increment the reference count on the segment table if this
	 * was a user page table page.  Note that we don't use vm_map_pageable
	 * to keep the count like we do for PT pages, this is mostly because
	 * it would be difficult to identify ST pages in pmap_pageable to
	 * release them.  We also avoid the overhead of vm_map_pageable.
	 */
#if defined(M68040) || defined(M68060)
	if (mmutype == MMU_68040) {
		u_int *este;

		for (este = &ste[NPTEPG / SG4_LEV3SIZE]; ste < este; ++ste) {
			*ste = ptpa | SG_U | SG_RW | SG_V;
			ptpa += SG4_LEV3SIZE * sizeof(st_entry_t);
		}
	}
	else
#endif
		*(int *)ste = (ptpa & SG_FRAME) | SG_RW | SG_V;
	if (pmap != pmap_kernel()) {
#ifdef DEBUG
		if (pmapdebug & (PDB_ENTER|PDB_PTPAGE|PDB_SEGTAB))
			printf("enter_pt: stab %p refcnt %d\n",
			       pmap->pm_stab, pmap->pm_sref);
#endif
	}
	/*
	 * Flush stale TLB info.
	 */
	if (pmap == pmap_kernel())
		TBIAS();
	else
		TBIAU();
	pmap->pm_ptpages++;
	splx(s);

	return 0;
}

#ifdef DEBUG
void
pmap_pvdump(pa)
	paddr_t pa;
{
	pv_entry_t pv;

	printf("pa %lx", pa);
	for (pv = pa_to_pvh(pa); pv; pv = pv->pv_next)
		printf(" -> pmap %p, va %lx, ptste %p, ptpmap %p, flags %x",
		       pv->pv_pmap, pv->pv_va, pv->pv_ptste, pv->pv_ptpmap,
		       pv->pv_flags);
	printf("\n");
}

/*
 * pmap_check_wiring:
 *
 *	Count the number of valid mappings in the specified PT page,
 *	and ensure that it is consistent with the number of wirings
 *	to that page that the VM system has.
 */
void
pmap_check_wiring(str, va)
	const char *str;
	vaddr_t va;
{
	pt_entry_t *pte;
	paddr_t pa;
	struct vm_page *pg;
	int count;

	if (!pmap_ste_v(pmap_kernel(), va) ||
	    !pmap_pte_v(pmap_pte(pmap_kernel(), va)))
		return;

	pa = pmap_pte_pa(pmap_pte(pmap_kernel(), va));
	pg = PHYS_TO_VM_PAGE(pa);
	if (pg->wire_count >= PAGE_SIZE / sizeof(pt_entry_t)) {
		panic("*%s*: 0x%lx: wire count %d", str, va, pg->wire_count);
		return;
	}

	count = 0;
	for (pte = (pt_entry_t *)va; pte < (pt_entry_t *)(va + PAGE_SIZE);
	     pte++)
		if (*pte)
			count++;
	if (pg->wire_count != count)
		panic("*%s*: 0x%lx: w%d/a%d",
		       str, va, pg->wire_count, count);
}
#endif

/*
 *	Routine:	pmap_virtual_space
 *
 *	Function:
 *		Report the range of available kernel virtual address
 *		space to the VM system during bootstrap.  Called by
 *		vm_bootstrap_steal_memory().
 */
void
pmap_virtual_space(vstartp, vendp)
	vaddr_t     *vstartp, *vendp;
{

	*vstartp = virtual_avail;
	*vendp = virtual_end;
}

/*
 *	Routine:	pmap_procwr
 *
 *	Function:
 *		Synchronize caches corresponding to [addr, addr+len) in
 *		p.
 *
 */
void
pmap_procwr(p, va, len)
	struct proc	*p;
	vaddr_t		va;
	u_long		len;
{
	(void)cachectl1(0x80000004, va, len, p);
}
