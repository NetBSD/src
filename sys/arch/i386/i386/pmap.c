/* 
 * Copyright (c) 1993 Charles Hannum.
 * Copyright (c) 1991 Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department and William Jolitz of UUNET Technologies Inc.
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
 *	from: @(#)pmap.c	7.7 (Berkeley)	5/12/91
 *	$Id: pmap.c,v 1.8.2.14 1993/11/09 10:53:58 mycroft Exp $
 */

/*
 * Derived originally from an old hp300 version by Mike Hibler.  The version
 * by William Jolitz has been heavily modified to allow non-contiguous
 * mapping of physical memory by Wolfgang Solfrank, and to fix several bugs
 * and greatly speedup it up by Charles Hannum.
 * 
 * A recursive map [a pde which points to the page directory] is used to map
 * the page tables using the pagetables themselves. This is done to reduce
 * the impact on kernel virtual memory for lots of sparse address space, and
 * to reduce the cost of memory to each process.
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
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/user.h>

#include <vm/vm.h>
#include <vm/vm_kern.h>
#include <vm/vm_page.h>

#include <machine/cpu.h>
#include <machine/cpufunc.h>

#include <i386/isa/isa.h>

#include "dma.h"

/*
 * Allocate various and sundry SYSMAPs used in the days of old VM
 * and not yet converted.  XXX.
 */
#define BSDVM_COMPAT	1

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
} enter_stats;
struct {
	int calls;
	int removes;
	int pvfirst;
	int pvsearch;
	int ptinvalid;
	int uflushes;
	int sflushes;
} remove_stats;

int debugmap = 0;
int pmapdebug = 0 /* 0xffff */;
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
#define PDB_PDRTAB	0x0400
#define PDB_PARANOIA	0x2000
#define PDB_WIRING	0x4000
#define PDB_PVDUMP	0x8000

int pmapvacflush = 0;
#define	PVF_ENTER	0x01
#define	PVF_REMOVE	0x02
#define	PVF_PROTECT	0x04
#define	PVF_TOTAL	0x80
#endif

/*
 * Get PDEs and PTEs for user/kernel address space
 */
#define	pmap_pde(m, v)	(&((m)->pm_pdir[((vm_offset_t)(v) >> PDSHIFT)&1023]))

#define pmap_pte_pa(pte)	(*(int *)(pte) & PG_FRAME)

#define pmap_pde_v(pte)		((pte)->pd_v)
#define pmap_pte_w(pte)		((pte)->pg_w)
#define pmap_pte_m(pte)		((pte)->pg_m)
#define pmap_pte_u(pte)		((pte)->pg_u)
#define pmap_pte_v(pte)		((pte)->pg_v)
#define pmap_pte_set_w(pte, v)		((pte)->pg_w = (v))
#define pmap_pte_set_prot(pte, v)	((pte)->pg_prot = (v) >> 1)

/*
 * Given a map and a machine independent protection code,
 * convert to a vax protection code.
 */
#define pte_prot(m, p)	(protection_codes[p])
int	protection_codes[8];

struct pmap	kernel_pmap_store;
pmap_t		kernel_pmap;

vm_offset_t    	avail_start;	/* PA of first available physical page */
vm_offset_t	avail_end;	/* PA of last available physical page */
vm_offset_t	virtual_avail;  /* VA of first avail page (after kernel bss)*/
vm_offset_t	virtual_end;	/* VA of last avail page (end of kernel AS) */
boolean_t	pmap_initialized = FALSE;	/* Has pmap_init completed? */
char		*pmap_attributes;	/* reference and modify bits */

boolean_t pmap_testbit __P((vm_offset_t, int));
void pmap_changebit __P((vm_offset_t, int, int));

/* XXX should be in a .h file somewhere */
#define	PMAP_CLEAR_MODIFY(pa)		pmap_changebit(pa, 0, ~PG_M)
#define	PMAP_CLEAR_REFERENCE(pa)	pmap_changebit(pa, 0, ~PG_U)
#define	PMAP_IS_REFERENCED(pa)		pmap_testbit(pa, PG_U)
#define	PMAP_IS_MODIFIED(pa)		pmap_testbit(pa, PG_M)

#define	pmap_valid_page(pa)	(pmap_initialized && pmap_page_index(pa) >= 0)

#if BSDVM_COMPAT
#include "msgbuf.h"

/*
 * All those kernel PT submaps that BSD is so fond of
 */
struct pte	*CMAP1, *CMAP2, *mmap;
caddr_t		CADDR1, CADDR2, vmmap;
struct pte	*msgbufmap;
#endif	/* BSDVM_COMPAT */

