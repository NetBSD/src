/*	$NetBSD: pmap.c,v 1.29 1997/07/28 20:41:58 jonathan Exp $	*/

/* 
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department and Ralph Campbell.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *	@(#)pmap.c	8.4 (Berkeley) 1/26/94
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/user.h>
#include <sys/buf.h>
#ifdef SYSVSHM
#include <sys/shm.h>
#endif

#include <vm/vm_kern.h>
#include <vm/vm_page.h>
#include <vm/vm_pageout.h>

#include <mips/cpuregs.h>
#include <mips/locore.h>
#include <mips/pte.h>
#include <machine/cpu.h>

/*
 * For each vm_page_t, there is a list of all currently valid virtual
 * mappings of that page.  An entry is a pv_entry_t, the list is pv_table.
 * XXX really should do this as a part of the higher level code.
 */
typedef struct pv_entry {
	struct pv_entry	*pv_next;	/* next pv_entry */
	struct pmap	*pv_pmap;	/* pmap where mapping lies */
	vm_offset_t	pv_va;		/* virtual address for mapping */
	int		pv_flags;	/* Some flags for the mapping */
} *pv_entry_t;
#define	PV_UNCACHED	0x0001		/* Page is mapped unchached */

pv_entry_t	pv_table;	/* array of entries, one per page */

#define pa_index(pa)		atop((pa) - first_phys_addr)
#define pa_to_pvh(pa)		(&pv_table[pa_index(pa)])

#ifdef DEBUG
struct {
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
	int cachehit;	/* new entry forced valid entry out */
} enter_stats;
struct {
	int calls;
	int removes;
	int flushes;
	int pidflushes;	/* HW pid stolen */
	int pvfirst;
	int pvsearch;
} remove_stats;

int pmapdebug = 0;
#define PDB_FOLLOW	0x0001
#define PDB_INIT	0x0002
#define PDB_ENTER	0x0004
#define PDB_REMOVE	0x0008
#define PDB_CREATE	0x0010
#define PDB_PTPAGE	0x0020
#define PDB_PVENTRY	0x0040
#define PDB_BITS	0x0080
#define PDB_COLLECT	0x0100
#define PDB_PROTECT	0x0200
#define PDB_TLBPID	0x0400
#define PDB_PARANOIA	0x2000
#define PDB_WIRING	0x4000
#define PDB_PVDUMP	0x8000

#endif /* DEBUG */

struct pmap	kernel_pmap_store;

vm_offset_t    	avail_start;	/* PA of first available physical page */
vm_offset_t	avail_end;	/* PA of last available physical page */
vm_size_t	mem_size;	/* memory size in bytes */
vm_offset_t	virtual_avail;  /* VA of first avail page (after kernel bss)*/
vm_offset_t	virtual_end;	/* VA of last avail page (end of kernel AS) */
int		mipspagesperpage;	/* PAGE_SIZE / NBPG */
#ifdef ATTR
char		*pmap_attributes;	/* reference and modify bits */
#endif
struct segtab	*free_segtab;		/* free list kept locally */
u_int		tlbpid_gen = 1;		/* TLB PID generation count */
int		tlbpid_cnt = 2;		/* next available TLB PID */
pt_entry_t	*Sysmap;		/* kernel pte table */
u_int		Sysmapsize;		/* number of pte's in Sysmap */


/* Forward function declarations */
int	pmap_remove_pv __P((pmap_t pmap, vm_offset_t va, vm_offset_t pa));
int	pmap_alloc_tlbpid __P((register struct proc *p));
void	pmap_zero_page __P((vm_offset_t phys));
void pmap_zero_page __P((vm_offset_t));
#ifdef MIPS3XXX		/* XXX - see comment in pmap_enter_pv() */
void pmap_enter_pv __P((pmap_t, vm_offset_t, vm_offset_t, u_int *));
#else
void pmap_enter_pv __P((pmap_t, vm_offset_t, vm_offset_t));
#endif
vm_page_t vm_page_alloc1 __P((void));
void vm_page_free1 __P((vm_page_t));
pt_entry_t *pmap_pte __P((pmap_t, vm_offset_t));

#ifdef MIPS3
void pmap_page_cache __P((vm_offset_t, int));
void mips_dump_segtab __P((struct proc *));
#endif
int pmap_is_page_ro __P((pmap_t, vm_offset_t, int));

/*
 *	Bootstrap the system enough to run with virtual memory.
 *	firstaddr is the first unused kseg0 address (not page aligned).
 */
void
pmap_bootstrap(firstaddr)
	vm_offset_t firstaddr;
{
	register int i;
#ifdef MIPS3
	register pt_entry_t *spte;
#endif
	vm_offset_t start = firstaddr;
	extern int maxmem, physmem;

#define	valloc(name, type, num) \
	    (name) = (type *)firstaddr; firstaddr = (vm_offset_t)((name)+(num))
	/*
	 * Allocate a PTE table for the kernel.
	 * The '1024' comes from PAGER_MAP_SIZE in vm_pager_init().
	 * This should be kept in sync.
	 * We also reserve space for kmem_alloc_pageable() for vm_fork().
	 */
	Sysmapsize = (VM_KMEM_SIZE + VM_MBUF_SIZE + VM_PHYS_SIZE +
		nbuf * MAXBSIZE + 16 * NCARGS) / NBPG + 1024 + 256;
#ifdef SYSVSHM
	Sysmapsize += shminfo.shmall;
#endif
	valloc(Sysmap, pt_entry_t, Sysmapsize);
#ifdef ATTR
	valloc(pmap_attributes, char, physmem);
#endif
	/*
	 * Allocate memory for pv_table.
	 * This will allocate more entries than we really need.
	 * We could do this in pmap_init when we know the actual
	 * phys_start and phys_end but its better to use kseg0 addresses
	 * rather than kernel virtual addresses mapped through the TLB.
	 */
	i = maxmem - mips_btop(MIPS_KSEG0_TO_PHYS(firstaddr));
	valloc(pv_table, struct pv_entry, i);

	/*
	 * Clear allocated memory.
	 */
	firstaddr = mips_round_page(firstaddr);
	bzero((caddr_t)start, firstaddr - start);

	avail_start = MIPS_KSEG0_TO_PHYS(firstaddr);
	avail_end = mips_ptob(maxmem);
	mem_size = avail_end - avail_start;

	virtual_avail = VM_MIN_KERNEL_ADDRESS;
	virtual_end = VM_MIN_KERNEL_ADDRESS + Sysmapsize * NBPG;
	/* XXX need to decide how to set cnt.v_page_size */
	mipspagesperpage = 1;

	simple_lock_init(&pmap_kernel()->pm_lock);
	pmap_kernel()->pm_count = 1;

#ifdef MIPS3
	/*
	 * The R4?00 stores only one copy of the Global bit in the
	 * translation lookaside buffer for each 2 page entry. 
	 * Thus invalid entrys must have the Global bit set so
	 * when Entry LO and Entry HI G bits are anded together
	 * they will produce a global bit to store in the tlb.
	 */
	if (CPUISMIPS3)
		for(i = 0, spte = Sysmap; i < Sysmapsize; i++, spte++)
			spte->pt_entry = MIPS3_PG_G;
#endif
}

