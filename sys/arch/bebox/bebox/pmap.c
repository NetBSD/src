/*	$NetBSD: pmap.c,v 1.1 1997/10/14 06:47:48 sakamoto Exp $	*/

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
#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/queue.h>
#include <sys/systm.h>
#include <sys/msgbuf.h>

#include <vm/vm.h>
#include <vm/vm_kern.h>
#include <vm/vm_page.h>
#include <vm/vm_pageout.h>

#include <machine/pcb.h>
#include <machine/powerpc.h>

vm_page_t vm_page_alloc1 __P((void));
void vm_page_free1 __P((vm_page_t));

pte_t *ptable;
int ptab_cnt = 0;
u_int ptab_mask;
#define	HTABSIZE	(ptab_cnt * 64)

struct pte_ovfl {
	LIST_ENTRY(pte_ovfl) po_list;	/* Linked list of overflow entries */
	struct pte po_pte;		/* PTE for this mapping */
};

LIST_HEAD(pte_ovtab, pte_ovfl) *potable; /* Overflow entries for ptable */

struct pmap kernel_pmap_;

int physmem;
static vm_offset_t phys_mem_start;
static vm_size_t phys_mem_size;
extern caddr_t msgbufaddr;

/*
 * Given a map and a machine independent protection code,
 * convert to a vax protection code.
 */
vm_offset_t	avail_start;	/* PA of first available physical page */
vm_offset_t	avail_end;	/* PA of last available physical page */
vm_offset_t	virtual_avail;  /* VA of first avail page (after kernel bss)*/
vm_offset_t	virtual_end;	/* VA of last avail page (end of kernel AS) */

/*
 * This is a cache of referenced/modified bits.
 * Bits herein are shifted by ATTRSHFT.
 */
static char *pmap_attrib;
#define	ATTRSHFT	4

struct pv_entry {
	struct pv_entry *pv_next;	/* Linked list of mappings */
	int pv_idx;			/* Index into ptable */
	vm_offset_t pv_va;		/* virtual address of mapping */
};

struct pv_entry *pv_table;

struct pv_page;
struct pv_page_info {
	LIST_ENTRY(pv_page) pgi_list;
	struct pv_entry *pgi_freelist;
	int pgi_nfree;
};
#define	NPVPPG	((NBPG - sizeof(struct pv_page_info)) / sizeof(struct pv_entry))
struct pv_page {
	struct pv_page_info pvp_pgi;
	struct pv_entry pvp_pv[NPVPPG];
};
LIST_HEAD(pv_page_list, pv_page) pv_page_freelist;
int pv_nfree;
int pv_pcnt;
static struct pv_entry *pmap_alloc_pv __P((void));
static void pmap_free_pv __P((struct pv_entry *));

struct po_page;
struct po_page_info {
	LIST_ENTRY(po_page) pgi_list;
	vm_page_t pgi_page;
	LIST_HEAD(po_freelist, pte_ovfl) pgi_freelist;
	int pgi_nfree;
};
#define	NPOPPG	((NBPG - sizeof(struct po_page_info)) / sizeof(struct pte_ovfl))
struct po_page {
	struct po_page_info pop_pgi;
	struct pte_ovfl pop_po[NPOPPG];
};
LIST_HEAD(po_page_list, po_page) po_page_freelist;
int po_nfree;
int po_pcnt;
static struct pte_ovfl *poalloc __P((void));
static void pofree __P((struct pte_ovfl *, int));

static u_int usedsr[NPMAPS / sizeof(u_int) / 8];

static int pmap_initialized;

/*
 * These small routines may have to be replaced,
 * if/when we support processors other that the 604.
 */
static inline void
tlbie(ea)
	caddr_t ea;
{
	asm volatile ("tlbie %0" :: "r"(ea));
}

static inline void
tlbsync()
{
	asm volatile ("sync; tlbsync; sync");
}

static void
tlbia()
{
	caddr_t i;
	
	asm volatile ("sync");
	for (i = 0; i < (caddr_t)0x00040000; i += 0x00001000)
		tlbie(i);
	tlbsync();
}

static inline int
ptesr(sr, addr)
	sr_t *sr;
	vm_offset_t addr;
{
	return sr[(u_int)addr >> ADDR_SR_SHFT];
}

