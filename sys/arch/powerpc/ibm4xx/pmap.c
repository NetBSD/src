/*	$NetBSD: pmap.c,v 1.1.2.5 2002/06/23 17:39:39 jdolecek Exp $	*/

/*
 * Copyright 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Eduardo Horvath and Simon Burge for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#undef PPC_4XX_NOCACHE

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/queue.h>
#include <sys/systm.h>
#include <sys/pool.h>
#include <sys/device.h>

#include <uvm/uvm.h>

#include <machine/cpu.h>
#include <machine/pcb.h>
#include <machine/powerpc.h>

#include <powerpc/spr.h>
#include <machine/tlb.h>

/*
 * kernmap is an array of PTEs large enough to map in
 * 4GB.  At 16KB/page it is 256K entries or 2MB.
 */
#define KERNMAP_SIZE	((0xffffffffU/NBPG)+1)
caddr_t kernmap;

#define MINCTX		2
#define NUMCTX		256
volatile struct pmap *ctxbusy[NUMCTX];

#define TLBF_USED	0x1
#define	TLBF_REF	0x2
#define	TLBF_LOCKED	0x4
#define	TLB_LOCKED(i)	(tlb_info[(i)].ti_flags & TLBF_LOCKED)
typedef struct tlb_info_s {
	char	ti_flags;
	char	ti_ctx;		/* TLB_PID assiciated with the entry */
	u_int	ti_va;
} tlb_info_t;

volatile tlb_info_t tlb_info[NTLB];
/* We'll use a modified FIFO replacement policy cause it's cheap */
volatile int tlbnext = TLB_NRESERVED;

u_long dtlb_miss_count = 0;
u_long itlb_miss_count = 0;
u_long ktlb_miss_count = 0;
u_long utlb_miss_count = 0;

/* Event counters -- XXX type `INTR' so we can see them with vmstat -i */
struct evcnt tlbmiss_ev = EVCNT_INITIALIZER(EVCNT_TYPE_INTR,
	NULL, "cpu", "tlbmiss");
struct evcnt tlbhit_ev = EVCNT_INITIALIZER(EVCNT_TYPE_INTR,
	NULL, "cpu", "tlbhit");
struct evcnt tlbflush_ev = EVCNT_INITIALIZER(EVCNT_TYPE_INTR,
	NULL, "cpu", "tlbflush");
struct evcnt tlbenter_ev = EVCNT_INITIALIZER(EVCNT_TYPE_INTR,
	NULL, "cpu", "tlbenter");

struct pmap kernel_pmap_;

int physmem;
static int npgs;
static u_int nextavail;
#ifndef MSGBUFADDR
extern paddr_t msgbuf_paddr;
#endif

static struct mem_region *mem, *avail;

/*
 * This is a cache of referenced/modified bits.
 * Bits herein are shifted by ATTRSHFT.
 */
static char *pmap_attrib;

#define PV_WIRED	0x1
#define PV_WIRE(pv)	((pv)->pv_va |= PV_WIRED)
#define	PV_CMPVA(va,pv)	(!(((pv)->pv_va^(va))&(~PV_WIRED)))

struct pv_entry {
	struct pv_entry *pv_next;	/* Linked list of mappings */
	vaddr_t pv_va;			/* virtual address of mapping */
	struct pmap *pv_pm;
};

struct pv_entry *pv_table;
static struct pool pv_pool;

static int pmap_initialized;

static int ctx_flush(int);

inline struct pv_entry *pa_to_pv(paddr_t);
static inline char *pa_to_attr(paddr_t);

static inline volatile u_int *pte_find(struct pmap *, vaddr_t);
static inline int pte_enter(struct pmap *, vaddr_t, u_int);

static void pmap_pinit(pmap_t);
static void pmap_release(pmap_t);
static inline int pmap_enter_pv(struct pmap *, vaddr_t, paddr_t);
static void pmap_remove_pv(struct pmap *, vaddr_t, paddr_t);


inline struct pv_entry *
pa_to_pv(paddr_t pa)
{
	int bank, pg;

	bank = vm_physseg_find(atop(pa), &pg);
	if (bank == -1)
		return NULL;
	return &vm_physmem[bank].pmseg.pvent[pg];
}

static inline char *
pa_to_attr(paddr_t pa)
{
	int bank, pg;

	bank = vm_physseg_find(atop(pa), &pg);
	if (bank == -1)
		return NULL;
	return &vm_physmem[bank].pmseg.attrs[pg];
}

/*
 * Insert PTE into page table.
 */
int
pte_enter(struct pmap *pm, vaddr_t va, u_int pte)
{
	int seg = STIDX(va);
	int ptn = PTIDX(va);
	paddr_t pa;

	if (!pm->pm_ptbl[seg]) {
		/* Don't allocate a page to clear a non-existent mapping. */
		if (!pte) return (1);
		/* Allocate a page XXXX this will sleep! */
		pa = 0;
		pm->pm_ptbl[seg] = (uint *)uvm_km_alloc1(kernel_map, NBPG, 1);
	}
	pm->pm_ptbl[seg][ptn] = pte;

	/* Flush entry. */
	ppc4xx_tlb_flush(va, pm->pm_ctx);
	return (1);
}

/*
 * Get a pointer to a PTE in a page table.
 */
volatile u_int *
pte_find(struct pmap *pm, vaddr_t va)
{
	int seg = STIDX(va);
	int ptn = PTIDX(va);

	if (pm->pm_ptbl[seg])
		return (&pm->pm_ptbl[seg][ptn]);

	return (NULL);
}

/*
 * This is called during initppc, before the system is really initialized.
 */
