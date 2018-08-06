/* $NetBSD: pmap.h,v 1.7 2018/08/06 12:50:56 ryo Exp $ */

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

#ifndef _AARCH64_PMAP_H_
#define _AARCH64_PMAP_H_

#ifdef __aarch64__

#include <sys/types.h>
#include <sys/pool.h>
#include <sys/queue.h>
#include <uvm/uvm_pglist.h>

#include <aarch64/pte.h>

#define PMAP_GROWKERNEL
#define PMAP_STEAL_MEMORY

#define __HAVE_VM_PAGE_MD

struct pmap {
	kmutex_t pm_lock;
	struct pool *pm_pvpool;
	pd_entry_t *pm_l0table;			/* L0 table: 512G*512 */
	paddr_t pm_l0table_pa;

	SLIST_HEAD(, vm_page) pm_vmlist;	/* for L[0123] tables */

	struct pmap_statistics pm_stats;
	unsigned int pm_refcnt;
	int pm_asid;
	bool pm_activated;
};

struct pv_entry;
struct vm_page_md {
	kmutex_t mdpg_pvlock;
	SLIST_ENTRY(vm_page) mdpg_vmlist;	/* L[0-3] table vm_page list */
	TAILQ_HEAD(, pv_entry) mdpg_pvhead;

	/* VM_PROT_READ means referenced, VM_PROT_WRITE means modified */
	uint32_t mdpg_flags;
};

/* each mdpg_pvlock will be initialized in pmap_init() */
#define VM_MDPAGE_INIT(pg)				\
	do {						\
		TAILQ_INIT(&(pg)->mdpage.mdpg_pvhead);	\
		(pg)->mdpage.mdpg_flags = 0;		\
	} while (/*CONSTCOND*/ 0)

#define l0pde_pa(pde)		((paddr_t)((pde) & LX_TBL_PA))
#define l0pde_index(v)		(((vaddr_t)(v) & L0_ADDR_BITS) >> L0_SHIFT)
#define l0pde_valid(pde)	(((pde) & LX_VALID) == LX_VALID)
/* l0pte always contains table entries */

#define l1pde_pa(pde)		((paddr_t)((pde) & LX_TBL_PA))
#define l1pde_index(v)		(((vaddr_t)(v) & L1_ADDR_BITS) >> L1_SHIFT)
#define l1pde_valid(pde)	(((pde) & LX_VALID) == LX_VALID)
#define l1pde_is_block(pde)	(((pde) & LX_TYPE) == LX_TYPE_BLK)
#define l1pde_is_table(pde)	(((pde) & LX_TYPE) == LX_TYPE_TBL)

#define l2pde_pa(pde)		((paddr_t)((pde) & LX_TBL_PA))
#define l2pde_index(v)		(((vaddr_t)(v) & L2_ADDR_BITS) >> L2_SHIFT)
#define l2pde_valid(pde)	(((pde) & LX_VALID) == LX_VALID)
#define l2pde_is_block(pde)	(((pde) & LX_TYPE) == LX_TYPE_BLK)
#define l2pde_is_table(pde)	(((pde) & LX_TYPE) == LX_TYPE_TBL)

#define l3pte_pa(pde)		((paddr_t)((pde) & LX_TBL_PA))
#define l3pte_executable(pde)	\
    (((pde) & (LX_BLKPAG_UXN|LX_BLKPAG_PXN)) != (LX_BLKPAG_UXN|LX_BLKPAG_PXN))
#define l3pte_readable(pde)	((pde) & LX_BLKPAG_AF)
#define l3pte_writable(pde)	\
    (((pde) & (LX_BLKPAG_AF|LX_BLKPAG_AP)) == (LX_BLKPAG_AF|LX_BLKPAG_AP_RW))
#define l3pte_index(v)		(((vaddr_t)(v) & L3_ADDR_BITS) >> L3_SHIFT)
#define l3pte_valid(pde)	(((pde) & LX_VALID) == LX_VALID)
#define l3pte_is_page(pde)	(((pde) & LX_TYPE) == L3_TYPE_PAG)
/* l3pte contains always page entries */

void pmap_bootstrap(vaddr_t, vaddr_t);
bool pmap_fault_fixup(struct pmap *, vaddr_t, vm_prot_t, bool user);

/* for ddb */
void pmap_db_pteinfo(vaddr_t, void (*)(const char *, ...));
pt_entry_t *kvtopte(vaddr_t);
pt_entry_t pmap_kvattr(vaddr_t, vm_prot_t);

/* Hooks for the pool allocator */
paddr_t vtophys(vaddr_t);
#define VTOPHYS_FAILED		((paddr_t)-1L)	/* POOL_PADDR_INVALID */
#define POOL_VTOPHYS(va)	vtophys((vaddr_t) (va))