/*
 * Bootstrap memory allocator. This function allows for early dynamic
 * memory allocation until the virtual memory system has been bootstrapped.
 * After that point, either kmem_alloc or malloc should be used. This
 * function works by stealing pages from the (to be) managed page pool,
 * stealing virtual address space, then mapping the pages and zeroing them.
 *
 * It should be used from pmap_bootstrap till vm_page_startup, afterwards
 * it cannot be used, and will generate a panic if tried. Note that this
 * memory will never be freed, and in essence it is wired down.
 */
void *
pmap_bootstrap_alloc(size)
	int size;
{
	vm_offset_t val;
	extern boolean_t vm_page_startup_initialized;

	if (vm_page_startup_initialized)
		panic("pmap_bootstrap_alloc: called after startup initialized");

	val = MIPS_PHYS_TO_KSEG0(avail_start);
	size = round_page(size);
	avail_start += size;

	bzero((caddr_t)val, size);
	return ((void *)val);
}

/*
 *	Initialize the pmap module.
 *	Called by vm_init, to initialize any structures that the pmap
 *	system needs to map virtual memory.
 */
void
pmap_init(phys_start, phys_end)
	vm_offset_t phys_start, phys_end;
{

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_INIT))
		printf("pmap_init(%lx, %lx)\n", phys_start, phys_end);
#endif
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
pmap_create(size)
	vm_size_t size;
{
	register pmap_t pmap;

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_CREATE))
		printf("pmap_create(%lx)\n", size);
#endif
	/*
	 * Software use map does not need a pmap
	 */
	if (size)
		return (NULL);

	/* XXX: is it ok to wait here? */
	pmap = (pmap_t) malloc(sizeof *pmap, M_VMPMAP, M_WAITOK);
#ifdef notifwewait
	if (pmap == NULL)
		panic("pmap_create: cannot allocate a pmap");
#endif
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
	register struct pmap *pmap;
{
	register int i;
	int s;
	extern struct vmspace vmspace0;
	extern struct user *proc0paddr;

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_CREATE))
		printf("pmap_pinit(%p)\n", pmap);
#endif
	simple_lock_init(&pmap->pm_lock);
	pmap->pm_count = 1;
	if (free_segtab) {
		s = splimp();
		pmap->pm_segtab = free_segtab;
		free_segtab = *(struct segtab **)free_segtab;
		pmap->pm_segtab->seg_tab[0] = NULL;
		splx(s);
	} else {
		register struct segtab *stp;
		vm_page_t mem;

		mem = vm_page_alloc1();
		if (mem == NULL)		/* XXX - fix this! */
			panic("pmap_pinit: segtab alloc failed");
		pmap_zero_page(VM_PAGE_TO_PHYS(mem));
		pmap->pm_segtab = stp = (struct segtab *)
			MIPS_PHYS_TO_KSEG0(VM_PAGE_TO_PHYS(mem));
		i = mipspagesperpage * (NBPG / sizeof(struct segtab));
		s = splimp();
		while (--i != 0) {
			stp++;
			*(struct segtab **)stp = free_segtab;
			free_segtab = stp;
		}
		splx(s);
	}
#ifdef DIAGNOSTIC
	for (i = 0; i < PMAP_SEGTABSIZE; i++)
		if (pmap->pm_segtab->seg_tab[i] != 0)
			panic("pmap_pinit: pm_segtab != 0");
#endif
	if (pmap == vmspace0.vm_map.pmap) {
		/*
		 * The initial process has already been allocated a TLBPID
		 * in mach_init().
		 */
		pmap->pm_tlbpid = 1;
		pmap->pm_tlbgen = tlbpid_gen;
		proc0paddr->u_pcb.pcb_segtab = (void *)pmap->pm_segtab;
	} else {
		pmap->pm_tlbpid = 0;
		pmap->pm_tlbgen = 0;
	}
}

/*
 *	Retire the given physical map from service.
 *	Should only be called if the map contains
 *	no valid mappings.
 */
void
pmap_destroy(pmap)
	register pmap_t pmap;
{
	int count;

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_CREATE))
		printf("pmap_destroy(%p)\n", pmap);
#endif
	if (pmap == NULL)
		return;

	simple_lock(&pmap->pm_lock);
	count = --pmap->pm_count;
	simple_unlock(&pmap->pm_lock);
	if (count == 0) {
		pmap_release(pmap);
		free((caddr_t)pmap, M_VMPMAP);
	}
}

/*
 * Release any resources held by the given physical map.
 * Called when a pmap initialized by pmap_pinit is being released.
 * Should only be called if the map contains no valid mappings.
 */
void
pmap_release(pmap)
	register pmap_t pmap;
{

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_CREATE))
		printf("pmap_release(%p)\n", pmap);
#endif

	if (pmap->pm_segtab) {
		register pt_entry_t *pte;
		register int i;
		int s;
#ifdef DIAGNOSTIC
		register int j;
#endif

		for (i = 0; i < PMAP_SEGTABSIZE; i++) {
			/* get pointer to segment map */
			pte = pmap->pm_segtab->seg_tab[i];
			if (!pte)
				continue;
#ifdef DIAGNOSTIC
			for (j = 0; j < NPTEPG; j++) {
				if ((pte+j)->pt_entry)
					panic("pmap_release: segmap not empty");
			}
#endif

#ifdef MIPS3
			/*
			 * The pica pmap.c flushed the segmap pages here.  I'm
			 * not sure why, but I suspect it's because the page(s)
			 * were being accessed by KSEG0 (cached) addresses and
			 * may cause cache coherency problems when the page
			 * is reused with KSEG2 (mapped) addresses.  This may
			 * cause problems on machines without secondary caches.
			 */
			if (CPUISMIPS3)
				mips3_HitFlushDCache(
				    (vm_offset_t)pte, PAGE_SIZE);
#endif
			vm_page_free1(
			    PHYS_TO_VM_PAGE(MIPS_KSEG0_TO_PHYS(pte)));

			pmap->pm_segtab->seg_tab[i] = NULL;
		}
		s = splimp();
		*(struct segtab **)pmap->pm_segtab = free_segtab;
		free_segtab = pmap->pm_segtab;
		splx(s);
		pmap->pm_segtab = NULL;
	}
}

