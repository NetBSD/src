/* $NetBSD: pmap.c,v 1.14 2000/12/30 13:42:12 bjh21 Exp $ */
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
#include "opt_uvmhist.h"
#include "arcvideo.h"

#include <sys/param.h>

__KERNEL_RCSID(0, "$NetBSD: pmap.c,v 1.14 2000/12/30 13:42:12 bjh21 Exp $");

#include <sys/kernel.h> /* for cold */
#include <sys/malloc.h>
#include <sys/pool.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>
#include <uvm/uvm_stat.h>

#include <machine/intr.h>
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
 * Global array of pv_entries.  Dynamically allocated at startup.
 */
struct pv_entry *pv_table;

/* Kernel pmap -- statically allocated to make life slightly less odd. */

struct pmap kernel_pmap_store;

static boolean_t pmap_initialised = FALSE;

static struct pool *pmap_pool;

static pmap_t active_pmap;

UVMHIST_DECL(pmaphist);
static struct uvm_history_ent pmaphistbuf[100];

       void pmap_init2(void);

static void pv_update(struct pv_entry *);
static struct pv_entry *pv_alloc(void);
static void pv_free(struct pv_entry *pv);
static struct pv_entry *pv_get(pmap_t pmap, int ppn, int lpn);
static void pv_release(pmap_t pmap, int ppn, int lpn);

static caddr_t pmap_find(paddr_t);

static void pmap_update_page(int);

void pmap_virtual_space(vaddr_t *, vaddr_t *);

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
	size_t pv_table_size;
	UVMHIST_FUNC("pmap_bootstrap");

	UVMHIST_INIT_STATIC(pmaphist, pmaphistbuf);
	UVMHIST_CALLED(pmaphist);
	/* Set up the bootstrap pv_table */
	pv_table_size = round_page(physmem * sizeof(struct pv_entry));
	pv_table =
	    (struct pv_entry *)pmap_steal_memory(pv_table_size, NULL, NULL);
	bzero(pv_table, pv_table_size);

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

vaddr_t
pmap_steal_memory(vsize_t size, vaddr_t *vstartp, vaddr_t *vendp)
{
	int i;
	vaddr_t addr;
	UVMHIST_FUNC("pmap_steal_memory");

	UVMHIST_CALLED(pmaphist);
	addr = NULL;
	size = round_page(size);
	for (i = 0; i < vm_nphysseg; i++) {
		if (vm_physmem[i].avail_start < vm_physmem[i].avail_end) {
			addr = (vaddr_t)
			    (MEMC_PHYS_BASE + ptoa(vm_physmem[i].avail_start));
			vm_physmem[i].avail_start++;
			break;
		}
	}

	pmap_virtual_space(vstartp, vendp);

	return addr;
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
	UVMHIST_FUNC("pmap_init");

	UVMHIST_CALLED(pmaphist);
	/* All deferred to pmap_create, because malloc() is nice. */
}

/*
 * pmap_init2: third stage pmap initialisation.
 *
 * This is invoked the first time pmap_create is called.  It sets up the pool
 * for allocating user pmaps, and frees some unnecessary memory.
 */
void
pmap_init2()
{
	struct pmap *pmap;
	struct pv_entry *new_pv_table, *old_pv_table;
	size_t pv_table_size;
	int lpn;
	UVMHIST_FUNC("pmap_init2");

	UVMHIST_CALLED(pmaphist);
	/* We can now call malloc().  Rationalise our memory usage. */
	pv_table_size = physmem * sizeof(struct pv_entry);
	new_pv_table = malloc(pv_table_size, M_VMPMAP, M_WAITOK);
	memcpy(new_pv_table, pv_table, pv_table_size);
	old_pv_table = pv_table;
	pv_table = new_pv_table;

	/* Fix up the kernel pmap to point at new pv_entries. */
	pmap = pmap_kernel();
	for (lpn = 0; lpn < 1024; lpn++)
		if (pmap->pm_entries[lpn] ==
		    old_pv_table + pmap->pm_entries[lpn]->pv_ppn)
			pmap->pm_entries[lpn] =
			    pv_table + pmap->pm_entries[lpn]->pv_ppn;

	uvm_pagefree(PHYS_TO_VM_PAGE(
	    PMAP_UNMAP_POOLPAGE((vaddr_t)old_pv_table)));

	/* Create pmap pool */
	pmap_pool = pool_create(sizeof(struct pmap), 0, 0, 0,
	    "pmappool", 0, NULL, NULL, M_VMPMAP);
	pmap_initialised = 1;
}