/* devmap */
struct pmap_devmap {
	vaddr_t pd_va;		/* virtual address */
	paddr_t pd_pa;		/* physical address */
	psize_t pd_size;	/* size of region */
	vm_prot_t pd_prot;	/* protection code */
	u_int pd_flags;		/* flags for pmap_kenter_pa() */
};

void pmap_devmap_register(const struct pmap_devmap *);
void pmap_devmap_bootstrap(const struct pmap_devmap *);
const struct pmap_devmap *pmap_devmap_find_pa(paddr_t, psize_t);
const struct pmap_devmap *pmap_devmap_find_va(vaddr_t, vsize_t);
vaddr_t pmap_devmap_phystov(paddr_t);
paddr_t pmap_devmap_vtophys(paddr_t);

/* devmap use L2 blocks. (2Mbyte) */
#define DEVMAP_TRUNC_ADDR(x)	((x) & ~L2_OFFSET)
#define DEVMAP_ROUND_SIZE(x)	(((x) + L2_SIZE - 1) & ~(L2_SIZE - 1))

#define	DEVMAP_ENTRY(va, pa, sz)			\
	{						\
		.pd_va = DEVMAP_TRUNC_ADDR(va),		\
		.pd_pa = DEVMAP_TRUNC_ADDR(pa),		\
		.pd_size = DEVMAP_ROUND_SIZE(sz),	\
		.pd_prot = VM_PROT_READ|VM_PROT_WRITE,	\
		.pd_flags = PMAP_NOCACHE		\
	}
#define	DEVMAP_ENTRY_END	{ 0 }

/* mmap cookie and flags */
#define AARCH64_MMAP_FLAG_SHIFT		(64 - PGSHIFT)
#define AARCH64_MMAP_FLAG_MASK		0xf
#define AARCH64_MMAP_WRITEBACK		0UL
#define AARCH64_MMAP_NOCACHE		1UL
#define AARCH64_MMAP_WRITECOMBINE	2UL
#define AARCH64_MMAP_DEVICE		3UL

#define ARM_MMAP_MASK			__BITS(63, AARCH64_MMAP_FLAG_SHIFT)
#define ARM_MMAP_WRITECOMBINE		__SHIFTIN(AARCH64_MMAP_WRITECOMBINE, ARM_MMAP_MASK)
#define ARM_MMAP_WRITEBACK		__SHIFTIN(AARCH64_MMAP_WRITEBACK, ARM_MMAP_MASK)
#define ARM_MMAP_NOCACHE		__SHIFTIN(AARCH64_MMAP_NOCACHE, ARM_MMAP_MASK)
#define ARM_MMAP_DEVICE			__SHIFTIN(AARCH64_MMAP_DEVICE, ARM_MMAP_MASK)

#define	PMAP_PTE			0x10000000 /* kenter_pa */
#define	PMAP_DEV			0x20000000 /* kenter_pa */

static inline u_int
aarch64_mmap_flags(paddr_t mdpgno)
{
	u_int nflag, pflag;

	/*
	 * aarch64 arch has 4 memory attribute:
	 *
	 *  WriteBack      - write back cache
	 *  WriteThru      - wite through cache
	 *  NoCache        - no cache
	 *  Device(nGnRnE) - no Gathering, no Reordering, no Early write ack
	 *
	 * but pmap has PMAP_{NOCACHE,WRITE_COMBINE,WRITE_BACK} flags.
	 */

	nflag = (mdpgno >> AARCH64_MMAP_FLAG_SHIFT) & AARCH64_MMAP_FLAG_MASK;
	switch (nflag) {
	case AARCH64_MMAP_DEVICE:
		pflag = PMAP_DEV;
		break;
	case AARCH64_MMAP_WRITECOMBINE:
		pflag = PMAP_WRITE_COMBINE;
		break;
	case AARCH64_MMAP_WRITEBACK:
		pflag = PMAP_WRITE_BACK;
		break;
	case AARCH64_MMAP_NOCACHE:
	default:
		pflag = PMAP_NOCACHE;
		break;
	}
	return pflag;
}


#define pmap_phys_address(pa)		aarch64_ptob((pa))
#define pmap_mmap_flags(ppn)		aarch64_mmap_flags((ppn))

#define pmap_update(pmap)		((void)0)
#define pmap_copy(dp,sp,d,l,s)		((void)0)
#define pmap_wired_count(pmap)		((pmap)->pm_stats.wired_count)
#define pmap_resident_count(pmap)	((pmap)->pm_stats.resident_count)

bool	pmap_extract_coherency(pmap_t, vaddr_t, paddr_t *, bool *);

#define	PMAP_MAPSIZE1	L2_SIZE

#elif defined(__arm__)

#include <arm/pmap.h>

#endif /* __arm__/__aarch64__ */

#endif /* !_AARCH64_PMAP_ */