/*
 *	Add a reference to the specified pmap.
 */
void
pmap_reference(pmap)
	pmap_t pmap;
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
	register pmap_t pmap;
	vm_offset_t sva, eva;
{
	register vm_offset_t nssva;
	register pt_entry_t *pte;
	unsigned entry;

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_REMOVE|PDB_PROTECT))
		printf("pmap_remove(%p, %lx, %lx)\n", pmap, sva, eva);
	remove_stats.calls++;
#endif
	if (pmap == NULL)
		return;

	if (!pmap->pm_segtab) {
		register pt_entry_t *pte;

		/* remove entries from kernel pmap */
#ifdef DIAGNOSTIC
		if (sva < VM_MIN_KERNEL_ADDRESS || eva > virtual_end)
			panic("pmap_remove: kva not in range");
#endif
		pte = kvtopte(sva);
		for (; sva < eva; sva += NBPG, pte++) {
			entry = pte->pt_entry;
			if (!mips_pg_v(entry))
				continue;
			if (mips_pg_wired(entry))
				pmap->pm_stats.wired_count--;
			pmap->pm_stats.resident_count--;
			if (pmap_remove_pv(pmap, sva, pfn_to_vad(entry))) {
#ifdef MIPS3
				if (CPUISMIPS3)
					MachFlushDCache(sva, PAGE_SIZE);
#endif /* mips3 */
			}
#ifdef ATTR
			pmap_attributes[atop(pfn_to_vad(entry))] = 0;
#endif


			if (CPUISMIPS3)
				/* See above about G bit */
				pte->pt_entry = MIPS3_PG_NV | MIPS3_PG_G;
			else
				pte->pt_entry = MIPS1_PG_NV;

			/*
			 * Flush the TLB for the given address.
			 */
			MachTLBFlushAddr(sva);
#ifdef DEBUG
			remove_stats.flushes++;

#endif
		}
		return;
	}

#ifdef DIAGNOSTIC
	if (eva > VM_MAXUSER_ADDRESS)
		panic("pmap_remove: uva not in range");
#endif
	while (sva < eva) {
		nssva = mips_trunc_seg(sva) + NBSEG;
		if (nssva == 0 || nssva > eva)
			nssva = eva;
		/*
		 * If VA belongs to an unallocated segment,
		 * skip to the next segment boundary.
		 */
		if (!(pte = pmap_segmap(pmap, sva))) {
			sva = nssva;
			continue;
		}
		/*
		 * Invalidate every valid mapping within this segment.
		 */
		pte += uvtopte(sva);
		for (; sva < nssva; sva += NBPG, pte++) {
			entry = pte->pt_entry;
			if (!mips_pg_v(entry))
				continue;
			if (mips_pg_wired(entry))
				pmap->pm_stats.wired_count--;
			pmap->pm_stats.resident_count--;
			if(pmap_remove_pv(pmap, sva, pfn_to_vad(entry))) {
#ifdef MIPS3
				if (CPUISMIPS3)
					MachFlushDCache(sva, PAGE_SIZE);
#endif /* mips3 */
			}
#ifdef ATTR
			pmap_attributes[atop(pfn_to_vad(entry))] = 0;
#endif
			pte->pt_entry = mips_pg_nv_bit();
			/*
			 * Flush the TLB for the given address.
			 */
			if (pmap->pm_tlbgen == tlbpid_gen) {
				MachTLBFlushAddr(sva | (pmap->pm_tlbpid <<
					MIPS_TLB_PID_SHIFT));
#ifdef DEBUG
				remove_stats.flushes++;
#endif
			}
		}
	}
}

/*
 *	pmap_page_protect:
 *
 *	Lower the permission for all mappings to a given page.
 */
void
pmap_page_protect(pa, prot)
	vm_offset_t pa;
	vm_prot_t prot;
{
	register pv_entry_t pv;
	register vm_offset_t va;
	int s;

#ifdef DEBUG
	if ((pmapdebug & (PDB_FOLLOW|PDB_PROTECT)) ||
	    (prot == VM_PROT_NONE && (pmapdebug & PDB_REMOVE)))
		printf("pmap_page_protect(%lx, %x)\n", pa, prot);
#endif
	if (!IS_VM_PHYSADDR(pa))
		return;

	switch (prot) {
	case VM_PROT_READ|VM_PROT_WRITE:
	case VM_PROT_ALL:
		break;

	/* copy_on_write */
	case VM_PROT_READ:
	case VM_PROT_READ|VM_PROT_EXECUTE:
		pv = pa_to_pvh(pa);
		s = splimp();
		/*
		 * Loop over all current mappings setting/clearing as appropos.
		 */
		if (pv->pv_pmap != NULL) {
			for (; pv; pv = pv->pv_next) {
				extern vm_offset_t pager_sva, pager_eva;

				va = pv->pv_va;

				/*
				 * XXX don't write protect pager mappings
				 */
				if (va >= pager_sva && va < pager_eva)
					continue;
				pmap_protect(pv->pv_pmap, va, va + PAGE_SIZE,
					prot);
			}
		}
		splx(s);
		break;

	/* remove_all */
	default:
		pv = pa_to_pvh(pa);
		s = splimp();
		while (pv->pv_pmap != NULL) {
			pmap_remove(pv->pv_pmap, pv->pv_va,
				    pv->pv_va + PAGE_SIZE);
		}
		splx(s);
	}
}

/*
 *	Set the physical protection on the
 *	specified range of this map as requested.
 */
void
pmap_protect(pmap, sva, eva, prot)
	register pmap_t pmap;
	vm_offset_t sva, eva;
	vm_prot_t prot;
{
	register vm_offset_t nssva;
	register pt_entry_t *pte;
	register unsigned entry;
	u_int p;

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_PROTECT))
		printf("pmap_protect(%p, %lx, %lx, %x)\n",
		    pmap, sva, eva, prot);
