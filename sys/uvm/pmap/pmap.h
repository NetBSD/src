/*	$NetBSD: pmap.h,v 1.21 2022/05/07 06:53:16 rin Exp $	*/

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

#ifndef	_UVM_PMAP_PMAP_H_
#define	_UVM_PMAP_PMAP_H_

#include <uvm/uvm_stat.h>
#ifdef UVMHIST
UVMHIST_DECL(pmapexechist);
UVMHIST_DECL(pmaphist);
UVMHIST_DECL(pmapsegtabhist);
#endif

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
 * Each seg_tab point an array of pt_entry [NPTEPG]
 */
typedef union pmap_segtab {
	union pmap_segtab *	seg_seg[PMAP_SEGTABSIZE];
	pt_entry_t *		seg_tab[PMAP_SEGTABSIZE];
} pmap_segtab_t;

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
#ifdef MULTIPROCESSOR
	kcpuset_t		*pm_active;	/* pmap was active on ... */
	kcpuset_t		*pm_onproc;	/* pmap is active on ... */
	volatile u_int		pm_shootdown_pending;
#endif
	pmap_segtab_t *		pm_segtab;	/* pointers to pages of PTEs */
	u_int			pm_count;	/* pmap reference count */
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

extern pmap_segtab_t pmap_kern_segtab;

/*
 * The current top of kernel VM
 */
extern vaddr_t pmap_curmaxkvaddr;

#define	pmap_wired_count(pmap) 	((pmap)->pm_stats.wired_count)
#define pmap_resident_count(pmap) ((pmap)->pm_stats.resident_count)

bool	pmap_remove_all(pmap_t);
void	pmap_set_modified(paddr_t);
bool	pmap_page_clear_attributes(struct vm_page_md *, u_int);
void	pmap_page_set_attributes(struct vm_page_md *, u_int);
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

//uint16_t pmap_pvlist_lock(struct vm_page_md *, bool);
kmutex_t *pmap_pvlist_lock_addr(struct vm_page_md *);

#define	PMAP_STEAL_MEMORY	/* enable pmap_steal_memory() */
#define	PMAP_GROWKERNEL		/* enable pmap_growkernel() */

/*
 * Alternate mapping hooks for pool pages.  Avoids thrashing the TLB.
 */
vaddr_t pmap_map_poolpage(paddr_t);
paddr_t pmap_unmap_poolpage(vaddr_t);
struct vm_page *pmap_md_alloc_poolpage(int);
#define	PMAP_ALLOC_POOLPAGE(flags)	pmap_md_alloc_poolpage(flags)
#define	PMAP_MAP_POOLPAGE(pa)		pmap_map_poolpage(pa)
#define	PMAP_UNMAP_POOLPAGE(va)		pmap_unmap_poolpage(va)

#define PMAP_COUNT(name)	(pmap_evcnt_##name.ev_count++ + 0)
#define PMAP_COUNTER(name, desc) \
struct evcnt pmap_evcnt_##name = \
	EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "pmap", desc); \
EVCNT_ATTACH_STATIC(pmap_evcnt_##name)

#endif	/* _KERNEL */
#endif	/* _UVM_PMAP_PMAP_H_ */
