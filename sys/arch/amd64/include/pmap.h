/*	$NetBSD: pmap.h,v 1.54 2018/08/17 14:39:51 maxv Exp $	*/

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

#ifndef	_AMD64_PMAP_H_
#define	_AMD64_PMAP_H_

#ifdef __x86_64__

#if defined(_KERNEL_OPT)
#include "opt_xen.h"
#endif

#include <sys/atomic.h>

#include <machine/pte.h>
#include <machine/segments.h>
#ifdef _KERNEL
#include <machine/cpufunc.h>
#endif

#include <uvm/uvm_object.h>
#ifdef XEN
#include <xen/xenfunc.h>
#include <xen/xenpmap.h>
#endif

/*
 * Mask to get rid of the sign-extended part of addresses.
 */
#define VA_SIGN_MASK		0xffff000000000000
#define VA_SIGN_NEG(va)		((va) | VA_SIGN_MASK)
/* XXXfvdl this one's not right. */
#define VA_SIGN_POS(va)		((va) & ~VA_SIGN_MASK)

#ifndef XEN
#define L4_SLOT_PTE		slotspace.area[SLAREA_PTE].sslot
#else
#define L4_SLOT_PTE		509
#endif
#define L4_SLOT_KERN		slotspace.area[SLAREA_MAIN].sslot
#define L4_SLOT_KERNBASE	511 /* pl4_i(KERNBASE) */

#define PDIR_SLOT_USERLIM	255
#define PDIR_SLOT_KERN	L4_SLOT_KERN
#define PDIR_SLOT_PTE	L4_SLOT_PTE

/*
 * The following defines give the virtual addresses of various MMU
 * data structures:
 * PTE_BASE: the base VA of the linear PTE mappings
 * PDP_BASE: the base VA of the recursive mapping of the PTD
 */

#ifndef XEN
extern pt_entry_t *pte_base;
#define PTE_BASE	pte_base
#else
#define PTE_BASE	((pt_entry_t *)VA_SIGN_NEG((L4_SLOT_PTE * NBPD_L4)))
#endif

#define L1_BASE	PTE_BASE
#define L2_BASE	((pd_entry_t *)((char *)L1_BASE + L4_SLOT_PTE * NBPD_L3))
#define L3_BASE	((pd_entry_t *)((char *)L2_BASE + L4_SLOT_PTE * NBPD_L2))
#define L4_BASE	((pd_entry_t *)((char *)L3_BASE + L4_SLOT_PTE * NBPD_L1))

#define PDP_BASE	L4_BASE

#define NKL4_MAX_ENTRIES	(unsigned long)64
#define NKL3_MAX_ENTRIES	(unsigned long)(NKL4_MAX_ENTRIES * 512)
#define NKL2_MAX_ENTRIES	(unsigned long)(NKL3_MAX_ENTRIES * 512)
#define NKL1_MAX_ENTRIES	(unsigned long)(NKL2_MAX_ENTRIES * 512)

#define NKL4_KIMG_ENTRIES	1
#define NKL3_KIMG_ENTRIES	1
#define NKL2_KIMG_ENTRIES	48

/*
 * Since kva space is below the kernel in its entirety, we start off
 * with zero entries on each level.
 */
#define NKL4_START_ENTRIES	0
#define NKL3_START_ENTRIES	0
#define NKL2_START_ENTRIES	0
#define NKL1_START_ENTRIES	0

#define PTP_MASK_INITIALIZER	{ L1_FRAME, L2_FRAME, L3_FRAME, L4_FRAME }
#define PTP_SHIFT_INITIALIZER	{ L1_SHIFT, L2_SHIFT, L3_SHIFT, L4_SHIFT }
#define NKPTP_INITIALIZER	{ NKL1_START_ENTRIES, NKL2_START_ENTRIES, \
				  NKL3_START_ENTRIES, NKL4_START_ENTRIES }
#define NKPTPMAX_INITIALIZER	{ NKL1_MAX_ENTRIES, NKL2_MAX_ENTRIES, \
				  NKL3_MAX_ENTRIES, NKL4_MAX_ENTRIES }
#define NBPD_INITIALIZER	{ NBPD_L1, NBPD_L2, NBPD_L3, NBPD_L4 }
#define PDES_INITIALIZER	{ L2_BASE, L3_BASE, L4_BASE }

#define PTP_LEVELS	4

/*
 * PG_AVAIL usage: we make use of the ignored bits of the PTE
 */

