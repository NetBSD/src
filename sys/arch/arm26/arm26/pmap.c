/* $NetBSD: pmap.c,v 1.4 2000/06/29 08:32:35 mrg Exp $ */
/*-
 * Copyright (c) 1997, 1998, 2000 Ben Harris
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
/* This file is part of NetBSD/arm26 -- a port of NetBSD to ARM2/3 machines. */

/*
 * pmap.c - Interfacing the MEMC to the BSD VM system.
 */

/*
 * The MEMC has variable page sizes, and moreover it's necessary to
 * change the page size depending on the amount of DRAM under the
 * MEMC's control.  This code currently only handles 32k pages.
 *
 * The bottom 32k of logically-mapped RAM is zero page, and is mapped
 * in all address spaces (though it's not accessible in user mode).
 * This contains interrupt vectors, and probably context-switching
 * code.  It doesn't appear in any pmap as it's never unmapped.
 * 
 * Wired pages are an interesting proposition.  We aren't allowed to
 * let page faults on them reach the MI fault handler, and in general
 * we shouldn't let them get dropped from the map in the MEMC at all.
 * This is tricky when we have kernel and user space shared, as we
 * have to ensure that no wired page in the kernel is mapped by any
 * user process or vice versa.
 */

/*
 * On an ARM3, the cache must be flushed whenever:
 * 
 * 1 - We remove read access to a page, since we need to ensure reads
 *     will abort.  Write access doesn't matter since the cache is
 *     write-through.
 *
 * 2 - We write to a page and need to be able to read it elsewhere.
 *     Since the MEMC can only map each page once, this only occurs
 *     without case one when one of the accesses is physical and the other
 *     logical, which I think only happens when doing odd things with
 *     /dev/mem.  In all other cases, a physical page should only be
 *     accessed through one of its mappings (virtual if it has one,
 *     physical if it doesn't).  pmap_find is useful for this.
 */

/*
 * We assume the following rules apply:
 *
 * Accesses to memory managed by user pmaps will always be from USR
 * mode, or from LDR/STR with the T bit set (which looks the same to
 * the MEMC).
 *
 * Accesses to memory managed by the kernel pmap will always be from
 * SVC mode.
 *
 * Breaking these rules (especially the first) is likely to upset
 * referenced/modified emulation.
 */

#include "opt_cputypes.h"
#include "opt_ddb.h"
#include "arcvideo.h"

#include <sys/param.h>

__KERNEL_RCSID(0, "$NetBSD: pmap.c,v 1.4 2000/06/29 08:32:35 mrg Exp $");

#include <sys/kernel.h> /* for cold */
#include <sys/malloc.h>
#define __POOL_EXPOSE /* We need to allocate one statically. */
#include <sys/pool.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <machine/machdep.h>
#include <machine/memcreg.h>

#include <arch/arm26/arm26/cpuvar.h>

/*
 * We store all the mapping information in the pv_table, and the pmaps
 * just contain references to it.  This is suggested in the Daemon
 * Book as a sensible thing to do on systems without real page tables.
 */
struct pv_entry {
	struct	pv_entry *pv_next;
	pmap_t	pv_pmap;
	int	pv_ppn;
	int	pv_lpn;
	u_int32_t	pv_activate;  /* MEMC command to activate mapping */
	u_int32_t	pv_deactivate;/* MEMC command to deactivate mapping */
	vm_prot_t	pv_prot; /* requested protection */
	u_int8_t	pv_ppl;  /* Actual PPL */
	u_int8_t	pv_vflags; /* Per-mapping flags */
#define PV_WIRED	0x01 /* This is a wired mapping */
	u_int8_t	pv_pflags; /* Per-physical-page flags */
#define PV_REFERENCED	0x01
#define PV_MODIFIED	0x02
};

struct pmap {
	int	pm_count;	/* Reference count */
	int	pm_flags;
#define PM_ACTIVE 0x00000001
	struct	pv_entry *pm_entries[1024];
};

/*
 * All physical pages must map to _some_ logical page, even though
 * they can have all access disabled.  This means we need a logical
 * page reseved for this purpose.  We use the last page of kernel VM.
 */
#define PMAP_DEAD_PAGE (atop(VM_MAX_KERNEL_ADDRESS) - 1)
#define PMAP_UNMAP_ENTRY_32K(ppn) \
	MEMC_TRANS_ENTRY_32K(ppn, PMAP_DEAD_PAGE, MEMC_PPL_NOACCESS)
