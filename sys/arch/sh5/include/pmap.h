/*	$NetBSD: pmap.h,v 1.16 2003/04/01 10:25:09 scw Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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

#ifndef	_SH5_PMAP_H
#define	_SH5_PMAP_H

#include <sh5/pte.h>

#if defined(_KERNEL)

/*
 * PMAP flags exported to UVM
 */
#define	PMAP_CACHE_VIVT		/* The insn cache is VIVT */
/*#define PMAP_PREFER*/		/* XXX: Not yet */

/*
 * Assume 512MB of KSEG1 KVA, but allow this to be over-ridden
 * if necessary.
 */
#ifndef	KERNEL_IPT_SIZE
#define	KERNEL_IPT_SIZE	(SH5_KSEG1_SIZE / NBPG)
#endif

struct pmap {
	int pm_refs;		/* pmap reference count */
	u_int pm_asid;		/* ASID for this pmap */
	u_int pm_asidgen;	/* ASID Generation number */
	vsid_t pm_vsid;		/* This pmap's vsid */
	struct pmap_statistics pm_stats;
};

#define	PMAP_ASID_UNASSIGNED	((u_int)(-1))
#define	PMAP_ASID_KERNEL	0
#define	PMAP_ASID_CACHEOPS	1
#define	PMAP_ASID_USER_START	2

typedef struct pmap *pmap_t;

#define	PMAP_NC		0x1000
#define	PMAP_UNMANAGED	0x2000

extern struct pmap kernel_pmap_store;
#define	pmap_kernel()	(&kernel_pmap_store)

extern int pmap_write_trap(struct proc *, int, vaddr_t);
extern boolean_t pmap_clear_bit(struct vm_page *, ptel_t);
extern boolean_t pmap_query_bit(struct vm_page *, ptel_t);

extern vaddr_t pmap_map_poolpage(paddr_t);
extern paddr_t pmap_unmap_poolpage(vaddr_t);
#define	PMAP_MAP_POOLPAGE(p)	pmap_map_poolpage((p))
#define	PMAP_UNMAP_POOLPAGE(v)	pmap_unmap_poolpage((v))

#define	PMAP_STEAL_MEMORY
extern vaddr_t pmap_steal_memory(vsize_t, vaddr_t *, vaddr_t *);

#define pmap_clear_modify(pg)		(pmap_clear_bit((pg), SH5_PTEL_M))
#define	pmap_clear_reference(pg)	(pmap_clear_bit((pg), SH5_PTEL_R))
#define	pmap_is_modified(pg)		(pmap_query_bit((pg), SH5_PTEL_M))
#define	pmap_is_referenced(pg)		(pmap_query_bit((pg), SH5_PTEL_R))

#define	pmap_resident_count(pm)		((pm)->pm_stats.resident_count)
#define	pmap_wired_count(pm)		((pm)->pm_stats.wired_count)

#define	pmap_phys_address(x)		(x)

static __inline void
pmap_remove_all(struct pmap *pmap)
{
	/* Nothing. */
}

/* Private pmap data and functions */
struct mem_region;
extern void	pmap_bootstrap(vaddr_t, paddr_t, struct mem_region *);
extern int	pmap_initialized;
extern u_int	pmap_ipt_hash(vsid_t vsid, vaddr_t va);  /* See exception.S */
extern vaddr_t	pmap_map_device(paddr_t, u_int);
extern int	pmap_page_is_cacheable(pmap_t, vaddr_t);

struct sh5_tlb_ops {
	void (*tlbinv_cookie)(pteh_t, tlbcookie_t);
	void (*tlbinv_all)(void);
	void (*tlbload)(void);		/* Not C-callable */
	u_int dtlb_slots;
	u_int itlb_slots;
};

#define	cpu_tlbinv_cookie	sh5_tlb_ops.tlbinv_cookie
#define	cpu_tlbinv_all		sh5_tlb_ops.tlbinv_all
#define	cpu_tlbload		sh5_tlb_ops.tlbload
#define	cpu_dtlb_slots		sh5_tlb_ops.dtlb_slots
#define	cpu_itlb_slots		sh5_tlb_ops.itlb_slots

extern struct sh5_tlb_ops sh5_tlb_ops;
#endif

#endif	/* _SH5_PMAP_H */