/*
 *	Bootstrap the system enough to run with virtual memory.
 *	Map the kernel's code and data, and allocate the system page table.
 *
 *	On the I386 this is called after mapping has already been enabled
 *	and just syncs the pmap module with what has already been done.
 *	[We can't call it easily with mapping off since the kernel is not
 *	mapped with PA == VA, hence we would have to relocate every address
 *	from the linked base (virtual) address 0xFE000000 to the actual
 *	(physical) address starting relative to 0]
 */
struct pte *pmap_pte();

void
pmap_bootstrap(virtual_start)
	vm_offset_t virtual_start;
{
#if BSDVM_COMPAT
	vm_offset_t va;
	struct pte *pte;
#endif
	extern int physmem;
	extern vm_offset_t reserve_dumppages(vm_offset_t);
	extern int IdlePTD;

	/* XXX: allow for msgbuf */
	avail_end -= i386_round_page(sizeof(struct msgbuf));

	virtual_avail = virtual_start;
	virtual_end = VM_MAX_KERNEL_ADDRESS;

	/*
	 * Initialize protection array.
	 */
	i386_protection_init();

	/*
	 * The kernel's pmap is statically allocated so we don't
	 * have to use pmap_create, which is unlikely to work
	 * correctly at this part of the boot sequence.
	 */
	kernel_pmap = &kernel_pmap_store;

#ifdef notdef
	/*
	 * Create Kernel page directory table and page maps.
	 * [ currently done in locore. i have wild and crazy ideas -wfj ]
	 */
	bzero(firstaddr, 4*NBPG);
	kernel_pmap->pm_pdir = firstaddr + VM_MIN_KERNEL_ADDRESS;
	kernel_pmap->pm_ptab = firstaddr + VM_MIN_KERNEL_ADDRESS + NBPG;

	firstaddr += NBPG;
	for (x = i386_btod(VM_MIN_KERNEL_ADDRESS);
		x < i386_btod(VM_MIN_KERNEL_ADDRESS)+3; x++) {
			struct pde *pde;
		pde = kernel_pmap->pm_pdir + x;
		*(int *)pde = firstaddr + x*NBPG | PG_V | PG_KW;
	}
#else
	kernel_pmap->pm_pdir = (pd_entry_t *)(KERNBASE + IdlePTD);
#endif

	simple_lock_init(&kernel_pmap->pm_lock);
	kernel_pmap->pm_count = 1;

#if BSDVM_COMPAT
	/*
	 * Allocate all the submaps we need
	 */
#define	SYSMAP(c, p, v, n)	\
	v = (c)va; va += ((n)*NBPG); p = pte; pte += (n);

	va = virtual_avail;
	pte = pmap_pte(kernel_pmap, va);

	SYSMAP(caddr_t		,CMAP1		,CADDR1	   ,1		)
	SYSMAP(caddr_t		,CMAP2		,CADDR2	   ,1		)
	SYSMAP(caddr_t		,mmap		,vmmap	   ,1		)
	SYSMAP(struct msgbuf *	,msgbufmap	,msgbufp   ,1		)
	virtual_avail = va;
#endif

	/*
	 * Reserve pmap space for mapping physical pages during dump.
	 */
	virtual_avail = reserve_dumppages(virtual_avail);

	/*
	 * reserve special hunk of memory for use by bus dma as a bounce
	 * buffer (contiguous virtual *and* physical memory).  XXX
	 */
#if	NDMA > 0
	isaphysmem = pmap_steal_memory(DMA_BOUNCE * NBPG);
#endif

	*(int *)PTD = 0;
	tlbflush();
}

void pmap_virtual_space(startp, endp)
	vm_offset_t	*startp;
	vm_offset_t	*endp;
{

	*startp = virtual_avail;
	*endp = virtual_end;
}

/*
 *	Initialize the pmap module.
 *	Called by vm_init, to initialize any structures that the pmap
 *	system needs to map virtual memory.
 */
void
pmap_init()
{
	vm_offset_t	addr, addr2;
	vm_size_t	npg, s;
	int		rv;
	extern int KPTphys;

	if (PAGE_SIZE != NBPG)
		panic("pmap_init: CLSIZE != 1");

	npg = pmap_page_index(avail_end - 1) + 1;
	s = (vm_size_t) (sizeof(struct pv_entry) * npg + npg);
	s = round_page(s);
	addr = (vm_offset_t) kmem_alloc(kernel_map, s);
	pv_table = (pv_entry_t) addr;
	addr += sizeof(struct pv_entry) * npg;
	pmap_attributes = (char *) addr;

#ifdef DEBUG
	if (pmapdebug & PDB_INIT)
		printf("pmap_init: %x bytes (%x pgs): tbl %x attr %x\n",
		       s, npg, pv_table, pmap_attributes);
#endif

	/*
	 * Now it is safe to enable pv_table recording.
	 */
	pmap_initialized = TRUE;
}