#if 0
{} /* shut up emacs */
#endif

/*
 * Global array of pv_entries.  Should be dynamically allocated based on
 * number of physical pages in system, or something.
 */
struct pv_entry pv_table[1024];

/* Kernel pmap -- statically allocated to make life slightly less odd. */

struct pmap kernel_pmap_store;

static boolean_t pmap_initialised = FALSE;

static struct pool pmap_pool_store;
static struct pool *pmap_pool = &pmap_pool_store;

static pmap_t active_pmap;

static void pv_update(struct pv_entry *);
static struct pv_entry *pv_alloc(void);
static void pv_free(struct pv_entry *pv);
static struct pv_entry *pv_get(pmap_t pmap, int ppn, int lpn);
static void pv_release(pmap_t pmap, int ppn, int lpn);

static caddr_t pmap_find(paddr_t);

static void pmap_update_page(int);

/*
 * No-one else wanted to take responsibility for the MEMC control register,
 * so it's ended up here.
 */

static register_t memc_control = 0;

register_t
update_memc(register_t mask, register_t value)
{
	register_t old;
	int s;

	s = splhigh();
	old = memc_control;
	memc_control &= ~mask;
	memc_control |= value & mask;
	MEMC_WRITE(memc_control);
	splx(s);
	return old;
}

/*
 * pmap_bootstrap: first-stage pmap initialisation
 * 
 * This is called very early, and has to get the pmap system into a
 * state where pmap_virtual_space and pmap_kenter_pa at least will
 * work.  If we need memory here, we have to work out how to get it
 * ourselves.
 */
void
pmap_bootstrap(int npages, paddr_t zp_physaddr)
{
	int i;
	struct pmap *pmap;

	/* Set up the kernel's pmap */
	pmap = pmap_kernel();
	bzero(pmap, sizeof(*pmap));
	pmap->pm_count = 1;
	pmap->pm_flags = PM_ACTIVE; /* Kernel pmap always is */
	/* pmap_pinit(pmap); */
	/* Clear the MEMC's page table */
	/* XXX Maybe we should leave zero page alone? */
	for (i=0; i < npages; i++)
		MEMC_WRITE(PMAP_UNMAP_ENTRY_32K(i));
	bzero(pv_table, sizeof(pv_table));
	/* TODO: Set up control register? */
	update_memc(0xffffffff, MEMC_CONTROL | MEMC_CTL_PGSZ_32K |
		    MEMC_CTL_LROMSPD_4N | MEMC_CTL_HROMSPD_4N |
#if NARCVIDEO > 0
		    MEMC_CTL_RFRSH_FLYBACK | MEMC_CTL_VIDEODMA
#else
		    MEMC_CTL_RFRSH_CONTIN
#endif
		    );
	/* Manually map zero page (it doesn't appear in pmaps) */
	MEMC_WRITE(MEMC_TRANS_ENTRY_32K(atop(zp_physaddr), 0,
					MEMC_PPL_NOACCESS));
}

/*
 * pmap_init: second stage pmap initialisation.
 *
 * Sets up everything that will be needed for normal running.  We can
 * allocate memory in page-rounded chunks with uvm_km_(z)alloc(), or
 * use the pool allocator.  malloc is still taboo.
 */
void
pmap_init()
{

	pool_init(pmap_pool, sizeof(struct pmap), 0, 0, 0, "pmappool",
		  0, NULL, NULL, M_VMPMAP);
	pmap_initialised = 1;
}

struct pmap *
pmap_create()
{
	struct pmap *pmap;

	pmap = pool_get(pmap_pool, PR_WAITOK);
	bzero(pmap, sizeof(*pmap));
	return pmap;
}

void
pmap_destroy(pmap_t pmap)
{
#ifdef DIAGNOSTIC
	int i;
#endif

	if (--pmap->pm_count > 0)
		return;
#ifdef DIAGNOSTIC
	if (pmap->pm_flags & PM_ACTIVE)
		panic("pmap_destroy: pmap is active");
	for (i = 0; i < 1024; i++)
		if (pmap->pm_entries[i] != NULL)
			panic("pmap_destroy: pmap isn't empty");
#endif
	pool_put(pmap_pool, pmap);
}