static inline int
pteidx(sr, addr)
	sr_t sr;
	vm_offset_t addr;
{
	int hash;
	
	hash = (sr & SR_VSID) ^ (((u_int)addr & ADDR_PIDX) >> ADDR_PIDX_SHFT);
	return hash & ptab_mask;
}

static inline int
ptematch(ptp, sr, va, which)
	pte_t *ptp;
	sr_t sr;
	vm_offset_t va;
	int which;
{
	return ptp->pte_hi
		== (((sr & SR_VSID) << PTE_VSID_SHFT)
		    | (((u_int)va >> ADDR_API_SHFT) & PTE_API)
		    | which);
}

/*
 * Try to insert page table entry *pt into the ptable at idx.
 *
 * Note: *pt mustn't have PTE_VALID set.
 * This is done here as required by Book III, 4.12.
 */
static int
pte_insert(idx, pt)
	int idx;
	pte_t *pt;
{
	pte_t *ptp;
	int i;
	
	/*
	 * First try primary hash.
	 */
	for (ptp = ptable + idx * 8, i = 8; --i >= 0; ptp++)
		if (!(ptp->pte_hi & PTE_VALID)) {
			*ptp = *pt;
			ptp->pte_hi &= ~PTE_HID;
			asm volatile ("sync");
			ptp->pte_hi |= PTE_VALID;
			return 1;
		}
	idx ^= ptab_mask;
	for (ptp = ptable + idx * 8, i = 8; --i >= 0; ptp++)
		if (!(ptp->pte_hi & PTE_VALID)) {
			*ptp = *pt;
			ptp->pte_hi |= PTE_HID;
			asm volatile ("sync");
			ptp->pte_hi |= PTE_VALID;
			return 1;
		}
	return 0;
}

/*
 * Spill handler.
 *
 * Tries to spill a page table entry from the overflow area.
 * Note that this routine runs in real mode on a separate stack,
 * with interrupts disabled.
 */
int
pte_spill(addr)
	vm_offset_t addr;
{
	int idx, i;
	sr_t sr;
	struct pte_ovfl *po;
	pte_t ps;
	pte_t *pt;

	asm ("mfsrin %0,%1" : "=r"(sr) : "r"(addr));
	idx = pteidx(sr, addr);
	for (po = potable[idx].lh_first; po; po = po->po_list.le_next)
		if (ptematch(&po->po_pte, sr, addr, 0)) {
			/*
			 * Now found an entry to be spilled into the real ptable.
			 */
			if (pte_insert(idx, &po->po_pte)) {
				LIST_REMOVE(po, po_list);
				pofree(po, 0);
				return 1;
			}
			/*
			 * Have to substitute some entry. Use the primary hash for this.
			 *
			 * Use low bits of timebase as random generator
			 */
			asm ("mftb %0" : "=r"(i));
			pt = ptable + idx * 8 + (i & 7);
			pt->pte_hi &= ~PTE_VALID;
			ps = *pt;
			asm volatile ("sync");
			tlbie(addr);
			tlbsync();
			*pt = po->po_pte;
			asm volatile ("sync");
			pt->pte_hi |= PTE_VALID;
			po->po_pte = ps;
			if (ps.pte_hi & PTE_HID) {
				/*
				 * We took an entry that was on the alternate hash
				 * chain, so move it to it's original chain.
				 */
				po->po_pte.pte_hi &= ~PTE_HID;
				LIST_REMOVE(po, po_list);
				LIST_INSERT_HEAD(potable + (idx ^ ptab_mask),
						 po, po_list);
			}
			return 1;
		}
	return 0;
}

/*
 * This is called during initppc, before the system is really initialized.
 */
