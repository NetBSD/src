/*	$NetBSD: pmap_private.h,v 1.4 2022/09/24 11:05:18 riastradh Exp $	*/

/*
 * Copyright (c) 1997 Charles D. Cranor and Washington University.
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

/*
 * Copyright (c) 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Frank van der Linden for Wasabi Systems, Inc.
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

#ifndef	_X86_PMAP_PRIVATE_H_
#define	_X86_PMAP_PRIVATE_H_

#ifndef	_MACHINE_PMAP_PRIVATE_H_X86
#error Include machine/pmap_private.h, not x86/pmap_private.h.
#endif

#ifdef _KERNEL_OPT
#include "opt_svs.h"
#endif

#include <sys/param.h>
#include <sys/types.h>

#include <sys/kcpuset.h>
#include <sys/mutex.h>
#include <sys/pool.h>
#include <sys/queue.h>
#include <sys/rwlock.h>

#include <machine/cpufunc.h>
#include <machine/pte.h>
#include <machine/vmparam.h>

#include <uvm/uvm_object.h>
#include <uvm/uvm_pmap.h>

struct pmap;

#define SLAREA_USER	0
#define SLAREA_PTE	1
#define SLAREA_MAIN	2
#define SLAREA_PCPU	3
#define SLAREA_DMAP	4
#define SLAREA_HYPV	5
#define SLAREA_ASAN	6
#define SLAREA_MSAN	7
#define SLAREA_KERN	8
#define SLSPACE_NAREAS	9

struct slotspace {
	struct {
		size_t sslot; /* start slot */
		size_t nslot; /* # of slots */
		bool active;  /* area is active */
	} area[SLSPACE_NAREAS];
};

extern struct slotspace slotspace;

#include <x86/gdt.h>

struct pcpu_entry {
	uint8_t gdt[MAXGDTSIZ];
	uint8_t ldt[MAX_USERLDT_SIZE];
	uint8_t idt[PAGE_SIZE];
	uint8_t tss[PAGE_SIZE];
	uint8_t ist0[PAGE_SIZE];
	uint8_t ist1[PAGE_SIZE];
	uint8_t ist2[PAGE_SIZE];
	uint8_t ist3[PAGE_SIZE];
	uint8_t rsp0[2 * PAGE_SIZE];
} __packed;

struct pcpu_area {
#ifdef SVS
	uint8_t utls[PAGE_SIZE];
#endif
	uint8_t ldt[PAGE_SIZE];
	struct pcpu_entry ent[MAXCPUS];
} __packed;

extern struct pcpu_area *pcpuarea;

#define PMAP_PCID_KERN	0
#define PMAP_PCID_USER	1

/*
 * pmap data structures: see pmap.c for details of locking.
 */

/*
 * we maintain a list of all non-kernel pmaps
 */

LIST_HEAD(pmap_head, pmap); /* struct pmap_head: head of a pmap list */

/*
 * linked list of all non-kernel pmaps
 */
extern struct pmap_head pmaps;
extern kmutex_t pmaps_lock;    /* protects pmaps */

/*
 * pool_cache(9) that pmaps are allocated from
 */
extern struct pool_cache pmap_cache;

/*
 * the pmap structure
 *
 * note that the pm_obj contains the lock pointer, the reference count,
 * page list, and number of PTPs within the pmap.
 *
 * pm_lock is the same as the lock for vm object 0.  Changes to
 * the other objects may only be made if that lock has been taken
 * (the other object locks are only used when uvm_pagealloc is called)
 */

struct pv_page;

struct pmap {
	struct uvm_object pm_obj[PTP_LEVELS-1];/* objects for lvl >= 1) */
	LIST_ENTRY(pmap) pm_list;	/* list of all pmaps */
	pd_entry_t *pm_pdir;		/* VA of PD */
	paddr_t pm_pdirpa[PDP_SIZE];	/* PA of PDs (read-only after create) */
	struct vm_page *pm_ptphint[PTP_LEVELS-1];
					/* pointer to a PTP in our pmap */
	struct pmap_statistics pm_stats;  /* pmap stats */
	struct pv_entry *pm_pve;	/* spare pv_entry */
	LIST_HEAD(, pv_page) pm_pvp_part;
	LIST_HEAD(, pv_page) pm_pvp_empty;
	LIST_HEAD(, pv_page) pm_pvp_full;

#if !defined(__x86_64__)
	vaddr_t pm_hiexec;		/* highest executable mapping */
#endif /* !defined(__x86_64__) */