void
pmap_activate(struct proc *p)
{
	pmap_t pmap;
	int i;

	pmap = p->p_vmspace->vm_map.pmap;

	if (pmap == pmap_kernel())
		return; /* kernel pmap is always active */

#ifdef DIAGNOSTIC
	if (active_pmap != NULL || (pmap->pm_flags & PM_ACTIVE) != 0)
		panic("pmap_activate");
#endif

	active_pmap = pmap;
	pmap->pm_flags |= PM_ACTIVE;
	for (i = 0; i < 1024; i++)
		if (pmap->pm_entries[i] != NULL)
			MEMC_WRITE(pmap->pm_entries[i]->pv_activate);
}

void
pmap_deactivate(struct proc *p)
{
	pmap_t pmap;
	int i;

	pmap = p->p_vmspace->vm_map.pmap;

	if (pmap == pmap_kernel())
		return; /* kernel pmap is always active */

#ifdef DIAGNOSTIC
	if (active_pmap != pmap || (pmap->pm_flags & PM_ACTIVE) == 0)
		panic("pmap_deactivate");
#endif

	active_pmap = NULL;
	pmap->pm_flags &=~ PM_ACTIVE;
	for (i = 0; i < 1024; i++)
		if (pmap->pm_entries[i] != NULL)
			MEMC_WRITE(pmap->pm_entries[i]->pv_deactivate);
	cpu_cache_flush();
}

void
pmap_unwire(pmap_t pmap, vaddr_t va)
{
	struct pv_entry *pv;

	if (pmap == NULL) return;
	pv = pmap->pm_entries[atop(va)];
	if (pv == NULL) return;
	pv->pv_vflags &= ~PV_WIRED;
}

void
pmap_collect(pmap)
	pmap_t pmap;
{
	/* This is allowed to be a no-op. */
}

void
pmap_copy(dst_pmap, src_pmap, dst_addr, len, src_addr)
	pmap_t dst_pmap, src_pmap;
	vaddr_t dst_addr, src_addr;
	vsize_t len;
{
	/* This is allowed to be a no-op. */
}

/*
 * Update the pv_activate and pv_deactivate fields in a pv_entry to
 * match the rest.
 */
static void
pv_update(struct pv_entry *pv)
{
	int ppl;
	int pflags;

	pflags = pv_table[pv->pv_ppn].pv_pflags;
	if (pv->pv_pmap == pmap_kernel()) {
		if (1) {
			/* Don't risk faults till everything's happy */
			ppl = MEMC_PPL_NOACCESS;
			pv_table[pv->pv_ppn].pv_pflags |=
			    PV_REFERENCED | PV_MODIFIED;
		} else 	if ((pflags & PV_REFERENCED) && (pflags & PV_MODIFIED))
			ppl = MEMC_PPL_NOACCESS;
		else {
			/*
			 * To keep SVC mode from seeing a page, we
			 * have to unmap it entirely.
			 */
			pv->pv_ppl = MEMC_PPL_NOACCESS;
			pv->pv_activate = pv->pv_deactivate =
			    PMAP_UNMAP_ENTRY_32K(pv->pv_ppn);
			return;
		}
	} else {
		if ((pv->pv_prot & VM_PROT_WRITE) &&
		    (pflags & PV_REFERENCED) && (pflags & PV_MODIFIED))
			ppl = MEMC_PPL_RDWR;
		else if ((pv->pv_prot & (VM_PROT_READ | VM_PROT_EXECUTE)) &&
			 (pflags & PV_REFERENCED))
		        ppl = MEMC_PPL_RDONLY;
		else
			ppl = MEMC_PPL_NOACCESS;
	}
	pv->pv_ppl = ppl;
	pv->pv_activate = MEMC_TRANS_ENTRY_32K(pv->pv_ppn, pv->pv_lpn, ppl);
	pv->pv_deactivate = PMAP_UNMAP_ENTRY_32K(pv->pv_ppn);
}


static struct pv_entry *
pv_alloc()
{
	struct pv_entry *pv;

	MALLOC(pv, struct pv_entry *, sizeof(*pv), M_VMPMAP, M_WAITOK);
	bzero(pv, sizeof(*pv));
	return pv;
}

static void
pv_free(struct pv_entry *pv)
{

	FREE(pv, M_VMPMAP);
}

