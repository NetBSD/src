/*	$NetBSD: xenpmap.h,v 1.15 2006/10/17 18:53:04 bouyer Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Christian Limpach.
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


#ifndef _XEN_XENPMAP_H_
#define _XEN_XENPMAP_H_

#define	INVALID_P2M_ENTRY	(~0UL)

void xpq_queue_machphys_update(paddr_t, paddr_t);
void xpq_queue_invlpg(vaddr_t);
void xpq_queue_pde_update(pd_entry_t *, pd_entry_t);
void xpq_queue_pte_update(pt_entry_t *, pt_entry_t);
void xpq_queue_unchecked_pte_update(pt_entry_t *, pt_entry_t);
void xpq_queue_pt_switch(paddr_t);
void xpq_flush_queue(void);
void xpq_queue_set_ldt(vaddr_t, uint32_t);
void xpq_queue_tlb_flush(void);
void xpq_queue_pin_table(paddr_t, int);
void xpq_queue_unpin_table(paddr_t);
int  xpq_update_foreign(pt_entry_t *, pt_entry_t, int);

extern paddr_t *xpmap_phys_to_machine_mapping;

#define	XPQ_PIN_L1_TABLE 1
#define	XPQ_PIN_L2_TABLE 2

#ifndef XEN
#define	PDE_GET(_pdp)						\
	*(_pdp)
#define PDE_SET(_pdp,_mapdp,_npde)				\
	*(_mapdp) = (_npde)
#define PDE_CLEAR(_pdp,_mapdp)					\
	*(_mapdp) = 0
#define PTE_SET(_ptp,_maptp,_npte)				\
	*(_maptp) = (_npte)
#define PTE_CLEAR(_ptp,_maptp)					\
	*(_maptp) = 0
#define PTE_ATOMIC_SET(_ptp,_maptp,_npte,_opte)			\
	(_opte) = x86_atomic_testset_ul((_maptp), (_npte))
#define PTE_ATOMIC_CLEAR(_ptp,_maptp,_opte)			\
	(_opte) = x86_atomic_testset_ul((_maptp), 0)
#define PDE_CLEARBITS(_pdp,_mapdp,_bits)			\
	*(_mapdp) &= ~(_bits)
#define PTE_ATOMIC_CLEARBITS(_ptp,_maptp,_bits)			\
	x86_atomic_clearbits_l((_maptp), (_bits))
#define PTE_SETBITS(_ptp,_maptp,_bits)				\
	*(_maptp) |= (_bits)
#define PTE_ATOMIC_SETBITS(_ptp,_maptp,_bits)			\
	x86_atomic_setbits_l((_maptp), (_bits))
#else
paddr_t *xpmap_phys_to_machine_mapping;

#define	PDE_GET(_pdp)						\
	(pmap_valid_entry(*(_pdp)) ? xpmap_mtop(*(_pdp)) : *(_pdp))
#define PDE_SET(_pdp,_mapdp,_npde) do {				\
	int _s = splvm();					\
	xpq_queue_pde_update((_mapdp), xpmap_ptom((_npde)));	\
	xpq_flush_queue();					\
	splx(_s);						\
} while (/*CONSTCOND*/0)
#define PDE_CLEAR(_pdp,_mapdp) do {				\
	int _s = splvm();					\
	xpq_queue_pde_update((_mapdp), 0);			\
	xpq_flush_queue();					\
	splx(_s);						\
} while (/*CONSTCOND*/0)
#define	PTE_GET(_ptp)						\
	(pmap_valid_entry(*(_ptp)) ? xpmap_mtop(*(_ptp)) : *(_ptp))
#define	PTE_GET_MA(_ptp)					\
	*(_ptp)
