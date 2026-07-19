/*	$NetBSD: pmap_pgmmu.h,v 1.1 2026/07/19 01:48:24 thorpej Exp $	*/

/*-     
 * Copyright (c) 2025, 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */             

#ifndef _PG68K_PMAP_PGMMU_H_
#define	_PG68K_PMAP_PGMMU_H_

#if defined(_KERNEL)

#if !defined(_MODULE)

#include <sys/queue.h>

#include <hb68k/pg68k/pgmmu.h>

#include <sys/kcore.h>
#include <m68k/kcore.h>

/*
 * Description of a pmap's MMU context.  In addition to the context
 * number, we also keep track of how many mappings we have in each
 * PMEG we have allocated so we know when we can free them back.
 *
 * The number of these context structures is limited to the total
 * number of contexts the MMU can handle.
 */
struct pmap_context {
	unsigned int ctx_num;
	union {
		struct pmap_context *ctx_next;
		uint8_t ctx_segrefs[PGMMU_NUM_SEGS];
	};
};

/*
 * This MMU is used in relatively resource-constrained machines,
 * so we aim to keep this structure as small as possible.
 */
struct pmap {
	TAILQ_ENTRY(pmap)      pm_list;		/* link on global list */
	struct pmap_context   *pm_context;	/* current context */
	uint16_t               pm_refcnt;	/* reference count */
	uint16_t               pm_busy;		/* "busy" (currently loaded in
						   to MMU, or involved in
						   some pmap operation) count */
	/*
	 * N.B. we also used wired_count internally to identify
	 * potential victims of resource theft.
	 */
	struct pmap_statistics pm_stats;	/* statistics */
};

/*
 * One entry per P->V mapping of a managed page.
 *
 * N.B. We want to keep this structure's size to be a multiple of
 * 8; we want to align them to 8 bytes in order to be able to use
 * the lower 3 bits of the pv_entry list head for page attributes.
 */
struct pv_entry {
/* 0*/	struct pv_entry *pv_next;	/* link on page list */
/* 4*/	pmap_t           pv_pmap;	/* pmap that contains mapping */
/* 8*/	vaddr_t          pv_vf;		/* virtual address + flags */
/*12*/	unsigned int     pv_pmeg;	/* PMEG that maps this VA */
/*16*/
};

/* Upper bits of pv_vf contain the virtual address */
#define	PV_VA(pv)	((pv)->pv_vf & ~PAGE_MASK)

/*
 * pmap-specific data store in the vm_page structure.
 *
 * We keep the Mod/Ref attrs in the lower 2 bits of the list head
 * pointer.  This is possible because both the Mod and Ref bits are
 * adjacent; we just need to shift them down 24 bit positions.
 *
 * Assumes that PV entries will be 4-byte aligned, but the allocator
 * guarantees this for us.
 */
#define	__HAVE_VM_PAGE_MD
struct vm_page_md {
	uintptr_t pvh_listx;		/* pv_entry list + attrs */
};

#define	PVH_MR_SHIFT	24
#define	PVH_MR_MASK	__BITS(0,1)
#define	PVH_ATTR_MASK	PVH_MR_MASK
#define	PVH_PV_MASK	(~PVH_ATTR_MASK)

#define	VM_MDPAGE_INIT(pg)					\
do {								\
	(pg)->mdpage.pvh_listx = 0;				\
} while (/*CONSTCOND*/0)

#define	VM_MDPAGE_PVS(pg)					\
	((struct pv_entry *)((pg)->mdpage.pvh_listx & (uintptr_t)PVH_PV_MASK))

#define	VM_MDPAGE_HEAD_PVP(pg)					\
	((struct pv_entry **)&(pg)->mdpage.pvh_listx)

#define	VM_MDPAGE_SETPVP(pvp, pv)				\
do {								\
	/*							\
	 * The page attributes are in the lower two bits of	\
	 * the first PV pointer.  Rather than comparing the	\
	 * address and branching, we just always preserve what	\
	 * might be there (either the attribute bits or zero	\
	 * bits).						\
	 */							\
	*(pvp) = (struct pv_entry *)				\
	    ((uintptr_t)(pv) |					\
	     (((uintptr_t)(*(pvp))) & (uintptr_t)PVH_ATTR_MASK));\
} while (/*CONSTCOND*/0)