static struct pv_entry *
pv_get(pmap_t pmap, int ppn, int lpn)
{
	struct pv_entry *pv;

	/* If the head entry's free use that. */
	pv = &pv_table[ppn];
	if (pv->pv_pmap == NULL)
		return pv;
	/* If this mapping exists already, use that. */
	for (pv = pv; pv != NULL; pv = pv->pv_next)
		if (pv->pv_pmap == pmap && pv->pv_lpn == lpn)
			return pv;
	/* Otherwise, allocate a new entry and link it in after the head. */
	pv = pv_alloc();
	pv->pv_next = pv_table[ppn].pv_next;
	pv_table[ppn].pv_next = pv;
	return pv;
}

/*
 * Release the pv_entry for a mapping.  Code derived from hp300 pmap.
 */
static void
pv_release(pmap_t pmap, int ppn, int lpn)
{
	struct pv_entry *pv, *npv;

	pv = &pv_table[ppn];
	/*
	 * If it is the first entry on the list, it is actually
	 * in the header and we must copy the following entry up
	 * to the header.  Otherwise we must search the list for
	 * the entry.  In either case we free the now unused entry.
	 */
	if (pmap == pv->pv_pmap && lpn == pv->pv_lpn) {
		npv = pv->pv_next;
		if (npv) {
			/* Pull up first entry from chain. */
			npv->pv_pflags = pv->pv_pflags;
			*pv = *npv;
			pv->pv_pmap->pm_entries[lpn] = pv;
			pv_free(npv);
		} else
			pv->pv_pmap = NULL;
	} else {
		for (npv = pv->pv_next; npv; npv = npv->pv_next) {
			if (pmap == npv->pv_pmap && lpn == npv->pv_lpn)
				break;
			pv = npv;
		}
#ifdef DIAGNOSTIC
		if (npv == NULL)
			panic("pv_release");
#endif
		pv->pv_next = npv->pv_next;
		pv_free(npv);
	}
	pmap->pm_entries[lpn] = NULL;
}


/* 
 * Insert the given physical page (pa) at the specified virtual
 * address (va) in the target physical map with the protection
 * requested.
 *
 * NB: This is the only routine which MAY NOT lazy-evaluate or lose
 * information.  That is, this routine must actually insert this page
 * into the given map NOW.
 */
int
pmap_enter(pmap_t pmap, vaddr_t va, paddr_t pa, vm_prot_t prot, int flags)
{
	int ppn, lpn, s;
	struct pv_entry *pv;

	ppn = atop(pa); lpn = atop(va);

#if 0
	printf("pmap_enter: mapping ppn %d at lpn %d in pmap %p\n",
	       ppn, lpn, pmap);
#endif
	s = splimp();
	/* Check if the physical page is mapped already elsewhere. */
	if (pmap == pmap_kernel()) {
		for (pv = &pv_table[ppn]; pv != NULL; pv = pv->pv_next)
			if (pv->pv_pmap != NULL &&
			    (pv->pv_pmap != pmap || pv->pv_lpn != lpn)) {
				if (pv->pv_vflags & PV_WIRED) {
					printf("new: pmap = %p, ppn = %d, "
					       "lpn = %d\n", pmap, ppn, lpn);
					printf("old: pmap = %p, ppn = %d, "
					       "lpn = %d\n", pv->pv_pmap,
					       pv->pv_ppn, pv->pv_lpn);
					panic("Mapping clash not handled");
				} else {
					pmap_remove(pv->pv_pmap,
						    pv->pv_lpn * PAGE_SIZE,
						    (pv->pv_lpn+1) *
						    PAGE_SIZE);
				}
			}
	} else {
		for (pv = &pv_table[ppn]; pv != NULL; pv = pv->pv_next)
			if (pv->pv_pmap == pmap_kernel() ||
			    (pv->pv_pmap == pmap && pv->pv_lpn != lpn)) {
				if (pv->pv_vflags & PV_WIRED) {
					printf("new: pmap = %p, ppn = %d, "
					       "lpn = %d\n", pmap, ppn, lpn);
					printf("old: pmap = %p, ppn = %d, "
					       "lpn = %d\n", pv->pv_pmap,
					       pv->pv_ppn, pv->pv_lpn);
					panic("Mapping clash not handled");
				} else {
					pmap_remove(pv->pv_pmap,
						    pv->pv_lpn * PAGE_SIZE,
						    (pv->pv_lpn+1) *
						    PAGE_SIZE);
				}
			}
	}

	/* Remove any existing mapping at this lpn */
	if (pmap->pm_entries[lpn] != NULL &&
	    pmap->pm_entries[lpn]->pv_ppn != ppn)
		pmap_remove(pmap, va, va + PAGE_SIZE);

	/* Make a note */
	pv = pv_get(pmap, ppn, lpn);
	pv->pv_pmap = pmap;
	pv->pv_ppn = ppn;
	pv->pv_lpn = lpn;
	pv->pv_prot = prot;
	pv->pv_vflags = 0;
	pv->pv_pflags = 0;
	if (flags & PMAP_WIRED)
		pv->pv_vflags |= PV_WIRED;
	pv_update(pv);
	pmap->pm_entries[lpn] = pv;
	splx(s);
	/* Poke the MEMC */
	if (pmap->pm_flags & PM_ACTIVE)
		MEMC_WRITE(pv->pv_activate);

	return KERN_SUCCESS;
}

