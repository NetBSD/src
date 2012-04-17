/*	$NetBSD: pmap.h,v 1.100.2.1 2012/04/17 00:06:05 yamt Exp $	*/

/*
 * Copyright (c) 2002, 2003 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe & Steve C. Woodford for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
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
 * Copyright (c) 1994,1995 Mark Brinicombe.
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
 *	This product includes software developed by Mark Brinicombe
 * 4. The name of the author may not be used to endorse or promote products
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

#ifndef	_ARM32_PMAP_H_
#define	_ARM32_PMAP_H_

#ifdef _KERNEL

#include <arm/cpuconf.h>
#include <arm/arm32/pte.h>
#ifndef _LOCORE
#if defined(_KERNEL_OPT)
#include "opt_arm32_pmap.h"
#endif
#include <arm/cpufunc.h>
#include <uvm/uvm_object.h>
#endif

/*
 * a pmap describes a processes' 4GB virtual address space.  this
 * virtual address space can be broken up into 4096 1MB regions which
 * are described by L1 PTEs in the L1 table.
 *
 * There is a line drawn at KERNEL_BASE.  Everything below that line
 * changes when the VM context is switched.  Everything above that line
 * is the same no matter which VM context is running.  This is achieved
 * by making the L1 PTEs for those slots above KERNEL_BASE reference
 * kernel L2 tables.
 *
 * The basic layout of the virtual address space thus looks like this:
 *
 *	0xffffffff
 *	.
 *	.
 *	.
 *	KERNEL_BASE
 *	--------------------
 *	.
 *	.
 *	.
 *	0x00000000
 */

/*
 * The number of L2 descriptor tables which can be tracked by an l2_dtable.
 * A bucket size of 16 provides for 16MB of contiguous virtual address
 * space per l2_dtable. Most processes will, therefore, require only two or
 * three of these to map their whole working set.
 */
#define	L2_BUCKET_LOG2	4
#define	L2_BUCKET_SIZE	(1 << L2_BUCKET_LOG2)

/*
 * Given the above "L2-descriptors-per-l2_dtable" constant, the number
 * of l2_dtable structures required to track all possible page descriptors
 * mappable by an L1 translation table is given by the following constants:
 */
#define	L2_LOG2		((32 - L1_S_SHIFT) - L2_BUCKET_LOG2)
#define	L2_SIZE		(1 << L2_LOG2)

/*
 * tell MI code that the cache is virtually-indexed.
 * ARMv6 is physically-tagged but all others are virtually-tagged.
 */
#if (ARM_MMU_V6 + ARM_MMU_V7) > 0
#define PMAP_CACHE_VIPT
#else
#define PMAP_CACHE_VIVT
#endif

#ifndef _LOCORE

struct l1_ttable;
struct l2_dtable;

/*
 * Track cache/tlb occupancy using the following structure
 */
union pmap_cache_state {
	struct {
		union {
			u_int8_t csu_cache_b[2];
			u_int16_t csu_cache;
		} cs_cache_u;

		union {
			u_int8_t csu_tlb_b[2];
			u_int16_t csu_tlb;
		} cs_tlb_u;
	} cs_s;
	u_int32_t cs_all;
};
#define	cs_cache_id	cs_s.cs_cache_u.csu_cache_b[0]
#define	cs_cache_d	cs_s.cs_cache_u.csu_cache_b[1]
#define	cs_cache	cs_s.cs_cache_u.csu_cache
#define	cs_tlb_id	cs_s.cs_tlb_u.csu_tlb_b[0]
#define	cs_tlb_d	cs_s.cs_tlb_u.csu_tlb_b[1]
#define	cs_tlb		cs_s.cs_tlb_u.csu_tlb

/*
 * Assigned to cs_all to force cacheops to work for a particular pmap
 */
#define	PMAP_CACHE_STATE_ALL	0xffffffffu

/*
 * This structure is used by machine-dependent code to describe
 * static mappings of devices, created at bootstrap time.
 */
struct pmap_devmap {
	vaddr_t		pd_va;		/* virtual address */
	paddr_t		pd_pa;		/* physical address */
	psize_t		pd_size;	/* size of region */
	vm_prot_t	pd_prot;	/* protection code */
	int		pd_cache;	/* cache attributes */
};

/*
 * The pmap structure itself
 */
struct pmap {
	u_int8_t		pm_domain;
	bool			pm_remove_all;
	bool			pm_activated;
	struct l1_ttable	*pm_l1;
	pd_entry_t		*pm_pl1vec;
	pd_entry_t		pm_l1vec;
	union pmap_cache_state	pm_cstate;
	struct uvm_object	pm_obj;
	kmutex_t		pm_obj_lock;
#define	pm_lock pm_obj.vmobjlock
	struct l2_dtable	*pm_l2[L2_SIZE];
	struct pmap_statistics	pm_stats;
	LIST_ENTRY(pmap)	pm_list;
};

/*
 * Physical / virtual address structure. In a number of places (particularly
 * during bootstrapping) we need to keep track of the physical and virtual
 * addresses of various pages
 */
typedef struct pv_addr {
	SLIST_ENTRY(pv_addr) pv_list;
	paddr_t pv_pa;
	vaddr_t pv_va;
	vsize_t pv_size;
} pv_addr_t;
typedef SLIST_HEAD(, pv_addr) pv_addrqh_t;