/*
 *	Used to map a range of physical addresses into kernel
 *	virtual address space.
 *
 *	For now, VM is already on, we only need to map the
 *	specified memory.
 */
vm_offset_t
pmap_map(virt, start, end, prot)
	vm_offset_t	virt;
	vm_offset_t	start;
	vm_offset_t	end;
	int		prot;
{

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_map(%x, %x, %x, %x)\n", virt, start, end, prot);
#endif
	while (start < end) {
		pmap_enter(kernel_pmap, virt, start, prot, FALSE);
		virt += NBPG;
		start += NBPG;
	}
	return virt;
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
 *
 * [ just allocate a ptd and mark it uninitialize -- should we track
 *   with a table which process has which ptd? -wfj ]
 */
pmap_t
pmap_create(size)
	vm_size_t	size;
{
	register pmap_t pmap;

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_CREATE))
		printf("pmap_create(%x)\n", size);
#endif

	/*
	 * Software use map does not need a pmap
	 */
	if (size)
		return NULL;

	/* XXX: is it ok to wait here? */
	pmap = (pmap_t) malloc(sizeof *pmap, M_VMPMAP, M_WAITOK);
#ifdef notifwewait
	if (pmap == NULL)
		panic("pmap_create: cannot allocate a pmap");
#endif
	bzero(pmap, sizeof(*pmap));
	pmap_pinit(pmap);
	return pmap;
}

/*
 * Initialize a preallocated and zeroed pmap structure,
 * such as one in a vmspace structure.
 */
void
pmap_pinit(pmap)
	register struct pmap *pmap;
{

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_CREATE))
		pg("pmap_pinit(%x)\n", pmap);
#endif

	/*
	 * No need to allocate page table space yet but we do need a
	 * valid page directory table.
	 */
	pmap->pm_pdir = (pd_entry_t *) kmem_alloc(kernel_map, NBPG);

	/* wire in kernel global address entries */
	bcopy(PTD+KPTDI, pmap->pm_pdir+KPTDI, NKPDE*4);

	/* install self-referential address mapping entry */
	*(int *)(pmap->pm_pdir+PTDPTDI) =
		(int)pmap_extract(kernel_pmap, pmap->pm_pdir) | PG_V | PG_KW;

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
	register pmap_t pmap;
{
	int count;

	if (pmap == NULL)
		return;

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_destroy(%x)\n", pmap);
#endif

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
	register struct pmap *pmap;
{

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		pg("pmap_release(%x)\n", pmap);
#endif

	/* sometimes 1, sometimes 0; could rearrange pmap_destroy */
#ifdef DIAGNOSTICx
	if (pmap->pm_count != 1)
		panic("pmap_release count");
#endif

	kmem_free(kernel_map, (vm_offset_t)pmap->pm_pdir, NBPG);
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
		printf("pmap_reference(%x)", pmap);
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
	struct pmap *pmap;
	register vm_offset_t sva;
	register vm_offset_t eva;
{
	register pt_entry_t *pte;
	vm_offset_t pa;
	register pv_entry_t pv, npv;
	int s, bits;

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_REMOVE|PDB_PROTECT))
		pg("pmap_remove(%x, %x, %x)", pmap, sva, eva);

	remove_stats.calls++;
#endif
	
	sva &= PG_FRAME;
	eva &= PG_FRAME;

	/* this is essential since we must check the PDE(sva) for precense */
	while (sva < eva) {
		pte = pmap_pte(pmap, sva);
		if (pte)
			break;
		sva = (sva & PD_MASK) + NBPD;
	}

	for (; sva < eva; sva += NBPG, pte++) {

		/* only check once in a while */
		if (!(sva & PT_MASK)) {
			if (!pmap_pde_v(pmap_pde(pmap, i386_ptob(sva)))) {
				/* We can race ahead here, straight to next pde.. */
				sva += NBPD -NBPG;
				pte += i386_btop(NBPD) -1;
				continue;
			}
		}

	        if (!pmap_pte_v(pte))
			continue;

		pa = pmap_pte_pa(pte);
		if (!pmap_valid_page(pa))
			continue;

#ifdef DEBUG
		remove_stats.removes++;
#endif

		/*
		 * Update statistics
		 */
		if (pmap_pte_w(pte))
			pmap->pm_stats.wired_count--;
		pmap->pm_stats.resident_count--;

		/*
		 * Invalidate the PTEs.
		 * XXX: should cluster them up and invalidate as many
		 * as possible at once.
		 */
#ifdef DEBUG
		if (pmapdebug & PDB_REMOVE)
			printf("remove: inv pte at %x(%x) ",
			       pte, *(int *)pte);
#endif
		bits = *(int *)pte & (PG_U|PG_M);
		*(int *)pte = 0;

#ifdef needednotdone
reduce wiring count on page table pages as references drop
#endif

		/*
		 * Remove from the PV table (raise IPL since we
		 * may be called at interrupt time).
		 */
		pv = pa_to_pvh(pa);
		s = splimp();
		/*
		 * If it is the first entry on the list, it is actually
		 * in the header and we must copy the following entry up
		 * to the header.  Otherwise we must search the list for
		 * the entry.  In either case we free the now unused entry.
		 */
		if (pmap == pv->pv_pmap && sva == pv->pv_va) {
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
			for (npv = pv->pv_next; npv; npv = npv->pv_next) {
#ifdef DEBUG
				remove_stats.pvsearch++;
#endif
				if (pmap == npv->pv_pmap && sva == npv->pv_va)
					break;
				pv = npv;
			}
#ifdef DEBUG
			if (npv == NULL)
				panic("pmap_remove: PA not in pv_tab");
#endif
			pv->pv_next = npv->pv_next;
			free((caddr_t)npv, M_VMPVENT);
		}

#ifdef notdef
[tally number of pagetable pages, if sharing of ptpages adjust here]
#endif
		/*
		 * Update saved attributes for managed page
		 */
		pmap_attributes[pmap_page_index(pa)] |= bits;
		splx(s);
	}

	/* only need to flush once */
	if (curproc && pmap == &curproc->p_vmspace->vm_pmap)
		tlbflush();
}

