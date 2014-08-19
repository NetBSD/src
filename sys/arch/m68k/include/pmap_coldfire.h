/*	$NetBSD: pmap_coldfire.h,v 1.2.8.2 2014/08/20 00:03:10 tls Exp $	*/
/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#ifndef _M68K_PMAP_COLDFIRE_H_
#define M68K_PMAP_COLDFIRE_H_

#ifdef _LOCORE
#error use assym.h instead
#endif

#if defined(_MODULE)
#error this file should not be included by loadable kernel modules
#endif

#ifdef _KERNEL_OPT
#include "opt_pmap.h"
#endif

#include <sys/cpu.h>
#include <sys/kcore.h>
#include <uvm/uvm_page.h>
#ifdef __PMAP_PRIVATE
#include <powerpc/booke/cpuvar.h>
#include <powerpc/cpuset.h>
#endif

#define	PMAP_NEED_PROCWR

#include <uvm/pmap/vmpagemd.h>

#include <m68k/pte_coldfire.h>

#define	NBSEG		(NBPG*NPTEPG)
#define	SEGSHIFT	(PGSHIFT + PGSHIFT - 2)
#define SEGOFSET	((1 << SEGSHIFT) - 1)
#define PMAP_SEGTABSIZE	(1 << (32 - SEGSHIFT))
#define	NPTEPG		(NBPG >> 2)

#define	KERNEL_PID	0

#define PMAP_TLB_MAX			  1
#define	PMAP_TLB_NUM_PIDS		256
#define	PMAP_INVALID_SEGTAB_ADDRESS	((pmap_segtab_t *)0xfeeddead)

#define	pmap_phys_address(x)		(x)

void	pmap_procwr(struct proc *, vaddr_t, size_t);
#define	PMAP_NEED_PROCWR

#ifdef __PMAP_PRIVATE
struct vm_page *
	pmap_md_alloc_poolpage(int flags);
vaddr_t	pmap_md_map_poolpage(paddr_t, vsize_t);
void	pmap_md_unmap_poolpage(vaddr_t, vsize_t);
bool	pmap_md_direct_mapped_vaddr_p(vaddr_t);
bool	pmap_md_io_vaddr_p(vaddr_t);
paddr_t	pmap_md_direct_mapped_vaddr_to_paddr(vaddr_t);
vaddr_t	pmap_md_direct_map_paddr(paddr_t);
void	pmap_md_init(void);

bool	pmap_md_tlb_check_entry(void *, vaddr_t, tlb_asid_t, pt_entry_t);

#ifdef PMAP_MINIMALTLB
vaddr_t	pmap_kvptefill(vaddr_t, vaddr_t, pt_entry_t);
#endif
#endif

void	pmap_md_page_syncicache(struct vm_page *, const kcpuset_t *);
vaddr_t	pmap_bootstrap(vaddr_t, vaddr_t, phys_ram_seg_t *, size_t);
bool	pmap_extract(struct pmap *, vaddr_t, paddr_t *);

static inline paddr_t vtophys(vaddr_t);

static inline paddr_t
vtophys(vaddr_t va)
{
	paddr_t pa;

	if (pmap_extract(pmap_kernel(), va, &pa))
		return pa;
	KASSERT(0);
	return (paddr_t) -1;
}

#ifdef __PMAP_PRIVATE
/*
 * Virtual Cache Alias helper routines.  Not a problem for Booke CPUs.
 */
static inline bool
pmap_md_vca_add(struct vm_page *pg, vaddr_t va, pt_entry_t *nptep)
{
	return false;
}

static inline void
pmap_md_vca_remove(struct vm_page *pg, vaddr_t va)
{

}

static inline void
pmap_md_vca_clean(struct vm_page *pg, vaddr_t va, int op)
{
}

static inline size_t
pmap_md_tlb_asid_max(void)
{
	return PMAP_TLB_NUM_PIDS - 1;
}
#endif

#define	POOL_VTOPHYS(va)	((paddr_t)(vaddr_t)(va))
#define	POOL_PHYSTOV(pa)	((vaddr_t)(paddr_t)(pa))

#include <uvm/pmap/pmap.h>

#endif /* !_M68K_PMAP_COLDFIRE_H_ */