	union descriptor *pm_ldt;	/* user-set LDT */
	size_t pm_ldt_len;		/* XXX unused, remove */
	int pm_ldt_sel;			/* LDT selector */

	kcpuset_t *pm_cpus;		/* mask of CPUs using pmap */
	kcpuset_t *pm_kernel_cpus;	/* mask of CPUs using kernel part
					 of pmap */
	kcpuset_t *pm_xen_ptp_cpus;	/* mask of CPUs which have this pmap's
					 ptp mapped */
	uint64_t pm_ncsw;		/* for assertions */
	LIST_HEAD(,vm_page) pm_gc_ptp;	/* PTPs queued for free */

	/* Used by NVMM and Xen */
	int (*pm_enter)(struct pmap *, vaddr_t, paddr_t, vm_prot_t, u_int);
	bool (*pm_extract)(struct pmap *, vaddr_t, paddr_t *);
	void (*pm_remove)(struct pmap *, vaddr_t, vaddr_t);
	int (*pm_sync_pv)(struct vm_page *, vaddr_t, paddr_t, int, uint8_t *,
	    pt_entry_t *);
	void (*pm_pp_remove_ent)(struct pmap *, struct vm_page *, pt_entry_t,
	    vaddr_t);
	void (*pm_write_protect)(struct pmap *, vaddr_t, vaddr_t, vm_prot_t);
	void (*pm_unwire)(struct pmap *, vaddr_t);

	void (*pm_tlb_flush)(struct pmap *);
	void *pm_data;

	kmutex_t pm_lock		/* locks for pm_objs */
	    __aligned(64);		/* give lock own cache line */
	krwlock_t pm_dummy_lock;	/* ugly hack for abusing uvm_object */
};

/* macro to access pm_pdirpa slots */
#ifdef PAE
#define pmap_pdirpa(pmap, index) \
	((pmap)->pm_pdirpa[l2tol3(index)] + l2tol2(index) * sizeof(pd_entry_t))
#else
#define pmap_pdirpa(pmap, index) \
	((pmap)->pm_pdirpa[0] + (index) * sizeof(pd_entry_t))
#endif

/*
 * global kernel variables
 */

/*
 * PDPpaddr is the physical address of the kernel's PDP.
 * - i386 non-PAE and amd64: PDPpaddr corresponds directly to the %cr3
 * value associated to the kernel process, proc0.
 * - i386 PAE: it still represents the PA of the kernel's PDP (L2). Due to
 * the L3 PD, it cannot be considered as the equivalent of a %cr3 any more.
 * - Xen: it corresponds to the PFN of the kernel's PDP.
 */
extern u_long PDPpaddr;

extern pd_entry_t pmap_pg_g;			/* do we support PTE_G? */
extern pd_entry_t pmap_pg_nx;			/* do we support PTE_NX? */
extern int pmap_largepages;
extern long nkptp[PTP_LEVELS];

#define pmap_valid_entry(E) 		((E) & PTE_P) /* is PDE or PTE valid? */

void		pmap_map_ptes(struct pmap *, struct pmap **, pd_entry_t **,
		    pd_entry_t * const **);
void		pmap_unmap_ptes(struct pmap *, struct pmap *);

bool		pmap_pdes_valid(vaddr_t, pd_entry_t * const *, pd_entry_t *,
		    int *lastlvl);

bool		pmap_is_curpmap(struct pmap *);

void		pmap_ept_transform(struct pmap *);

#ifndef __HAVE_DIRECT_MAP
void		pmap_vpage_cpu_init(struct cpu_info *);
#endif
vaddr_t		slotspace_rand(int, size_t, size_t, size_t, vaddr_t);

vaddr_t reserve_dumppages(vaddr_t); /* XXX: not a pmap fn */