#define PTE_SET(_ptp,_maptp,_npte) do {				\
	int _s = splvm();					\
	xpq_queue_pte_update((_maptp), xpmap_ptom((_npte)));	\
	xpq_flush_queue();					\
	splx(_s);						\
} while (/*CONSTCOND*/0)
#define PTE_SET_MA(_ptp,_maptp,_npte) do {			\
	int _s = splvm();					\
	xpq_queue_pte_update((_maptp), (_npte));		\
	xpq_flush_queue();					\
	splx(_s);						\
} while (/*CONSTCOND*/0)
#define PTE_SET_MA_UNCHECKED(_ptp,_maptp,_npte) do {		\
	_s = splvm();						\
	xpq_queue_unchecked_pte_update((_maptp), (_npte));	\
	xpq_flush_queue();					\
	splx(_s);						\
} while (/*CONSTCOND*/0)
#define PTE_CLEAR(_ptp,_maptp) do {				\
	int _s = splvm();					\
	xpq_queue_pte_update((_maptp), 0);			\
	xpq_flush_queue();					\
	splx(_s);						\
} while (/*CONSTCOND*/0)
#define PTE_ATOMIC_SET(_ptp,_maptp,_npte,_opte) do {		\
	int _s;							\
	(_opte) = PTE_GET(_ptp);				\
	_s = splvm();						\
	xpq_queue_pte_update((_maptp), xpmap_ptom((_npte)));	\
	xpq_flush_queue();					\
	splx(_s);						\
} while (/*CONSTCOND*/0)
#define PTE_ATOMIC_SET_MA(_ptp,_maptp,_npte,_opte) do {		\
	int _s;							\
	(_opte) = *(_ptp);					\
	_s = splvm();						\
	xpq_queue_pte_update((_maptp), (_npte));		\
	xpq_flush_queue();					\
	splx(_s);						\
} while (/*CONSTCOND*/0)
#define PTE_ATOMIC_CLEAR(_ptp,_maptp,_opte) do {		\
	int _s;							\
	(_opte) = PTE_GET(_ptp);				\
	_s = splvm();						\
	xpq_queue_pte_update((_maptp), 0);			\
	xpq_flush_queue();					\
	splx(_s);						\
} while (/*CONSTCOND*/0)
#define PTE_ATOMIC_CLEAR_MA(_ptp,_maptp,_opte) do {		\
	int _s;							\
	(_opte) = *(_ptp);					\
	_s = splvm();						\
	xpq_queue_pte_update((_maptp), 0);			\
	xpq_flush_queue();					\
	splx(_s);						\
} while (/*CONSTCOND*/0)
#define PDE_CLEARBITS(_pdp,_mapdp,_bits) do {			\
	int _s = splvm();					\
	xpq_queue_pte_update((_mapdp), *(_pdp) & ~((_bits) & ~PG_FRAME)); \
	xpq_flush_queue();					\
	splx(_s);						\
} while (/*CONSTCOND*/0)
#define PTE_CLEARBITS(_ptp,_maptp,_bits) do {			\
	int _s = splvm();					\
	xpq_queue_pte_update((_maptp), *(_ptp) & ~((_bits) & ~PG_FRAME)); \
	xpq_flush_queue();					\
	splx(_s);						\
} while (/*CONSTCOND*/0)
#define PDE_ATOMIC_CLEARBITS(_pdp,_mapdp,_bits) do {		\
	int _s = splvm();					\
	xpq_queue_pde_update((_mapdp), *(_pdp) & ~((_bits) & ~PG_FRAME)); \
	xpq_flush_queue();					\
	splx(_s);						\
} while (/*CONSTCOND*/0)
#define PTE_ATOMIC_CLEARBITS(_ptp,_maptp,_bits) do {		\
	int _s = splvm();					\
	xpq_queue_pte_update((_maptp), *(_ptp) & ~((_bits) & ~PG_FRAME)); \
	xpq_flush_queue();					\
	splx(_s);						\
} while (/*CONSTCOND*/0)
#define PTE_SETBITS(_ptp,_maptp,_bits) do {			\
	int _s = splvm();					\
	xpq_queue_pte_update((_maptp), *(_ptp) | ((_bits) & ~PG_FRAME)); \
	xpq_flush_queue();					\
	splx(_s);						\
} while (/*CONSTCOND*/0)
#define PDE_ATOMIC_SETBITS(_pdp,_mapdp,_bits) do {		\
	int _s = splvm();					\
	xpq_queue_pde_update((_mapdp), *(_pdp) | ((_bits) & ~PG_FRAME)); \
	xpq_flush_queue();					\
	splx(_s);						\
} while (/*CONSTCOND*/0)
#define PTE_ATOMIC_SETBITS(_ptp,_maptp,_bits) do {		\
	int _s = splvm();					\
	xpq_queue_pte_update((_maptp), *(_ptp) | ((_bits) & ~PG_FRAME)); \
	xpq_flush_queue();					\
	splx(_s);						\
} while (/*CONSTCOND*/0)
#define PDE_COPY(_dpdp,_madpdp,_spdp) do {			\
	int _s = splvm();					\
	xpq_queue_pde_update((_madpdp), *(_spdp));		\
	xpq_flush_queue();					\
	splx(_s);						\
} while (/*CONSTCOND*/0)
#define	PTE_UPDATES_FLUSH() do {				\
	int _s = splvm();					\
	xpq_flush_queue();					\
	splx(_s);						\
} while (/*CONSTCOND*/0)