struct pmap *
pmap_create()
{
	struct pmap *pmap;
	UVMHIST_FUNC("pmap_create");

	UVMHIST_CALLED(pmaphist);
	if (!pmap_initialised) 
		pmap_init2();
	pmap = pool_get(pmap_pool, PR_WAITOK);
	bzero(pmap, sizeof(*pmap));
	pmap->pm_count = 1;
	return pmap;
}

void
pmap_destroy(pmap_t pmap)
{
#ifdef DIAGNOSTIC
	int i;
#endif
	UVMHIST_FUNC("pmap_destroy");

	UVMHIST_CALLED(pmaphist);
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
	UVMHIST_FUNC("pmap_activate");

	UVMHIST_CALLED(pmaphist);
	pmap = p->p_vmspace->vm_map.pmap;

	if (pmap == pmap_kernel())
		return; /* kernel pmap is always active */

	KASSERT(active_pmap == NULL);
	KASSERT((pmap->pm_flags & PM_ACTIVE) == 0);

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
	UVMHIST_FUNC("pmap_deactivate");

	UVMHIST_CALLED(pmaphist);
	pmap = p->p_vmspace->vm_map.pmap;

	if (pmap == pmap_kernel())
		return; /* kernel pmap is always active */

	KASSERT(pmap == active_pmap);
	KASSERT(pmap->pm_flags & PM_ACTIVE);

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
	UVMHIST_FUNC("pmap_unwire");

	UVMHIST_CALLED(pmaphist);
	if (pmap == NULL) return;
	pv = pmap->pm_entries[atop(va)];
	if (pv == NULL) return;
	pv->pv_vflags &= ~PV_WIRED;
}

void
pmap_collect(pmap)
	pmap_t pmap;
{
	UVMHIST_FUNC("pmap_collect");

	UVMHIST_CALLED(pmaphist);
	/* This is allowed to be a no-op. */
}