#endif
	if (pmap == NULL)
		return;

	if ((prot & VM_PROT_READ) == VM_PROT_NONE) {
		pmap_remove(pmap, sva, eva);
		return;
	}

	p = (prot & VM_PROT_WRITE) ? mips_pg_rw_bit() : mips_pg_ro_bit();

	if (!pmap->pm_segtab) {
		/*
		 * Change entries in kernel pmap.
		 * This will trap if the page is writeable (in order to set
		 * the dirty bit) even if the dirty bit is already set. The
		 * optimization isn't worth the effort since this code isn't
		 * executed much. The common case is to make a user page
		 * read-only.
		 */
#ifdef DIAGNOSTIC
		if (sva < VM_MIN_KERNEL_ADDRESS || eva > virtual_end)
			panic("pmap_protect: kva not in range");
#endif
		pte = kvtopte(sva);
		for (; sva < eva; sva += NBPG, pte++) {
			entry = pte->pt_entry;
			if (!mips_pg_v(entry))
				continue;
			entry = (entry & ~(mips_pg_m_bit() |
			    mips_pg_ro_bit())) | p;
			pte->pt_entry = entry;
			/*
			 * Update the TLB if the given address is in the cache.
			 */
			MachTLBUpdate(sva, entry);
		}
		return;
	}

#ifdef DIAGNOSTIC
	if (eva > VM_MAXUSER_ADDRESS)
		panic("pmap_protect: uva not in range");
#endif
	while (sva < eva) {
		nssva = mips_trunc_seg(sva) + NBSEG;
		if (nssva == 0 || nssva > eva)
			nssva = eva;
		/*
		 * If VA belongs to an unallocated segment,
		 * skip to the next segment boundary.
		 */
		if (!(pte = pmap_segmap(pmap, sva))) {
			sva = nssva;
			continue;
		}
		/*
		 * Change protection on every valid mapping within this segment.
		 */
		pte += (sva >> PGSHIFT) & (NPTEPG - 1);
		for (; sva < nssva; sva += NBPG, pte++) {
			entry = pte->pt_entry;
			if (!mips_pg_v(entry))
				continue;
			entry = (entry & ~(mips_pg_m_bit() |
			    mips_pg_ro_bit())) | p;
			pte->pt_entry = entry;
			/*
			 * Update the TLB if the given address is in the cache.
			 */
			if (pmap->pm_tlbgen == tlbpid_gen)
				MachTLBUpdate(sva | (pmap->pm_tlbpid <<
					MIPS_TLB_PID_SHIFT), entry);
		}
	}
}

/*
 *	Return RO protection of page.
 */
int
pmap_is_page_ro(pmap, va, entry)
	pmap_t	    pmap;
	vm_offset_t va;
	int         entry;
{
	return (entry & mips_pg_ro_bit());
}

#ifdef MIPS3
/*
 *	pmap_page_cache:
 *
 *	Change all mappings of a page to cached/uncached.
 */
void
pmap_page_cache(pa,mode)
	vm_offset_t pa;
{
	register pv_entry_t pv;
	register pt_entry_t *pte;
	register unsigned entry;
	register unsigned newmode;
	int s;

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_ENTER))
		printf("pmap_page_uncache(%lx)\n", pa);
#endif
	if (!IS_VM_PHYSADDR(pa))
		return;

	newmode = mode & PV_UNCACHED ? MIPS3_PG_UNCACHED : MIPS3_PG_CACHED;
	pv = pa_to_pvh(pa);
	s = splimp();
	while (pv) {
		pv->pv_flags = (pv->pv_flags & ~PV_UNCACHED) | mode;
		if (!pv->pv_pmap->pm_segtab) {
		/*
		 * Change entries in kernel pmap.
		 */
			pte = kvtopte(pv->pv_va);
			entry = pte->pt_entry;
			if (entry & MIPS3_PG_V) {
				entry = (entry & ~MIPS3_PG_CACHEMODE) | newmode;
				pte->pt_entry = entry;
				MachTLBUpdate(pv->pv_va, entry);
			}
		}
		else {
			if ((pte = pmap_segmap(pv->pv_pmap, pv->pv_va))
			    != NULL) {
				pte += (pv->pv_va >> PGSHIFT) & (NPTEPG - 1);
				entry = pte->pt_entry;
				if (entry & MIPS3_PG_V) {
					entry = (entry & ~MIPS3_PG_CACHEMODE)
					    | newmode;
					pte->pt_entry = entry;
					if (pv->pv_pmap->pm_tlbgen ==
					    tlbpid_gen)
						MachTLBUpdate(pv->pv_va |
						    (pv->pv_pmap->pm_tlbpid <<
						    MIPS3_TLB_PID_SHIFT),
						    entry);
				}
			}
		}
		pv = pv->pv_next;
	}

	splx(s);
}
#endif	/* MIPS3 */	/* r4000,r4400,r4600 */

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
void
pmap_enter(pmap, va, pa, prot, wired)
	register pmap_t pmap;
	vm_offset_t va;
	register vm_offset_t pa;
	vm_prot_t prot;
	boolean_t wired;
{
	register pt_entry_t *pte;
	u_int npte;
	register int i;
	vm_page_t mem;

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_ENTER))
		printf("pmap_enter(%p, %lx, %lx, %x, %x)\n",
		       pmap, va, pa, prot, wired);
#endif
#ifdef DIAGNOSTIC
	if (!pmap)
		panic("pmap_enter: pmap");
	if (!pmap->pm_segtab) {
#ifdef DEBUG
		enter_stats.kernel++;
#endif
		if (va < VM_MIN_KERNEL_ADDRESS || va >= virtual_end)
			panic("pmap_enter: kva");
	} else {
#ifdef DEBUG
		enter_stats.user++;
#endif
		if (va >= VM_MAXUSER_ADDRESS)
			panic("pmap_enter: uva");
	}
	if (pa & 0x80000000)
		panic("pmap_enter: pa");
	if (!(prot & VM_PROT_READ))
		panic("pmap_enter: prot");
#endif

	if (IS_VM_PHYSADDR(pa)) {
		if (!(prot & VM_PROT_WRITE))
			npte = mips_pg_ropage_bit();
		else {
			mem = PHYS_TO_VM_PAGE(pa);
			if ((int)va < 0) {
				/*
				 * Don't bother to trap on kernel writes,
				 * just record page as dirty.
				 */
				npte = mips_pg_rwpage_bit();
				mem->flags &= ~PG_CLEAN;
			} else
#ifdef ATTR
				if ((pmap_attributes[atop(pa)] &
				    PMAP_ATTR_MOD) || !(mem->flags & PG_CLEAN))
#else
				if (!(mem->flags & PG_CLEAN))
#endif
					npte = mips_pg_rwpage_bit();
			else
					npte = mips_pg_cwpage_bit();
				}
#ifdef DEBUG
		enter_stats.managed++;
#endif
#ifdef MIPS3XXX
		pmap_enter_pv(pmap, va, pa, &npte);
#else
		pmap_enter_pv(pmap, va, pa);
#endif
	} else {
		/*
		 * Assumption: if it is not part of our managed memory
		 * then it must be device memory which may be volitile.
		 */
#ifdef DEBUG
		enter_stats.unmanaged++;
#endif
		if (CPUISMIPS3) {
			npte = (prot & VM_PROT_WRITE) ?
			    (MIPS3_PG_IOPAGE & ~MIPS3_PG_G) :
			    (MIPS3_PG_IOPAGE & ~(MIPS3_PG_G | MIPS3_PG_M));
		} else  {
			npte = (prot & VM_PROT_WRITE) ?
			    (MIPS1_PG_M | MIPS1_PG_N) :
			    (MIPS1_PG_RO | MIPS1_PG_N);
		}
	}

	/*
	 * The only time we need to flush the cache is if we
	 * execute from a physical address and then change the data.
	 * This is the best place to do this.
	 * pmap_protect() and pmap_remove() are mostly used to switch
	 * between R/W and R/O pages.
	 * NOTE: we only support cache flush for read only text.
	 */
