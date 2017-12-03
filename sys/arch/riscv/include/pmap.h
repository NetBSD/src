/* $NetBSD: pmap.h,v 1.1.18.2 2017/12/03 11:36:39 jdolecek Exp $ */

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
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

#ifndef _RISCV_PMAP_H_
#define _RISCV_PMAP_H_

#ifdef _KERNEL_OPT
#include "opt_modular.h"
#endif

#if !defined(_MODULE)

#include <sys/types.h>
#include <sys/pool.h>
#include <sys/evcnt.h>

#include <uvm/pmap/vmpagemd.h>

#include <riscv/pte.h>

#define	PMAP_SEGTABSIZE		(__SHIFTOUT(PTE_PPN0, PTE_PPN0) + 1)
#define	PMAP_PDETABSIZE		(__SHIFTOUT(PTE_PPN0, PTE_PPN0) + 1)

#define NBSEG		(NBPG*NPTEPG)
#ifdef _LP64
#define NBXSEG		(NBSEG*NSEGPG)
#define XSEGSHIFT	(SEGSHIFT + PGSHIFT - 3)
#define XSEGOFSET	(PTE_PPN1|SEGOFSET)
#define SEGSHIFT	(PGSHIFT + PGSHIFT - 3)
#else
#define SEGSHIFT	(PGSHIFT + PGSHIFT - 2)
#endif
#define SEGOFSET	(PTE_PPN0|PAGE_MASK)

#define KERNEL_PID	0

#define PMAP_HWPAGEWALKER		1
#define PMAP_TLB_NUM_PIDS		256
#define PMAP_TLB_MAX			1
#ifdef _LP64
#define PMAP_INVALID_PDETAB_ADDRESS	((pmap_pdetab_t *)(VM_MIN_KERNEL_ADDRESS - PAGE_SIZE))
#define PMAP_INVALID_SEGTAB_ADDRESS	((pmap_segtab_t *)(VM_MIN_KERNEL_ADDRESS - PAGE_SIZE))
#else
#define PMAP_INVALID_PDETAB_ADDRESS	((pmap_pdetab_t *)0xdeadbeef)
#define PMAP_INVALID_SEGTAB_ADDRESS	((pmap_segtab_t *)0xdeadbeef)
#endif
#define PMAP_TLB_FLUSH_ASID_ON_RESET	false

#define pmap_phys_address(x)		(x)

#define PMAP_NEED_PROCWR
static inline void
pmap_procwr(struct proc *p, vaddr_t va, vsize_t len)
{
	__asm __volatile("fence\trw,rw; fence.i");
}


#include <uvm/pmap/tlb.h>

#include <uvm/pmap/pmap_tlb.h>

#define PMAP_GROWKERNEL
#define PMAP_STEAL_MEMORY

#ifdef _KERNEL

#define __HAVE_PMAP_MD
struct pmap_md {
	paddr_t md_ptbr;
};

struct vm_page *
        pmap_md_alloc_poolpage(int flags);
vaddr_t pmap_md_map_poolpage(paddr_t, vsize_t);
void    pmap_md_unmap_poolpage(vaddr_t, vsize_t);
bool    pmap_md_direct_mapped_vaddr_p(vaddr_t);
bool    pmap_md_io_vaddr_p(vaddr_t);
paddr_t pmap_md_direct_mapped_vaddr_to_paddr(vaddr_t);
vaddr_t pmap_md_direct_map_paddr(paddr_t);
void    pmap_md_init(void);
bool    pmap_md_tlb_check_entry(void *, vaddr_t, tlb_asid_t, pt_entry_t);
//void    pmap_md_page_syncicache(struct vm_page *, const kcpuset_t *);

void	pmap_md_pdetab_activate(struct pmap *);
void	pmap_md_pdetab_init(struct pmap *);

#ifdef __PMAP_PRIVATE
static inline void
pmap_md_page_syncicache(struct vm_page *pg, const kcpuset_t *kc)
{
	__asm __volatile("fence\trw,rw; fence.i");
}

/*
 * Virtual Cache Alias helper routines.  Not a problem for RISCV CPUs.
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
#endif /* __PMAP_PRIVATE */
#endif /* _KERNEL */

#define POOL_VTOPHYS(va)	((paddr_t)((vaddr_t)(va)-VM_MAX_KERNEL_ADDRESS))
#define POOL_PHYSTOV(pa)	((vaddr_t)(paddr_t)(pa)+VM_MAX_KERNEL_ADDRESS)

#include <uvm/pmap/pmap.h>

#endif /* !_MODULE */

#if defined(MODULAR) || defined(_MODULE)
/*
 * Define a compatible vm_page_md so that struct vm_page is the same size
 * whether we are using modules or not.
 */
#ifndef __HAVE_VM_PAGE_MD
#define __HAVE_VM_PAGE_MD

struct vm_page_md {
	uintptr_t mdpg_dummy[3];
};
#endif /* !__HVE_VM_PAGE_MD */

__CTASSERT(sizeof(struct vm_page_md) == sizeof(uintptr_t)*3);

#endif /* MODULAR || _MODULE */

#endif /* !_RISCV_PMAP_H_ */