extern pv_addrqh_t pmap_freeq;
extern pv_addr_t kernelpages;
extern pv_addr_t systempage;
extern pv_addr_t kernel_l1pt;

/*
 * Determine various modes for PTEs (user vs. kernel, cacheable
 * vs. non-cacheable).
 */
#define	PTE_KERNEL	0
#define	PTE_USER	1
#define	PTE_NOCACHE	0
#define	PTE_CACHE	1
#define	PTE_PAGETABLE	2

/*
 * Flags that indicate attributes of pages or mappings of pages.
 *
 * The PVF_MOD and PVF_REF flags are stored in the mdpage for each
 * page.  PVF_WIRED, PVF_WRITE, and PVF_NC are kept in individual
 * pv_entry's for each page.  They live in the same "namespace" so
 * that we can clear multiple attributes at a time.
 *
 * Note the "non-cacheable" flag generally means the page has
 * multiple mappings in a given address space.
 */
#define	PVF_MOD		0x01		/* page is modified */
#define	PVF_REF		0x02		/* page is referenced */
#define	PVF_WIRED	0x04		/* mapping is wired */
#define	PVF_WRITE	0x08		/* mapping is writable */
#define	PVF_EXEC	0x10		/* mapping is executable */
#ifdef PMAP_CACHE_VIVT
#define	PVF_UNC		0x20		/* mapping is 'user' non-cacheable */
#define	PVF_KNC		0x40		/* mapping is 'kernel' non-cacheable */
#define	PVF_NC		(PVF_UNC|PVF_KNC)
#endif
#ifdef PMAP_CACHE_VIPT
#define	PVF_NC		0x20		/* mapping is 'kernel' non-cacheable */
#define	PVF_MULTCLR	0x40		/* mapping is multi-colored */
#endif
#define	PVF_COLORED	0x80		/* page has or had a color */
#define	PVF_KENTRY	0x0100		/* page entered via pmap_kenter_pa */
#define	PVF_KMPAGE	0x0200		/* page is used for kmem */
#define	PVF_DIRTY	0x0400		/* page may have dirty cache lines */
#define	PVF_KMOD	0x0800		/* unmanaged page is modified  */
#define	PVF_KWRITE	(PVF_KENTRY|PVF_WRITE)
#define	PVF_DMOD	(PVF_MOD|PVF_KMOD|PVF_KMPAGE)

/*
 * Commonly referenced structures
 */
extern int		pmap_debug_level; /* Only exists if PMAP_DEBUG */

/*
 * Macros that we need to export
 */
#define	pmap_resident_count(pmap)	((pmap)->pm_stats.resident_count)
#define	pmap_wired_count(pmap)		((pmap)->pm_stats.wired_count)

#define	pmap_is_modified(pg)	\
	(((pg)->mdpage.pvh_attrs & PVF_MOD) != 0)
#define	pmap_is_referenced(pg)	\
	(((pg)->mdpage.pvh_attrs & PVF_REF) != 0)
#define	pmap_is_page_colored_p(md)	\
	(((md)->pvh_attrs & PVF_COLORED) != 0)

#define	pmap_copy(dp, sp, da, l, sa)	/* nothing */

#define pmap_phys_address(ppn)		(arm_ptob((ppn)))
u_int arm32_mmap_flags(paddr_t);
#define ARM32_MMAP_WRITECOMBINE	0x40000000
#define ARM32_MMAP_CACHEABLE		0x20000000
#define pmap_mmap_flags(ppn)			arm32_mmap_flags(ppn)

/*
 * Functions that we need to export
 */
void	pmap_procwr(struct proc *, vaddr_t, int);
void	pmap_remove_all(pmap_t);
bool	pmap_extract(pmap_t, vaddr_t, paddr_t *);

#define	PMAP_NEED_PROCWR
#define PMAP_GROWKERNEL		/* turn on pmap_growkernel interface */
#define	PMAP_ENABLE_PMAP_KMPAGE	/* enable the PMAP_KMPAGE flag */

#if (ARM_MMU_V6 + ARM_MMU_V7) > 0
#define	PMAP_PREFER(hint, vap, sz, td)	pmap_prefer((hint), (vap), (td))
void	pmap_prefer(vaddr_t, vaddr_t *, int);
#endif

void	pmap_icache_sync_range(pmap_t, vaddr_t, vaddr_t);

/* Functions we use internally. */
#ifdef PMAP_STEAL_MEMORY
void	pmap_boot_pagealloc(psize_t, psize_t, psize_t, pv_addr_t *);
void	pmap_boot_pageadd(pv_addr_t *);
vaddr_t	pmap_steal_memory(vsize_t, vaddr_t *, vaddr_t *);
#endif
void	pmap_bootstrap(vaddr_t, vaddr_t);

void	pmap_do_remove(pmap_t, vaddr_t, vaddr_t, int);
int	pmap_fault_fixup(pmap_t, vaddr_t, vm_prot_t, int);
bool	pmap_get_pde_pte(pmap_t, vaddr_t, pd_entry_t **, pt_entry_t **);
bool	pmap_get_pde(pmap_t, vaddr_t, pd_entry_t **);
void	pmap_set_pcb_pagedir(pmap_t, struct pcb *);

