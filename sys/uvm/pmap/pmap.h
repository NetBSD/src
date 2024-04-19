/*	$NetBSD: pmap.h,v 1.26.2.1 2024/04/19 09:18:28 martin Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell.
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
 *	@(#)pmap.h	8.1 (Berkeley) 6/10/93
 */

/*
 * Copyright (c) 1987 Carnegie-Mellon University
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell.
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
 *	@(#)pmap.h	8.1 (Berkeley) 6/10/93
 */

#ifdef _KERNEL_OPT
#include "opt_efi.h"
#endif

#ifndef	_UVM_PMAP_PMAP_H_
#define	_UVM_PMAP_PMAP_H_

#include <sys/rwlock.h>
#include <uvm/uvm_object.h>
#include <uvm/uvm_stat.h>

#ifdef UVMHIST
UVMHIST_DECL(pmapexechist);
UVMHIST_DECL(pmaphist);
UVMHIST_DECL(pmapxtabhist);
#endif

/*
 * Alternate mapping hooks for pool pages.  Avoids thrashing the TLB.
 */
struct vm_page *pmap_md_alloc_poolpage(int);

#if !defined(KASAN)
vaddr_t pmap_map_poolpage(paddr_t);
paddr_t pmap_unmap_poolpage(vaddr_t);
#define	PMAP_ALLOC_POOLPAGE(flags)	pmap_md_alloc_poolpage(flags)
#define	PMAP_MAP_POOLPAGE(pa)		pmap_map_poolpage(pa)
#define	PMAP_UNMAP_POOLPAGE(va)		pmap_unmap_poolpage(va)

#if defined(_LP64)
#define PMAP_DIRECT
static __inline int
pmap_direct_process(paddr_t pa, voff_t pgoff, size_t len,
    int (*process)(void *, size_t, void *), void *arg)
{
        vaddr_t va = pmap_md_direct_map_paddr(pa);

        return process((void *)(va + pgoff), len, arg);
}
#endif
#endif

#define PMAP_MAP_PDETABPAGE(pa)		pmap_md_map_poolpage(pa, PAGE_SIZE)
#define PMAP_MAP_SEGTABPAGE(pa)		pmap_md_map_poolpage(pa, PAGE_SIZE)
#define PMAP_MAP_PTEPAGE(pa)		pmap_md_map_poolpage(pa, PAGE_SIZE)

/*
 * The user address space is mapped using a two level structure where
 * virtual address bits 31..22 are used to index into a segment table which
 * points to a page worth of PTEs (4096 page can hold 1024 PTEs).
 * Bits 21..12 are then used to index a PTE which describes a page within
 * a segment.
 */

#define pmap_trunc_seg(x)	((vaddr_t)(x) & ~SEGOFSET)
#define pmap_round_seg(x)	(((vaddr_t)(x) + SEGOFSET) & ~SEGOFSET)

/*
 * Each ptpage maps a "segment" worth of address space.  That is
 * NPTEPG * PAGE_SIZE.
 */

typedef struct {
	pt_entry_t ppg_ptes[NPTEPG];
} pmap_ptpage_t;

#if defined(PMAP_HWPAGEWALKER)
typedef union pmap_pdetab {
	pd_entry_t		pde_pde[PMAP_PDETABSIZE];
	union pmap_pdetab *	pde_next;
} pmap_pdetab_t;
#endif
#if !defined(PMAP_HWPAGEWALKER) || !defined(PMAP_MAP_PDETABPAGE)
typedef union pmap_segtab {
#ifdef _LP64
	union pmap_segtab *	seg_seg[PMAP_SEGTABSIZE];
#endif
	pmap_ptpage_t *		seg_ppg[PMAP_SEGTABSIZE];
#ifdef PMAP_HWPAGEWALKER
	pd_entry_t		seg_pde[PMAP_PDETABSIZE];
#endif
	union pmap_segtab *	seg_next;
} pmap_segtab_t;
#endif


#ifdef _KERNEL
struct pmap;
typedef bool (*pte_callback_t)(struct pmap *, vaddr_t, vaddr_t,
	pt_entry_t *, uintptr_t);

/*
 * Common part of the bootstraping the system enough to run with
 * virtual memory.
 */
void pmap_bootstrap_common(void);

pt_entry_t *pmap_pte_lookup(struct pmap *, vaddr_t);
pt_entry_t *pmap_pte_reserve(struct pmap *, vaddr_t, int);
void pmap_pte_process(struct pmap *, vaddr_t, vaddr_t, pte_callback_t,
	uintptr_t);
void pmap_segtab_activate(struct pmap *, struct lwp *);
void pmap_segtab_deactivate(struct pmap *);
void pmap_segtab_init(struct pmap *);
void pmap_segtab_destroy(struct pmap *, pte_callback_t, uintptr_t);
#ifdef PMAP_HWPAGEWALKER
pd_entry_t *pmap_pde_lookup(struct pmap *, vaddr_t, paddr_t *);
bool pmap_pdetab_fixup(struct pmap *, vaddr_t);
#endif
extern kmutex_t pmap_segtab_lock;
#endif /* _KERNEL */

#ifdef MULTIPROCESSOR
#include <sys/kcpuset.h>
#endif
#include <uvm/pmap/pmap_tlb.h>

/*
 * Machine dependent pmap structure.
 */
struct pmap {
	struct uvm_object	pm_uobject;
#define pm_refcnt		pm_uobject.uo_refs /* pmap reference count */
#define pm_pvp_list		pm_uobject.memq