void
pmap_bootstrap(kernelstart, kernelend, size)
	u_int kernelstart, kernelend, size;
{
	int i;
	u_int sz;

	phys_mem_start = 0;			/* physical memory start 0x0 */
	phys_mem_size = (vm_size_t)size;	/* from BeBox's BootROM */
	physmem = btoc(phys_mem_size);		/* all page size */

	avail_start = (kernelend + PGOFSET) & ~PGOFSET;
	avail_end = ctob(physmem);
	virtual_avail = (vm_offset_t)(KERNEL_SR << ADDR_SR_SHFT);
	virtual_end = virtual_avail + SEGMENT_LENGTH;

	/* allow for msgbuf */
	msgbufaddr = (caddr_t)virtual_avail;
	virtual_avail += round_page(MSGBUFSIZE);
	avail_end -= round_page(MSGBUFSIZE);

#ifdef	HTABENTS
	ptab_cnt = HTABENTS;
#else /* HTABENTS */
	for (i = 0; i < 32; i++)	/* XXX 32bit Implementation */
		if ((0x80000000 >> i) & avail_end) {
			ptab_cnt = (0x80000000 >> i) >> 7;
			break;
		}
	if (ffs(physmem) > 1)
		ptab_cnt <<= 1;
	ptab_cnt /= 64;
#endif /* HTABENTS */
	avail_start = (avail_start + (HTABSIZE - 1)) & ~(HTABSIZE - 1);
	ptable = (pte_t *)avail_start;		/* HASH TABLE */
	bzero((void *)ptable, HTABSIZE);
	ptab_mask = ptab_cnt - 1;
	avail_start += HTABSIZE;

	sz = round_page(sizeof(struct pte_ovtab) * ptab_cnt);
	potable = (struct pte_ovtab *)avail_start;
	avail_start += sz;

	for (i = 0; i < ptab_cnt; i++)
		LIST_INIT(potable + i);
	LIST_INIT(&pv_page_freelist);

	/*
	 * Initialize kernel pmap and hardware.
	 */
#if NPMAPS >= KERNEL_SEGMENT / 16
	usedsr[KERNEL_SEGMENT / 16 / (sizeof usedsr[0] * 8)]
		|= 1 << ((KERNEL_SEGMENT / 16) % (sizeof usedsr[0] * 8));
#endif
	for (i = 0; i < 16; i++) {
		pmap_kernel()->pm_sr[i] = EMPTY_SEGMENT;
		asm volatile ("mtsrin %0,%1"
			      :: "r"(EMPTY_SEGMENT), "r"(i << ADDR_SR_SHFT));
	}
	pmap_kernel()->pm_sr[KERNEL_SR] = KERNEL_SEGMENT;
	asm volatile ("mtsr %0,%1"
		      :: "n"(KERNEL_SR), "r"(KERNEL_SEGMENT));
	asm volatile ("sync; mtsdr1 %0; isync"
		      :: "r"((u_int)ptable | (ptab_mask >> 10)));
	tlbia();
}

void *
pmap_bootstrap_alloc(size)
	int size;
{
	vm_size_t sz;
	vm_offset_t val;

	sz = round_page(size);
	val = avail_start;
	avail_start += sz;
	if (avail_start > avail_end)
		panic("pmap_bootstrap_alloc");

	bzero((caddr_t)val, size);

	return ((void *)val);
}

/*
 * Restrict given range to physical memory
 */
void
pmap_real_memory(start, size)
	vm_offset_t *start;
	vm_size_t *size;
{
	if (*start + *size > phys_mem_start
	    && *start < phys_mem_start + phys_mem_size) {
		if (*start < phys_mem_start) {
			*size -= phys_mem_start - *start;
			*start = phys_mem_start;
		}
		if (*start + *size > phys_mem_start + phys_mem_size)
			*size = phys_mem_start + phys_mem_size - *start;
		return;
	}
	*size = 0;
}

/*
 * Initialize anything else for pmap handling.
 * Called during vm_init().
 */
void
pmap_init(vmstart, vmend)
	vm_offset_t vmstart;
	vm_offset_t vmend;
{
	struct pv_entry *pv;
	vm_size_t sz;
	vm_offset_t addr;
	int i, s;
	
	sz = (vm_size_t)((sizeof(struct pv_entry) + 1)
		* btoc(vmend - vmstart));
	sz = round_page(sz);
	addr = (vm_offset_t)kmem_alloc(kernel_map, sz);
	s = splimp();
	pv = pv_table = (struct pv_entry *)addr;
	for (i = btoc(vmend - vmstart); --i >= 0;)
		pv++->pv_idx = -1;
	LIST_INIT(&pv_page_freelist);
	pmap_attrib = (char *)pv;
	bzero(pv, btoc(vmend - vmstart));
	pmap_initialized = 1;
	splx(s);
}

/*
 * Return the index of the given page in terms of pmap_next_page() calls.
 */
int
pmap_page_index(pa)
	vm_offset_t pa;
{
	if (phys_mem_start < pa && pa < (phys_mem_start + phys_mem_size)) {
		pa &= ~PGOFSET;
		return (btoc(pa));
	} else {
		return (-1);
	}
}

