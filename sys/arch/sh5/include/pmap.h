/*	$NetBSD: pmap.h,v 1.1 2002/07/05 13:32:00 scw Exp $	*/

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

/*
 * Assume 512MB of KSEG1 KVA, but allow this to be over-ridden
 * if necessary.
 */
#ifndef	KERNEL_IPT_SIZE
#define	KERNEL_IPT_SIZE	(SH5_KSEG1_SIZE / NBPG)
#endif


#if defined(_KERNEL)
#if SH5_NEFF_BITS > 32
#define	KERNEL_IPT_SHIFT	3
#define	LDPTE			ld.q
#define	STPTE			st.q
#else
#define	KERNEL_IPT_SHIFT	2
#define	LDPTE			ld.l
#define	STPTE			st.l
#endif
#endif

#if defined(_KERNEL)
struct pmap {
	int pm_refs;		/* pmap reference count */
	u_int pm_asid;		/* ASID for this pmap */
	u_int pm_asidgen;	/* ASID Generation number */
	vsid_t pm_vsid;		/* This pmap's vsid */
	struct pmap_statistics pm_stats;
};

#define	PMAP_ASID_RESERVED	0

typedef struct pmap *pmap_t;

#define	PMAP_NC		0x1000

extern struct pmap kernel_pmap_store;
#define	pmap_kernel()	(&kernel_pmap_store)

extern int pmap_write_trap(int, vaddr_t);
extern boolean_t pmap_clear_bit(struct vm_page *, int);
extern boolean_t pmap_query_bit(struct vm_page *, int);

extern vaddr_t pmap_map_poolpage(paddr_t);
extern paddr_t pmap_unmap_poolpage(vaddr_t);
#define	PMAP_MAP_POOLPAGE(p)	pmap_map_poolpage((p))
#define	PMAP_UNMAP_POOLPAGE(v)	pmap_unmap_poolpage((v))

#define pmap_clear_modify(pg)		(pmap_clear_bit((pg), SH5_PTEL_M))
#define	pmap_clear_reference(pg)	(pmap_clear_bit((pg), SH5_PTEL_R))
#define	pmap_is_modified(pg)		(pmap_query_bit((pg), SH5_PTEL_M))
#define	pmap_is_referenced(pg)		(pmap_query_bit((pg), SH5_PTEL_R))

#define	pmap_resident_count(pm)		((pm)->pm_stats.resident_count)
#define	pmap_wired_count(pm)		((pm)->pm_stats.wired_count)

#define	pmap_phys_address(x)		(x)

/* Private pmap data and functions */
extern int	pmap_initialized;
extern u_int	pmap_ipt_hash(vsid_t vsid, vaddr_t va);  /* See exception.S */
extern vaddr_t	pmap_bootstrap_mapping(paddr_t, u_int);

extern void (*__cpu_tlbinv)(pteh_t, pteh_t);
extern void (*__cpu_tlbinv_cookie)(pteh_t, u_int);
extern void (*__cpu_tlbinv_all)(void);
extern void (*__cpu_tlbload)(void);	/* Not C-callable */
#endif

#endif	/* _SH5_PMAP_H */