#ifdef MIPS1
	if ((!CPUISMIPS3) && prot == (VM_PROT_READ | VM_PROT_EXECUTE)) {
		MachFlushICache(MIPS_PHYS_TO_KSEG0(pa), PAGE_SIZE);
	}
#endif

	if (!pmap->pm_segtab) {
		/* enter entries into kernel pmap */
		pte = kvtopte(va);

		/*
		 * XXX more thought... what does ROPAGE mean here? 
		 * is it correc to set all the ROPAGE bits for mips3,
		 * but just the valid (and not read-only) bit on mips1?
		 */
		if (CPUISMIPS3)
			npte |= vad_to_pfn(pa) | MIPS3_PG_ROPAGE | MIPS3_PG_G;
		else
			npte |= vad_to_pfn(pa) | MIPS1_PG_V | MIPS1_PG_G;

		if (wired) {
			pmap->pm_stats.wired_count += mipspagesperpage;
			npte |= mips_pg_wired_bit();
		}
		i = mipspagesperpage;
		do {
			if (!mips_pg_v(pte->pt_entry)) {
				pmap->pm_stats.resident_count++;
			} else {
#ifdef DIAGNOSTIC
				if (mips_pg_wired(pte->pt_entry))
					panic("pmap_enter: kernel wired");
#endif
			}
			/*
			 * Update the same virtual address entry.
			 */
			MachTLBUpdate(va, npte);
			pte->pt_entry = npte;
			va += NBPG;
			npte += vad_to_pfn(NBPG);
			pte++;
		} while (--i != 0);
		return;
	}

	if (!(pte = pmap_segmap(pmap, va))) {
		mem = vm_page_alloc1();
		if (mem == NULL)		/* XXX - fix this! */
			panic("pmap_enter: segmap alloc failed");
		pmap_zero_page(VM_PAGE_TO_PHYS(mem));
		pmap_segmap(pmap, va) = pte = (pt_entry_t *)
			MIPS_PHYS_TO_KSEG0(VM_PAGE_TO_PHYS(mem));
#ifdef DIAGNOSTIC
		for (i = 0; i < NPTEPG; i++) {
			if ((pte+i)->pt_entry)
				panic("pmap_enter: new segmap not empty");
		}
#endif
	}
	pte += (va >> PGSHIFT) & (NPTEPG - 1);

	/*
	 * Now validate mapping with desired protection/wiring.
	 * Assume uniform modified and referenced status for all
	 * MIPS pages in a MACH page.
	 */
	if (CPUISMIPS3)
		npte |= vad_to_pfn(pa);
	else
		npte |= vad_to_pfn(pa) | MIPS1_PG_V;

	if (wired) {
		pmap->pm_stats.wired_count += mipspagesperpage;
		npte |= mips_pg_wired_bit();
	}
#ifdef DEBUG
	if (pmapdebug & PDB_ENTER) {
		printf("pmap_enter: new pte %x", npte);
		if (pmap->pm_tlbgen == tlbpid_gen)
			printf(" tlbpid %d", pmap->pm_tlbpid);
		printf("\n");
	}
#endif
	i = mipspagesperpage;
	do {
		pte->pt_entry = npte;
		if (pmap->pm_tlbgen == tlbpid_gen)
			MachTLBUpdate(va | (pmap->pm_tlbpid <<
				MIPS_TLB_PID_SHIFT), npte);
		va += NBPG;
		npte += vad_to_pfn(NBPG);
		pte++;
	} while (--i != 0);
#ifdef MIPS3
	if (CPUISMIPS3 && (prot == (VM_PROT_READ | VM_PROT_EXECUTE))) {
#ifdef DEBUG
		if (pmapdebug & PDB_ENTER)
			printf("pmap_enter: flush I cache va %lx (%lx)\n",
			    va - mipspagesperpage * NBPG, pa);
#endif
		MachFlushICache(va - mipspagesperpage * NBPG, PAGE_SIZE);
	}
#endif
}

/*
 *	Routine:	pmap_change_wiring
 *	Function:	Change the wiring attribute for a map/virtual-address
 *			pair.
 *	In/out conditions:
 *			The mapping must already exist in the pmap.
 */
void
pmap_change_wiring(pmap, va, wired)
	register pmap_t	pmap;
	vm_offset_t va;
	boolean_t wired;
{
	register pt_entry_t *pte;
	u_int p;
	register int i;

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_WIRING))
		printf("pmap_change_wiring(%p, %lx, %x)\n", pmap, va, wired);
#endif
	if (pmap == NULL)
		return;

	p = wired ? mips_pg_wired_bit() : 0;

	/*
	 * Don't need to flush the TLB since PG_WIRED is only in software.
	 */
	if (!pmap->pm_segtab) {
		/* change entries in kernel pmap */
#ifdef DIAGNOSTIC
		if (va < VM_MIN_KERNEL_ADDRESS || va >= virtual_end)
			panic("pmap_change_wiring");
#endif
		pte = kvtopte(va);
	} else {
		if (!(pte = pmap_segmap(pmap, va)))
			return;
		pte += (va >> PGSHIFT) & (NPTEPG - 1);
	}

	i = mipspagesperpage;
	if (!mips_pg_wired(pte->pt_entry) && p)
		pmap->pm_stats.wired_count += i;
	else if (mips_pg_wired(pte->pt_entry) && !p)
		pmap->pm_stats.wired_count -= i;
	do {
		if (mips_pg_v(pte->pt_entry))
			pte->pt_entry =
			    (pte->pt_entry & ~mips_pg_wired_bit()) | p;
		pte++;
	} while (--i != 0);
}

/*
 *	Routine:	pmap_extract
 *	Function:
 *		Extract the physical page address associated
 *		with the given map/virtual_address pair.
 */