	krwlock_t		pm_obj_lock;	/* lock for pm_uobject */
#define pm_lock pm_uobject.vmobjlock

	struct pglist		pm_ppg_list;
#if defined(PMAP_HWPAGEWALKER)
	struct pglist		pm_pdetab_list;
#endif
#if !defined(PMAP_HWPAGEWALKER) || !defined(PMAP_MAP_PDETABPAGE)
	struct pglist		pm_segtab_list;
#endif
#ifdef MULTIPROCESSOR
	kcpuset_t		*pm_active;	/* pmap was active on ... */
	kcpuset_t		*pm_onproc;	/* pmap is active on ... */
	volatile u_int		pm_shootdown_pending;
#endif
#if defined(PMAP_HWPAGEWALKER)
	pmap_pdetab_t *		pm_pdetab;	/* pointer to HW PDEs */
#endif
#if !defined(PMAP_HWPAGEWALKER) || !defined(PMAP_MAP_PDETABPAGE)
	pmap_segtab_t *		pm_segtab;	/* pointers to pages of PTEs; or  */
						/* virtual shadow of HW PDEs */
#endif
	u_int			pm_flags;
#define	PMAP_DEFERRED_ACTIVATE	__BIT(0)
	struct pmap_statistics	pm_stats;	/* pmap statistics */
	vaddr_t			pm_minaddr;
	vaddr_t			pm_maxaddr;
#ifdef __HAVE_PMAP_MD
	struct pmap_md		pm_md;
#endif
	struct pmap_asid_info	pm_pai[1];
};


#ifdef	_KERNEL
static inline void
pmap_lock(struct pmap *pm)
{

	rw_enter(pm->pm_lock, RW_WRITER);
}

static inline void
pmap_unlock(struct pmap *pm)
{

	rw_exit(pm->pm_lock);
}

struct pmap_kernel {
	struct pmap kernel_pmap;
#if defined(MULTIPROCESSOR) && PMAP_TLB_MAX > 1
	struct pmap_asid_info kernel_pai[PMAP_TLB_MAX-1];
#endif
};

struct pmap_limits {
	paddr_t avail_start;
	paddr_t avail_end;
	vaddr_t virtual_start;
	vaddr_t virtual_end;
};

/*
 * Initialize the kernel pmap.
 */
#ifdef MULTIPROCESSOR
#define PMAP_SIZE	offsetof(struct pmap, pm_pai[PMAP_TLB_MAX])
#else
#define PMAP_SIZE	sizeof(struct pmap)
#endif

/*
 * The pools from which pmap structures and sub-structures are allocated.
 */
extern struct pool pmap_pmap_pool;
extern struct pool pmap_pv_pool;
extern struct pool_allocator pmap_pv_page_allocator;

extern struct pmap_kernel kernel_pmap_store;
extern struct pmap_limits pmap_limits;

extern u_int pmap_page_colormask;

/*
 * The current top of kernel VM
 */
extern vaddr_t pmap_curmaxkvaddr;

#if defined(PMAP_HWPAGEWALKER)
extern pmap_pdetab_t pmap_kern_pdetab;
#else
extern pmap_segtab_t pmap_kern_segtab;
#endif

#define	pmap_wired_count(pmap) 	((pmap)->pm_stats.wired_count)
#define pmap_resident_count(pmap) ((pmap)->pm_stats.resident_count)

bool	pmap_remove_all(pmap_t);
void	pmap_set_modified(paddr_t);
bool	pmap_page_clear_attributes(struct vm_page_md *, u_long);
void	pmap_page_set_attributes(struct vm_page_md *, u_long);
void	pmap_pvlist_lock_init(size_t);
#ifdef PMAP_VIRTUAL_CACHE_ALIASES
void	pmap_page_cache(struct vm_page_md *, bool);
#endif

#if defined(__HAVE_PMAP_PV_TRACK) && !defined(PMAP_PV_TRACK_ONLY_STUBS)
void	pmap_pv_protect(paddr_t, vm_prot_t);
#endif

#define	PMAP_WB		0
#define	PMAP_WBINV	1
#define	PMAP_INV	2

kmutex_t *pmap_pvlist_lock_addr(struct vm_page_md *);

#define	PMAP_STEAL_MEMORY	/* enable pmap_steal_memory() */
#define	PMAP_GROWKERNEL		/* enable pmap_growkernel() */

#define PMAP_COUNT(name)	(pmap_evcnt_##name.ev_count++ + 0)
#define PMAP_COUNTER(name, desc) \
struct evcnt pmap_evcnt_##name = \
	EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap", desc); \
EVCNT_ATTACH_STATIC(pmap_evcnt_##name)


static inline pt_entry_t *
kvtopte(vaddr_t va)
{

	return pmap_pte_lookup(pmap_kernel(), va);
}

/* for ddb */
void pmap_db_pmap_print(struct pmap *, void (*)(const char *, ...) __printflike(1, 2));
void pmap_db_mdpg_print(struct vm_page *, void (*)(const char *, ...) __printflike(1, 2));

#if defined(EFI_RUNTIME)
struct pmap *
	pmap_efirt(void);

#define pmap_activate_efirt()	pmap_md_activate_efirt()
#define pmap_deactivate_efirt()	pmap_md_deactivate_efirt()

#endif

#endif	/* _KERNEL */
#endif	/* _UVM_PMAP_PMAP_H_ */