/*
 * Create and return a physical map.
 */
struct pmap *
pmap_create(size)
	vm_size_t size;
{
	struct pmap *pm;
	
	pm = (struct pmap *)malloc(sizeof *pm, M_VMPMAP, M_WAITOK);
	bzero((caddr_t)pm, sizeof *pm);
	pmap_pinit(pm);
	return pm;
}

/*
 * Initialize a preallocated and zeroed pmap structure.
 */
void
pmap_pinit(pm)
	struct pmap *pm;
{
	int i, j;
	
	/*
	 * Allocate some segment registers for this pmap.
	 */
	pm->pm_refs = 1;
	for (i = 0; i < sizeof usedsr / sizeof usedsr[0]; i++)
		if (usedsr[i] != 0xffffffff) {
			j = ffs(~usedsr[i]) - 1;
			usedsr[i] |= 1 << j;
			pm->pm_sr[0] = (i * sizeof usedsr[0] * 8 + j) * 16;
			for (i = 1; i < 16; i++)
				pm->pm_sr[i] = pm->pm_sr[i - 1] + 1;
			return;
		}
	panic("out of segments");
}

/*
 * Add a reference to the given pmap.
 */
void
pmap_reference(pm)
	struct pmap *pm;
{
	pm->pm_refs++;
}

/*
 * Retire the given pmap from service.
 * Should only be called if the map contains no valid mappings.
 */
void
pmap_destroy(pm)
	struct pmap *pm;
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
void
pmap_release(pm)
	struct pmap *pm;
{
	int i, j;
	
	if (!pm->pm_sr[0])
		panic("pmap_release");
	i = pm->pm_sr[0] / 16;
	j = i % (sizeof usedsr[0] * 8);
	i /= sizeof usedsr[0] * 8;
	usedsr[i] &= ~(1 << j);
}

/*
 * Copy the range specified by src_addr/len
 * from the source map to the range dst_addr/len
 * in the destination map.
 *
 * This routine is only advisory and need not do anything.
 */
void
pmap_copy(dst_pmap, src_pmap, dst_addr, len, src_addr)
	struct pmap *dst_pmap, *src_pmap;
	vm_offset_t dst_addr, src_addr;
	vm_size_t len;
{
}

/*
 * Require that all active physical maps contain no
 * incorrect entries NOW.
 */
void
pmap_update()
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
pmap_collect(pm)
	struct pmap *pm;
{
}

/*
 * Make the specified pages pageable or not as requested.
 *
 * This routine is merely advisory.
 */
void
pmap_pageable(pm, start, end, pageable)
	struct pmap *pm;
	vm_offset_t start, end;
	int pageable;
{
}

/*
 * Fill the given physical page with zeroes.
 */
void
pmap_zero_page(pa)
	vm_offset_t pa;
{
	bzero((caddr_t)pa, NBPG);
}

/*
 * Copy the given physical source page to its destination.
 */
void
pmap_copy_page(src, dst)
	vm_offset_t src, dst;
{
	bcopy((caddr_t)src, (caddr_t)dst, NBPG);
}

static struct pv_entry *
pmap_alloc_pv()
{
	struct pv_page *pvp;
	struct pv_entry *pv;
	int i;
	
	if (pv_nfree == 0) {
		if (!(pvp = (struct pv_page *)kmem_alloc(kernel_map, NBPG)))
			panic("pmap_alloc_pv: kmem_alloc() failed");
		pv_pcnt++;
		pvp->pvp_pgi.pgi_freelist = pv = &pvp->pvp_pv[1];
		for (i = NPVPPG - 2; --i >= 0; pv++)
			pv->pv_next = pv + 1;
		pv->pv_next = 0;
		pv_nfree += pvp->pvp_pgi.pgi_nfree = NPVPPG - 1;
		LIST_INSERT_HEAD(&pv_page_freelist, pvp, pvp_pgi.pgi_list);
		pv = pvp->pvp_pv;
	} else {
		pv_nfree--;
		pvp = pv_page_freelist.lh_first;
		if (--pvp->pvp_pgi.pgi_nfree <= 0)
			LIST_REMOVE(pvp, pvp_pgi.pgi_list);
		pv = pvp->pvp_pgi.pgi_freelist;
		pvp->pvp_pgi.pgi_freelist = pv->pv_next;
	}
	return pv;
}