#endif

/*   
 * On Xen-2, the start of the day virual memory starts at KERNTEXTOFF
 * (0xc0100000). On Xen-3 for domain0 it starts at KERNBASE (0xc0000000).
 * So the offset between physical and virtual address is different on
 * Xen-2 and Xen-3 for domain0.
 * starting with xen-3.0.2, we can add notes so that virual memory starts
 * at KERNBASE for domU as well.
 */  
#if defined(XEN3) && (defined(DOM0OPS) || !defined(XEN_COMPAT_030001))
#define XPMAP_OFFSET	0
#else
#define	XPMAP_OFFSET	(KERNTEXTOFF - KERNBASE)
#endif

static __inline paddr_t
xpmap_mtop(paddr_t mpa)
{
	return ((machine_to_phys_mapping[mpa >> PAGE_SHIFT] << PAGE_SHIFT) +
	    XPMAP_OFFSET) | (mpa & ~PG_FRAME);
}

static __inline paddr_t
xpmap_ptom(paddr_t ppa)
{
	return (xpmap_phys_to_machine_mapping[(ppa -
	    XPMAP_OFFSET) >> PAGE_SHIFT] << PAGE_SHIFT)
		| (ppa & ~PG_FRAME);
}

static __inline paddr_t
xpmap_ptom_masked(paddr_t ppa)
{
	return (xpmap_phys_to_machine_mapping[(ppa -
	    XPMAP_OFFSET) >> PAGE_SHIFT] << PAGE_SHIFT);
}

#ifdef XEN3
static inline void
MULTI_update_va_mapping(
	multicall_entry_t *mcl, vaddr_t va,
	paddr_t new_val, unsigned long flags)
{
	mcl->op = __HYPERVISOR_update_va_mapping;
	mcl->args[0] = va;
#if defined(__x86_64__)
	mcl->args[1] = new_val;
	mcl->args[2] = flags;
#else
	mcl->args[1] = new_val;
	mcl->args[2] = 0;
	mcl->args[3] = flags;
#endif
}

static inline void
MULTI_update_va_mapping_otherdomain(
	multicall_entry_t *mcl, vaddr_t va,
	paddr_t new_val, unsigned long flags, domid_t domid)
{
	mcl->op = __HYPERVISOR_update_va_mapping_otherdomain;
	mcl->args[0] = va;
#if defined(__x86_64__)
	mcl->args[1] = new_val;
	mcl->args[2] = flags;
	mcl->args[3] = domid;
#else
	mcl->args[1] = new_val;   
	mcl->args[2] = 0;
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

#endif /* XEN3 */

#endif /* _XEN_XENPMAP_H_ */