void	pmap_debug(int);
void	pmap_postinit(void);

void	vector_page_setprot(int);

const struct pmap_devmap *pmap_devmap_find_pa(paddr_t, psize_t);
const struct pmap_devmap *pmap_devmap_find_va(vaddr_t, vsize_t);

/* Bootstrapping routines. */
void	pmap_map_section(vaddr_t, vaddr_t, paddr_t, int, int);
void	pmap_map_entry(vaddr_t, vaddr_t, paddr_t, int, int);
vsize_t	pmap_map_chunk(vaddr_t, vaddr_t, paddr_t, vsize_t, int, int);
void	pmap_link_l2pt(vaddr_t, vaddr_t, pv_addr_t *);
void	pmap_devmap_bootstrap(vaddr_t, const struct pmap_devmap *);
void	pmap_devmap_register(const struct pmap_devmap *);

/*
 * Special page zero routine for use by the idle loop (no cache cleans). 
 */
bool	pmap_pageidlezero(paddr_t);
#define PMAP_PAGEIDLEZERO(pa)	pmap_pageidlezero((pa))

/*
 * used by dumpsys to record the PA of the L1 table
 */
uint32_t pmap_kernel_L1_addr(void);
/*
 * The current top of kernel VM
 */
extern vaddr_t	pmap_curmaxkvaddr;

/*
 * Useful macros and constants 
 */

/* Virtual address to page table entry */
static inline pt_entry_t *
vtopte(vaddr_t va)
{
	pd_entry_t *pdep;
	pt_entry_t *ptep;

	if (pmap_get_pde_pte(pmap_kernel(), va, &pdep, &ptep) == false)
		return (NULL);
	return (ptep);
}

/*
 * Virtual address to physical address
 */
static inline paddr_t
vtophys(vaddr_t va)
{
	paddr_t pa;

	if (pmap_extract(pmap_kernel(), va, &pa) == false)
		return (0);	/* XXXSCW: Panic? */

	return (pa);
}

/*
 * The new pmap ensures that page-tables are always mapping Write-Thru.
 * Thus, on some platforms we can run fast and loose and avoid syncing PTEs
 * on every change.
 *
 * Unfortunately, not all CPUs have a write-through cache mode.  So we
 * define PMAP_NEEDS_PTE_SYNC for C code to conditionally do PTE syncs,
 * and if there is the chance for PTE syncs to be needed, we define
 * PMAP_INCLUDE_PTE_SYNC so e.g. assembly code can include (and run)
 * the code.
 */
extern int pmap_needs_pte_sync;
#if defined(_KERNEL_OPT)
/*
 * StrongARM SA-1 caches do not have a write-through mode.  So, on these,
 * we need to do PTE syncs.  If only SA-1 is configured, then evaluate
 * this at compile time.
 */
#if (ARM_MMU_SA1 + ARM_MMU_V6 + ARM_MMU_V7 != 0) && (ARM_NMMUS == 1) 
#define	PMAP_NEEDS_PTE_SYNC	1
#define	PMAP_INCLUDE_PTE_SYNC
#elif (ARM_MMU_SA1 == 0)
#define	PMAP_NEEDS_PTE_SYNC	0
#endif
#endif /* _KERNEL_OPT */

/*
 * Provide a fallback in case we were not able to determine it at
 * compile-time.
 */
#ifndef PMAP_NEEDS_PTE_SYNC
#define	PMAP_NEEDS_PTE_SYNC	pmap_needs_pte_sync
#define	PMAP_INCLUDE_PTE_SYNC
#endif

#define	PTE_SYNC(pte)							\
do {									\
	if (PMAP_NEEDS_PTE_SYNC)					\
		cpu_dcache_wb_range((vaddr_t)(pte), sizeof(pt_entry_t));\
} while (/*CONSTCOND*/0)

#define	PTE_SYNC_RANGE(pte, cnt)					\
do {									\
	if (PMAP_NEEDS_PTE_SYNC) {					\
		cpu_dcache_wb_range((vaddr_t)(pte),			\
		    (cnt) << 2); /* * sizeof(pt_entry_t) */		\
	}								\
} while (/*CONSTCOND*/0)

#define	l1pte_valid(pde)	((pde) != 0)
#define	l1pte_section_p(pde)	(((pde) & L1_TYPE_MASK) == L1_TYPE_S)
#define	l1pte_page_p(pde)	(((pde) & L1_TYPE_MASK) == L1_TYPE_C)
#define	l1pte_fpage_p(pde)	(((pde) & L1_TYPE_MASK) == L1_TYPE_F)

#define l2pte_index(v)		(((v) & L2_ADDR_BITS) >> L2_S_SHIFT)
#define	l2pte_valid(pte)	(((pte) & L2_TYPE_MASK) != L2_TYPE_INV)
#define	l2pte_pa(pte)		((pte) & L2_S_FRAME)
#define l2pte_minidata(pte)	(((pte) & \
				 (L2_B | L2_C | L2_XS_T_TEX(TEX_XSCALE_X)))\
				 == (L2_C | L2_XS_T_TEX(TEX_XSCALE_X)))