static void
pmap_free_pv(pv)
	struct pv_entry *pv;
{
	struct pv_page *pvp;
	
	pvp = (struct pv_page *)trunc_page(pv);
	switch (++pvp->pvp_pgi.pgi_nfree) {
	case 1:
		LIST_INSERT_HEAD(&pv_page_freelist, pvp, pvp_pgi.pgi_list);
	default:
		pv->pv_next = pvp->pvp_pgi.pgi_freelist;
		pvp->pvp_pgi.pgi_freelist = pv;
		pv_nfree++;
		break;
	case NPVPPG:
		pv_nfree -= NPVPPG - 1;
		pv_pcnt--;
		LIST_REMOVE(pvp, pvp_pgi.pgi_list);
		kmem_free(kernel_map, (vm_offset_t)pvp, NBPG);
		break;
	}
}

/*
 * We really hope that we don't need overflow entries
 * before the VM system is initialized!							XXX
 */
static struct pte_ovfl *
poalloc()
{
	struct po_page *pop;
	struct pte_ovfl *po;
	vm_page_t mem;
	int i;
	
	if (!pmap_initialized)
		panic("poalloc");
	
	if (po_nfree == 0) {
		/*
		 * Since we cannot use maps for potable allocation,
		 * we have to steal some memory from the VM system.			XXX
		 */
		mem = vm_page_alloc1();
		po_pcnt++;
		pop = (struct po_page *)VM_PAGE_TO_PHYS(mem);
		pop->pop_pgi.pgi_page = mem;
		LIST_INIT(&pop->pop_pgi.pgi_freelist);
		for (i = NPOPPG - 1, po = pop->pop_po + 1; --i >= 0; po++)
			LIST_INSERT_HEAD(&pop->pop_pgi.pgi_freelist, po, po_list);
		po_nfree += pop->pop_pgi.pgi_nfree = NPOPPG - 1;
		LIST_INSERT_HEAD(&po_page_freelist, pop, pop_pgi.pgi_list);
		po = pop->pop_po;
	} else {
		po_nfree--;
		pop = po_page_freelist.lh_first;
		if (--pop->pop_pgi.pgi_nfree <= 0)
			LIST_REMOVE(pop, pop_pgi.pgi_list);
		po = pop->pop_pgi.pgi_freelist.lh_first;
		LIST_REMOVE(po, po_list);
	}
	return po;
}

static void
pofree(po, freepage)
	struct pte_ovfl *po;
	int freepage;
{
	struct po_page *pop;
	
	pop = (struct po_page *)trunc_page(po);
	switch (++pop->pop_pgi.pgi_nfree) {
	case NPOPPG:
		if (!freepage)
			break;
		po_nfree -= NPOPPG - 1;
		po_pcnt--;
		LIST_REMOVE(pop, pop_pgi.pgi_list);
		vm_page_free1(pop->pop_pgi.pgi_page);
		return;
	case 1:
		LIST_INSERT_HEAD(&po_page_freelist, pop, pop_pgi.pgi_list);
	default:
		break;
	}
	LIST_INSERT_HEAD(&pop->pop_pgi.pgi_freelist, po, po_list);
	po_nfree++;
}

/*
 * This returns whether this is the first mapping of a page.
 */
static inline int
pmap_enter_pv(pteidx, va, pind)
	int pteidx;
	vm_offset_t va;
	u_int pind;
{
	struct pv_entry *pv, *npv;
	int s, first;
	
	if (!pmap_initialized)
		return 0;

	s = splimp();

	pv = &pv_table[pind];
	if (first = pv->pv_idx == -1) {
		/*
		 * No entries yet, use header as the first entry.
		 */
		pv->pv_va = va;
		pv->pv_idx = pteidx;
		pv->pv_next = NULL;
	} else {
		/*
		 * There is at least one other VA mapping this page.
		 * Place this entry after the header.
		 */
		npv = pmap_alloc_pv();
		npv->pv_va = va;
		npv->pv_idx = pteidx;
		npv->pv_next = pv->pv_next;
		pv->pv_next = npv;
	}
	splx(s);
	return first;
}

static void
pmap_remove_pv(pteidx, va, pind, pte)
	int pteidx;
	vm_offset_t va;
	int pind;
	struct pte *pte;
{
	struct pv_entry *pv, *npv;

	if (pind < 0)
		return;