void
pmap_bootstrap(u_int kernelstart, u_int kernelend)
{
	struct mem_region *mp, *mp1;
	int cnt, i;
	u_int s, e, sz;

	/*
	 * Allocate the kernel page table at the end of
	 * kernel space so it's in the locked TTE.
	 */
	kernmap = (caddr_t)kernelend;

	/*
	 * Initialize kernel page table.
	 */
	for (i = 0; i < STSZ; i++) {
		pmap_kernel()->pm_ptbl[i] = 0;
	}
	ctxbusy[0] = ctxbusy[1] = pmap_kernel();

	/*
	 * Announce page-size to the VM-system
	 */
	uvmexp.pagesize = NBPG;
	uvm_setpagesize();

	/*
	 * Get memory.
	 */
	mem_regions(&mem, &avail);
	for (mp = mem; mp->size; mp++) {
		physmem += btoc(mp->size);
		printf("+%lx,",mp->size);
	}
	printf("\n");
	ppc4xx_tlb_init();
	/*
	 * Count the number of available entries.
	 */
	for (cnt = 0, mp = avail; mp->size; mp++)
		cnt++;

	/*
	 * Page align all regions.
	 * Non-page aligned memory isn't very interesting to us.
	 * Also, sort the entries for ascending addresses.
	 */
	kernelstart &= ~PGOFSET;
	kernelend = (kernelend + PGOFSET) & ~PGOFSET;
	for (mp = avail; mp->size; mp++) {
		s = mp->start;
		e = mp->start + mp->size;
		printf("%08x-%08x -> ",s,e);
		/*
		 * Check whether this region holds all of the kernel.
		 */
		if (s < kernelstart && e > kernelend) {
			avail[cnt].start = kernelend;
			avail[cnt++].size = e - kernelend;
			e = kernelstart;
		}
		/*
		 * Look whether this regions starts within the kernel.
		 */
		if (s >= kernelstart && s < kernelend) {
			if (e <= kernelend)
				goto empty;
			s = kernelend;
		}
		/*
		 * Now look whether this region ends within the kernel.
		 */
		if (e > kernelstart && e <= kernelend) {
			if (s >= kernelstart)
				goto empty;
			e = kernelstart;
		}
		/*
		 * Now page align the start and size of the region.
		 */
		s = round_page(s);
		e = trunc_page(e);
		if (e < s)
			e = s;
		sz = e - s;
		printf("%08x-%08x = %x\n",s,e,sz);
		/*
		 * Check whether some memory is left here.
		 */
		if (sz == 0) {
		empty:
			memmove(mp, mp + 1,
				(cnt - (mp - avail)) * sizeof *mp);
			cnt--;
			mp--;
			continue;
		}
		/*
		 * Do an insertion sort.
		 */
		npgs += btoc(sz);
		for (mp1 = avail; mp1 < mp; mp1++)
			if (s < mp1->start)
				break;
		if (mp1 < mp) {
			memmove(mp1 + 1, mp1, (char *)mp - (char *)mp1);
			mp1->start = s;
			mp1->size = sz;
		} else {
			mp->start = s;
			mp->size = sz;
		}
	}

	/*
	 * We cannot do pmap_steal_memory here,
	 * since we don't run with translation enabled yet.
	 */
#ifndef MSGBUFADDR
	/*
	 * allow for msgbuf
	 */
	sz = round_page(MSGBUFSIZE);
	mp = NULL;
	for (mp1 = avail; mp1->size; mp1++)
		if (mp1->size >= sz)
			mp = mp1;
	if (mp == NULL)
		panic("not enough memory?");

	npgs -= btoc(sz);
	msgbuf_paddr = mp->start + mp->size - sz;
	mp->size -= sz;
	if (mp->size <= 0)
		memmove(mp, mp + 1, (cnt - (mp - avail)) * sizeof *mp);
#endif

	printf("Loading pages\n");
	for (mp = avail; mp->size; mp++)
		uvm_page_physload(atop(mp->start), atop(mp->start + mp->size),
			atop(mp->start), atop(mp->start + mp->size),
			VM_FREELIST_DEFAULT);

	/*
	 * Initialize kernel pmap and hardware.
	 */
	/* Setup TLB pid allocator so it knows we alreadu using PID 1 */
	pmap_kernel()->pm_ctx = KERNEL_PID;
	nextavail = avail->start;


	evcnt_attach_static(&tlbhit_ev);
	evcnt_attach_static(&tlbmiss_ev);
	evcnt_attach_static(&tlbflush_ev);
	evcnt_attach_static(&tlbenter_ev);
	printf("Done\n");
}

/*
 * Restrict given range to physical memory
 *
 * (Used by /dev/mem)
 */
void
pmap_real_memory(paddr_t *start, psize_t *size)
{
	struct mem_region *mp;

	for (mp = mem; mp->size; mp++) {
		if (*start + *size > mp->start &&
		    *start < mp->start + mp->size) {
			if (*start < mp->start) {
				*size -= mp->start - *start;
				*start = mp->start;
			}
			if (*start + *size > mp->start + mp->size)
				*size = mp->start + mp->size - *start;
			return;
		}
	}
	*size = 0;
}

/*
 * Initialize anything else for pmap handling.
 * Called during vm_init().
 */
void
pmap_init(void)
{
	struct pv_entry *pv;
	vsize_t sz;
	vaddr_t addr;
	int i, s;
	int bank;
	char *attr;

	sz = (vsize_t)((sizeof(struct pv_entry) + 1) * npgs);
	sz = round_page(sz);
	addr = uvm_km_zalloc(kernel_map, sz);
	s = splvm();
	pv = pv_table = (struct pv_entry *)addr;
	for (i = npgs; --i >= 0;)
		pv++->pv_pm = NULL;
	pmap_attrib = (char *)pv;
	memset(pv, 0, npgs);

	pv = pv_table;
	attr = pmap_attrib;
	for (bank = 0; bank < vm_nphysseg; bank++) {
		sz = vm_physmem[bank].end - vm_physmem[bank].start;
		vm_physmem[bank].pmseg.pvent = pv;
		vm_physmem[bank].pmseg.attrs = attr;
		pv += sz;
		attr += sz;
	}

	pmap_initialized = 1;
	splx(s);

	/* Setup a pool for additional pvlist structures */
	pool_init(&pv_pool, sizeof(struct pv_entry), 0, 0, 0, "pv_entry", NULL);
}

/*
 * How much virtual space is available to the kernel?
 */
void
pmap_virtual_space(vaddr_t *start, vaddr_t *end)
{

#if 0
	/*
	 * Reserve one segment for kernel virtual memory
	 */
	*start = (vaddr_t)(KERNEL_SR << ADDR_SR_SHFT);
	*end = *start + SEGMENT_LENGTH;
#else
	*start = (vaddr_t) VM_MIN_KERNEL_ADDRESS;
	*end = (vaddr_t) VM_MAX_KERNEL_ADDRESS;
#endif
}

#ifdef PMAP_GROWKERNEL
/*
 * Preallocate kernel page tables to a specified VA.
 * This simply loops through the first TTE for each
 * page table from the beginning of the kernel pmap, 
 * reads the entry, and if the result is
 * zero (either invalid entry or no page table) it stores
 * a zero there, populating page tables in the process.
 * This is not the most efficient technique but i don't
 * expect it to be called that often.
 */