/* L1 and L2 page table macros */
#define pmap_pde_v(pde)		l1pte_valid(*(pde))
#define pmap_pde_section(pde)	l1pte_section_p(*(pde))
#define pmap_pde_page(pde)	l1pte_page_p(*(pde))
#define pmap_pde_fpage(pde)	l1pte_fpage_p(*(pde))

#define	pmap_pte_v(pte)		l2pte_valid(*(pte))
#define	pmap_pte_pa(pte)	l2pte_pa(*(pte))

/* Size of the kernel part of the L1 page table */
#define KERNEL_PD_SIZE	\
	(L1_TABLE_SIZE - (KERNEL_BASE >> L1_S_SHIFT) * sizeof(pd_entry_t))

/************************* ARM MMU configuration *****************************/

#if (ARM_MMU_GENERIC + ARM_MMU_SA1 + ARM_MMU_V6 + ARM_MMU_V7) != 0
void	pmap_copy_page_generic(paddr_t, paddr_t);
void	pmap_zero_page_generic(paddr_t);

void	pmap_pte_init_generic(void);
#if defined(CPU_ARM8)
void	pmap_pte_init_arm8(void);
#endif
#if defined(CPU_ARM9)
void	pmap_pte_init_arm9(void);
#endif /* CPU_ARM9 */
#if defined(CPU_ARM10)
void	pmap_pte_init_arm10(void);
#endif /* CPU_ARM10 */
#if defined(CPU_ARM11)
void	pmap_pte_init_arm11(void);
#endif /* CPU_ARM11 */
#if defined(CPU_ARM11MPCORE)
void	pmap_pte_init_arm11mpcore(void);
#endif
#endif /* (ARM_MMU_GENERIC + ARM_MMU_SA1) != 0 */

#if ARM_MMU_SA1 == 1
void	pmap_pte_init_sa1(void);
#endif /* ARM_MMU_SA1 == 1 */

#if ARM_MMU_XSCALE == 1
void	pmap_copy_page_xscale(paddr_t, paddr_t);
void	pmap_zero_page_xscale(paddr_t);

void	pmap_pte_init_xscale(void);

void	xscale_setup_minidata(vaddr_t, vaddr_t, paddr_t);

#define	PMAP_UAREA(va)		pmap_uarea(va)
void	pmap_uarea(vaddr_t);
#endif /* ARM_MMU_XSCALE == 1 */

#if ARM_MMU_V7 == 1
void	pmap_pte_init_armv7(void);
#endif /* ARM_MMU_V7 */

extern pt_entry_t		pte_l1_s_cache_mode;
extern pt_entry_t		pte_l1_s_cache_mask;

extern pt_entry_t		pte_l2_l_cache_mode;
extern pt_entry_t		pte_l2_l_cache_mask;

extern pt_entry_t		pte_l2_s_cache_mode;
extern pt_entry_t		pte_l2_s_cache_mask;

extern pt_entry_t		pte_l1_s_cache_mode_pt;
extern pt_entry_t		pte_l2_l_cache_mode_pt;
extern pt_entry_t		pte_l2_s_cache_mode_pt;

extern pt_entry_t		pte_l1_s_wc_mode;
extern pt_entry_t		pte_l2_l_wc_mode;
extern pt_entry_t		pte_l2_s_wc_mode;

extern pt_entry_t		pte_l1_s_prot_u;
extern pt_entry_t		pte_l1_s_prot_w;
extern pt_entry_t		pte_l1_s_prot_ro;
extern pt_entry_t		pte_l1_s_prot_mask;

extern pt_entry_t		pte_l2_s_prot_u;
extern pt_entry_t		pte_l2_s_prot_w;
extern pt_entry_t		pte_l2_s_prot_ro;
extern pt_entry_t		pte_l2_s_prot_mask;

extern pt_entry_t		pte_l2_l_prot_u;
extern pt_entry_t		pte_l2_l_prot_w;
extern pt_entry_t		pte_l2_l_prot_ro;
extern pt_entry_t		pte_l2_l_prot_mask;

extern pt_entry_t		pte_l1_s_proto;
extern pt_entry_t		pte_l1_c_proto;
extern pt_entry_t		pte_l2_s_proto;

extern void (*pmap_copy_page_func)(paddr_t, paddr_t);
extern void (*pmap_zero_page_func)(paddr_t);

#endif /* !_LOCORE */

/*****************************************************************************/

/*
 * Definitions for MMU domains
 */
#define	PMAP_DOMAINS		15	/* 15 'user' domains (0-14) */
#define	PMAP_DOMAIN_KERNEL	15	/* The kernel uses domain #15 */

/*
 * These macros define the various bit masks in the PTE.
 *
 * We use these macros since we use different bits on different processor
 * models.
 */
#define	L1_S_PROT_U_generic	(L1_S_AP(AP_U))
#define	L1_S_PROT_W_generic	(L1_S_AP(AP_W))
#define	L1_S_PROT_RO_generic	(0)
#define	L1_S_PROT_MASK_generic	(L1_S_PROT_U|L1_S_PROT_W|L1_S_PROT_RO)

#define	L1_S_PROT_U_xscale	(L1_S_AP(AP_U))
#define	L1_S_PROT_W_xscale	(L1_S_AP(AP_W))
#define	L1_S_PROT_RO_xscale	(0)
#define	L1_S_PROT_MASK_xscale	(L1_S_PROT_U|L1_S_PROT_W|L1_S_PROT_RO)