void
pmap_copy(dst_pmap, src_pmap, dst_addr, len, src_addr)
	pmap_t dst_pmap, src_pmap;
	vaddr_t dst_addr, src_addr;
	vsize_t len;
{
	UVMHIST_FUNC("pmap_copy");

	UVMHIST_CALLED(pmaphist);
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
	UVMHIST_FUNC("pv_update");

	UVMHIST_CALLED(pmaphist);
	pflags = pv_table[pv->pv_ppn].pv_pflags;
	if (pv->pv_pmap == pmap_kernel())
		/*
		 * Referenced/modified emulation is almost impossible in
		 * SVC mode.
		 */
		ppl = MEMC_PPL_NOACCESS;
	else {
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
	UVMHIST_FUNC("pv_get");

	UVMHIST_CALLED(pmaphist);
	UVMHIST_LOG(pmaphist, "(pmap=%p, ppn=%d, lpn=%d)", pmap, ppn, lpn, 0);
	/* If the head entry's free use that. */
	pv = &pv_table[ppn];
	if (pv->pv_pmap == NULL) {
		UVMHIST_LOG(pmaphist, "<-- head (pv=%p)", pv, 0, 0, 0);
		return pv;
	}
	/* If this mapping exists already, use that. */
	for (pv = pv; pv != NULL; pv = pv->pv_next)
		if (pv->pv_pmap == pmap && pv->pv_lpn == lpn) {
			UVMHIST_LOG(pmaphist, "<-- existing (pv=%p)",
			    pv, 0, 0, 0);
			return pv;
		}
	/* Otherwise, allocate a new entry and link it in after the head. */
	pv = pv_alloc();
	pv->pv_next = pv_table[ppn].pv_next;
	pv_table[ppn].pv_next = pv;
	UVMHIST_LOG(pmaphist, "<-- new (pv=%p)", pv, 0, 0, 0);
	return pv;
}

/*
 * Release the pv_entry for a mapping.  Code derived from hp300 pmap.
 */
static void
pv_release(pmap_t pmap, int ppn, int lpn)
{
	struct pv_entry *pv, *npv;
	UVMHIST_FUNC("pv_release");

	UVMHIST_CALLED(pmaphist);
	UVMHIST_LOG(pmaphist, "(pmap=%p, ppn=%d, lpn=%d)", pmap, ppn, lpn, 0);
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
			UVMHIST_LOG(pmaphist, "pv=%p; pull-up", pv, 0, 0, 0);
			/* Pull up first entry from chain. */
			npv->pv_pflags = pv->pv_pflags;
			*pv = *npv;
			pv->pv_pmap->pm_entries[pv->pv_lpn] = pv;
			pv_free(npv);
		} else {
			UVMHIST_LOG(pmaphist, "pv=%p; empty", pv, 0, 0, 0);
			pv->pv_pmap = NULL;
		}
	} else {
		for (npv = pv->pv_next; npv; npv = npv->pv_next) {
			if (pmap == npv->pv_pmap && lpn == npv->pv_lpn)
				break;
			pv = npv;
		}
		KASSERT(npv != NULL);
		UVMHIST_LOG(pmaphist, "pv=%p; tail", pv, 0, 0, 0);
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
	struct pv_entry *pv, *ppv;
	UVMHIST_FUNC("pmap_enter");

	UVMHIST_CALLED(pmaphist);
	ppn = atop(pa); lpn = atop(va);

	UVMHIST_LOG(pmaphist, "mapping ppn %d at lpn %d in pmap %p",
	       ppn, lpn, pmap, 0);
	s = splimp();

	/* Remove any existing mapping at this lpn */
	if (pmap->pm_entries[lpn] != NULL &&
	    pmap->pm_entries[lpn]->pv_ppn != ppn)
		pmap_remove(pmap, va, va + PAGE_SIZE);

	/* Make a note */
	pv = pv_get(pmap, ppn, lpn);
	ppv = &pv_table[ppn];
	pv->pv_pmap = pmap;
	pv->pv_ppn = ppn;
	pv->pv_lpn = lpn;
	pv->pv_prot = prot;
	pv->pv_vflags = 0;
	/* pv->pv_pflags = 0; */
	if (flags & PMAP_WIRED)
		pv->pv_vflags |= PV_WIRED;
	if (flags & VM_PROT_WRITE)
		ppv->pv_pflags |= PV_REFERENCED | PV_MODIFIED;
	else if (flags & (VM_PROT_ALL))
		ppv->pv_pflags |= PV_REFERENCED;
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
	UVMHIST_FUNC("pmap_remove");

	UVMHIST_CALLED(pmaphist);
	slpn = atop(sva); elpn = atop(eva);
	UVMHIST_LOG(pmaphist, "clearing from lpn %d to lpn %d in pmap %p",
	       slpn, elpn - 1, pmap, 0);
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
	UVMHIST_FUNC("pmap_extract");

	UVMHIST_CALLED(pmaphist);
	pv = pmap->pm_entries[atop(va)];
	if (pv == NULL)
		return FALSE;
	*ppa = ptoa(pv->pv_ppn);
	return TRUE;
}

void
pmap_kenter_pa(vaddr_t va, paddr_t pa, vm_prot_t prot)
{
	UVMHIST_FUNC("pmap_kenter_pa");

	UVMHIST_CALLED(pmaphist);
	pmap_enter(pmap_kernel(), va, pa, prot, prot | PMAP_WIRED);
}

void
pmap_kenter_pgs(vaddr_t va, struct vm_page **pages, int npages)
{
	UVMHIST_FUNC("pmap_kenter_pgs");

	UVMHIST_CALLED(pmaphist);
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
	UVMHIST_FUNC("pmap_kremove");

	UVMHIST_CALLED(pmaphist);
	pmap_remove(pmap_kernel(), va, va+len);
}