	/*
	 * First transfer reference/change bits to cache.
	 */
	pmap_attrib[pind] |= (pte->pte_lo & (PTE_REF | PTE_CHG)) >> ATTRSHFT;
	
	/*
	 * Remove from the PV table.
	 */
	pv = &pv_table[pind];
	
	/*
	 * If it is the first entry on the list, it is actually
	 * in the header and we must copy the following entry up
	 * to the header.  Otherwise we must search the list for
	 * the entry.  In either case we free the now unused entry.
	 */
	if (pteidx == pv->pv_idx && va == pv->pv_va) {
		npv = pv->pv_next;
		if (npv) {
			*pv = *npv;
			pmap_free_pv(npv);
		} else
			pv->pv_idx = -1;
	} else {
		for (; npv = pv->pv_next; pv = npv)
			if (pteidx == npv->pv_idx && va == npv->pv_va)
				break;
		if (npv) {
			pv->pv_next = npv->pv_next;
			pmap_free_pv(npv);
		}
#ifdef	DIAGNOSTIC
		else
			panic("pmap_remove_pv: not on list\n");
#endif
	}
}

/*
 * Insert physical page at pa into the given pmap at virtual address va.
 */
void
pmap_enter(pm, va, pa, prot, wired)
	struct pmap *pm;
	vm_offset_t va, pa;
	vm_prot_t prot;
	int wired;
{
	sr_t sr;
	int idx, i, s;
	pte_t pte;
	struct pte_ovfl *po;

	/*
	 * Have to remove any existing mapping first.
	 */
	pmap_remove(pm, va, va + NBPG - 1);
	
	/*
	 * Compute the HTAB index.
	 */
	idx = pteidx(sr = ptesr(pm->pm_sr, va), va);
	/*
	 * Construct the PTE.
	 *
	 * Note: Don't set the valid bit for correct operation of tlb update.
	 */
	pte.pte_hi = ((sr & SR_VSID) << PTE_VSID_SHFT)
		| ((va & ADDR_PIDX) >> ADDR_API_SHFT);
	pte.pte_lo = (pa & PTE_RPGN) | PTE_M | PTE_I | PTE_G;

	if (pa >= phys_mem_start && pa < phys_mem_start + phys_mem_size) {
		pte.pte_lo &= ~(PTE_I | PTE_G);
	}

	if (prot & VM_PROT_WRITE)
		pte.pte_lo |= PTE_RW;
	else
		pte.pte_lo |= PTE_RO;
	
	/*
	 * Now record mapping for later back-translation.
	 */
	if (pmap_initialized && (i = pmap_page_index(pa)) != -1)
		if (pmap_enter_pv(idx, va, i)) {
			/* 
			 * Flush the real memory from the cache.
			 */
			syncicache((void *)pa, NBPG);
		}
	
	s = splimp();
	/*
	 * Try to insert directly into HTAB.
	 */
	if (pte_insert(idx, &pte)) {
		splx(s);
		return;
	}
	/*
	 * Have to allocate overflow entry.
	 *
	 * Note, that we must use real addresses for these.
	 */
	po = poalloc();
	po->po_pte = pte;
	LIST_INSERT_HEAD(potable + idx, po, po_list);
	splx(s);
}

/*
 * Remove the given range of mapping entries.
 */
void
pmap_remove(pm, va, endva)
	struct pmap *pm;
	vm_offset_t va, endva;
{
	int idx, i, s;
	sr_t sr;
	pte_t *ptp;
	struct pte_ovfl *po, *npo;
	
	s = splimp();
	while (va < endva) {
		idx = pteidx(sr = ptesr(pm->pm_sr, va), va);
		for (ptp = ptable + idx * 8, i = 8; --i >= 0; ptp++)
			if (ptematch(ptp, sr, va, PTE_VALID)) {
				pmap_remove_pv(idx, va, pmap_page_index(ptp->pte_lo), ptp);
				ptp->pte_hi &= ~PTE_VALID;
				asm volatile ("sync");
				tlbie(va);
				tlbsync();
			}
		for (ptp = ptable + (idx ^ ptab_mask) * 8, i = 8; --i >= 0; ptp++)
			if (ptematch(ptp, sr, va, PTE_VALID | PTE_HID)) {
				pmap_remove_pv(idx, va, pmap_page_index(ptp->pte_lo), ptp);
				ptp->pte_hi &= ~PTE_VALID;
				asm volatile ("sync");
				tlbie(va);
				tlbsync();
			}
		for (po = potable[idx].lh_first; po; po = npo) {
			npo = po->po_list.le_next;
			if (ptematch(&po->po_pte, sr, va, 0)) {
				pmap_remove_pv(idx, va, pmap_page_index(po->po_pte.pte_lo),
					       &po->po_pte);
				LIST_REMOVE(po, po_list);
				pofree(po, 1);
			}
		}
		va += NBPG;
	}
	splx(s);
}