#define	L1_S_PROT_U_armv6	(L1_S_AP(AP_R) | L1_S_AP(AP_U))
#define	L1_S_PROT_W_armv6	(L1_S_AP(AP_W))
#define	L1_S_PROT_RO_armv6	(L1_S_AP(AP_R) | L1_S_AP(AP_RO))
#define	L1_S_PROT_MASK_armv6	(L1_S_PROT_U|L1_S_PROT_W|L1_S_PROT_RO)

#define	L1_S_PROT_U_armv7	(L1_S_AP(AP_R) | L1_S_AP(AP_U))
#define	L1_S_PROT_W_armv7	(L1_S_AP(AP_W))
#define	L1_S_PROT_RO_armv7	(L1_S_AP(AP_R) | L1_S_AP(AP_RO))
#define	L1_S_PROT_MASK_armv7	(L1_S_PROT_U|L1_S_PROT_W|L1_S_PROT_RO)

#define	L1_S_CACHE_MASK_generic	(L1_S_B|L1_S_C)
#define	L1_S_CACHE_MASK_xscale	(L1_S_B|L1_S_C|L1_S_XS_TEX(TEX_XSCALE_X))
#define	L1_S_CACHE_MASK_armv6	(L1_S_B|L1_S_C|L1_S_XS_TEX(TEX_ARMV6_TEX))
#define	L1_S_CACHE_MASK_armv7	(L1_S_B|L1_S_C)

#define	L2_L_PROT_U_generic	(L2_AP(AP_U))
#define	L2_L_PROT_W_generic	(L2_AP(AP_W))
#define	L2_L_PROT_RO_generic	(0)
#define	L2_L_PROT_MASK_generic	(L2_L_PROT_U|L2_L_PROT_W|L2_L_PROT_RO)

#define	L2_L_PROT_U_xscale	(L2_AP(AP_U))
#define	L2_L_PROT_W_xscale	(L2_AP(AP_W))
#define	L2_L_PROT_RO_xscale	(0)
#define	L2_L_PROT_MASK_xscale	(L2_L_PROT_U|L2_L_PROT_W|L2_L_PROT_RO)

#define	L2_L_PROT_U_armv6n	(L2_AP0(AP_R) | L2_AP0(AP_U))
#define	L2_L_PROT_W_armv6n	(L2_AP0(AP_W))
#define	L2_L_PROT_RO_armv6n	(L2_AP0(AP_R) | L2_AP0(AP_RO))
#define	L2_L_PROT_MASK_armv6n	(L2_L_PROT_U|L2_L_PROT_W|L2_L_PROT_RO)

#define	L2_L_PROT_U_armv7	(L2_AP0(AP_R) | L2_AP0(AP_U))
#define	L2_L_PROT_W_armv7	(L2_AP0(AP_W))
#define	L2_L_PROT_RO_armv7	(L2_AP0(AP_R) | L2_AP0(AP_RO))
#define	L2_L_PROT_MASK_armv7	(L2_L_PROT_U|L2_L_PROT_W|L2_L_PROT_RO)

#define	L2_L_CACHE_MASK_generic	(L2_B|L2_C)
#define	L2_L_CACHE_MASK_xscale	(L2_B|L2_C|L2_XS_L_TEX(TEX_XSCALE_X))
#define	L2_L_CACHE_MASK_armv6	(L2_B|L2_C|L2_V6_L_TEX(TEX_ARMV6_TEX))
#define	L2_L_CACHE_MASK_armv7	(L2_B|L2_C)

#define	L2_S_PROT_U_generic	(L2_AP(AP_U))
#define	L2_S_PROT_W_generic	(L2_AP(AP_W))
#define	L2_S_PROT_RO_generic	(0)
#define	L2_S_PROT_MASK_generic	(L2_S_PROT_U|L2_S_PROT_W|L2_S_PROT_RO)

#define	L2_S_PROT_U_xscale	(L2_AP0(AP_U))
#define	L2_S_PROT_W_xscale	(L2_AP0(AP_W))
#define	L2_S_PROT_RO_xscale	(0)
#define	L2_S_PROT_MASK_xscale	(L2_S_PROT_U|L2_S_PROT_W|L2_S_PROT_RO)

#define	L2_S_PROT_U_armv6n	(L2_AP0(AP_R) | L2_AP0(AP_U))
#define	L2_S_PROT_W_armv6n	(L2_AP0(AP_W))
#define	L2_S_PROT_RO_armv6n	(L2_AP0(AP_R) | L2_AP0(AP_RO))
#define	L2_S_PROT_MASK_armv6n	(L2_S_PROT_U|L2_S_PROT_W|L2_S_PROT_RO)

#define	L2_S_PROT_U_armv7	(L2_AP0(AP_R) | L2_AP0(AP_U))
#define	L2_S_PROT_W_armv7	(L2_AP0(AP_W))
#define	L2_S_PROT_RO_armv7	(L2_AP0(AP_R) | L2_AP0(AP_RO))
#define	L2_S_PROT_MASK_armv7	(L2_S_PROT_U|L2_S_PROT_W|L2_S_PROT_RO)

