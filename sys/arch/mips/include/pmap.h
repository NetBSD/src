/*	$NetBSD: pmap.h,v 1.54.26.7 2010/01/22 07:41:10 matt Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)pmap.h	8.1 (Berkeley) 6/10/93
 */

/*
 * Copyright (c) 1987 Carnegie-Mellon University
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)pmap.h	8.1 (Berkeley) 6/10/93
 */

#ifndef	_MIPS_PMAP_H_
#define	_MIPS_PMAP_H_

#include <mips/cpuregs.h>	/* for KSEG0 below */
//#include <mips/pte.h>

/*
 * The user address space is 2Gb (0x0 - 0x80000000).
 * User programs are laid out in memory as follows:
 *			address
 *	USRTEXT		0x00001000
 *	USRDATA		USRTEXT + text_size
 *	USRSTACK	0x7FFFFFFF
 *
 * The user address space is mapped using a two level structure where
 * virtual address bits 30..22 are used to index into a segment table which
 * points to a page worth of PTEs (4096 page can hold 1024 PTEs).
 * Bits 21..12 are then used to index a PTE which describes a page within
 * a segment.
 *
 * The wired entries in the TLB will contain the following:
 *	0-1	(UPAGES)	for curproc user struct and kernel stack.
 *
 * Note: The kernel doesn't use the same data structures as user programs.
 * All the PTE entries are stored in a single array in Sysmap which is
 * dynamically allocated at boot time.
 */

#define mips_trunc_seg(x)	((vaddr_t)(x) & ~SEGOFSET)
#define mips_round_seg(x)	(((vaddr_t)(x) + SEGOFSET) & ~SEGOFSET)

#define PMAP_SEGTABSIZE		(1 << (31 - SEGSHIFT))

union pt_entry;

struct segtab {
	union pt_entry	*seg_tab[PMAP_SEGTABSIZE];
};

/*
 * Structure defining an tlb entry data set.
 */
struct tlb {
	vaddr_t	tlb_hi;		/* should be 64 bits */
	uint32_t tlb_lo0;	/* XXX maybe 64 bits (only 32 really used) */
	uint32_t tlb_lo1;	/* XXX maybe 64 bits (only 32 really used) */
};

struct tlbmask {
	vaddr_t	tlb_hi;		/* should be 64 bits */
	uint32_t tlb_lo0;	/* XXX maybe 64 bits (only 32 really used) */
	uint32_t tlb_lo1;	/* XXX maybe 64 bits (only 32 really used) */
	uint32_t tlb_mask;
};

struct pmap;
typedef bool (*pte_callback_t)(struct pmap *, vaddr_t, vaddr_t,
	union pt_entry *, uintptr_t);
union pt_entry *pmap_pte_lookup(struct pmap *, vaddr_t);
union pt_entry *pmap_pte_reserve(struct pmap *, vaddr_t, int);
void pmap_pte_process(struct pmap *, vaddr_t, vaddr_t, pte_callback_t,
	uintptr_t);
void pmap_segtab_activate(struct lwp *);
void pmap_segtab_alloc(struct pmap *);
void pmap_segtab_free(struct pmap *);

/*
 * Per cpu asid info
 */
struct pmap_asid_info {
	uint32_t	pai_asid;	/* TLB address space tag */
	uint32_t	pai_asid_generation; /* its generation number */
	struct tlb	*pai_tlb;
};

#ifdef MULTIPROCESSOR
#define	PMAP_PAI(pmap, ci)	(&(pmap)->pm_pai[(ci)->ci_cpuid])
#else
#define	PMAP_PAI(pmap, ci)	(&(pmap)->pm_pai[0])
#endif
#define	PMAP_PAI_ASIDVALID_P(pai, ci)	\
		((pai)->pai_asid != (ci)->ci_pmap_asid_reserved \
		 && (pai)->pai_asid_generation == (ci)->ci_pmap_asid_generation)

/*
 * Machine dependent pmap structure.
 */
typedef struct pmap {
	kmutex_t		pm_lock;	/* lock on pmap */
	struct segtab		*pm_segtab;	/* pointers to pages of PTEs */
#ifdef MULTIPROCESSOR
	uint32_t		pm_cpus;	/* pmap was active on ... */
#endif
	int			pm_count;	/* pmap reference count */
	struct pmap_statistics	pm_stats;	/* pmap statistics */
	struct pmap_asid_info	pm_pai[1];
} *pmap_t;