/* Remove a range of virtual mappings from a pmap */
void
pmap_remove(pmap_t pmap, vaddr_t sva, vaddr_t eva)
{
	int slpn, elpn, lpn, s;
	struct pv_entry *pv;

	slpn = atop(sva); elpn = atop(eva);
#if 0
	printf("pmap_remove: clearing from lpn %d to lpn %d in pmap %p\n",
	       slpn, elpn - 1, pmap);
#endif
	s = splimp();
	for (lpn = slpn; lpn < elpn; lpn++) {
		pv = pmap->pm_entries[lpn];
		if (pv != NULL) {
			if (pmap->pm_flags & PM_ACTIVE) {
				MEMC_WRITE(pv->pv_deactivate);
				cpu_cache_flush();
			}
			pmap->pm_entries[lpn] = NULL;
			pv_release(pmap, pv->pv_ppn, lpn);
		}
	}
	splx(s);
}

boolean_t
pmap_extract(pmap_t pmap, vaddr_t va, paddr_t *ppa)
{
	struct pv_entry *pv;

	pv = pmap->pm_entries[atop(va)];
	if (pv == NULL)
		return FALSE;
	*ppa = ptoa(pv->pv_ppn);
	return TRUE;
}

void
pmap_kenter_pa(vaddr_t va, paddr_t pa, vm_prot_t prot)
{

	pmap_enter(pmap_kernel(), va, pa, prot, PMAP_WIRED);
}

void
pmap_kenter_pgs(vaddr_t va, struct vm_page **pages, int npages)
{

	while (npages > 0) {
		pmap_kenter_pa(va, (*pages)->phys_addr, VM_PROT_ALL);
		va += NBPG;
		pages++;
		npages--;
	}
}

void
pmap_kremove(vaddr_t va, vsize_t len)
{

	pmap_remove(pmap_kernel(), va, va+len);
}

boolean_t
pmap_is_modified(page)
	struct vm_page *page;
{
	int ppn;

	ppn = atop(page->phys_addr);
	return (pv_table[ppn].pv_pflags & PV_MODIFIED) != 0;
}

boolean_t
pmap_is_referenced(page)
	struct vm_page *page;
{
	int ppn;

	ppn = atop(page->phys_addr);
	return (pv_table[ppn].pv_pflags & PV_REFERENCED) != 0;
}

static void
pmap_update_page(int ppn)
{
	struct pv_entry *pv;

	for (pv = &pv_table[ppn]; pv != NULL; pv = pv->pv_next)
		if (pv->pv_pmap != NULL) {
			pv_update(pv);
			if (pv->pv_pmap->pm_flags & PM_ACTIVE) {
				if (pv->pv_activate)
					MEMC_WRITE(pv->pv_activate);
				else
					MEMC_WRITE(pv->pv_deactivate);
			}
		}
}

boolean_t
pmap_clear_modify(struct vm_page *page)
{
	int ppn;
	boolean_t rv;

	ppn = atop(page->phys_addr);
	rv = pmap_is_modified(page);
	pv_table[ppn].pv_pflags &= ~PV_MODIFIED;
	pmap_update_page(ppn);
	return rv;
}

boolean_t
pmap_clear_reference(struct vm_page *page)
{
	int ppn;
	boolean_t rv;

	ppn = atop(page->phys_addr);
	rv = pmap_is_referenced(page);
	pv_table[ppn].pv_pflags &= ~PV_REFERENCED;
	pmap_update_page(ppn);
	return rv;
}