/*
 *	Routine:	pmap_remove_all
 *	Function:
 *		Removes this physical page from
 *		all physical maps in which it resides.
 *		Reflects back modify bits to the pager.
 */
void
pmap_remove_all(pa)
	vm_offset_t pa;
{
	register pv_entry_t pv;
	int s;

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_REMOVE|PDB_PROTECT))
		printf("pmap_remove_all(%x)", pa);
	/*pmap_pvdump(pa);*/
#endif

	if (!pmap_valid_page(pa))
		return;

	pv = pa_to_pvh(pa);
	s = splimp();
	/*
	 * Do it the easy way for now
	 */
	while (pv->pv_pmap != NULL) {
#ifdef DEBUG
		if (!pmap_pde_v(pmap_pde(pv->pv_pmap, pv->pv_va)) ||
		    pmap_pte_pa(pmap_pte(pv->pv_pmap, pv->pv_va)) != pa)
			panic("pmap_remove_all: bad mapping");
#endif
		pmap_remove(pv->pv_pmap, pv->pv_va, pv->pv_va + NBPG);
	}
	splx(s);
}

/*
 *	Routine:	pmap_copy_on_write
 *	Function:
 *		Remove write privileges from all
 *		physical maps for this physical page.
 */
void
pmap_copy_on_write(pa)
	vm_offset_t pa;
{

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_PROTECT))
		printf("pmap_copy_on_write(%x)", pa);
#endif
	pmap_changebit(pa, PG_RO, ~PG_RW);
}

/*
 *	Set the physical protection on the
 *	specified range of this map as requested.
 */
void
pmap_protect(pmap, sva, eva, prot)
	register pmap_t	pmap;
	vm_offset_t	sva, eva;
	vm_prot_t	prot;
{
	register pt_entry_t *pte;
	register int i386prot;

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_PROTECT))
		printf("pmap_protect(%x, %x, %x, %x)", pmap, sva, eva, prot);
#endif

	if (pmap == NULL)
		return;

	if ((prot & VM_PROT_READ) == VM_PROT_NONE) {
		pmap_remove(pmap, sva, eva);
		return;
	}

	if (prot & VM_PROT_WRITE)
		return;

	sva &= PG_FRAME;
	eva &= PG_FRAME;

	/* this is essential since we must check the PDE(sva) for precense */
	while (sva < eva) {
		pte = pmap_pte(pmap, sva);
		if (pte)
			break;
		sva = (sva & PD_MASK) + NBPD;
	}

	for (; sva < eva; sva += NBPG, pte++) {

		/* only check once in a while */
		if (!(sva & PT_MASK)) {
			if (!pmap_pde_v(pmap_pde(pmap, i386_ptob(sva)))) {
				/* We can race ahead here, straight to next pde.. */
				sva += NBPD -NBPG;
				pte += i386_btop(NBPD) -1;
				continue;
			}
		}

		if (!pmap_pte_v(pte))
			continue;

		i386prot = pte_prot(pmap, prot);
		if (sva < VM_MAXUSER_ADDRESS)	/* see also pmap_enter() */
			i386prot |= PG_u;
		else if (sva < VM_MAX_ADDRESS)
			i386prot |= PG_u | PG_RW;
		/* clear VAC here if PG_RO? */
		pmap_pte_set_prot(pte, i386prot);
	}

	if (curproc && pmap == &curproc->p_vmspace->vm_pmap)
		tlbflush();
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
void
pmap_enter(pmap, va, pa, prot, wired)
	register pmap_t pmap;
	vm_offset_t va;
	register vm_offset_t pa;
	vm_prot_t prot;
	boolean_t wired;
{
	register pt_entry_t *pte;
	register int npte;
	boolean_t cacheable = TRUE;

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_ENTER))
		printf("pmap_enter(%x, %x, %x, %x, %x)",
		       pmap, va, pa, prot, wired);
