/*	$NetBSD: xenpmap.h,v 1.39.12.1 2018/07/28 04:37:42 pgoyette Exp $	*/

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
void xen_mcast_tlbflush(kcpuset_t *);
void xen_bcast_tlbflush(void);
void xen_mcast_invlpg(vaddr_t, kcpuset_t *);
void xen_bcast_invlpg(vaddr_t);
void xen_copy_page(paddr_t, paddr_t);
void xen_pagezero(paddr_t);

void pmap_xen_resume(void);
void pmap_xen_suspend(void);
void pmap_map_recursive_entries(void);
void pmap_unmap_recursive_entries(void);

void xen_kpm_sync(struct pmap *, int);

#define xpq_queue_pin_l1_table(pa)	\
	xpq_queue_pin_table(pa, MMUEXT_PIN_L1_TABLE)
#define xpq_queue_pin_l2_table(pa)	\
	xpq_queue_pin_table(pa, MMUEXT_PIN_L2_TABLE)
#define xpq_queue_pin_l3_table(pa)	\
	xpq_queue_pin_table(pa, MMUEXT_PIN_L3_TABLE)
#define xpq_queue_pin_l4_table(pa)	\
	xpq_queue_pin_table(pa, MMUEXT_PIN_L4_TABLE)

extern unsigned long *xpmap_phys_to_machine_mapping;

static __inline paddr_t
xpmap_mtop_masked(paddr_t mpa)
{
	return (
	    (paddr_t)machine_to_phys_mapping[mpa >> PAGE_SHIFT] << PAGE_SHIFT);
}

static __inline paddr_t
xpmap_mtop(paddr_t mpa)
{
	return (xpmap_mtop_masked(mpa) | (mpa & ~PG_FRAME));
}

static __inline paddr_t
xpmap_ptom_masked(paddr_t ppa)
{
	return (
	    (paddr_t)xpmap_phys_to_machine_mapping[ppa >> PAGE_SHIFT]
	    << PAGE_SHIFT);
}

static __inline paddr_t
xpmap_ptom(paddr_t ppa)
{
	return (xpmap_ptom_masked(ppa) | (ppa & ~PG_FRAME));
}

static __inline void
xpmap_ptom_map(paddr_t ppa, paddr_t mpa)
{
	xpmap_phys_to_machine_mapping[ppa >> PAGE_SHIFT] = mpa >> PAGE_SHIFT;
}

static __inline void
xpmap_ptom_unmap(paddr_t ppa)
{
	xpmap_phys_to_machine_mapping[ppa >> PAGE_SHIFT] = INVALID_P2M_ENTRY;
}

static __inline bool
xpmap_ptom_isvalid(paddr_t ppa)
{
	return (
	    xpmap_phys_to_machine_mapping[ppa >> PAGE_SHIFT]
	    != INVALID_P2M_ENTRY);
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
	mcl->args[2] = (new_val >> 32);
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
	mcl->args[2] = (new_val >> 32);
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