static pte_t *
pte_find(pm, va)
	struct pmap *pm;
	vm_offset_t va;
{
	int idx, i;
	sr_t sr;
	pte_t *ptp;
	struct pte_ovfl *po;

	idx = pteidx(sr = ptesr(pm->pm_sr, va), va);
	for (ptp = ptable + idx * 8, i = 8; --i >= 0; ptp++)
		if (ptematch(ptp, sr, va, PTE_VALID))
			return ptp;
	for (ptp = ptable + (idx ^ ptab_mask) * 8, i = 8; --i >= 0; ptp++)
		if (ptematch(ptp, sr, va, PTE_VALID | PTE_HID))
			return ptp;
	for (po = potable[idx].lh_first; po; po = po->po_list.le_next)
		if (ptematch(&po->po_pte, sr, va, 0))
			return &po->po_pte;
	return 0;
}

/*
 * Get the physical page address for the given pmap/virtual address.
 */
vm_offset_t
pmap_extract(pm, va)
	struct pmap *pm;
	vm_offset_t va;
{
	pte_t *ptp;
	vm_offset_t o;
	int s = splimp();
	
	if (!(ptp = pte_find(pm, va))) {
		splx(s);
		return 0;
	}
	o = (ptp->pte_lo & PTE_RPGN) | (va & ADDR_POFF);
	splx(s);
	return o;
}

/*
 * Lower the protection on the specified range of this pmap.
 *
 * There are only two cases: either the protection is going to 0,
 * or it is going to read-only.
 */
void
pmap_protect(pm, sva, eva, prot)
	struct pmap *pm;
	vm_offset_t sva, eva;
	vm_prot_t prot;
{
	pte_t *ptp;
	int valid, s;
	
	if (prot & VM_PROT_READ) {
		s = splimp();
		while (sva < eva) {
			if (ptp = pte_find(pm, sva)) {
				valid = ptp->pte_hi & PTE_VALID;
				ptp->pte_hi &= ~PTE_VALID;
				asm volatile ("sync");
				tlbie(sva);
				tlbsync();
				ptp->pte_lo &= ~PTE_PP;
				ptp->pte_lo |= PTE_RO;
				asm volatile ("sync");
				ptp->pte_hi |= valid;
			}
			sva += NBPG;
		}
		splx(s);
		return;
	}
	pmap_remove(pm, sva, eva);
}

void
ptemodify(pa, mask, val)
	vm_offset_t pa;
	u_int mask;
	u_int val;
{
	struct pv_entry *pv;
	pte_t *ptp;
	struct pte_ovfl *po;
	int i, s;
	
	i = pmap_page_index(pa);
	if (i < 0)
		return;

	/*
	 * First modify bits in cache.
	 */
	pmap_attrib[i] &= ~mask >> ATTRSHFT;
	pmap_attrib[i] |= val >> ATTRSHFT;
	
	pv = pv_table + i;
	if (pv->pv_idx < 0)
		return;

	s = splimp();
	for (; pv; pv = pv->pv_next) {
		for (ptp = ptable + pv->pv_idx * 8, i = 8; --i >= 0; ptp++)
			if ((ptp->pte_hi & PTE_VALID)
			    && (ptp->pte_lo & PTE_RPGN) == pa) {
				ptp->pte_hi &= ~PTE_VALID;
				asm volatile ("sync");
				tlbie(pv->pv_va);
				tlbsync();
				ptp->pte_lo &= ~mask;
				ptp->pte_lo |= val;
				asm volatile ("sync");
				ptp->pte_hi |= PTE_VALID;
			}
		for (ptp = ptable + (pv->pv_idx ^ ptab_mask) * 8, i = 8; --i >= 0; ptp++)
			if ((ptp->pte_hi & PTE_VALID)
			    && (ptp->pte_lo & PTE_RPGN) == pa) {
				ptp->pte_hi &= ~PTE_VALID;
				asm volatile ("sync");
				tlbie(pv->pv_va);
				tlbsync();
				ptp->pte_lo &= ~mask;
				ptp->pte_lo |= val;
				asm volatile ("sync");
				ptp->pte_hi |= PTE_VALID;
			}
		for (po = potable[pv->pv_idx].lh_first; po; po = po->po_list.le_next)
			if ((po->po_pte.pte_lo & PTE_RPGN) == pa) {
				po->po_pte.pte_lo &= ~mask;
				po->po_pte.pte_lo |= val;
			}
	}
	splx(s);
}