#endif

	if (pmap == NULL)
		return;

	if (va > VM_MAX_KERNEL_ADDRESS)
		panic("pmap_enter: too big");
	/* also, should not muck with PTD va! */

#ifdef DEBUG
	if (pmap == kernel_pmap)
		enter_stats.kernel++;
	else
		enter_stats.user++;
#endif

	/*
	 * Page Directory table entry not valid, we need a new PT page
	 */
	pte = pmap_pte(pmap, va);
	if (!pte)
		panic("ptdi %x", pmap->pm_pdir[PTDPTDI]);

#ifdef DEBUG
	if (pmapdebug & PDB_ENTER)
		printf("enter: pte %x, *pte %x ", pte, *(int *)pte);
#endif

	if (pmap_pte_v(pte)) {
		register vm_offset_t opa;

		opa = pmap_pte_pa(pte);

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
			if (wired && !pmap_pte_w(pte) || !wired && pmap_pte_w(pte)) {
#ifdef DEBUG
				if (pmapdebug & PDB_ENTER)
					pg("enter: wiring change -> %x ", wired);
#endif
				if (wired)
					pmap->pm_stats.wired_count++;
				else
					pmap->pm_stats.wired_count--;
#ifdef DEBUG
				enter_stats.wchange++;
#endif
			}
			goto validate;
		}
		
		/*
		 * Mapping has changed, invalidate old range and fall through to
		 * handle validating new mapping.
		 */
#ifdef DEBUG
		if (pmapdebug & PDB_ENTER)
			printf("enter: removing old mapping %x pa %x ", va, opa);
#endif
		pmap_remove(pmap, va, va + NBPG);
#ifdef DEBUG
		enter_stats.mchange++;