extern struct vm_page *vm_page_alloc1 __P((void));
extern void vm_page_free1 __P((struct vm_page *));

vaddr_t kbreak = VM_MIN_KERNEL_ADDRESS;

vaddr_t 
pmap_growkernel(maxkvaddr)
        vaddr_t maxkvaddr; 
{
	int s;
	int seg;
	paddr_t pg;
	struct pmap *pm = pmap_kernel();
	
	s = splvm();

	/* Align with the start of a page table */
	for (kbreak &= ~(PTMAP-1); kbreak < maxkvaddr;
	     kbreak += PTMAP) {
		seg = STIDX(kbreak);

		if (pte_find(pm, kbreak)) continue;
 
		if (uvm.page_init_done) {
			pg = (paddr_t)VM_PAGE_TO_PHYS(vm_page_alloc1());
		} else {
			if (!uvm_page_physget(&pg))
				panic("pmap_growkernel: no memory");
		}
		if (!pg) panic("pmap_growkernel: no pages");
		pmap_zero_page((paddr_t)pg);

		/* XXX This is based on all phymem being addressable */
		pm->pm_ptbl[seg] = (u_int *)pg;
	}
	splx(s);
	return (kbreak);
}

/*
 *	vm_page_alloc1:
 *
 *	Allocate and return a memory cell with no associated object.
 */
