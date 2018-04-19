/*	$NetBSD: vmpagemd.h,v 1.8 2018/04/19 21:50:10 christos Exp $	*/

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Raytheon BBN Technologies Corp and Defense Advanced Research Projects
 * Agency and which was developed by Matt Thomas of 3am Software Foundry.
 *
 * This material is based upon work supported by the Defense Advanced Research
 * Projects Agency and Space and Naval Warfare Systems Center, Pacific, under
 * Contract No. N66001-09-C-2073.
 * Approved for Public Release, Distribution Unlimited
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

#ifndef _UVM_PMAP_VMPAGEMD_H_
#define _UVM_PMAP_VMPAGEMD_H_

#ifdef _LOCORE
#error use assym.h instead
#endif

//#ifdef _MODULE
//#error this file should not be included by loadable kernel modules
//#endif

#ifdef _KERNEL_OPT
#include "opt_modular.h"
#include "opt_multiprocessor.h"
#endif

#include <sys/atomic.h>
#include <sys/mutex.h>

#define	__HAVE_VM_PAGE_MD

typedef struct pv_entry {
	struct pv_entry *pv_next;
	struct pmap *pv_pmap;
	vaddr_t pv_va;
#define	PV_KENTER		__BIT(0)
} *pv_entry_t;

#ifndef _MODULE

#define	VM_PAGEMD_REFERENCED	__BIT(0)	/* page has been referenced */
#define	VM_PAGEMD_MODIFIED	__BIT(1)	/* page has been modified */
#define	VM_PAGEMD_POOLPAGE	__BIT(2)	/* page is used as a poolpage */
#define	VM_PAGEMD_EXECPAGE	__BIT(3)	/* page is exec mapped */
#ifdef PMAP_VIRTUAL_CACHE_ALIASES
#define	VM_PAGEMD_UNCACHED	__BIT(4)	/* page is mapped uncached */
#endif

#ifdef PMAP_VIRTUAL_CACHE_ALIASES
#define	VM_PAGEMD_CACHED_P(mdpg)	(((mdpg)->mdpg_attrs & VM_PAGEMD_UNCACHED) == 0)
#define	VM_PAGEMD_UNCACHED_P(mdpg)	(((mdpg)->mdpg_attrs & VM_PAGEMD_UNCACHED) != 0)
#endif
#define	VM_PAGEMD_MODIFIED_P(mdpg)	(((mdpg)->mdpg_attrs & VM_PAGEMD_MODIFIED) != 0)
#define	VM_PAGEMD_REFERENCED_P(mdpg)	(((mdpg)->mdpg_attrs & VM_PAGEMD_REFERENCED) != 0)
#define	VM_PAGEMD_POOLPAGE_P(mdpg)	(((mdpg)->mdpg_attrs & VM_PAGEMD_POOLPAGE) != 0)
#define	VM_PAGEMD_EXECPAGE_P(mdpg)	(((mdpg)->mdpg_attrs & VM_PAGEMD_EXECPAGE) != 0)

#endif /* !_MODULE */

struct vm_page_md {
	volatile unsigned long mdpg_attrs;	/* page attributes */
	struct pv_entry mdpg_first;		/* pv_entry first */
#if defined(MULTIPROCESSOR) || defined(MODULAR) || defined(_MODULE)
	kmutex_t *mdpg_lock;			/* pv list lock */
#endif
};

#ifndef _MODULE
#if defined(MULTIPROCESSOR) || defined(MODULAR)
#define	VM_PAGEMD_PVLIST_LOCK_INIT(mdpg) 	\
	(mdpg)->mdpg_lock = NULL
#else
#define	VM_PAGEMD_PVLIST_LOCK_INIT(mdpg)	do { } while (/*CONSTCOND*/ 0)
#endif /* MULTIPROCESSOR || MODULAR */

#define	VM_PAGEMD_PVLIST_LOCK(mdpg)		\
	pmap_pvlist_lock(mdpg, 1)
#define	VM_PAGEMD_PVLIST_READLOCK(mdpg)		\
	pmap_pvlist_lock(mdpg, 0)
#define	VM_PAGEMD_PVLIST_UNLOCK(mdpg)		\
	pmap_pvlist_unlock(mdpg)
#define	VM_PAGEMD_PVLIST_LOCKED_P(mdpg)		\
	pmap_pvlist_locked_p(mdpg)
#define	VM_PAGEMD_PVLIST_GEN(mdpg)		\
	((mdpg)->mdpg_attrs >> 16)

#ifdef _KERNEL
#if defined(MULTIPROCESSOR) || defined(MODULAR)
kmutex_t *pmap_pvlist_lock_addr(struct vm_page_md *);
#else
extern kmutex_t pmap_pvlist_mutex;
static __inline kmutex_t *
pmap_pvlist_lock_addr(struct vm_page_md *mdpg)
{
	return &pmap_pvlist_mutex;
}
#endif

static __inline uintptr_t
pmap_pvlist_lock(struct vm_page_md *mdpg, uintptr_t increment)
{
	mutex_spin_enter(pmap_pvlist_lock_addr(mdpg));
	const uintptr_t gen = VM_PAGEMD_PVLIST_GEN(mdpg);
	mdpg->mdpg_attrs += increment << 16;
	return gen;
}

static __inline uintptr_t
pmap_pvlist_unlock(struct vm_page_md *mdpg)
{
	const uintptr_t gen = VM_PAGEMD_PVLIST_GEN(mdpg);
	mutex_spin_exit(pmap_pvlist_lock_addr(mdpg));
	return gen;
}

static __inline bool
pmap_pvlist_locked_p(struct vm_page_md *mdpg)
{
	return mutex_owned(pmap_pvlist_lock_addr(mdpg));
}
#endif /* _KERNEL */

#define VM_MDPAGE_INIT(pg)						\
do {									\
	(pg)->mdpage.mdpg_first.pv_next = NULL;				\
	(pg)->mdpage.mdpg_first.pv_pmap = NULL;				\
	(pg)->mdpage.mdpg_first.pv_va = (pg)->phys_addr;		\
	(pg)->mdpage.mdpg_attrs = 0;					\
	VM_PAGEMD_PVLIST_LOCK_INIT(&(pg)->mdpage);			\
} while (/* CONSTCOND */ 0)

#endif /* _MODULE */

#endif /* _UVM_PMAP_VMPAGEMD_H_ */
