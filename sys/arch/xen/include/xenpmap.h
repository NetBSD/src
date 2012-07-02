/*	$NetBSD: xenpmap.h,v 1.33.2.2 2012/07/02 21:01:48 jdc Exp $	*/

/*
 *
 * Copyright (c) 2004 Christian Limpach.
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


#ifndef _XEN_XENPMAP_H_
#define _XEN_XENPMAP_H_

#ifdef _KERNEL_OPT
#include "opt_xen.h"
#endif

#include <sys/types.h>
#include <sys/kcpuset.h>

#define	INVALID_P2M_ENTRY	(~0UL)

void xpq_queue_machphys_update(paddr_t, paddr_t);
void xpq_queue_invlpg(vaddr_t);
void xpq_queue_pte_update(paddr_t, pt_entry_t);
void xpq_queue_pt_switch(paddr_t);
void xpq_flush_queue(void);
void xpq_queue_set_ldt(vaddr_t, uint32_t);
void xpq_queue_tlb_flush(void);
void xpq_queue_pin_table(paddr_t, int);
void xpq_queue_unpin_table(paddr_t);
int  xpq_update_foreign(paddr_t, pt_entry_t, int);
void xen_vcpu_mcast_invlpg(vaddr_t, vaddr_t, kcpuset_t *);
void xen_vcpu_bcast_invlpg(vaddr_t, vaddr_t);
void xen_mcast_tlbflush(kcpuset_t *);
void xen_bcast_tlbflush(void);
void xen_mcast_invlpg(vaddr_t, kcpuset_t *);
void xen_bcast_invlpg(vaddr_t);

void pmap_xen_resume(void);
void pmap_xen_suspend(void);
void pmap_map_recursive_entries(void);
void pmap_unmap_recursive_entries(void);

#if defined(PAE) || defined(__x86_64__)
void xen_kpm_sync(struct pmap *, int);
#endif /* PAE || __x86_64__ */

#define xpq_queue_pin_l1_table(pa)	\
	xpq_queue_pin_table(pa, MMUEXT_PIN_L1_TABLE)
#define xpq_queue_pin_l2_table(pa)	\
	xpq_queue_pin_table(pa, MMUEXT_PIN_L2_TABLE)
#define xpq_queue_pin_l3_table(pa)	\
	xpq_queue_pin_table(pa, MMUEXT_PIN_L3_TABLE)
#define xpq_queue_pin_l4_table(pa)	\
	xpq_queue_pin_table(pa, MMUEXT_PIN_L4_TABLE)

extern unsigned long *xpmap_phys_to_machine_mapping;

/*   
 * On Xen-2, the start of the day virtual memory starts at KERNTEXTOFF
 * (0xc0100000). On Xen-3 for domain0 it starts at KERNBASE (0xc0000000).
 * So the offset between physical and virtual address is different on
 * Xen-2 and Xen-3 for domain0.
 * starting with xen-3.0.2, we can add notes so that virtual memory starts
 * at KERNBASE for domU as well.
 */  
#if defined(DOM0OPS) || !defined(XEN_COMPAT_030001)
#define XPMAP_OFFSET	0
#else
#define	XPMAP_OFFSET	(KERNTEXTOFF - KERNBASE)
#endif

#define mfn_to_pfn(mfn) (machine_to_phys_mapping[(mfn)])
#define pfn_to_mfn(pfn) (xpmap_phys_to_machine_mapping[(pfn)])

static __inline paddr_t
xpmap_mtop_masked(paddr_t mpa)
{
	return (
	    ((paddr_t)machine_to_phys_mapping[mpa >> PAGE_SHIFT] << PAGE_SHIFT)
	    + XPMAP_OFFSET);
}

static __inline paddr_t
xpmap_mtop(paddr_t mpa)
{
	return (xpmap_mtop_masked(mpa) | (mpa & ~PG_FRAME));
}

static __inline paddr_t
xpmap_ptom_masked(paddr_t ppa)
{
	return (((paddr_t)xpmap_phys_to_machine_mapping[(ppa -
	    XPMAP_OFFSET) >> PAGE_SHIFT]) << PAGE_SHIFT);
}

static __inline paddr_t
xpmap_ptom(paddr_t ppa)
{
	return (xpmap_ptom_masked(ppa) | (ppa & ~PG_FRAME));
}

static inline void
MULTI_update_va_mapping(
	multicall_entry_t *mcl, vaddr_t va,
	pt_entry_t new_val, unsigned long flags)
{
	mcl->op = __HYPERVISOR_update_va_mapping;
	mcl->args[0] = va;
#if defined(__x86_64__)
	mcl->args[1] = new_val;
	mcl->args[2] = flags;
#else
	mcl->args[1] = (new_val & 0xffffffff);
#ifdef PAE
	mcl->args[2] = (new_val >> 32);
#else
	mcl->args[2] = 0;
#endif
	mcl->args[3] = flags;
#endif
}

static inline void
MULTI_update_va_mapping_otherdomain(
	multicall_entry_t *mcl, vaddr_t va,
	pt_entry_t new_val, unsigned long flags, domid_t domid)
{
	mcl->op = __HYPERVISOR_update_va_mapping_otherdomain;
	mcl->args[0] = va;
#if defined(__x86_64__)
	mcl->args[1] = new_val;
	mcl->args[2] = flags;
	mcl->args[3] = domid;
#else
	mcl->args[1] = (new_val & 0xffffffff);
#ifdef PAE
	mcl->args[2] = (new_val >> 32);
#else
	mcl->args[2] = 0;
#endif
	mcl->args[3] = flags;
	mcl->args[4] = domid;
#endif
}
#if defined(__x86_64__)
#define MULTI_UVMFLAGS_INDEX 2
#define MULTI_UVMDOMID_INDEX 3
#else
#define MULTI_UVMFLAGS_INDEX 3
#define MULTI_UVMDOMID_INDEX 4
#endif

#if defined(__x86_64__)
void xen_set_user_pgd(paddr_t);
#endif

#endif /* _XEN_XENPMAP_H_ */