#define	L2_S_CACHE_MASK_generic	(L2_B|L2_C)
#define	L2_S_CACHE_MASK_xscale	(L2_B|L2_C|L2_XS_T_TEX(TEX_XSCALE_X))
#define	L2_XS_CACHE_MASK_armv6	(L2_B|L2_C|L2_V6_XS_TEX(TEX_ARMV6_TEX))
#define	L2_S_CACHE_MASK_armv6n	L2_XS_CACHE_MASK_armv6
#ifdef	ARMV6_EXTENDED_SMALL_PAGE
#define	L2_S_CACHE_MASK_armv6c	L2_XS_CACHE_MASK_armv6
#else
#define	L2_S_CACHE_MASK_armv6c	L2_S_CACHE_MASK_generic
#endif
#define	L2_S_CACHE_MASK_armv7	(L2_B|L2_C)


#define	L1_S_PROTO_generic	(L1_TYPE_S | L1_S_IMP)
#define	L1_S_PROTO_xscale	(L1_TYPE_S)
#define	L1_S_PROTO_armv6	(L1_TYPE_S)
#define	L1_S_PROTO_armv7	(L1_TYPE_S)

#define	L1_C_PROTO_generic	(L1_TYPE_C | L1_C_IMP2)
#define	L1_C_PROTO_xscale	(L1_TYPE_C)
#define	L1_C_PROTO_armv6	(L1_TYPE_C)
#define	L1_C_PROTO_armv7	(L1_TYPE_C)

#define	L2_L_PROTO		(L2_TYPE_L)

#define	L2_S_PROTO_generic	(L2_TYPE_S)
#define	L2_S_PROTO_xscale	(L2_TYPE_XS)
#ifdef	ARMV6_EXTENDED_SMALL_PAGE
#define	L2_S_PROTO_armv6c	(L2_TYPE_XS)    /* XP=0, extended small page */
#else
#define	L2_S_PROTO_armv6c	(L2_TYPE_S)	/* XP=0, subpage APs */
#endif
#define	L2_S_PROTO_armv6n	(L2_TYPE_S)	/* with XP=1 */
#define	L2_S_PROTO_armv7	(L2_TYPE_S)

/*
 * User-visible names for the ones that vary with MMU class.
 */

#if ARM_NMMUS > 1
/* More than one MMU class configured; use variables. */
#define	L1_S_PROT_U		pte_l1_s_prot_u
#define	L1_S_PROT_W		pte_l1_s_prot_w
#define	L1_S_PROT_RO		pte_l1_s_prot_ro
#define	L1_S_PROT_MASK		pte_l1_s_prot_mask

#define	L2_S_PROT_U		pte_l2_s_prot_u
#define	L2_S_PROT_W		pte_l2_s_prot_w
#define	L2_S_PROT_RO		pte_l2_s_prot_ro
#define	L2_S_PROT_MASK		pte_l2_s_prot_mask

#define	L2_L_PROT_U		pte_l2_l_prot_u
#define	L2_L_PROT_W		pte_l2_l_prot_w
#define	L2_L_PROT_RO		pte_l2_l_prot_ro
#define	L2_L_PROT_MASK		pte_l2_l_prot_mask

#define	L1_S_CACHE_MASK		pte_l1_s_cache_mask
#define	L2_L_CACHE_MASK		pte_l2_l_cache_mask
#define	L2_S_CACHE_MASK		pte_l2_s_cache_mask

#define	L1_S_PROTO		pte_l1_s_proto
#define	L1_C_PROTO		pte_l1_c_proto
#define	L2_S_PROTO		pte_l2_s_proto

#define	pmap_copy_page(s, d)	(*pmap_copy_page_func)((s), (d))
#define	pmap_zero_page(d)	(*pmap_zero_page_func)((d))
#elif (ARM_MMU_GENERIC + ARM_MMU_SA1) != 0
#define	L1_S_PROT_U		L1_S_PROT_U_generic
#define	L1_S_PROT_W		L1_S_PROT_W_generic
#define	L1_S_PROT_RO		L1_S_PROT_RO_generic
#define	L1_S_PROT_MASK		L1_S_PROT_MASK_generic

#define	L2_S_PROT_U		L2_S_PROT_U_generic
#define	L2_S_PROT_W		L2_S_PROT_W_generic
#define	L2_S_PROT_RO		L2_S_PROT_RO_generic
#define	L2_S_PROT_MASK		L2_S_PROT_MASK_generic

#define	L2_L_PROT_U		L2_L_PROT_U_generic
#define	L2_L_PROT_W		L2_L_PROT_W_generic
#define	L2_L_PROT_RO		L2_L_PROT_RO_generic
#define	L2_L_PROT_MASK		L2_L_PROT_MASK_generic

#define	L1_S_CACHE_MASK		L1_S_CACHE_MASK_generic
#define	L2_L_CACHE_MASK		L2_L_CACHE_MASK_generic
#define	L2_S_CACHE_MASK		L2_S_CACHE_MASK_generic

#define	L1_S_PROTO		L1_S_PROTO_generic
#define	L1_C_PROTO		L1_C_PROTO_generic
#define	L2_S_PROTO		L2_S_PROTO_generic