typedef enum tlbwhy {
	TLBSHOOT_REMOVE_ALL,
	TLBSHOOT_KENTER,
	TLBSHOOT_KREMOVE,
	TLBSHOOT_FREE_PTP,
	TLBSHOOT_REMOVE_PTE,
	TLBSHOOT_SYNC_PV,
	TLBSHOOT_WRITE_PROTECT,
	TLBSHOOT_ENTER,
	TLBSHOOT_NVMM,
	TLBSHOOT_BUS_DMA,
	TLBSHOOT_BUS_SPACE,
	TLBSHOOT__MAX,
} tlbwhy_t;

void		pmap_tlb_init(void);
void		pmap_tlb_cpu_init(struct cpu_info *);
void		pmap_tlb_shootdown(pmap_t, vaddr_t, pt_entry_t, tlbwhy_t);
void		pmap_tlb_shootnow(void);
void		pmap_tlb_intr(void);

/*
 * inline functions
 */

/*
 * pmap_update_pg: flush one page from the TLB (or flush the whole thing
 *	if hardware doesn't support one-page flushing)
 */

__inline static void __unused
pmap_update_pg(vaddr_t va)
{
	invlpg(va);
}

/*
 * various address inlines
 *
 *  vtopte: return a pointer to the PTE mapping a VA, works only for
 *  user and PT addresses
 *
 *  kvtopte: return a pointer to the PTE mapping a kernel VA
 */

#include <lib/libkern/libkern.h>

static __inline pt_entry_t * __unused
vtopte(vaddr_t va)
{

	KASSERT(va < VM_MIN_KERNEL_ADDRESS);

	return (PTE_BASE + pl1_i(va));
}

static __inline pt_entry_t * __unused
kvtopte(vaddr_t va)
{
	pd_entry_t *pde;

	KASSERT(va >= VM_MIN_KERNEL_ADDRESS);

	pde = L2_BASE + pl2_i(va);
	if (*pde & PTE_PS)
		return ((pt_entry_t *)pde);

	return (PTE_BASE + pl1_i(va));
}

#ifdef XENPV
#include <sys/bitops.h>

#define XPTE_MASK	L1_FRAME
/* Selects the index of a PTE in (A)PTE_BASE */
#define XPTE_SHIFT	(L1_SHIFT - ilog2(sizeof(pt_entry_t)))

/* PTE access inline functions */

/*
 * Get the machine address of the pointed pte
 * We use hardware MMU to get value so works only for levels 1-3
 */

static __inline paddr_t
xpmap_ptetomach(pt_entry_t *pte)
{
	pt_entry_t *up_pte;
	vaddr_t va = (vaddr_t) pte;

	va = ((va & XPTE_MASK) >> XPTE_SHIFT) | (vaddr_t) PTE_BASE;
	up_pte = (pt_entry_t *) va;

	return (paddr_t) (((*up_pte) & PTE_FRAME) + (((vaddr_t) pte) & (~PTE_FRAME & ~VA_SIGN_MASK)));
}

/* Xen helpers to change bits of a pte */
#define XPMAP_UPDATE_DIRECT	1	/* Update direct map entry flags too */

paddr_t	vtomach(vaddr_t);
#define vtomfn(va) (vtomach(va) >> PAGE_SHIFT)
#endif	/* XENPV */

#ifdef __HAVE_PCPU_AREA
extern struct pcpu_area *pcpuarea;
#define PDIR_SLOT_PCPU		510
#define PMAP_PCPU_BASE		(VA_SIGN_NEG((PDIR_SLOT_PCPU * NBPD_L4)))
#endif

void	svs_quad_copy(void *, void *, long);

#ifdef _KERNEL_OPT
#include "opt_efi.h"
#endif

#ifdef EFI_RUNTIME
void *		pmap_activate_sync(struct pmap *);
void		pmap_deactivate_sync(struct pmap *, void *);
bool		pmap_is_user(struct pmap *);
#else
static inline bool
pmap_is_user(struct pmap *pmap)
{

	KASSERT(pmap != pmap_kernel());
	return true;
}
#endif

#endif	/* _X86_PMAP_PRIVATE_H_ */