/*
 * For each struct vm_page, there is a list of all currently valid virtual
 * mappings of that page.  An entry is a pv_entry_t, the list is pv_table.
 * XXX really should do this as a part of the higher level code.
 */
typedef struct pv_entry {
	struct pv_entry	*pv_next;	/* next pv_entry */
	struct pmap	*pv_pmap;	/* pmap where mapping lies */
	vaddr_t		pv_va;		/* virtual address for mapping */
	int		pv_flags;	/* some flags for the mapping */
} *pv_entry_t;

#define	PV_UNCACHED	0x0001		/* page is mapped uncached */
#define	PV_MODIFIED	0x0002		/* page has been modified */
#define	PV_REFERENCED	0x0004		/* page has been recently referenced */


#ifdef	_KERNEL

struct pmap_kernel {
	struct pmap kernel_pmap;
#ifdef MULTIPROCESSOR
	struct pmap_asid_info kernel_pai[MAXCPUS-1];
#endif
};

extern struct pmap_kernel kernel_pmap_store;
extern paddr_t mips_avail_start;
extern paddr_t mips_avail_end;
extern vaddr_t mips_virtual_end;

#define pmap_kernel()		(&kernel_pmap_store.kernel_pmap)
#define	pmap_wired_count(pmap) 	((pmap)->pm_stats.wired_count)
#define pmap_resident_count(pmap) ((pmap)->pm_stats.resident_count)

#define pmap_phys_address(x)	mips_ptob(x)

static __inline void
pmap_remove_all(struct pmap *pmap)
{
	/* Nothing. */
}

/*
 *	Bootstrap the system enough to run with virtual memory.
 */
void	pmap_bootstrap(void);

void	pmap_set_modified(paddr_t);
void	pmap_procwr(struct proc *, vaddr_t, size_t);
#define	PMAP_NEED_PROCWR

uint32_t pmap_tlb_asid_alloc(pmap_t pmap, struct cpu_info *ci);
void	pmap_tlb_invalidate_asid(pmap_t pmap);
int	pmap_tlb_update(pmap_t pmap, vaddr_t, uint32_t);
void	pmap_tlb_invalidate_addr(pmap_t pmap, vaddr_t);

/*
 * pmap_prefer() helps reduce virtual-coherency exceptions in
 * the virtually-indexed cache on mips3 CPUs.
 */
#ifdef MIPS3_PLUS
#define PMAP_PREFER(pa, va, sz, td)	pmap_prefer((pa), (va), (td))
void	pmap_prefer(vaddr_t, vaddr_t *, int);
#endif /* MIPS3_PLUS */

#define	PMAP_STEAL_MEMORY	/* enable pmap_steal_memory() */

/*
 * Alternate mapping hooks for pool pages.  Avoids thrashing the TLB.
 */
vaddr_t mips_pmap_map_poolpage(paddr_t);
paddr_t mips_pmap_unmap_poolpage(vaddr_t);
struct vm_page *mips_pmap_alloc_poolpage(int);
#define	PMAP_ALLOC_POOLPAGE(flags)	mips_pmap_alloc_poolpage(flags)
#define	PMAP_MAP_POOLPAGE(pa)		mips_pmap_map_poolpage(pa)
#define	PMAP_UNMAP_POOLPAGE(va)		mips_pmap_unmap_poolpage(va)

/*
 * Other hooks for the pool allocator.
 */
#ifdef _LP64
#define	POOL_VTOPHYS(va)	(MIPS_KSEG0_P(va) \
				    ? MIPS_KSEG0_TO_PHYS(va) \
				    : MIPS_XKPHYS_TO_PHYS(va))
#else
#define	POOL_VTOPHYS(va)	MIPS_KSEG0_TO_PHYS((vaddr_t)(va))
#endif

/*
 * Select CCA to use for unmanaged pages.
 */
#define	PMAP_CCA_FOR_PA(pa)	CCA_UNCACHED		/* uncached */

#if defined(_MIPS_PADDR_T_64BIT) || defined(_LP64)
#define PMAP_NOCACHE	0x4000000000000000ULL
#endif

#endif	/* _KERNEL */
#endif	/* _MIPS_PMAP_H_ */