int
ptebits(pa, bit)
	vm_offset_t pa;
	int bit;
{
	struct pv_entry *pv;
	pte_t *ptp;
	struct pte_ovfl *po;
	int i, s, bits = 0;

	i = pmap_page_index(pa);
	if (i < 0)
		return 0;

	/*
	 * First try the cache.
	 */
	bits |= (pmap_attrib[i] << ATTRSHFT) & bit;
	if (bits == bit)
		return bits;

	pv = pv_table + i;
	if (pv->pv_idx < 0)
		return 0;
	
	s = splimp();
	for (; pv; pv = pv->pv_next) {
		for (ptp = ptable + pv->pv_idx * 8, i = 8; --i >= 0; ptp++)
			if ((ptp->pte_hi & PTE_VALID)
			    && (ptp->pte_lo & PTE_RPGN) == pa) {
				bits |= ptp->pte_lo & bit;
				if (bits == bit) {
					splx(s);
					return bits;
				}
			}
		for (ptp = ptable + (pv->pv_idx ^ ptab_mask) * 8, i = 8; --i >= 0; ptp++)
			if ((ptp->pte_hi & PTE_VALID)
			    && (ptp->pte_lo & PTE_RPGN) == pa) {
				bits |= ptp->pte_lo & bit;
				if (bits == bit) {
					splx(s);
					return bits;
				}
			}
		for (po = potable[pv->pv_idx].lh_first; po; po = po->po_list.le_next)
			if ((po->po_pte.pte_lo & PTE_RPGN) == pa) {
				bits |= po->po_pte.pte_lo & bit;
				if (bits == bit) {
					splx(s);
					return bits;
				}
			}
	}
	splx(s);
	return bits;
}

/*
 * Lower the protection on the specified physical page.
 *
 * There are only two cases: either the protection is going to 0,
 * or it is going to read-only.
 */
void
pmap_page_protect(pa, prot)
	vm_offset_t pa;
	vm_prot_t prot;
{
	vm_offset_t va;
	pte_t *ptp;
	struct pte_ovfl *po, *npo;
	int i, s, pind, idx;
	
	pa &= ~ADDR_POFF;
	if (prot & VM_PROT_READ) {
		ptemodify(pa, PTE_PP, PTE_RO);
		return;
	}

	pind = pmap_page_index(pa);
	if (pind < 0)
		return;

	s = splimp();
	while (pv_table[pind].pv_idx >= 0) {
		idx = pv_table[pind].pv_idx;
		va = pv_table[pind].pv_va;
		for (ptp = ptable + idx * 8, i = 8; --i >= 0; ptp++)
			if ((ptp->pte_hi & PTE_VALID)
			    && (ptp->pte_lo & PTE_RPGN) == pa) {
				pmap_remove_pv(idx, va, pind, ptp);
				ptp->pte_hi &= ~PTE_VALID;
				asm volatile ("sync");
				tlbie(va);
				tlbsync();
			}
		for (ptp = ptable + (idx ^ ptab_mask) * 8, i = 8; --i >= 0; ptp++)
			if ((ptp->pte_hi & PTE_VALID)
			    && (ptp->pte_lo & PTE_RPGN) == pa) {
				pmap_remove_pv(idx, va, pind, ptp);
				ptp->pte_hi &= ~PTE_VALID;
				asm volatile ("sync");
				tlbie(va);
				tlbsync();
			}
		for (po = potable[idx].lh_first; po; po = npo) {
			npo = po->po_list.le_next;
			if ((po->po_pte.pte_lo & PTE_RPGN) == pa) {
				pmap_remove_pv(idx, va, pind, &po->po_pte);
				LIST_REMOVE(po, po_list);
				pofree(po, 1);
			}
		}
	}
	splx(s);
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