/*
 * Work out if a given page fault was our fault (e.g. through
 * referenced/modified emulation).  If it was, handle it and return
 * TRUE.  Otherwise, return FALSE.
 */
boolean_t
pmap_fault(struct pmap *pmap, vaddr_t va, vm_prot_t atype)
{
	int lpn, ppn;
	struct pv_entry *pv, *ppv;

	lpn = atop(va);
	pv = pmap->pm_entries[lpn];
	if (pv == NULL)
		return FALSE;
	ppn = pv->pv_ppn;
	ppv = &pv_table[ppn];
	if (pmap == pmap_kernel()) {
		/*
		 * If we allow the kernel to access the page, we can't
		 * stop it writing to it as well, so we have to handle
		 * referenced and modified bits together.
		 */
		if ((ppv->pv_pflags & PV_REFERENCED) == 0 ||
		    (ppv->pv_pflags & PV_MODIFIED) == 0) {
			ppv->pv_pflags |= PV_REFERENCED | PV_MODIFIED;
			pmap_update_page(ppn);
			return TRUE;
		}
	} else {
		if ((ppv->pv_pflags & PV_REFERENCED) == 0) {
			ppv->pv_pflags |= PV_REFERENCED;
			pmap_update_page(ppn);
			return TRUE;
		}
		if ((atype & VM_PROT_WRITE) && (pv->pv_prot & VM_PROT_WRITE) &&
		    (ppv->pv_pflags & PV_MODIFIED) == 0) {
			ppv->pv_pflags |= PV_MODIFIED;
			pmap_update_page(ppn);
			return TRUE;
		}
	}
	return FALSE;
}

void
pmap_page_protect(struct vm_page *page, vm_prot_t prot)
{
	int ppn;
	struct pv_entry *pv;

	ppn = atop(page->phys_addr);
	if (prot == VM_PROT_NONE) {
#if 0
		printf("pmap_page_protect: removing ppn %d\n", ppn);
#endif
		pv = &pv_table[ppn];
		while (pv->pv_pmap != NULL) {
			if (pv->pv_pmap->pm_flags & PM_ACTIVE) {
				MEMC_WRITE(pv->pv_deactivate);
				cpu_cache_flush();
			}
			pv->pv_pmap->pm_entries[pv->pv_lpn] = NULL;
			pv_release(pv->pv_pmap, pv->pv_ppn, pv->pv_lpn);
		}
	} else if (prot != VM_PROT_ALL) {
		for (pv = &pv_table[ppn]; pv != NULL; pv = pv->pv_next)
			if (pv->pv_pmap != NULL) {
				pv->pv_prot &= prot;
				pv_update(pv);
				if (pv->pv_pmap->pm_flags & PM_ACTIVE)
					MEMC_WRITE(pv->pv_activate);
			}
	}
}

paddr_t
pmap_phys_address(ppn)
	int ppn;
{
	panic("pmap_phys_address not implemented");
}

void
pmap_protect(pmap_t pmap, vaddr_t sva, vaddr_t eva, vm_prot_t prot)
{
	struct pv_entry *pv;
	int s, slpn, elpn, lpn;

	if (prot == VM_PROT_NONE) {
		pmap_remove(pmap, sva, eva);
		return;
	}
	if (prot & VM_PROT_WRITE)
		return; /* apparently we're meant to */
	if (pmap == pmap_kernel())
		return; /* can't restrict kernel w/o unmapping. */

	slpn = atop(sva); elpn = atop(eva);
	s = splimp();
	for (lpn = slpn; lpn < elpn; lpn++) {
		pv = pmap->pm_entries[lpn];
		if (pv != NULL) {
			pv->pv_prot &= prot;
			pv_update(pv);
			if (pv->pv_pmap->pm_flags & PM_ACTIVE)
				MEMC_WRITE(pv->pv_activate);
		}
	}
	splx(s);
}

void
pmap_reference(pmap_t pmap)
{

	pmap->pm_count++;
}

/*
 * If there were any actions we could delay committing to hardware,
 * this would be where they'd be committed.  We should perhaps delay
 * cache flushes till here, but I've heard rumours that pmap_update()
 * isn't called as often as it should be, so I'll play it safe for
 * now.
 */
void
pmap_update()
{

	/* Do nothing */
}

/*
 * See if the given physical address has a logical mapping.  Return
 * its address if so, otherwise return the logical address in MEMC
 * physical space.  This means that we can safely write here without
 * flushing the cache.  Only necessary on ARM3.
 */