struct vm_page *
vm_page_alloc1()
{
	struct vm_page *pg = uvm_pagealloc(NULL, 0, NULL, UVM_PGA_USERESERVE);
	if (pg) {
		pg->wire_count = 1;	/* no mappings yet */
		pg->flags &= ~PG_BUSY;	/* never busy */
	}
	return pg;
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
	struct vm_page *mem;
{
#ifdef DIAGNOSTIC
	if (mem->flags != (PG_CLEAN|PG_FAKE)) {
		printf("Freeing invalid page %p\n", mem);
		printf("pa = %llx\n", (unsigned long long)VM_PAGE_TO_PHYS(mem));
#ifdef DDB
		Debugger();
#endif
		return;
	}
#endif
	mem->flags |= PG_BUSY;
	mem->wire_count = 0;
	uvm_pagefree(mem);
}
#endif

/*
 * Create and return a physical map.
 */
struct pmap *
pmap_create(void)
{
	struct pmap *pm;

	pm = (struct pmap *)malloc(sizeof *pm, M_VMPMAP, M_WAITOK);
	memset((caddr_t)pm, 0, sizeof *pm);
	pmap_pinit(pm);
	return pm;
}

/*
 * Initialize a preallocated and zeroed pmap structure.
 */
void
pmap_pinit(struct pmap *pm)
{
	int i;

	/*
	 * Allocate some segment registers for this pmap.
	 */
	pm->pm_refs = 1;
	for (i = 0; i < STSZ; i++)
		pm->pm_ptbl[i] = NULL;
}

/*
 * Add a reference to the given pmap.
 */
void
pmap_reference(struct pmap *pm)
{

	pm->pm_refs++;
}

/*
 * Retire the given pmap from service.
 * Should only be called if the map contains no valid mappings.
 */
void
pmap_destroy(struct pmap *pm)
{

	if (--pm->pm_refs == 0) {
		pmap_release(pm);
		free((caddr_t)pm, M_VMPMAP);
	}
}

/*
 * Release any resources held by the given physical map.
 * Called when a pmap initialized by pmap_pinit is being released.
 */
static void
pmap_release(struct pmap *pm)
{
	int i;

	for (i = 0; i < STSZ; i++)
		if (pm->pm_ptbl[i]) {
			uvm_km_free(kernel_map, (vaddr_t)pm->pm_ptbl[i], NBPG);
			pm->pm_ptbl[i] = NULL;
		}
	if (pm->pm_ctx) ctx_free(pm);
}

/*
 * Copy the range specified by src_addr/len
 * from the source map to the range dst_addr/len
 * in the destination map.
 *
 * This routine is only advisory and need not do anything.
 */
void
pmap_copy(struct pmap *dst_pmap, struct pmap *src_pmap, vaddr_t dst_addr,
	  vsize_t len, vaddr_t src_addr)
{
}

/*
 * Require that all active physical maps contain no
 * incorrect entries NOW.
 */
void
pmap_update(struct pmap *pmap)
{
}

/*
 * Garbage collects the physical map system for
 * pages which are no longer used.
 * Success need not be guaranteed -- that is, there
 * may well be pages which are not referenced, but
 * others may be collected.
 * Called by the pageout daemon when pages are scarce.
 */
void
pmap_collect(struct pmap *pm)
{
}

/*
 * Fill the given physical page with zeroes.
 */
void
pmap_zero_page(paddr_t pa)
{

#ifdef PPC_4XX_NOCACHE
	memset((caddr_t)pa, 0, NBPG);
#else
	int i;

	for (i = NBPG/CACHELINESIZE; i > 0; i--) {
		__asm __volatile ("dcbz 0,%0" :: "r"(pa));
		pa += CACHELINESIZE;
	}
#endif
}

/*
 * Copy the given physical source page to its destination.
 */
void
pmap_copy_page(paddr_t src, paddr_t dst)
{

	memcpy((caddr_t)dst, (caddr_t)src, NBPG);
	dcache_flush_page(dst);
}

/*
 * This returns whether this is the first mapping of a page.
 */
static inline int
pmap_enter_pv(struct pmap *pm, vaddr_t va, paddr_t pa)
{
	struct pv_entry *pv, *npv = NULL;
	int s;

	if (!pmap_initialized)
		return 0;

	s = splvm();

	pv = pa_to_pv(pa);
for (npv = pv; npv; npv = npv->pv_next)
if (npv->pv_va == va && npv->pv_pm == pm) {
printf("Duplicate pv: va %lx pm %p\n", va, pm);
Debugger();
return (1);
}

	if (!pv->pv_pm) {
		/*
		 * No entries yet, use header as the first entry.
		 */
		pv->pv_va = va;
		pv->pv_pm = pm;
		pv->pv_next = NULL;
	} else {
		/*
		 * There is at least one other VA mapping this page.
		 * Place this entry after the header.
		 */
		npv = pool_get(&pv_pool, PR_WAITOK);
		if (!npv) return (0);
		npv->pv_va = va;
		npv->pv_pm = pm;
		npv->pv_next = pv->pv_next;
		pv->pv_next = npv;
	}
	splx(s);
	return (1);
}

static void
pmap_remove_pv(struct pmap *pm, vaddr_t va, paddr_t pa)
{
	struct pv_entry *pv, *npv;

	/*
	 * Remove from the PV table.
	 */
	pv = pa_to_pv(pa);
	if (!pv) return;

	/*
	 * If it is the first entry on the list, it is actually
	 * in the header and we must copy the following entry up
	 * to the header.  Otherwise we must search the list for
	 * the entry.  In either case we free the now unused entry.
	 */
	if (pm == pv->pv_pm && PV_CMPVA(va, pv)) {
		if ((npv = pv->pv_next)) {
			*pv = *npv;
			pool_put(&pv_pool, npv);
		} else
			pv->pv_pm = NULL;
	} else {
		for (; (npv = pv->pv_next) != NULL; pv = npv)
			if (pm == npv->pv_pm && PV_CMPVA(va, npv))
				break;
		if (npv) {
			pv->pv_next = npv->pv_next;
			pool_put(&pv_pool, npv);
		}
	}
}

/*
 * Insert physical page at pa into the given pmap at virtual address va.
 */
int
pmap_enter(struct pmap *pm, vaddr_t va, paddr_t pa, vm_prot_t prot, int flags)
{
	int s;
	u_int tte;
	int managed;

	/*
	 * Have to remove any existing mapping first.
	 */
	pmap_remove(pm, va, va + NBPG);

	if (flags & PMAP_WIRED) flags |= prot;

	/* If it has no protections don't bother w/the rest */
	if (!(flags & VM_PROT_ALL))
		return (0);

	managed = 0;
	if (vm_physseg_find(atop(pa), NULL) != -1)
		managed = 1;

	/*
	 * Generate TTE.
	 *
	 * XXXX
	 *
	 * Since the kernel does not handle execution privileges properly,
	 * we will handle read and execute permissions together.
	 */
	tte = TTE_PA(pa) | TTE_EX;
	/* XXXX -- need to support multiple page sizes. */
	tte |= TTE_SZ_16K;
#ifdef	DIAGNOSTIC
	if ((flags & (PME_NOCACHE | PME_WRITETHROUG)) ==
		(PME_NOCACHE | PME_WRITETHROUG))
		panic("pmap_enter: uncached & writethrough\n");
#endif
	if (flags & PME_NOCACHE)
		/* Must be I/O mapping */
		tte |= TTE_I | TTE_G;
#ifdef PPC_4XX_NOCACHE
	tte |= TTE_I;
#else
	else if (flags & PME_WRITETHROUG)
		/* Uncached and writethrough are not compatible */
		tte |= TTE_W;
#endif
	if (pm == pmap_kernel())
		tte |= TTE_ZONE(ZONE_PRIV);
	else
		tte |= TTE_ZONE(ZONE_USER);

	if (flags & VM_PROT_WRITE)
		tte |= TTE_WR;

	/*
	 * Now record mapping for later back-translation.
	 */
	if (pmap_initialized && managed) {
		char *attr;

		if (!pmap_enter_pv(pm, va, pa)) {
			/* Could not enter pv on a managed page */
			return 1;
		}

		/* Now set attributes. */
		attr = pa_to_attr(pa);
#ifdef DIAGNOSTIC
		if (!attr)
			panic("managed but no attr\n");
#endif
		if (flags & VM_PROT_ALL)
			*attr |= PTE_HI_REF;
		if (flags & VM_PROT_WRITE)
			*attr |= PTE_HI_CHG;
	}

	s = splvm();
	pm->pm_stats.resident_count++;

	/* Insert page into page table. */
	pte_enter(pm, va, tte);

	/* If this is a real fault, enter it in the tlb */
	if (tte && ((flags & PMAP_WIRED) == 0)) {
		ppc4xx_tlb_enter(pm->pm_ctx, va, tte);
	}
	splx(s);

	/* Flush the real memory from the instruction cache. */
	if ((prot & VM_PROT_EXECUTE) && (tte & TTE_I) == 0)
		__syncicache((void *)pa, PAGE_SIZE);

	return 0;
}

void
pmap_unwire(struct pmap *pm, vaddr_t va)
{
	struct pv_entry *pv, *npv;
	paddr_t pa;
	int s = splvm();

        if (pm == NULL) { 
                return;
        }

	if (!pmap_extract(pm, va, &pa)) {
		return;
	}

	va |= PV_WIRED;

	pv = pa_to_pv(pa);
	if (!pv) return;

	/*
	 * If it is the first entry on the list, it is actually
	 * in the header and we must copy the following entry up
	 * to the header.  Otherwise we must search the list for
	 * the entry.  In either case we free the now unused entry.
	 */
	for (npv = pv; (npv = pv->pv_next) != NULL; pv = npv) {
		if (pm == npv->pv_pm && PV_CMPVA(va, npv)) {
			npv->pv_va &= ~PV_WIRED;
			break;
		}
	}
	splx(s);
}

void
pmap_kenter_pa(vaddr_t va, paddr_t pa, vm_prot_t prot)
{
	int s;
	u_int tte;
	struct pmap *pm = pmap_kernel();

	/*
	 * Have to remove any existing mapping first.
	 */

	/*
	 * Generate TTE.
	 *
	 * XXXX
	 *
	 * Since the kernel does not handle execution privileges properly,
	 * we will handle read and execute permissions together.
	 */
	tte = 0;
	if (prot & VM_PROT_ALL) {

		tte = TTE_PA(pa) | TTE_EX | TTE_ZONE(ZONE_PRIV);
		/* XXXX -- need to support multiple page sizes. */
		tte |= TTE_SZ_16K;
#ifdef DIAGNOSTIC
		if ((prot & (PME_NOCACHE | PME_WRITETHROUG)) ==
			(PME_NOCACHE | PME_WRITETHROUG))
			panic("pmap_kenter_pa: uncached & writethrough\n");
#endif
		if (prot & PME_NOCACHE)
			/* Must be I/O mapping */
			tte |= TTE_I | TTE_G;
#ifdef PPC_4XX_NOCACHE
		tte |= TTE_I;
#else
		else if (prot & PME_WRITETHROUG)
			/* Uncached and writethrough are not compatible */
			tte |= TTE_W;
#endif
		if (prot & VM_PROT_WRITE)
			tte |= TTE_WR;
	}

	s = splvm();
	pm->pm_stats.resident_count++;

	/* Insert page into page table. */
	pte_enter(pm, va, tte);
	splx(s);
}

void
pmap_kremove(vaddr_t va, vsize_t len)
{

	while (len > 0) {
		pte_enter(pmap_kernel(), va, 0);
		va += PAGE_SIZE;
		len -= PAGE_SIZE;
	}
}

/*
 * Remove the given range of mapping entries.
 */
void
pmap_remove(struct pmap *pm, vaddr_t va, vaddr_t endva)
{
	int s;
	paddr_t pa;
	volatile u_int *ptp;

	s = splvm();
	while (va < endva) {

		if ((ptp = pte_find(pm, va)) && (pa = *ptp)) {
			pa = TTE_PA(pa);
			pmap_remove_pv(pm, va, pa);
			*ptp = 0;
			ppc4xx_tlb_flush(va, pm->pm_ctx);
			pm->pm_stats.resident_count--;
		}
		va += NBPG;
	}

	splx(s);
}

/*
 * Get the physical page address for the given pmap/virtual address.
 */
boolean_t
pmap_extract(struct pmap *pm, vaddr_t va, paddr_t *pap)
{
	int seg = STIDX(va);
	int ptn = PTIDX(va);
	u_int pa = 0;
	int s = splvm();

	if (pm->pm_ptbl[seg] && (pa = pm->pm_ptbl[seg][ptn])) {
		*pap = TTE_PA(pa) | (va & PGOFSET);
	}
	splx(s);
	return (pa != 0);
}

/*
 * Lower the protection on the specified range of this pmap.
 *
 * There are only two cases: either the protection is going to 0,
 * or it is going to read-only.
 */
void
pmap_protect(struct pmap *pm, vaddr_t sva, vaddr_t eva, vm_prot_t prot)
{
	volatile u_int *ptp;
	int s;

	if (prot & VM_PROT_READ) {
		s = splvm();
		while (sva < eva) {
			if ((ptp = pte_find(pm, sva)) != NULL) {
				*ptp &= ~TTE_WR;
				ppc4xx_tlb_flush(sva, pm->pm_ctx);
			}
			sva += NBPG;
		}
		splx(s);
		return;
	}
	pmap_remove(pm, sva, eva);
}

boolean_t
check_attr(struct vm_page *pg, u_int mask, int clear)
{
	paddr_t pa = VM_PAGE_TO_PHYS(pg);
	int s;
	char *attr;
	int rv;

	/*
	 * First modify bits in cache.
	 */
	s = splvm();
	attr = pa_to_attr(pa);
	if (attr == NULL)
		return FALSE;

	rv = ((*attr & mask) != 0);
	if (clear) {
		*attr &= ~mask;
		pmap_page_protect(pg, (mask == PTE_HI_CHG) ? VM_PROT_READ : 0);
	}
	splx(s);
	return rv;
}


/*
 * Lower the protection on the specified physical page.
 *
 * There are only two cases: either the protection is going to 0,
 * or it is going to read-only.
 */
void
pmap_page_protect(struct vm_page *pg, vm_prot_t prot)
{
	paddr_t pa = VM_PAGE_TO_PHYS(pg);
	vaddr_t va;
	struct pv_entry *pvh, *pv, *npv;
	struct pmap *pm;

	pvh = pa_to_pv(pa);
	if (pvh == NULL)
		return;

	/* Handle extra pvs which may be deleted in the operation */
	for (pv = pvh->pv_next; pv; pv = npv) {
		npv = pv->pv_next;

		pm = pv->pv_pm;
		va = pv->pv_va;
		pmap_protect(pm, va, va+NBPG, prot);
	}
	/* Now check the head pv */
	if (pvh->pv_pm) {
		pv = pvh;
		pm = pv->pv_pm;
		va = pv->pv_va;
		pmap_protect(pm, va, va+NBPG, prot);
	}
}

/*
 * Activate the address space for the specified process.  If the process
 * is the current process, load the new MMU context.
 */
void
pmap_activate(struct proc *p)
{
#if 0
	struct pcb *pcb = &p->p_addr->u_pcb;
	pmap_t pmap = p->p_vmspace->vm_map.pmap;

	/*
	 * XXX Normally performed in cpu_fork().
	 */
	printf("pmap_activate(%p), pmap=%p\n",p,pmap);
	if (pcb->pcb_pm != pmap) {
		pcb->pcb_pm = pmap;
		(void) pmap_extract(pmap_kernel(), (vaddr_t)pcb->pcb_pm,
		    (paddr_t *)&pcb->pcb_pmreal);
	}

	if (p == curproc) {
		/* Store pointer to new current pmap. */
		curpm = pcb->pcb_pmreal;
	}
#endif
}

/*
 * Deactivate the specified process's address space.
 */
void
pmap_deactivate(struct proc *p)
{
}

/*
 * Synchronize caches corresponding to [addr, addr+len) in p.
 */
void
pmap_procwr(struct proc *p, vaddr_t va, size_t len)
{
	struct pmap *pm = p->p_vmspace->vm_map.pmap;
	int msr, ctx, opid;


	/*
	 * Need to turn off IMMU and switch to user context.
	 * (icbi uses DMMU).
	 */
	if (!(ctx = pm->pm_ctx)) {
		/* No context -- assign it one */
		ctx_alloc(pm);
		ctx = pm->pm_ctx;
	}
	__asm __volatile("mfmsr %0;"
		"li %1, 0x20;"
		"andc %1,%0,%1;"
		"mtmsr %1;"
		"sync;isync;"
		"mfpid %1;"
		"mtpid %2;"
		"sync; isync;"
		"1:" 
		"dcbf 0,%3;"
		"icbi 0,%3;"
		"addi %3,%3,32;"
		"addic. %4,%4,-32;"
		"bge 1b;"
		"mtpid %1;"
		"mtmsr %0;"
		"sync; isync"
		: "=&r" (msr), "=&r" (opid)
		: "r" (ctx), "r" (va), "r" (len));
}


/* This has to be done in real mode !!! */
void
ppc4xx_tlb_flush(vaddr_t va, int pid)
{
	u_long i, found;
	u_long msr;

	/* If there's no context then it can't be mapped. */
	if (!pid) return;

	asm("mfpid %1;"			/* Save PID */
		"mfmsr %2;"		/* Save MSR */
		"li %0,0;"		/* Now clear MSR */
		"mtmsr %0;"
		"mtpid %4;"		/* Set PID */
		"sync;"
		"tlbsx. %0,0,%3;"	/* Search TLB */
		"sync;"
		"mtpid %1;"		/* Restore PID */
		"mtmsr %2;"		/* Restore MSR */
		"sync;isync;"
		"li %1,1;"
		"beq 1f;"
		"li %1,0;"
		"1:"
		: "=&r" (i), "=&r" (found), "=&r" (msr)
		: "r" (va), "r" (pid));
	if (found && !TLB_LOCKED(i)) {

		/* Now flush translation */
		asm volatile(
			"tlbwe %0,%1,0;"
			"sync;isync;"
			: : "r" (0), "r" (i));

		tlb_info[i].ti_ctx = 0;
		tlb_info[i].ti_flags = 0;
		tlbnext = i;
		/* Successful flushes */
		tlbflush_ev.ev_count++;
	}
}

void
ppc4xx_tlb_flush_all(void)
{
	u_long i;

	for (i = 0; i < NTLB; i++)
		if (!TLB_LOCKED(i)) {
			asm volatile(
				"tlbwe %0,%1,0;"
				"sync;isync;"
				: : "r" (0), "r" (i));
			tlb_info[i].ti_ctx = 0;
			tlb_info[i].ti_flags = 0;
		}

	asm volatile("sync;isync");
}

/* Find a TLB entry to evict. */
static int
ppc4xx_tlb_find_victim(void)
{
	int flags;

	for (;;) {
		if (++tlbnext >= NTLB)
			tlbnext = TLB_NRESERVED;
		flags = tlb_info[tlbnext].ti_flags;
		if (!(flags & TLBF_USED) || 
			(flags & (TLBF_LOCKED | TLBF_REF)) == 0) {
			u_long va, stack = (u_long)&va;

			if (!((tlb_info[tlbnext].ti_va ^ stack) & (~PGOFSET)) &&
			    (tlb_info[tlbnext].ti_ctx == KERNEL_PID) &&
			     (flags & TLBF_USED)) {
				/* Kernel stack page */
				flags |= TLBF_USED;
				tlb_info[tlbnext].ti_flags = flags;
			} else {
				/* Found it! */
				return (tlbnext);
			}
		} else {
			tlb_info[tlbnext].ti_flags = (flags & ~TLBF_REF);
		}
	}
}

void
ppc4xx_tlb_enter(int ctx, vaddr_t va, u_int pte)
{
	u_long th, tl, idx;
	tlbpid_t pid;
	u_short msr;
	paddr_t pa;
	int s, sz;

  
	tlbenter_ev.ev_count++;

	sz = (pte & TTE_SZ_MASK) >> TTE_SZ_SHIFT;
	pa = (pte & TTE_RPN_MASK(sz));
	th = (va & TLB_EPN_MASK) | (sz << TLB_SIZE_SHFT) | TLB_VALID;
	tl = (pte & ~TLB_RPN_MASK) | pa;
	tl |= ppc4xx_tlbflags(va, pa);

	s = splhigh();
	idx = ppc4xx_tlb_find_victim();

#ifdef DIAGNOSTIC
	if ((idx < TLB_NRESERVED) || (idx >= NTLB)) {
		panic("ppc4xx_tlb_enter: repacing entry %ld\n", idx);
	}
#endif
	
	tlb_info[idx].ti_va = (va & TLB_EPN_MASK);
	tlb_info[idx].ti_ctx = ctx;
	tlb_info[idx].ti_flags = TLBF_USED | TLBF_REF;

	asm volatile(
		"mfmsr %0;"			/* Save MSR */
		"li %1,0;"
		"tlbwe %1,%3,0;"		/* Invalidate old entry. */
		"mtmsr %1;"			/* Clear MSR */
		"mfpid %1;"			/* Save old PID */
		"mtpid %2;"			/* Load translation ctx */
		"sync; isync;"
#ifdef DEBUG
		"andi. %3,%3,63;"
		"tweqi %3,0;" 			/* XXXXX DEBUG trap on index 0 */
#endif
		"tlbwe %4,%3,1; tlbwe %5,%3,0;"	/* Set TLB */
		"sync; isync;"
		"mtpid %1; mtmsr %0;"		/* Restore PID and MSR */
		"sync; isync;"
	: "=&r" (msr), "=&r" (pid)
	: "r" (ctx), "r" (idx), "r" (tl), "r" (th));
	splx(s);
}

void
ppc4xx_tlb_unpin(int i)
{

	if (i == -1)
		for (i = 0; i < TLB_NRESERVED; i++)
			tlb_info[i].ti_flags &= ~TLBF_LOCKED;
	else
		tlb_info[i].ti_flags &= ~TLBF_LOCKED;
}

void
ppc4xx_tlb_init(void)
{
	int i;

	/* Mark reserved TLB entries */
	for (i = 0; i < TLB_NRESERVED; i++) {
		tlb_info[i].ti_flags = TLBF_LOCKED | TLBF_USED;
		tlb_info[i].ti_ctx = KERNEL_PID;
	}

	/* Setup security zones */
	/* Z0 - accessible by kernel only if TLB entry permissions allow
	 * Z1,Z2 - access is controlled by TLB entry permissions
	 * Z3 - full access regardless of TLB entry permissions
	 */

	asm volatile(
		"mtspr %0,%1;"
		"sync;"
		::  "K"(SPR_ZPR), "r" (0x1b000000));
}


/*
 * We should pass the ctx in from trap code.
 */
int
pmap_tlbmiss(vaddr_t va, int ctx)
{
	volatile u_int *pte;
	u_long tte;

	tlbmiss_ev.ev_count++;

	/*
	 * XXXX We will reserve 0-0x80000000 for va==pa mappings.
	 */
	if (ctx != KERNEL_PID || (va & 0x80000000)) {
		pte = pte_find((struct pmap *)ctxbusy[ctx], va);
		if (pte == NULL) {
			/* Map unmanaged addresses directly for kernel access */
			return 1;
		}
		tte = *pte;
		if (tte == 0) {
			return 1;
		}
	} else {
		/* Create a 16MB writeable mapping. */
#ifdef PPC_4XX_NOCACHE
		tte = TTE_PA(va) | TTE_ZONE(ZONE_PRIV) | TTE_SZ_16M | TTE_I | TTE_WR;
#else
		tte = TTE_PA(va) | TTE_ZONE(ZONE_PRIV) | TTE_SZ_16M | TTE_WR;
#endif
	}
	tlbhit_ev.ev_count++;
	ppc4xx_tlb_enter(ctx, va, tte);

	return 0;
}

/*
 * Flush all the entries matching a context from the TLB.
 */
static int
ctx_flush(int cnum)
{
	int i;

	/* We gotta steal this context */
	for (i = TLB_NRESERVED; i < NTLB; i++) {
		if (tlb_info[i].ti_ctx == cnum) {
			/* Can't steal ctx if it has a locked entry. */
			if (TLB_LOCKED(i)) {
#ifdef DIAGNOSTIC
				printf("ctx_flush: can't invalidate "
					"locked mapping %d "
					"for context %d\n", i, cnum);
#ifdef DDB
				Debugger();
#endif
#endif
				return (1);
			}
#ifdef DIAGNOSTIC
			if (i < TLB_NRESERVED)
				panic("TLB entry %d not locked\n", i);
#endif
			/* Invalidate particular TLB entry regardless of locked status */
			asm volatile("tlbwe %0,%1,0" : :"r"(0),"r"(i));
			tlb_info[i].ti_flags = 0;
		}
	}
	return (0);
}

/*
 * Allocate a context.  If necessary, steal one from someone else.
 *
 * The new context is flushed from the TLB before returning.
 */
int
ctx_alloc(struct pmap *pm)
{
	int s, cnum;
	static int next = MINCTX;

	if (pm == pmap_kernel()) {
#ifdef DIAGNOSTIC
		printf("ctx_alloc: kernel pmap!\n");
#endif
		return (0);
	}
	s = splvm();

	/* Find a likely context. */
	cnum = next;
	do {
		if ((++cnum) > NUMCTX)
			cnum = MINCTX;
	} while (ctxbusy[cnum] != NULL && cnum != next);

	/* Now clean it out */
oops:
	if (cnum < MINCTX)
		cnum = MINCTX; /* Never steal ctx 0 or 1 */
	if (ctx_flush(cnum)) {
		/* oops -- something's wired. */
		if ((++cnum) > NUMCTX)
			cnum = MINCTX;
		goto oops;
	}

	if (ctxbusy[cnum]) {
#ifdef DEBUG
		/* We should identify this pmap and clear it */
		printf("Warning: stealing context %d\n", cnum);
#endif
		ctxbusy[cnum]->pm_ctx = 0;
	}
	ctxbusy[cnum] = pm;
	next = cnum;
	splx(s);
	pm->pm_ctx = cnum;

	return cnum;
}

/*
 * Give away a context.
 */
void
ctx_free(struct pmap *pm)
{
	int oldctx;

	oldctx = pm->pm_ctx;

	if (oldctx == 0)
		panic("ctx_free: freeing kernel context");
#ifdef DIAGNOSTIC
	if (ctxbusy[oldctx] == 0)
		printf("ctx_free: freeing free context %d\n", oldctx);
	if (ctxbusy[oldctx] != pm) {
		printf("ctx_free: freeing someone esle's context\n "
		       "ctxbusy[%d] = %p, pm->pm_ctx = %p\n",
		       oldctx, (void *)(u_long)ctxbusy[oldctx], pm);
#ifdef DDB
		Debugger();
#endif
	}
#endif
	/* We should verify it has not been stolen and reallocated... */
	ctxbusy[oldctx] = NULL;
	ctx_flush(oldctx);
}


#ifdef DEBUG
/*
 * Test ref/modify handling.
 */
void pmap_testout __P((void));
void
pmap_testout()
{
	vaddr_t va;
	volatile int *loc;
	int val = 0;
	paddr_t pa;
	struct vm_page *pg;
	int ref, mod;

	/* Allocate a page */
	va = (vaddr_t)uvm_km_alloc1(kernel_map, NBPG, 1);
	loc = (int*)va;

	pmap_extract(pmap_kernel(), va, &pa);
	pg = PHYS_TO_VM_PAGE(pa);
	pmap_unwire(pmap_kernel(), va);

	pmap_remove(pmap_kernel(), va, va+1);
	pmap_enter(pmap_kernel(), va, pa, VM_PROT_ALL, 0);
	pmap_update(pmap_kernel());

	/* Now clear reference and modify */
	ref = pmap_clear_reference(pg);
	mod = pmap_clear_modify(pg);
	printf("Clearing page va %p pa %lx: ref %d, mod %d\n",
	       (void *)(u_long)va, (long)pa,
	       ref, mod);

	/* Check it's properly cleared */
	ref = pmap_is_referenced(pg);
	mod = pmap_is_modified(pg);
	printf("Checking cleared page: ref %d, mod %d\n",
	       ref, mod);

	/* Reference page */
	val = *loc;

	ref = pmap_is_referenced(pg);
	mod = pmap_is_modified(pg);
	printf("Referenced page: ref %d, mod %d val %x\n",
	       ref, mod, val);

	/* Now clear reference and modify */
	ref = pmap_clear_reference(pg);
	mod = pmap_clear_modify(pg);
	printf("Clearing page va %p pa %lx: ref %d, mod %d\n",
	       (void *)(u_long)va, (long)pa,
	       ref, mod);
	
	/* Modify page */
	*loc = 1;

	ref = pmap_is_referenced(pg);
	mod = pmap_is_modified(pg);
	printf("Modified page: ref %d, mod %d\n",
	       ref, mod);

	/* Now clear reference and modify */
	ref = pmap_clear_reference(pg);
	mod = pmap_clear_modify(pg);
	printf("Clearing page va %p pa %lx: ref %d, mod %d\n",
	       (void *)(u_long)va, (long)pa,
	       ref, mod);

	/* Check it's properly cleared */
	ref = pmap_is_referenced(pg);
	mod = pmap_is_modified(pg);
	printf("Checking cleared page: ref %d, mod %d\n",
	       ref, mod);

	/* Modify page */
	*loc = 1;

	ref = pmap_is_referenced(pg);
	mod = pmap_is_modified(pg);
	printf("Modified page: ref %d, mod %d\n",
	       ref, mod);

	/* Check pmap_protect() */
	pmap_protect(pmap_kernel(), va, va+1, VM_PROT_READ);
	pmap_update(pmap_kernel());
	ref = pmap_is_referenced(pg);
	mod = pmap_is_modified(pg);
	printf("pmap_protect(VM_PROT_READ): ref %d, mod %d\n",
	       ref, mod);

	/* Now clear reference and modify */
	ref = pmap_clear_reference(pg);
	mod = pmap_clear_modify(pg);
	printf("Clearing page va %p pa %lx: ref %d, mod %d\n",
	       (void *)(u_long)va, (long)pa,
	       ref, mod);

	/* Reference page */
	val = *loc;

	ref = pmap_is_referenced(pg);
	mod = pmap_is_modified(pg);
	printf("Referenced page: ref %d, mod %d val %x\n",
	       ref, mod, val);

	/* Now clear reference and modify */
	ref = pmap_clear_reference(pg);
	mod = pmap_clear_modify(pg);
	printf("Clearing page va %p pa %lx: ref %d, mod %d\n",
	       (void *)(u_long)va, (long)pa,
	       ref, mod);
	
	/* Modify page */
#if 0
	pmap_enter(pmap_kernel(), va, pa, VM_PROT_ALL, 0);
	pmap_update(pmap_kernel());
#endif
	*loc = 1;

	ref = pmap_is_referenced(pg);
	mod = pmap_is_modified(pg);
	printf("Modified page: ref %d, mod %d\n",
	       ref, mod);

	/* Check pmap_protect() */
	pmap_protect(pmap_kernel(), va, va+1, VM_PROT_NONE);
	pmap_update(pmap_kernel());
	ref = pmap_is_referenced(pg);
	mod = pmap_is_modified(pg);
	printf("pmap_protect(): ref %d, mod %d\n",
	       ref, mod);

	/* Now clear reference and modify */
	ref = pmap_clear_reference(pg);
	mod = pmap_clear_modify(pg);
	printf("Clearing page va %p pa %lx: ref %d, mod %d\n",
	       (void *)(u_long)va, (long)pa,
	       ref, mod);

	/* Reference page */
	val = *loc;

	ref = pmap_is_referenced(pg);
	mod = pmap_is_modified(pg);
	printf("Referenced page: ref %d, mod %d val %x\n",
	       ref, mod, val);

	/* Now clear reference and modify */
	ref = pmap_clear_reference(pg);
	mod = pmap_clear_modify(pg);
	printf("Clearing page va %p pa %lx: ref %d, mod %d\n",
	       (void *)(u_long)va, (long)pa,
	       ref, mod);
	
	/* Modify page */
#if 0
	pmap_enter(pmap_kernel(), va, pa, VM_PROT_ALL, 0);
	pmap_update(pmap_kernel());
#endif
	*loc = 1;

	ref = pmap_is_referenced(pg);
	mod = pmap_is_modified(pg);
	printf("Modified page: ref %d, mod %d\n",
	       ref, mod);

	/* Check pmap_pag_protect() */
	pmap_page_protect(pg, VM_PROT_READ);
	ref = pmap_is_referenced(pg);
	mod = pmap_is_modified(pg);
	printf("pmap_page_protect(VM_PROT_READ): ref %d, mod %d\n",
	       ref, mod);

	/* Now clear reference and modify */
	ref = pmap_clear_reference(pg);
	mod = pmap_clear_modify(pg);
	printf("Clearing page va %p pa %lx: ref %d, mod %d\n",
	       (void *)(u_long)va, (long)pa,
	       ref, mod);

	/* Reference page */
	val = *loc;

	ref = pmap_is_referenced(pg);
	mod = pmap_is_modified(pg);
	printf("Referenced page: ref %d, mod %d val %x\n",
	       ref, mod, val);

	/* Now clear reference and modify */
	ref = pmap_clear_reference(pg);
	mod = pmap_clear_modify(pg);
	printf("Clearing page va %p pa %lx: ref %d, mod %d\n",
	       (void *)(u_long)va, (long)pa,
	       ref, mod);
	
	/* Modify page */
#if 0
	pmap_enter(pmap_kernel(), va, pa, VM_PROT_ALL, 0);
	pmap_update(pmap_kernel());
#endif
	*loc = 1;

	ref = pmap_is_referenced(pg);
	mod = pmap_is_modified(pg);
	printf("Modified page: ref %d, mod %d\n",
	       ref, mod);

	/* Check pmap_pag_protect() */
	pmap_page_protect(pg, VM_PROT_NONE);
	ref = pmap_is_referenced(pg);
	mod = pmap_is_modified(pg);
	printf("pmap_page_protect(): ref %d, mod %d\n",
	       ref, mod);

	/* Now clear reference and modify */
	ref = pmap_clear_reference(pg);
	mod = pmap_clear_modify(pg);
	printf("Clearing page va %p pa %lx: ref %d, mod %d\n",
	       (void *)(u_long)va, (long)pa,
	       ref, mod);


	/* Reference page */
	val = *loc;

	ref = pmap_is_referenced(pg);
	mod = pmap_is_modified(pg);
	printf("Referenced page: ref %d, mod %d val %x\n",
	       ref, mod, val);

	/* Now clear reference and modify */
	ref = pmap_clear_reference(pg);
	mod = pmap_clear_modify(pg);
	printf("Clearing page va %p pa %lx: ref %d, mod %d\n",
	       (void *)(u_long)va, (long)pa,
	       ref, mod);
	
	/* Modify page */
#if 0
	pmap_enter(pmap_kernel(), va, pa, VM_PROT_ALL, 0);
	pmap_update(pmap_kernel());
#endif
	*loc = 1;

	ref = pmap_is_referenced(pg);
	mod = pmap_is_modified(pg);
	printf("Modified page: ref %d, mod %d\n",
	       ref, mod);

	/* Unmap page */
	pmap_remove(pmap_kernel(), va, va+1);
	pmap_update(pmap_kernel());
	ref = pmap_is_referenced(pg);
	mod = pmap_is_modified(pg);
	printf("Unmapped page: ref %d, mod %d\n", ref, mod);

	/* Now clear reference and modify */
	ref = pmap_clear_reference(pg);
	mod = pmap_clear_modify(pg);
	printf("Clearing page va %p pa %lx: ref %d, mod %d\n",
	       (void *)(u_long)va, (long)pa, ref, mod);

	/* Check it's properly cleared */
	ref = pmap_is_referenced(pg);
	mod = pmap_is_modified(pg);
	printf("Checking cleared page: ref %d, mod %d\n",
	       ref, mod);

	pmap_enter(pmap_kernel(), va, pa, VM_PROT_ALL, 
		VM_PROT_ALL|PMAP_WIRED);
	uvm_km_free(kernel_map, (vaddr_t)va, NBPG);
}
#endif