#define PG_W		PG_AVAIL1	/* "wired" mapping */
#define PG_PVLIST	PG_AVAIL2	/* mapping has entry on pvlist */
/* PG_AVAIL3 not used */

#define	PG_X		0		/* XXX dummy */

void svs_pmap_sync(struct pmap *, int);
void svs_lwp_switch(struct lwp *, struct lwp *);
void svs_pdir_switch(struct pmap *);
void svs_init(void);
extern bool svs_enabled;

#include <x86/pmap.h>

#ifndef XEN
#define pmap_pa2pte(a)			(a)
#define pmap_pte2pa(a)			((a) & PG_FRAME)
#define pmap_pte_set(p, n)		do { *(p) = (n); } while (0)
#define pmap_pte_cas(p, o, n)		atomic_cas_64((p), (o), (n))
#define pmap_pte_testset(p, n)		\
    atomic_swap_ulong((volatile unsigned long *)p, n)
#define pmap_pte_setbits(p, b)		\
    atomic_or_ulong((volatile unsigned long *)p, b)
#define pmap_pte_clearbits(p, b)	\
    atomic_and_ulong((volatile unsigned long *)p, ~(b))
#define pmap_pte_flush()		/* nothing */
#else
extern kmutex_t pte_lock;

static __inline pt_entry_t
pmap_pa2pte(paddr_t pa)
{
	return (pt_entry_t)xpmap_ptom_masked(pa);
}

static __inline paddr_t
pmap_pte2pa(pt_entry_t pte)
{
	return xpmap_mtop_masked(pte & PG_FRAME);
}

static __inline void
pmap_pte_set(pt_entry_t *pte, pt_entry_t npte)
{
	int s = splvm();
	xpq_queue_pte_update(xpmap_ptetomach(pte), npte);
	splx(s);
}

static __inline pt_entry_t
pmap_pte_cas(volatile pt_entry_t *ptep, pt_entry_t o, pt_entry_t n)
{
	pt_entry_t opte;

	mutex_enter(&pte_lock);
	opte = *ptep;
	if (opte == o) {
		xpq_queue_pte_update(xpmap_ptetomach(__UNVOLATILE(ptep)), n);
		xpq_flush_queue();
	}

	mutex_exit(&pte_lock);
	return opte;
}

static __inline pt_entry_t
pmap_pte_testset(volatile pt_entry_t *pte, pt_entry_t npte)
{
	pt_entry_t opte;

	mutex_enter(&pte_lock);
	opte = *pte;
	xpq_queue_pte_update(xpmap_ptetomach(__UNVOLATILE(pte)), npte);
	xpq_flush_queue();
	mutex_exit(&pte_lock);
	return opte;
}

static __inline void
pmap_pte_setbits(volatile pt_entry_t *pte, pt_entry_t bits)
{
	mutex_enter(&pte_lock);
	xpq_queue_pte_update(xpmap_ptetomach(__UNVOLATILE(pte)), (*pte) | bits);
	xpq_flush_queue();
	mutex_exit(&pte_lock);
}

static __inline void
pmap_pte_clearbits(volatile pt_entry_t *pte, pt_entry_t bits)
{
	mutex_enter(&pte_lock);
	xpq_queue_pte_update(xpmap_ptetomach(__UNVOLATILE(pte)),
	    (*pte) & ~bits);
	xpq_flush_queue();
	mutex_exit(&pte_lock);
}

static __inline void
pmap_pte_flush(void)
{
	int s = splvm();
	xpq_flush_queue();
	splx(s);
}
#endif

#ifdef __HAVE_DIRECT_MAP
#define PMAP_DIRECT

static __inline int
pmap_direct_process(paddr_t pa, voff_t pgoff, size_t len,
    int (*process)(void *, size_t, void *), void *arg)
{
	vaddr_t va = PMAP_DIRECT_MAP(pa);

	return process((void *)(va + pgoff), len, arg);
}

#endif /* __HAVE_DIRECT_MAP */

void pmap_changeprot_local(vaddr_t, vm_prot_t);

#include <x86/pmap_pv.h>

#define	__HAVE_VM_PAGE_MD
#define	VM_MDPAGE_INIT(pg) \
	memset(&(pg)->mdpage, 0, sizeof((pg)->mdpage)); \
	PMAP_PAGE_INIT(&(pg)->mdpage.mp_pp)

struct vm_page_md {
	struct pmap_page mp_pp;
};

#else	/*	!__x86_64__	*/

#include <i386/pmap.h>

#endif	/*	__x86_64__	*/

#endif	/* _AMD64_PMAP_H_ */