#endif
	}

	/*
	 * Enter on the PV list if part of our managed memory
	 * Note that we raise IPL while manipulating pv_table
	 * since pmap_enter can be called at interrupt time.
	 */
	if (pmap_valid_page(pa)) {
		register pv_entry_t pv, npv;
		int s;

#ifdef DEBUG
		enter_stats.managed++;
#endif
		pv = pa_to_pvh(pa);
		s = splimp();
#ifdef DEBUG
		if (pmapdebug & PDB_ENTER)
			printf("enter: pv at %x: %x/%x/%x ",
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
			pv->pv_flags = 0;
		}
		/*
		 * There is at least one other VA mapping this page.
		 * Place this entry after the header.
		 */
		else {
			/*printf("second time: ");*/
#ifdef DEBUG
			for (npv = pv; npv; npv = npv->pv_next)
				if (pmap == npv->pv_pmap && va == npv->pv_va)
					panic("pmap_enter: already in pv_tab");
#endif
			npv = (pv_entry_t)
				malloc(sizeof *npv, M_VMPVENT, M_NOWAIT);
			if (npv == NULL)
				panic("pmap_enter: malloc returned NULL");
			npv->pv_va = va;
			npv->pv_pmap = pmap;
			npv->pv_next = pv->pv_next;
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
		cacheable = FALSE;
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
	 * I386 pages in a MACH page.
	 */
	npte = (pa & PG_FRAME) | pte_prot(pmap, prot) | PG_V;
	npte |= *(int *)pte & (PG_M|PG_U);
	if (wired)
		npte |= PG_W;

	if (va < VM_MAXUSER_ADDRESS)	/* i.e. below USRSTACK */
		npte |= PG_u;
	else if (va < VM_MAX_ADDRESS)
		/*
		 * Page tables need to be user RW, for some reason, and the
		 * user area must be writable too.  Anything above
		 * VM_MAXUSER_ADDRESS is protected from user access by
		 * the user data and code segment descriptors, so this is OK.
		 */
		npte |= PG_u | PG_RW;

#ifdef DEBUG
	if (pmapdebug & PDB_ENTER)
		printf("enter: new pte value %x ", npte);
#endif

	*(int *)pte = npte;
	tlbflush();
}

/*
 *      pmap_page_protect:
 *
 *      Lower the permission for all mappings to a given page.
 */
void
pmap_page_protect(phys, prot)
        vm_offset_t     phys;
        vm_prot_t       prot;
{

        switch (prot) {
	    case VM_PROT_READ:
	    case VM_PROT_READ|VM_PROT_EXECUTE:
                pmap_copy_on_write(phys);
                break;
	    case VM_PROT_ALL:
                break;
	    default:
                pmap_remove_all(phys);
                break;
        }
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
	vm_offset_t	va;
	boolean_t	wired;
{
	register pt_entry_t *pte;

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_change_wiring(%x, %x, %x)", pmap, va, wired);
#endif

	pte = pmap_pte(pmap, va);
	if (!pte)
		return;

#ifdef DEBUG
	/*
	 * Page not valid.  Should this ever happen?
	 * Just continue and change wiring anyway.
	 */
	if (!pmap_pte_v(pte)) {
		if (pmapdebug & PDB_PARANOIA)
			pg("pmap_change_wiring: invalid PTE for %x ", va);
	}
#endif

	if (wired && !pmap_pte_w(pte) || !wired && pmap_pte_w(pte)) {
		if (wired)
			pmap->pm_stats.wired_count++;
		else
			pmap->pm_stats.wired_count--;
	}

	/*
	 * Wiring is not a hardware characteristic so there is no need
	 * to invalidate TLB.
	 */
	pmap_pte_set_w(pte, wired);
}

/*
 *	Routine:	pmap_pte
 *	Function:
 *		Extract the page table entry associated
 *		with the given map/virtual_address pair.
 * [ what about induced faults -wfj]
 */
struct pte *
pmap_pte(pmap, va)
	register pmap_t	pmap;
	vm_offset_t va;
{
	struct pte *ptp;

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_pte(%x, %x) ->\n", pmap, va);
#endif

	if (!pmap || !pmap_pde_v(pmap_pde(pmap, va)))
		return NULL;

	if (pmap->pm_pdir[PTDPTDI].pd_pfnum == PTDpde.pd_pfnum ||
	    pmap == kernel_pmap)
		/* current address space or kernel */
		ptp = PTmap;
	else {
		/* alternate address space */
		if (pmap->pm_pdir[PTDPTDI].pd_pfnum != APTDpde.pd_pfnum) {
			APTDpde = pmap->pm_pdir[PTDPTDI];
			tlbflush();
		}
		ptp = APTmap;
	}

	return ptp + i386_btop(va);
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
	register struct pte *pte;
	register vm_offset_t pa;

#ifdef DEBUGx
	if (pmapdebug & PDB_FOLLOW)
		pg("pmap_extract(%x, %x) -> ", pmap, va);
#endif

	pte = pmap_pte(pmap, va);
	if (!pte)
		return NULL;
	if (!pmap_pte_v(pte))
		return NULL;

	pa = pmap_pte_pa(pte);
#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("%x\n", pa);
#endif
	return pa | (va & ~PG_FRAME);
}

/*
 *	Copy the range specified by src_addr/len
 *	from the source map to the range dst_addr/len
 *	in the destination map.
 *
 *	This routine is only advisory and need not do anything.
 */
void pmap_copy(dst_pmap, src_pmap, dst_addr, len, src_addr)
	pmap_t		dst_pmap;
	pmap_t		src_pmap;
	vm_offset_t	dst_addr;
	vm_size_t	len;
	vm_offset_t	src_addr;
{

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_copy(%x, %x, %x, %x, %x)",
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
void pmap_update()
{

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_update()");
#endif
	tlbflush();
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
 * [ needs to be written -wfj ]  XXXX
 */
void
pmap_collect(pmap)
	pmap_t		pmap;
{
	register vm_offset_t pa;
	register pv_entry_t pv;
	register int *pte;
	vm_offset_t kpa;
	int s;

#ifdef DEBUG
	int *pde;
	int opmapdebug;
	printf("pmap_collect(%x) ", pmap);
#endif
	if (pmap != kernel_pmap)
		return;

}

void
pmap_activate(pmap, pcbp)
	register pmap_t pmap;
	struct pcb *pcbp;
{

#ifdef DEBUG
	if (pmapdebug & (PDB_FOLLOW|PDB_PDRTAB))
		pg("pmap_activate(%x, %x) ", pmap, pcbp);
#endif

	PMAP_ACTIVATE(pmap, pcbp);
}

/*
 *	Routine:	pmap_kernel
 *	Function:
 *		Returns the physical map handle for the kernel.
 */
pmap_t
pmap_kernel()
{

    	return kernel_pmap;
}

/*
 *	pmap_zero_page zeros the specified by mapping it into
 *	virtual memory and using bzero to clear its contents.
 */
void
pmap_zero_page(phys)
	register vm_offset_t phys;
{

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_zero_page(%x)", phys);
#endif

	*(int *)CMAP2 = (phys & PG_FRAME) | PG_V | PG_KW /*| PG_N*/;
	tlbflush();
	bzero(CADDR2, NBPG);
}

/*
 *	pmap_copy_page copies the specified page by mapping
 *	it into virtual memory and using bcopy to copy its
 *	contents.
 */
void
pmap_copy_page(src, dst)
	register vm_offset_t src, dst;
{

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_copy_page(%x, %x)", src, dst);
#endif

	*(int *)CMAP1 = (src & PG_FRAME) | PG_V | PG_KW;
	*(int *)CMAP2 = (dst & PG_FRAME) | PG_V | PG_KW /*| PG_N*/;
	tlbflush();
	bcopy(CADDR1, CADDR2, NBPG);
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
		printf("pmap_pageable(%x, %x, %x, %x)",
		       pmap, sva, eva, pageable);
#endif

	/*
	 * If we are making a PT page pageable then all valid
	 * mappings must be gone from that page.  Hence it should
	 * be all zeros and there is no need to clean it.
	 * Assumptions:
	 *	- we are called with only one page at a time
	 *	- PT pages have only one pv_table entry
	 */
	if (pmap == kernel_pmap && pageable && sva + NBPG == eva) {
		register vm_offset_t pa;
		register struct pte *pte;

#ifdef DEBUG
		register pv_entry_t pv;

		if ((pmapdebug & (PDB_FOLLOW|PDB_PTPAGE)) == PDB_PTPAGE)
			printf("pmap_pageable(%x, %x, %x, %x)",
			       pmap, sva, eva, pageable);
#endif

		pte = pmap_pte(pmap, sva);
		if (!pte)
			return;
		if (!pmap_pte_v(pte))
			return;

		pa = pmap_pte_pa(pte);

#ifdef DEBUG
		if (!pmap_valid_page(pa))
			return;

		pv = pa_to_pvh(pa);
		if (pv->pv_va != sva || pv->pv_next) {
			pg("pmap_pageable: bad PT page va %x next %x\n",
			       pv->pv_va, pv->pv_next);
			return;
		}
#endif

		/*
		 * Mark it unmodified to avoid pageout
		 */
		PMAP_CLEAR_MODIFY(pa);

#ifdef needsomethinglikethis
		if (pmapdebug & PDB_PTPAGE)
			pg("pmap_pageable: PT page %x(%x) unmodified\n",
			       sva, *(int *)pmap_pte(pmap, sva));
		if (pmapdebug & PDB_WIRING)
			pmap_check_wiring("pageable", sva);
#endif
	}
}

/*
 *	Clear the modify bits on the specified physical page.
 */

void
pmap_clear_modify(pa)
	vm_offset_t	pa;
{

	PMAP_CLEAR_MODIFY(pa);
}

/*
 *	pmap_clear_reference:
 *
 *	Clear the reference bit on the specified physical page.
 */

void pmap_clear_reference(pa)
	vm_offset_t	pa;
{

	PMAP_CLEAR_REFERENCE(pa);
}

/*
 *	pmap_is_referenced:
 *
 *	Return whether or not the specified physical page is referenced
 *	by any physical maps.
 */

boolean_t
pmap_is_referenced(pa)
	vm_offset_t	pa;
{

	return PMAP_IS_REFERENCED(pa);
}

/*
 *	pmap_is_modified:
 *
 *	Return whether or not the specified physical page is modified
 *	by any physical maps.
 */

boolean_t
pmap_is_modified(pa)
	vm_offset_t	pa;
{

	return PMAP_IS_MODIFIED(pa);
}

vm_offset_t
pmap_phys_address(ppn)
	int ppn;
{

	return i386_ptob(ppn);
}

/*
 * Miscellaneous support routines follow
 */

i386_protection_init()
{

	protection_codes[VM_PROT_NONE | VM_PROT_NONE | VM_PROT_NONE] = 0;
	protection_codes[VM_PROT_READ | VM_PROT_NONE | VM_PROT_NONE] =
	protection_codes[VM_PROT_READ | VM_PROT_NONE | VM_PROT_EXECUTE] =
	protection_codes[VM_PROT_NONE | VM_PROT_NONE | VM_PROT_EXECUTE] = PG_RO;
	protection_codes[VM_PROT_NONE | VM_PROT_WRITE | VM_PROT_NONE] =
	protection_codes[VM_PROT_NONE | VM_PROT_WRITE | VM_PROT_EXECUTE] =
	protection_codes[VM_PROT_READ | VM_PROT_WRITE | VM_PROT_NONE] =
	protection_codes[VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE] = PG_RW;
}

boolean_t
pmap_testbit(pa, setbits)
	register vm_offset_t pa;
	int setbits;
{
	register pv_entry_t pv;
	register int *pte;
	int s;

	if (!pmap_valid_page(pa))
		return FALSE;

	pv = pa_to_pvh(pa);
	s = splimp();
	/*
	 * Check saved info first
	 */
	if (pmap_attributes[pmap_page_index(pa)] & setbits) {
		splx(s);
		return TRUE;
	}
	/*
	 * Not found, check current mappings returning
	 * immediately if found.
	 */
	if (pv->pv_pmap != NULL) {
		for (; pv; pv = pv->pv_next) {
			pte = (int *) pmap_pte(pv->pv_pmap, pv->pv_va);
			if (*pte & setbits) {
				splx(s);
				return TRUE;
			}
		}
	}
	splx(s);
	return FALSE;
}

/*
 * Modify pte bits for all ptes corresponding to the given physical address.
 * We use `maskbits' rather than `clearbits' because we're always passing
 * constants and the latter would require an extra inversion at run-time.
 */
void
pmap_changebit(pa, setbits, maskbits)
	register vm_offset_t pa;
	int setbits, maskbits;
{
	register pv_entry_t pv;
	register int *pte, npte;
	vm_offset_t va;
	int s;

#ifdef DEBUG
	if (pmapdebug & PDB_BITS)
		printf("pmap_changebit(%x, %x, %x)",
		       pa, setbits, ~maskbits);
#endif

	if (!pmap_valid_page(pa))
		return;

	pv = pa_to_pvh(pa);
	s = splimp();
	/*
	 * Clear saved attributes (modify, reference)
	 */
	if (~maskbits)
		pmap_attributes[pmap_page_index(pa)] &= maskbits;
	/*
	 * Loop over all current mappings setting/clearing as appropos
	 * If setting RO do we need to clear the VAC?
	 */
	if (pv->pv_pmap != NULL) {
#ifdef DEBUG
		int toflush = 0;
#endif
		for (; pv; pv = pv->pv_next) {
#ifdef DEBUG
			toflush |= (pv->pv_pmap == kernel_pmap) ? 2 : 1;
#endif
			va = pv->pv_va;

                        /*
                         * XXX don't write protect pager mappings
                         */
                        if ((PG_RO && setbits == PG_RO) ||
			    (PG_RW && maskbits == ~PG_RW)) {
                                extern vm_offset_t pager_sva, pager_eva;

                                if (va >= pager_sva && va < pager_eva)
                                        continue;
                        }

			pte = (int *) pmap_pte(pv->pv_pmap, va);
			npte = (*pte & maskbits) | setbits;
			if (*pte != npte)
				*pte = npte;
			va += NBPG;
			pte++;

			if (curproc && pv->pv_pmap == &curproc->p_vmspace->vm_pmap)
				tlbflush();
		}
#ifdef somethinglikethis
		if (setem && bit == PG_RO && (pmapvacflush & PVF_PROTECT)) {
			if ((pmapvacflush & PVF_TOTAL) || toflush == 3)
				DCIA();
			else if (toflush == 2)
				DCIS();
			else
				DCIU();
		}
#endif
	}
	splx(s);
}

#ifdef DEBUG
pmap_pvdump(pa)
	vm_offset_t pa;
{
	register pv_entry_t pv;

	printf("pa %x", pa);
	for (pv = pa_to_pvh(pa); pv; pv = pv->pv_next) {
		printf(" -> pmap %x, va %x, flags %x",
		       pv->pv_pmap, pv->pv_va, pv->pv_flags);
		pads(pv->pv_pmap);
	}
	printf(" ");
}

#ifdef notyet
pmap_check_wiring(str, va)
	char *str;
	vm_offset_t va;
{
	vm_map_entry_t entry;
	register int count, *pte;

	va = trunc_page(va);
	if (!pmap_pde_v(pmap_pde(kernel_pmap, va)) ||
	    !pmap_pte_v(pmap_pte(kernel_pmap, va)))
		return;

	if (!vm_map_lookup_entry(pt_map, va, &entry)) {
		pg("wired_check: entry for %x not found\n", va);
		return;
	}
	count = 0;
	for (pte = (int *)va; pte < (int *)(va + NBPG); pte++)
		if (*pte)
			count++;
	if (entry->wired_count != count)
		pg("*%s*: %x: w%d/a%d\n",
		       str, va, entry->wired_count, count);
}
#endif

/* print address space of pmap*/
pads(pm) pmap_t pm; {
	unsigned va, i, j;
	register struct pte *pte;

	if(pm == kernel_pmap) return;
	for (i = 0; i < 1024; i++) 
		if(pm->pm_pdir[i].pd_v)
			for (j = 0; j < 1024 ; j++) {
				va = (i<<22)+(j<<12);
				if (pm == kernel_pmap && va < VM_MIN_KERNEL_ADDRESS)
						continue;
				if (pm != kernel_pmap && va > VM_MAX_ADDRESS)
						continue;
				pte = pmap_pte(pm, va);
				if(pmap_pte_v(pte)) 
					printf("%x:%x ", va, *(int *)pte); 
			} ;
				
}
#endif