vm_offset_t
pmap_extract(pmap, va)
	register pmap_t	pmap;
	vm_offset_t va;
{
	register vm_offset_t pa;

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_extract(%p, %lx) -> ", pmap, va);
#endif

	if (!pmap->pm_segtab) {
#ifdef DIAGNOSTIC
		if (va < VM_MIN_KERNEL_ADDRESS || va >= virtual_end)
			panic("pmap_extract");
#endif
		pa = pfn_to_vad(kvtopte(va)->pt_entry);
	} else {
		register pt_entry_t *pte;

		if (!(pte = pmap_segmap(pmap, va)))
			pa = 0;
		else {
			pte += (va >> PGSHIFT) & (NPTEPG - 1);
			pa = pfn_to_vad(pte->pt_entry);
		}
	}
	if (pa)
		pa |= va & PGOFSET;

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_extract: pa %lx\n", pa);
#endif
	return (pa);
}

/*
 *	Copy the range specified by src_addr/len
 *	from the source map to the range dst_addr/len
 *	in the destination map.
 *
 *	This routine is only advisory and need not do anything.
 */
void
pmap_copy(dst_pmap, src_pmap, dst_addr, len, src_addr)
	pmap_t dst_pmap;
	pmap_t src_pmap;
	vm_offset_t dst_addr;
	vm_size_t len;
	vm_offset_t src_addr;
{

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_copy(%p, %p, %lx, %lx, %lx)\n",
		       dst_pmap, src_pmap, dst_addr, len, src_addr);
#endif
}

/*
 *	Require that all active physical maps contain no
 *	incorrect entries NOW.  [This update includes
 *	forcing updates of any address map caching.]
 *
 *	Generally used to insure that a thread about
 *	to run will see a semantically correct world.
 */
void
pmap_update()
{

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_update()\n");
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
	pmap_t pmap;
{

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_collect(%p)\n", pmap);
#endif
}

/*
 *	pmap_zero_page zeros the specified (machine independent)
 *	page.
 */
void
pmap_zero_page(phys)
	vm_offset_t phys;
{
	register int *p, *end;

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_zero_page(%lx)\n", phys);
#endif
#ifdef MIPS3
	if (CPUISMIPS3) {
		/*XXX FIXME Not very sophisticated */
		/*	MachFlushCache();*/
		MachFlushDCache(phys, NBPG);
	}
#endif
	p = (int *)MIPS_PHYS_TO_KSEG0(phys);
	end = p + PAGE_SIZE / sizeof(int);
	/* XXX blkclr()? */
	do {
		p[0] = 0;
		p[1] = 0;
		p[2] = 0;
		p[3] = 0;

		p[4] = 0;
		p[5] = 0;
		p[6] = 0;
		p[7] = 0;

		p[8] = 0;
		p[9] = 0;
		p[10] = 0;
		p[11] = 0;

		p[12] = 0;
		p[13] = 0;
		p[14] = 0;
		p[15] = 0;
		p += 16;
	} while (p != end);
#ifdef MIPS3
	/* 
	 * If  we have a virtually-indexed, physically-tagged WB cache,
	 * and no L2 cache to warn of aliased mappings,	we must force a
	 * writeback of the destination out of the L1  cache.  If we don't,
	 * later reads (from virtual addresses mapped to the destination PA)
	 * might read old stale DRAM footprint, not the just-written data.
	 * XXX  Do we need to also invalidate any cache lines matching
	 *      the destination as well?
	 */
	if (CPUISMIPS3) {
		/*XXX FIXME Not very sophisticated */
		/*	MachFlushCache();*/
		MachFlushDCache(phys, NBPG);
	}
#endif
}

/*
 *	pmap_copy_page copies the specified (machine independent)
 *	page.
 */
void
pmap_copy_page(src, dst)
	vm_offset_t src, dst;
{
	register int *s, *d, *end;
	register int tmp0, tmp1, tmp2, tmp3;
	register int tmp4, tmp5, tmp6, tmp7;

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_copy_page(%lx, %lx)\n", src, dst);
#endif
#ifdef MIPS3
	/* 
	 * If  we have a virtually-indexed, physically-tagged cache,
	 * and no L2 cache to warn of aliased mappings,  we must force an
	 * write-back of all L1 cache lines of the source physical address,
	 * irrespective of their  virtual address (cache indexes).
	 * If we don't, our copy loop might read and copy stale DRAM
	 * footprint instead of the fresh (but dirty) data in a WB cache.
	 * XXX invalidate any cached lines of the destination PA
	 *     here also?
	 */
	if (CPUISMIPS3) {
		/*XXX FIXME Not very sophisticated */
		/*	MachFlushCache(); */
		MachFlushDCache(src, NBPG);
	}
#endif
	s = (int *)MIPS_PHYS_TO_KSEG0(src);
	d = (int *)MIPS_PHYS_TO_KSEG0(dst);
	end = s + PAGE_SIZE / sizeof(int);
	do {
		tmp0 = s[0];
		tmp1 = s[1];
		tmp2 = s[2];
		tmp3 = s[3];
		d[0] = tmp0;
		d[1] = tmp1;
		d[2] = tmp2;
		d[3] = tmp3;

		tmp4 = s[4];
		tmp5 = s[5];
		tmp6 = s[6];
		tmp7 = s[7];
		d[4] = tmp4;
		d[5] = tmp5;
		d[6] = tmp6;
		d[7] = tmp7;

		tmp0 = s[8];
		tmp1 = s[9];
		tmp2 = s[10];
		tmp3 = s[11];
		d[8] = tmp0;
		d[9] = tmp1;
		d[10] = tmp2;
		d[11] = tmp3;

		tmp4 = s[12];
		tmp5 = s[13];
		tmp6 = s[14];
		tmp7 = s[15];
		d[12] = tmp4;
		d[13] = tmp5;
		d[14] = tmp6;
		d[15] = tmp7;

		s += 16;
		d += 16;
	} while (s != end);
#ifdef MIPS3
	/* 
	 * If  we have a virtually-indexed, physically-tagged WB cache,
	 * and no L2 cache to warn of aliased mappings,	we must force a
	 * writeback of the destination out of the L1  cache.  If we don't,
	 * later reads (from virtual addresses mapped to the destination PA)
	 * might read old stale DRAM footprint, not the just-written data.
	 * XXX  Do we need to also invalidate any cache lines matching
	 *      the destination as well?
	 */
	if (CPUISMIPS3) {
		/*XXX FIXME Not very sophisticated */
		/*	MachFlushCache();*/
		MachFlushDCache(dst, NBPG);
	}
#endif
}

/*
 *	Routine:	pmap_pageable
 *	Function:
 *		Make the specified pages (by pmap, offset)
 *		pageable (or not) as requested.
 *
 *		A page which is not pageable may not take
 *		a fault; therefore, its page table entry
 *		must remain valid for the duration.
 *
 *		This routine is merely advisory; pmap_enter
 *		will specify that these pages are to be wired
 *		down (or not) as appropriate.
 */
void
pmap_pageable(pmap, sva, eva, pageable)
	pmap_t		pmap;
	vm_offset_t	sva, eva;
	boolean_t	pageable;
{

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_pageable(%p, %lx, %lx, %x)\n",
		       pmap, sva, eva, pageable);
#endif
}

