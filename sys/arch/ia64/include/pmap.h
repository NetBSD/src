/*-
 * Copyright (c) 1998, 1999, 2000, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center and by Chris G. Demetriou.
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

/*-
 * Copyright (c) 1991 Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department and William Jolitz of UUNET Technologies Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
 * Derived from hp300 version by Mike Hibler, this version by William
 * Jolitz uses a recursive map [a pde points to the page directory] to
 * map the page tables using the pagetables themselves. This is done to
 * reduce the impact on kernel virtual memory for lots of sparse address
 * space, and to reduce the cost of memory to each process.
 *
 *	from: hp300: @(#)pmap.h	7.2 (Berkeley) 12/16/90
 *	from: @(#)pmap.h	7.4 (Berkeley) 5/12/91
 *	from: i386 pmap.h,v 1.54 1997/11/20 19:30:35 bde Exp
 * $FreeBSD: releng/10.1/sys/ia64/include/pmap.h 268201 2014-07-02 23:57:55Z marcel $
 */

#ifndef _IA64_PMAP_H_
#define _IA64_PMAP_H_

#include <sys/types.h>
#include <sys/queue.h>
#include <sys/lock.h>
#include <sys/mutex.h>

#include <machine/pte.h>
#include <machine/vmparam.h>

#include <machine/md_var.h>

typedef char	vm_memattr_t;

#define VM_MEMATTR_WRITE_BACK           ((vm_memattr_t)PTE_MA_WB)
#define VM_MEMATTR_UNCACHEABLE          ((vm_memattr_t)PTE_MA_UC)
#define VM_MEMATTR_UNCACHEABLE_EXPORTED ((vm_memattr_t)PTE_MA_UCE)
#define VM_MEMATTR_WRITE_COMBINING      ((vm_memattr_t)PTE_MA_WC)
#define VM_MEMATTR_NATPAGE              ((vm_memattr_t)PTE_MA_NATPAGE)
#define VM_MEMATTR_DEFAULT              VM_MEMATTR_WRITE_BACK

#ifdef _KERNEL

#define MAXKPT		(PAGE_SIZE/sizeof(vaddr_t))

#define	vtophys(va)	pmap_kextract((vaddr_t)(va))

#endif /* _KERNEL */

/*
 * Pmap stuff
 */
struct	pv_entry;
struct	pv_chunk;

struct pmap {
	kmutex_t		pm_mtx;
	TAILQ_HEAD(,pv_chunk)	pm_pvchunk;	/* list of mappings in pmap */
	uint32_t		pm_rid[IA64_VM_MINKERN_REGION];
	struct pmap_statistics	pm_stats;	/* pmap statistics */
	uint64_t		pm_refcount;	/* pmap reference count, atomic */
};

typedef struct pmap *pmap_t;

#ifdef _KERNEL

#define	PMAP_LOCK(pmap)		mutex_enter(&(pmap)->pm_mtx)
#define	PMAP_LOCK_ASSERT(pmap)	KASSERT(mutex_owned(&(pmap)->pm_mtx))
#define	PMAP_LOCK_DESTROY(pmap)	mutex_destroy(&(pmap)->pm_mtx)
#define	PMAP_LOCK_INIT(pmap)	mutex_init(&(pmap)->pm_mtx, MUTEX_DEFAULT, IPL_NONE)
#define	PMAP_LOCKED(pmap)	mutex_owned(&(pmap)->pm_mtx)
#define	PMAP_MTX(pmap)		(&(pmap)->pm_mtx)
#define	PMAP_TRYLOCK(pmap)	mutex_tryenter(&(pmap)->pm_mtx)
#define	PMAP_UNLOCK(pmap)	mutex_exit(&(pmap)->pm_mtx)

#endif

/*
 * For each vm_page_t, there is a list of all currently valid virtual
 * mappings of that page.  An entry is a pv_entry_t, the list is pv_list.
 */
typedef struct pv_entry {
	vaddr_t	pv_va;		/* virtual address for mapping */
	TAILQ_ENTRY(pv_entry)	pv_list;
} *pv_entry_t;

void pmap_bootstrap(void);

/* optional pmap API functions, according to pmap(9) */
#define PMAP_STEAL_MEMORY
#define PMAP_GROWKERNEL

#define PMAP_NEED_PROCWR
void pmap_procwr(struct proc *, vaddr_t, vsize_t);

/*
 * Alternate mapping hooks for pool pages.  Avoids thrashing the TLB.
 */
/* XXX
#define	PMAP_MAP_POOLPAGE(pa)		IA64_PHYS_TO_RR7((pa))
#define	PMAP_UNMAP_POOLPAGE(va)		IA64_RR_MASK((va))
*/

/*
 * Macros for locking pmap structures.
 *
 * Note that we if we access the kernel pmap in interrupt context, it
 * is only to update statistics.  Since stats are updated using atomic
 * operations, locking the kernel pmap is not necessary.  Therefore,
 * it is not necessary to block interrupts when locking pmap strucutres.
 */
/* XXX
#define	PMAP_LOCK(pmap)		mutex_enter(&(pmap)->pm_slock)
#define	PMAP_UNLOCK(pmap)	mutex_exit(&(pmap)->pm_slock)
*/

/*
 * pmap-specific data store in the vm_page structure.
 */
#define	__HAVE_VM_PAGE_MD

struct vm_page_md {
	TAILQ_HEAD(,pv_entry)	pv_list;
	vm_memattr_t		memattr;
#if 0 /* XXX freebsd */	
	uint8_t		pv_flags;
	uint8_t		aflags;
#endif	
};

#define	VM_MDPAGE_INIT(pg)						\
do {									\
	TAILQ_INIT(&(pg)->mdpage.pv_list);				\
	(pg)->mdpage.memattr = VM_MEMATTR_DEFAULT;			\
} while (/*CONSTCOND*/0)

#ifdef	_KERNEL

extern uint64_t pmap_vhpt_base[];
extern uint64_t pmap_vhpt_log2size;

vaddr_t pmap_page_to_va(struct vm_page*);

/* Machine-architecture private */
vaddr_t pmap_alloc_vhpt(void);
void pmap_bootstrap(void);
void pmap_invalidate_all(void);
paddr_t pmap_kextract(vaddr_t va);
struct pmap *pmap_switch(struct pmap *pmap);
void pmap_remove_all_phys(struct vm_page*);  /* used in only pmap_page_protect */

#endif /* _KERNEL */

#endif /* _IA64_PMAP_H_ */