#define	VM_MDPAGE_MR(pg)					\
	(((pg)->mdpage.pvh_listx & PVH_MR_MASK) << PVH_MR_SHIFT)

#define	VM_MDPAGE_ADD_MR(pg, a)					\
do {								\
	(pg)->mdpage.pvh_listx |=				\
	    ((a) >> PVH_MR_SHIFT) & PVH_MR_MASK;		\
} while (/*CONSTCOND*/0)

#define	VM_MDPAGE_SET_MR(pg, v)					\
do {								\
	(pg)->mdpage.pvh_listx =				\
	    ((pg)->mdpage.pvh_listx & ~PVH_MR_MASK) |		\
	    (((v) >> PVH_MR_SHIFT) & PVH_MR_MASK);		\
} while (/*CONSTCOND*/0)

#define	pmap_copy(dp, sp, da, l, sa)	__nothing

#define	pmap_resident_count(pmap)	((pmap)->pm_stats.resident_count)
#define	pmap_wired_count(pmap)		((pmap)->pm_stats.wired_count)

/*
 * pmap_bootstrap1() is called before the MMU is turned on (well, for
 * this MMU, it's always on...)
 * pmap_bootstrap2() is called after.
 */
paddr_t	pmap_bootstrap1(paddr_t/*nextpa*/, paddr_t/*reloff*/);
void *	pmap_bootstrap2(void);

/*
 * Variant of pmap_extract() that returns additional information about
 * the mapping.  Used by bus_dma(9).
 */
bool	pmap_extract_info(pmap_t, vaddr_t, paddr_t *, int *);

/*
 * pmap hook for trap()'s page fault handler.  We don't need to
 * do anything special, so it just goes to uvm_fault().
 */
#define	pmap_fault(m, v, t)	uvm_fault((m), (v), (t))

/* 
 * Functions to determine if a VA/PA range is a static mapping created
 * by pmap_bootstrap1(), for the benefit of bus_space(9).
 */
bool	pmap_pa_has_static_mapping(paddr_t, size_t, vm_prot_t,
	    vaddr_t *, int *);
bool	pmap_va_is_static_mapping(vaddr_t, size_t);

/* Kernel debugger support functions. */
struct pmap_db_write_text_context {
	unsigned int	pmeidx;
	pm_entry_t	opme;
};
bool	pmap_db_write_text_enter(vaddr_t, struct pmap_db_write_text_context *);
void	pmap_db_write_text_exit(struct pmap_db_write_text_context *);

/*
 * Functions exported for compatibility with the Hibler pmap, where
 * these are needed by other shared m68k code.
 *
 * XXX Clean this up eventually.
 */

paddr_t		vtophys(vaddr_t);

extern char *	vmmap;
extern void *	msgbufaddr;

/* Kernel crash dump support. */
phys_ram_seg_t *	pmap_init_kcore_hdr(cpu_kcore_hdr_t *);

/*
 * pmap_bootstrap1() may need to relocate global references, and perform
 * VA <-> PA conversions.  We don't need to do anything for these accesses
 * on this MMU, because the MMU is always on.
 */
#undef PMAP_BOOTSTRAP_RELOC_GLOB
#define	PMAP_BOOTSTRAP_RELOC_GLOB(va)	(va)

#undef PMAP_BOOTSTRAP_RELOC_PA
#define	PMAP_BOOTSTRAP_RELOC_PA(pa)	(pa)

#undef PMAP_BOOTSTRAP_VA_TO_PA
#define	PMAP_BOOTSTRAP_VA_TO_PA(va)	(va)

#undef PMAP_BOOTSTRAP_PA_TO_VA
#define	PMAP_BOOTSTRAP_PA_TO_VA(pa)	(pa)

#endif /* _MODULE */

/*
 * Some pmap(9) API macros should be defined here for module(7).
 * Luckily, all m68k pmap implementations behave this way. (see PR/54869)
 */
#define	pmap_update(pmap)		__nothing

#endif /* _KERNEL */

#endif /* _PG68K_PMAP_PGMMU_H_ */