/*
 *	Clear the modify bits on the specified physical page.
 */
void
pmap_clear_modify(pa)
	vm_offset_t pa;
{

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_clear_modify(%lx)\n", pa);
#endif
#ifdef ATTR
	pmap_attributes[atop(pa)] &= ~PMAP_ATTR_MOD;
#endif
}

/*
 *	pmap_clear_reference:
 *
 *	Clear the reference bit on the specified physical page.
 */
void
pmap_clear_reference(pa)
	vm_offset_t pa;
{

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_clear_reference(%lx)\n", pa);
#endif
#ifdef ATTR
	pmap_attributes[atop(pa)] &= ~PMAP_ATTR_REF;
#endif
}

/*
 *	pmap_is_referenced:
 *
 *	Return whether or not the specified physical page is referenced
 *	by any physical maps.
 */
boolean_t
pmap_is_referenced(pa)
	vm_offset_t pa;
{
#ifdef ATTR
	return (pmap_attributes[atop(pa)] & PMAP_ATTR_REF);
#else
	return (FALSE);
#endif
}

/*
 *	pmap_is_modified:
 *
 *	Return whether or not the specified physical page is modified
 *	by any physical maps.
 */
boolean_t
pmap_is_modified(pa)
	vm_offset_t pa;
{
#ifdef ATTR
	return (pmap_attributes[atop(pa)] & PMAP_ATTR_MOD);
#else
	return (FALSE);
#endif
}

vm_offset_t
pmap_phys_address(ppn)
	int ppn;
{

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_phys_address(%x)\n", ppn);
#endif
	return (mips_ptob(ppn));
}

/*
 * Miscellaneous support routines
 */

/*
 * Allocate a hardware PID and return it.
 * It takes almost as much or more time to search the TLB for a
 * specific PID and flush those entries as it does to flush the entire TLB.
 * Therefore, when we allocate a new PID, we just take the next number. When
 * we run out of numbers, we flush the TLB, increment the generation count
 * and start over. PID zero is reserved for kernel use.
 * This is called only by switch().
 */
int
pmap_alloc_tlbpid(p)
	register struct proc *p;
{
	register pmap_t pmap;
	register int id;

	pmap = p->p_vmspace->vm_map.pmap;
	if (pmap->pm_tlbgen != tlbpid_gen) {
		id = tlbpid_cnt;
		if (id == MIPS_TLB_NUM_PIDS) {
			MachTLBFlush();
			/* reserve tlbpid_gen == 0 to alway mean invalid */
			if (++tlbpid_gen == 0)
				tlbpid_gen = 1;
			id = 1;
		}
		tlbpid_cnt = id + 1;
		pmap->pm_tlbpid = id;
		pmap->pm_tlbgen = tlbpid_gen;
	} else
		id = pmap->pm_tlbpid;

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_TLBPID)) {
		if (curproc)
			printf("pmap_alloc_tlbpid: curproc %d '%s' ",
				curproc->p_pid, curproc->p_comm);
		else
			printf("pmap_alloc_tlbpid: curproc <none> ");
		printf("segtab %p tlbpid %d pid %d '%s'\n",
			pmap->pm_segtab, id, p->p_pid, p->p_comm);
	}
#endif
	return (id);
}

/*
 * Enter the pmap and virtual address into the
 * physical to virtual map table.
 */
void
#ifdef MIPS3XXX		/* XXX - see comment below */
pmap_enter_pv(pmap, va, pa, npte)
#else
pmap_enter_pv(pmap, va, pa)
#endif
	pmap_t pmap;
	vm_offset_t va, pa;
#ifdef MIPS3XXX
	u_int *npte;
#endif
{
	register pv_entry_t pv, npv;
	int s;

	pv = pa_to_pvh(pa);
	s = splimp();
#ifdef DEBUG
	if (pmapdebug & PDB_ENTER)
		printf("pmap_enter: pv %p: was %lx/%p/%p\n",
		       pv, pv->pv_va, pv->pv_pmap, pv->pv_next);
#endif
	if (pv->pv_pmap == NULL) {
		/*
		 * No entries yet, use header as the first entry
		 */
#ifdef DEBUG
		if (pmapdebug & PDB_PVENTRY)
			printf("pmap_enter: first pv: pmap %p va %lx\n",
				pmap, va);
		enter_stats.firstpv++;
#endif
		pv->pv_va = va;
		pv->pv_flags = 0;
		pv->pv_pmap = pmap;
		pv->pv_next = NULL;
	} else {
#ifdef MIPS3XXX
/*
 * This isn't needed with the DECstations - the virtual coherency exception
 * handler appears to be able to deal with this.  It may need to be enabled
 * for other R4K systems.
 */
			if (!(pv->pv_flags & PV_UNCACHED)) {
			/*
			 * There is at least one other VA mapping this page.
			 * Check if they are cache index compatible. If not
			 * remove all mappings, flush the cache and set page
			 * to be mapped uncached. Caching will be restored
			 * when pages are mapped compatible again.
			 */
				for (npv = pv; npv; npv = npv->pv_next) {
					/*
					 * Check cache aliasing incompatibility
					 */
					if((npv->pv_va & mips_CacheAliasMask) != (va & mips_CacheAliasMask)) {
						pmap_page_cache(pa,PV_UNCACHED);
						MachFlushDCache(pv->pv_va, PAGE_SIZE);
						*npte = (*npte & ~MIPS3_PG_CACHEMODE) | MIPS3_PG_UNCACHED;
						break;
					}
				}
			}
			else {
				*npte = (*npte & ~MIPS3_PG_CACHEMODE) | MIPS3_PG_UNCACHED;
			}
#endif
		/*
		 * There is at least one other VA mapping this page.
		 * Place this entry after the header.
		 *
		 * Note: the entry may already be in the table if
		 * we are only changing the protection bits.
		 */
		for (npv = pv; npv; npv = npv->pv_next) {
			if (pmap == npv->pv_pmap && va == npv->pv_va) {
#ifdef DIAGNOSTIC
				pt_entry_t *pte;
				unsigned entry;

				if (!pmap->pm_segtab)
					entry = kvtopte(va)->pt_entry;
				else {
					pte = pmap_segmap(pmap, va);
					if (pte) {
						pte += (va >> PGSHIFT) &
						    (NPTEPG - 1);
						entry = pte->pt_entry;
					} else
						entry = 0;
				}
				if (!mips_pg_v(entry) ||
				    pfn_to_vad(entry) != pa)
					printf(
		"pmap_enter: found va %lx pa %lx in pv_table but != %x\n",
						va, pa, entry);
#endif
				goto fnd;
			}
		}
#ifdef DEBUG
		if (pmapdebug & PDB_PVENTRY)
			printf("pmap_enter: new pv: pmap %p va %lx\n",
				pmap, va);
#endif
		/* can this cause us to recurse forever? */
		npv = (pv_entry_t)
			malloc(sizeof *npv, M_VMPVENT, M_NOWAIT);
		npv->pv_va = va;
		npv->pv_pmap = pmap;
		npv->pv_flags = pv->pv_flags;
		npv->pv_next = pv->pv_next;
		pv->pv_next = npv;
#ifdef DEBUG
		if (!npv->pv_next)
			enter_stats.secondpv++;
#endif
	fnd:
		;
	}
	splx(s);
}