#define	pmap_copy_page(s, d)	pmap_copy_page_generic((s), (d))
#define	pmap_zero_page(d)	pmap_zero_page_generic((d))
#elif ARM_MMU_V6N != 0
#define	L1_S_PROT_U		L1_S_PROT_U_armv6
#define	L1_S_PROT_W		L1_S_PROT_W_armv6
#define	L1_S_PROT_RO		L1_S_PROT_RO_armv6
#define	L1_S_PROT_MASK		L1_S_PROT_MASK_armv6

#define	L2_S_PROT_U		L2_S_PROT_U_armv6n
#define	L2_S_PROT_W		L2_S_PROT_W_armv6n
#define	L2_S_PROT_RO		L2_S_PROT_RO_armv6n
#define	L2_S_PROT_MASK		L2_S_PROT_MASK_armv6n

#define	L2_L_PROT_U		L2_L_PROT_U_armv6n
#define	L2_L_PROT_W		L2_L_PROT_W_armv6n
#define	L2_L_PROT_RO		L2_L_PROT_RO_armv6n
#define	L2_L_PROT_MASK		L2_L_PROT_MASK_armv6n

#define	L1_S_CACHE_MASK		L1_S_CACHE_MASK_armv6
#define	L2_L_CACHE_MASK		L2_L_CACHE_MASK_armv6
#define	L2_S_CACHE_MASK		L2_S_CACHE_MASK_armv6n

/* These prototypes make writeable mappings, while the other MMU types
 * make read-only mappings. */
#define	L1_S_PROTO		L1_S_PROTO_armv6
#define	L1_C_PROTO		L1_C_PROTO_armv6
#define	L2_S_PROTO		L2_S_PROTO_armv6n

#define	pmap_copy_page(s, d)	pmap_copy_page_generic((s), (d))
#define	pmap_zero_page(d)	pmap_zero_page_generic((d))
#elif ARM_MMU_V6C != 0
#define	L1_S_PROT_U		L1_S_PROT_U_generic
#define	L1_S_PROT_W		L1_S_PROT_W_generic
#define	L1_S_PROT_RO		L1_S_PROT_RO_generic
#define	L1_S_PROT_MASK		L1_S_PROT_MASK_generic

#define	L2_S_PROT_U		L2_S_PROT_U_generic
#define	L2_S_PROT_W		L2_S_PROT_W_generic
#define	L2_S_PROT_RO		L2_S_PROT_RO_generic
#define	L2_S_PROT_MASK		L2_S_PROT_MASK_generic

#define	L2_L_PROT_U		L2_L_PROT_U_generic
#define	L2_L_PROT_W		L2_L_PROT_W_generic
#define	L2_L_PROT_RO		L2_L_PROT_RO_generic
#define	L2_L_PROT_MASK		L2_L_PROT_MASK_generic

#define	L1_S_CACHE_MASK		L1_S_CACHE_MASK_generic
#define	L2_L_CACHE_MASK		L2_L_CACHE_MASK_generic
#define	L2_S_CACHE_MASK		L2_S_CACHE_MASK_generic

#define	L1_S_PROTO		L1_S_PROTO_generic
#define	L1_C_PROTO		L1_C_PROTO_generic
#define	L2_S_PROTO		L2_S_PROTO_generic

#define	pmap_copy_page(s, d)	pmap_copy_page_generic((s), (d))
#define	pmap_zero_page(d)	pmap_zero_page_generic((d))
#elif ARM_MMU_XSCALE == 1
#define	L1_S_PROT_U		L1_S_PROT_U_generic
#define	L1_S_PROT_W		L1_S_PROT_W_generic
#define	L1_S_PROT_RO		L1_S_PROT_RO_generic
#define	L1_S_PROT_MASK		L1_S_PROT_MASK_generic

#define	L2_S_PROT_U		L2_S_PROT_U_xscale
#define	L2_S_PROT_W		L2_S_PROT_W_xscale
#define	L2_S_PROT_RO		L2_S_PROT_RO_xscale
#define	L2_S_PROT_MASK		L2_S_PROT_MASK_xscale

#define	L2_L_PROT_U		L2_L_PROT_U_generic
#define	L2_L_PROT_W		L2_L_PROT_W_generic
#define	L2_L_PROT_RO		L2_L_PROT_RO_generic
#define	L2_L_PROT_MASK		L2_L_PROT_MASK_generic

#define	L1_S_CACHE_MASK		L1_S_CACHE_MASK_xscale
#define	L2_L_CACHE_MASK		L2_L_CACHE_MASK_xscale
#define	L2_S_CACHE_MASK		L2_S_CACHE_MASK_xscale

#define	L1_S_PROTO		L1_S_PROTO_xscale
#define	L1_C_PROTO		L1_C_PROTO_xscale
#define	L2_S_PROTO		L2_S_PROTO_xscale

#define	pmap_copy_page(s, d)	pmap_copy_page_xscale((s), (d))
#define	pmap_zero_page(d)	pmap_zero_page_xscale((d))
#elif ARM_MMU_V7 == 1
#define	L1_S_PROT_U		L1_S_PROT_U_armv7
#define	L1_S_PROT_W		L1_S_PROT_W_armv7
#define	L1_S_PROT_RO		L1_S_PROT_RO_armv7
#define	L1_S_PROT_MASK		L1_S_PROT_MASK_armv7

