/*	$NetBSD: pmap.h,v 1.16.18.4 2008/12/02 11:57:30 skrll Exp $	*/

/*	$OpenBSD: pmap.h,v 1.35 2007/12/14 18:32:23 deraadt Exp $	*/

/*
 * Copyright (c) 2002-2004 Michael Shalayeff
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
 * IN NO EVENT SHALL THE AUTHOR OR HIS RELATIVES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF MIND, USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *	Pmap header for hppa.
 */

#ifndef	_HPPA_PMAP_H_
#define	_HPPA_PMAP_H_

#include <sys/mutex.h>
#include <machine/pte.h>

#include <uvm/uvm_pglist.h>
#include <uvm/uvm_object.h>

#ifdef	_KERNEL

struct pmap {
	struct uvm_object pm_obj;	/* object (lck by object lock) */
#define	pm_lock	pm_obj.vmobjlock
	struct vm_page	*pm_ptphint;
	struct vm_page	*pm_pdir_pg;	/* vm_page for pdir */
	volatile u_int32_t *pm_pdir;	/* page dir (read-only after create) */
	pa_space_t	pm_space;	/* space id (read-only after create) */
	u_int		pm_pid;		/* prot id (read-only after create) */

	struct pmap_statistics	pm_stats;
};
typedef struct pmap *pmap_t;

#define PMAP_NC		0x100

#define	HPPA_MAX_PID	0xfffa
#define	HPPA_SID_MAX	0x7ffd
#if 0
#define	HPPA_SID_KERNEL	0
#endif
#define	HPPA_PID_KERNEL	2

struct pv_entry {			/* locked by its list's pvh_lock */
	struct pv_entry	*pv_next;
	struct pmap	*pv_pmap;	/* the pmap */
	vaddr_t		pv_va;		/* the virtual address */
	struct vm_page	*pv_ptp;	/* the vm_page of the PTP */
};

extern struct pmap kernel_pmap_store;

#if defined(HP7100LC_CPU) || defined(HP7300LC_CPU)
extern int pmap_hptsize;
extern struct pdc_hwtlb pdc_hwtlb;
#endif

/*
 * pool quickmaps
 */
#define	PMAP_MAP_POOLPAGE(pa)	(pa)
#define	PMAP_UNMAP_POOLPAGE(va)	(va)

/*
 * according to the parisc manual aliased va's should be
 * different by high 12 bits only.
 */
#define	PMAP_PREFER(o,h,s,td)	do {					\
	vaddr_t pmap_prefer_hint;					\
	pmap_prefer_hint = (*(h) & HPPA_PGAMASK) | ((o) & HPPA_PGAOFF);	\
	if (pmap_prefer_hint < *(h))					\
		pmap_prefer_hint += HPPA_PGALIAS;			\
	*(h) = pmap_prefer_hint;					\
} while(0)

#define	pmap_sid2pid(s)			(((s) + 1) << 1)
#define pmap_kernel()			(&kernel_pmap_store)
#define	pmap_resident_count(pmap)	((pmap)->pm_stats.resident_count)
#define pmap_wired_count(pmap)		((pmap)->pm_stats.wired_count)
#define	pmap_update(p)

#define pmap_copy(dpmap,spmap,da,len,sa)

#define pmap_clear_modify(pg)	pmap_changebit(pg, 0, PTE_PROT(TLB_DIRTY))
#define pmap_clear_reference(pg) \
				pmap_changebit(pg, PTE_PROT(TLB_REFTRAP), 0)
#define pmap_is_modified(pg)	pmap_testbit(pg, PTE_PROT(TLB_DIRTY))
#define pmap_is_referenced(pg)	pmap_testbit(pg, PTE_PROT(TLB_REFTRAP))
#define pmap_phys_address(ppn)	((ppn) << PAGE_SHIFT)

void	pmap_activate(struct lwp *);

void pmap_bootstrap(vaddr_t);
bool pmap_changebit(struct vm_page *, u_int, u_int);
bool pmap_testbit(struct vm_page *, u_int);
void pmap_write_protect(struct pmap *, vaddr_t, vaddr_t, vm_prot_t);
void pmap_remove(struct pmap *pmap, vaddr_t sva, vaddr_t eva);
void pmap_page_remove(struct vm_page *pg);

static inline void
pmap_deactivate(struct lwp *l)
{
	/* Nothing. */
}

static inline void
pmap_remove_all(struct pmap *pmap)
{
	/* Nothing. */
}

static inline int
pmap_prot(struct pmap *pmap, int prot)
{
	extern u_int hppa_prot[];
	return (hppa_prot[prot] | (pmap == pmap_kernel()? 0 : TLB_USER));
}

static inline void
pmap_page_protect(struct vm_page *pg, vm_prot_t prot)
{
	if ((prot & UVM_PROT_WRITE) == 0) {
		if (prot & (UVM_PROT_RX))
			pmap_changebit(pg, 0, PTE_PROT(TLB_WRITE));
		else
			pmap_page_remove(pg);
	}
}

static inline void
pmap_protect(struct pmap *pmap, vaddr_t sva, vaddr_t eva, vm_prot_t prot)
{
	if ((prot & UVM_PROT_WRITE) == 0) {
		if (prot & (UVM_PROT_RX))
			pmap_write_protect(pmap, sva, eva, prot);
		else
			pmap_remove(pmap, sva, eva);
	}
}

#define	pmap_sid(pmap, va) \
	((((va) & 0xc0000000) != 0xc0000000) ? \
	 (pmap)->pm_space : HPPA_SID_KERNEL)

#endif /* _KERNEL */

#endif /* _HPPA_PMAP_H_ */