/*
 * Remove a physical to virtual address translation.
 * Returns TRUE if it was the last mapping and cached, else FALSE.
 */
int
pmap_remove_pv(pmap, va, pa)
	pmap_t pmap;
	vm_offset_t va, pa;
{
	register pv_entry_t pv, npv;
	int s, last;

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_PVENTRY))
		printf("pmap_remove_pv(%p, %lx, %lx)\n", pmap, va, pa);
#endif
	/*
	 * Remove page from the PV table (raise IPL since we
	 * may be called at interrupt time).
	 */
	if (!IS_VM_PHYSADDR(pa))
		return(TRUE);
	pv = pa_to_pvh(pa);
	s = splimp();
	/*
	 * If it is the first entry on the list, it is actually
	 * in the header and we must copy the following entry up
	 * to the header.  Otherwise we must search the list for
	 * the entry.  In either case we free the now unused entry.
	 */
	if (pmap == pv->pv_pmap && va == pv->pv_va) {
		last = (pv->pv_flags & PV_UNCACHED) ? FALSE : TRUE;
		npv = pv->pv_next;
		if (npv) {
			*pv = *npv;
			free((caddr_t)npv, M_VMPVENT);
		} else
			pv->pv_pmap = NULL;
#ifdef DEBUG
		remove_stats.pvfirst++;
#endif
	} else {
		last = FALSE;
		for (npv = pv->pv_next; npv; pv = npv, npv = npv->pv_next) {
#ifdef DEBUG
			remove_stats.pvsearch++;
#endif
			if (pmap == npv->pv_pmap && va == npv->pv_va)
				break;
		}
		if (npv) {
			pv->pv_next = npv->pv_next;
			free((caddr_t)npv, M_VMPVENT);
		}
	}
	splx(s);
	return(last);
}

/*
 *	vm_page_alloc1:
 *
 *	Allocate and return a memory cell with no associated object.
 */
vm_page_t
vm_page_alloc1()
{
	register vm_page_t	mem;
	int		spl;

	spl = splimp();				/* XXX */
	simple_lock(&vm_page_queue_free_lock);
	if (vm_page_queue_free.tqh_first == NULL) {
		simple_unlock(&vm_page_queue_free_lock);
		splx(spl);
		return (NULL);
	}

	mem = vm_page_queue_free.tqh_first;
	TAILQ_REMOVE(&vm_page_queue_free, mem, pageq);

	cnt.v_free_count--;
	simple_unlock(&vm_page_queue_free_lock);
	splx(spl);

	mem->flags = PG_BUSY | PG_CLEAN | PG_FAKE;
	mem->wire_count = 0;

	/*
	 *	Decide if we should poke the pageout daemon.
	 *	We do this if the free count is less than the low
	 *	water mark, or if the free count is less than the high
	 *	water mark (but above the low water mark) and the inactive
	 *	count is less than its target.
	 *
	 *	We don't have the counts locked ... if they change a little,
	 *	it doesn't really matter.
	 */

	if (cnt.v_free_count < cnt.v_free_min ||
	    (cnt.v_free_count < cnt.v_free_target &&
	     cnt.v_inactive_count < cnt.v_inactive_target))
		thread_wakeup((void *)&vm_pages_needed);
	return (mem);
}

/*
 *	vm_page_free1:
 *
 *	Returns the given page to the free list,
 *	disassociating it with any VM object.
 *
 *	Object and page must be locked prior to entry.
 */
void
vm_page_free1(mem)
	register vm_page_t	mem;
{

	if (mem->flags & PG_ACTIVE) {
		TAILQ_REMOVE(&vm_page_queue_active, mem, pageq);
		mem->flags &= ~PG_ACTIVE;
		cnt.v_active_count--;
	}

	if (mem->flags & PG_INACTIVE) {
		TAILQ_REMOVE(&vm_page_queue_inactive, mem, pageq);
		mem->flags &= ~PG_INACTIVE;
		cnt.v_inactive_count--;
	}

	if (!(mem->flags & PG_FICTITIOUS)) {
		int	spl;

		spl = splimp();
		simple_lock(&vm_page_queue_free_lock);
		TAILQ_INSERT_TAIL(&vm_page_queue_free, mem, pageq);

		cnt.v_free_count++;
		simple_unlock(&vm_page_queue_free_lock);
		splx(spl);
	}
}

pt_entry_t *
pmap_pte(pmap, va)
	pmap_t pmap;
	vm_offset_t va;
{
	pt_entry_t *pte = NULL;

	if (pmap->pm_segtab == NULL)
		pte = kvtopte(va);
	else if ((pte = pmap_segmap(pmap, va)) != NULL)
		pte += (va >> PGSHIFT) & (NPTEPG - 1);
	return pte;
}

#ifdef MIPS3
/*
 * Find first virtual address >= *vap that doesn't cause
 * a cache alias conflict.
 */
void
pmap_prefer(foff, vap)
	register vm_offset_t foff;
	register vm_offset_t *vap;
{
	register vm_offset_t	va = *vap;
	register long		d;

	if (CPUISMIPS3) {
		d = foff - va;
		/* Use 64K to prevent virtual coherency exceptions */
		d &= (0x10000 - 1);
		*vap = va + d;
	}
}
#endif