#define	L2_S_PROT_U		L2_S_PROT_U_armv7
#define	L2_S_PROT_W		L2_S_PROT_W_armv7
#define	L2_S_PROT_RO		L2_S_PROT_RO_armv7
#define	L2_S_PROT_MASK		L2_S_PROT_MASK_armv7

#define	L2_L_PROT_U		L2_L_PROT_U_armv7
#define	L2_L_PROT_W		L2_L_PROT_W_armv7
#define	L2_L_PROT_RO		L2_L_PROT_RO_armv7
#define	L2_L_PROT_MASK		L2_L_PROT_MASK_armv7

#define	L1_S_CACHE_MASK		L1_S_CACHE_MASK_armv7
#define	L2_L_CACHE_MASK		L2_L_CACHE_MASK_armv7
#define	L2_S_CACHE_MASK		L2_S_CACHE_MASK_armv7

/* These prototypes make writeable mappings, while the other MMU types
 * make read-only mappings. */
#define	L1_S_PROTO		L1_S_PROTO_armv7
#define	L1_C_PROTO		L1_C_PROTO_armv7
#define	L2_S_PROTO		L2_S_PROTO_armv7

#define	pmap_copy_page(s, d)	pmap_copy_page_generic((s), (d))
#define	pmap_zero_page(d)	pmap_zero_page_generic((d))
#endif /* ARM_NMMUS > 1 */

/*
 * Macros to set and query the write permission on page descriptors.
 */
#define l1pte_set_writable(pte)	(((pte) & ~L1_S_PROT_RO) | L1_S_PROT_W)
#define l1pte_set_readonly(pte)	(((pte) & ~L1_S_PROT_W) | L1_S_PROT_RO)
#define l2pte_set_writable(pte)	(((pte) & ~L2_S_PROT_RO) | L2_S_PROT_W)
#define l2pte_set_readonly(pte)	(((pte) & ~L2_S_PROT_W) | L2_S_PROT_RO)

#define l2pte_writable_p(pte)	(((pte) & L2_S_PROT_W) == L2_S_PROT_W && \
				 (L2_S_PROT_RO == 0 || \
				  ((pte) & L2_S_PROT_RO) != L2_S_PROT_RO))

/*
 * These macros return various bits based on kernel/user and protection.
 * Note that the compiler will usually fold these at compile time.
 */
#define	L1_S_PROT(ku, pr)	((((ku) == PTE_USER) ? L1_S_PROT_U : 0) | \
				 (((pr) & VM_PROT_WRITE) ? L1_S_PROT_W : L1_S_PROT_RO))

#define	L2_L_PROT(ku, pr)	((((ku) == PTE_USER) ? L2_L_PROT_U : 0) | \
				 (((pr) & VM_PROT_WRITE) ? L2_L_PROT_W : L2_L_PROT_RO))

#define	L2_S_PROT(ku, pr)	((((ku) == PTE_USER) ? L2_S_PROT_U : 0) | \
				 (((pr) & VM_PROT_WRITE) ? L2_S_PROT_W : L2_S_PROT_RO))

/*
 * Macros to test if a mapping is mappable with an L1 Section mapping
 * or an L2 Large Page mapping.
 */
#define	L1_S_MAPPABLE_P(va, pa, size)					\
	((((va) | (pa)) & L1_S_OFFSET) == 0 && (size) >= L1_S_SIZE)

#define	L2_L_MAPPABLE_P(va, pa, size)					\
	((((va) | (pa)) & L2_L_OFFSET) == 0 && (size) >= L2_L_SIZE)

/*
 * Hooks for the pool allocator.
 */
#define	POOL_VTOPHYS(va)	vtophys((vaddr_t) (va))

#ifndef _LOCORE

/*
 * pmap-specific data store in the vm_page structure.
 */
#define	__HAVE_VM_PAGE_MD
struct vm_page_md {
	SLIST_HEAD(,pv_entry) pvh_list;		/* pv_entry list */
	int pvh_attrs;				/* page attributes */
	u_int uro_mappings;
	u_int urw_mappings;
	union {
		u_short s_mappings[2];	/* Assume kernel count <= 65535 */
		u_int i_mappings;
	} k_u;
#define	kro_mappings	k_u.s_mappings[0]
#define	krw_mappings	k_u.s_mappings[1]
#define	k_mappings	k_u.i_mappings
};

/*
 * Set the default color of each page.
 */
#if ARM_MMU_V6 > 0
#define	VM_MDPAGE_PVH_ATTRS_INIT(pg) \
	(pg)->mdpage.pvh_attrs = (pg)->phys_addr & arm_cache_prefer_mask
#else
#define	VM_MDPAGE_PVH_ATTRS_INIT(pg) \
	(pg)->mdpage.pvh_attrs = 0
#endif
 
#define	VM_MDPAGE_INIT(pg)						\
do {									\
	SLIST_INIT(&(pg)->mdpage.pvh_list);				\
	VM_MDPAGE_PVH_ATTRS_INIT(pg);					\
	(pg)->mdpage.uro_mappings = 0;					\
	(pg)->mdpage.urw_mappings = 0;					\
	(pg)->mdpage.k_mappings = 0;					\
} while (/*CONSTCOND*/0)

#endif /* !_LOCORE */

#endif /* _KERNEL */

#endif	/* _ARM32_PMAP_H_ */