static caddr_t
pmap_find(paddr_t pa)
{
#ifdef CPU_ARM3
	struct pv_entry *pv;
		
	for (pv = &pv_table[atop(pa)]; pv != NULL; pv = pv->pv_next)
		if (pv->pv_pmap != NULL && (pv->pv_pmap->pm_flags & PM_ACTIVE))
			return (caddr_t)ptoa(pv->pv_lpn);
#endif
	return MEMC_PHYS_BASE + pa;
}

void
pmap_zero_page(paddr_t pa)
{

	bzero(pmap_find(pa), PAGE_SIZE);
	pv_table[atop(pa)].pv_pflags |= PV_MODIFIED | PV_REFERENCED;
}

void
pmap_copy_page(paddr_t src, paddr_t dest)
{

	memcpy(pmap_find(dest), pmap_find(src), PAGE_SIZE);
	pv_table[atop(src)].pv_pflags |= PV_REFERENCED;
	pv_table[atop(dest)].pv_pflags |= PV_MODIFIED | PV_REFERENCED;
}

/*
 * This is meant to return the range of kernel vm that is available
 * after loading the kernel.  Since NetBSD/arm26 runs the kernel from
 * physically-mapped space, we just return all of kernel vm.  Oh,
 * except for the single page at the end where we map
 * otherwise-unmapped pages.
 */
void
pmap_virtual_space(vaddr_t *vstartp, vaddr_t *vendp)
{
	
	*vstartp = VM_MIN_KERNEL_ADDRESS;
	*vendp = VM_MAX_KERNEL_ADDRESS - PAGE_SIZE;
}

/*
 * Check if the given access should have aborted.  Used in various abort
 * handlers to work out what happened.
 */
boolean_t
pmap_confess(vaddr_t va, vm_prot_t atype)
{
	pmap_t pmap;
	struct pv_entry *pv;

	/* XXX Assume access was from user mode (or equiv). */
	pmap = va < VM_MIN_KERNEL_ADDRESS ? active_pmap : pmap_kernel();
	pv = pmap->pm_entries[atop(va)];
	if (pv == NULL)
		return TRUE;
	if (pv->pv_ppl == MEMC_PPL_NOACCESS)
		return TRUE;
	if (pv->pv_ppl == MEMC_PPL_RDONLY && (atype & VM_PROT_WRITE))
		return TRUE;
	return FALSE;
}

#ifdef DDB
#include <ddb/db_output.h>

void
pmap_dump(struct pmap *pmap)
{
	int i;
	struct pv_entry *pv;
	int pflags;

	db_printf("PMAP %p:\n", pmap);
	db_printf("\tcount = %d, flags = %d\n",
		  pmap->pm_count, pmap->pm_flags);
	for (i = 0; i < 1024; i++)
		if ((pv = pmap->pm_entries[i]) != NULL) {
			db_printf("\t%03d->%p: ", i, pv);
			if (pv->pv_pmap != pmap)
				db_printf("pmap=%p!, ", pv->pv_pmap);
			db_printf("ppn=%d, lpn=%d, ",
				  pv->pv_ppn, pv->pv_lpn);
			db_printf("act=%08x, deact=%08x,\n",
				  pv->pv_activate, pv->pv_deactivate);
			db_printf("\t\tprot=%c%c%c, ",
				  pv->pv_prot & VM_PROT_READ ? 'r' : '-',
				  pv->pv_prot & VM_PROT_WRITE ? 'w' : '-',
				  pv->pv_prot & VM_PROT_EXECUTE ? 'x' : '-');
			db_printf("ppl=");
			switch(pv->pv_ppl) {
			case MEMC_PPL_RDWR:
				db_printf("rw, ");
				break;
			case MEMC_PPL_RDONLY:
				db_printf("r-, ");
				break;
			case MEMC_PPL_NOACCESS:
				db_printf("--, ");
				break;
			default:
				db_printf("0x%x, ", pv->pv_ppl);
				break;
			}
			db_printf("vflags=%c, ",
				  pv->pv_vflags & PV_WIRED ? 'W' : '-');
			pflags = pv_table[pv->pv_ppn].pv_pflags;
			db_printf("pflags=%c%c\n",
				  pflags & PV_MODIFIED ? 'M' : '-',
				  pflags & PV_REFERENCED ? 'R' : '-');

		}
}
#endif