boolean_t
pmap_is_modified(page)
	struct vm_page *page;
{
	int ppn;
	UVMHIST_FUNC("pmap_is_modified");

	UVMHIST_CALLED(pmaphist);
	ppn = atop(page->phys_addr);
	return (pv_table[ppn].pv_pflags & PV_MODIFIED) != 0;
}

boolean_t
pmap_is_referenced(page)
	struct vm_page *page;
{
	int ppn;
	UVMHIST_FUNC("pmap_is_referenced");

	UVMHIST_CALLED(pmaphist);
	ppn = atop(page->phys_addr);
	return (pv_table[ppn].pv_pflags & PV_REFERENCED) != 0;
}

static void
pmap_update_page(int ppn)
{
	struct pv_entry *pv;
	UVMHIST_FUNC("pmap_update_page");

	UVMHIST_CALLED(pmaphist);
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
	UVMHIST_FUNC("pmap_clear_modify");

	UVMHIST_CALLED(pmaphist);
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
	UVMHIST_FUNC("pmap_clear_reference");

	UVMHIST_CALLED(pmaphist);
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
	UVMHIST_FUNC("pmap_fault");

	UVMHIST_CALLED(pmaphist);
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
	/*
	 * It wasn't a referenced/modified fault.
	 * If it looks like the access should have been allowed, try pushing
	 * the mapping back into the MEMC.
	 */
	if ((atype & ~pv->pv_prot) == 0) {
		UVMHIST_LOG(pmaphist,
		    "MEMC miss; pmap = %p, lpn = %d, ppn = %d",
		    pmap, lpn, ppn, 0);
		MEMC_WRITE(pv->pv_activate);
		/*
		 * If the new mapping is writeable, we should flush the cache
		 * so that stale data for an existing mapping doesn't get
		 * reused.
		 * XXX: Should this be done more lazily?
		 */
		if (pv->pv_prot & VM_PROT_WRITE)
			cpu_cache_flush();
		return TRUE;
	}
	return FALSE;
}

void
pmap_page_protect(struct vm_page *page, vm_prot_t prot)
{
	int ppn;
	struct pv_entry *pv;
	UVMHIST_FUNC("pmap_page_protect");

	UVMHIST_CALLED(pmaphist);
	ppn = atop(page->phys_addr);
	if (prot == VM_PROT_NONE) {
		UVMHIST_LOG(pmaphist, "removing ppn %d\n", ppn, 0, 0, 0);
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
	UVMHIST_FUNC("pmap_protect");

	UVMHIST_CALLED(pmaphist);
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
	UVMHIST_FUNC("pmap_reference");

	UVMHIST_CALLED(pmaphist);
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
	UVMHIST_FUNC("pmap_update");

	UVMHIST_CALLED(pmaphist);
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
#endif
	UVMHIST_FUNC("pmap_find");

	UVMHIST_CALLED(pmaphist);
#ifdef CPU_ARM3
	for (pv = &pv_table[atop(pa)]; pv != NULL; pv = pv->pv_next)
		if (pv->pv_pmap != NULL && (pv->pv_pmap->pm_flags & PM_ACTIVE))
			return (caddr_t)ptoa(pv->pv_lpn);
#endif
	return MEMC_PHYS_BASE + pa;
}

void
pmap_zero_page(paddr_t pa)
{
	UVMHIST_FUNC("pmap_zero_page");

	UVMHIST_CALLED(pmaphist);
	bzero(pmap_find(pa), PAGE_SIZE);
	pv_table[atop(pa)].pv_pflags |= PV_MODIFIED | PV_REFERENCED;
}

void
pmap_copy_page(paddr_t src, paddr_t dest)
{
	UVMHIST_FUNC("pmap_copy_page");

	UVMHIST_CALLED(pmaphist);
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
	UVMHIST_FUNC("pmap_virtual_space");

	UVMHIST_CALLED(pmaphist);
	if (vstartp != NULL)
		*vstartp = VM_MIN_KERNEL_ADDRESS;
	if (vendp != NULL)
		*vendp = VM_MAX_KERNEL_ADDRESS - PAGE_SIZE;
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
