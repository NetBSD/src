/*	$NetBSD: vmparam.h,v 1.3 2011/01/18 01:02:54 matt Exp $	*/
/*-
 * Copyright (c) 2010, 2011 The NetBSD Foundation, Inc.
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

#ifndef _POWERPC_BOOKE_VMPARAM_H_
#define _POWERPC_BOOKE_VMPARAM_H_

/*
 * Most of the definitions in this can be overriden by a machine-specific
 * vmparam.h if required.  Otherwise a port can just include this file
 * get the right thing to happen.
 */

/*
 * BookE processors have 4K pages.  Override the PAGE_* definitions to be
 * compile-time constants.
 */
#define	PAGE_SHIFT	12
#define	PAGE_SIZE	(1 << PAGE_SHIFT)
#define	PAGE_MASK	(PAGE_SIZE - 1)

#ifndef	USRSTACK
#define	USRSTACK	VM_MAXUSER_ADDRESS
#endif

#ifndef	MAXTSIZ
#define	MAXTSIZ		(2*256*1024*1024)	/* maximum text size */
#endif

#ifndef	MAXDSIZ
#define	MAXDSIZ		(13*256*1024*1024U)	/* maximum data size */
#endif

#ifndef	MAXSSIZ
#define	MAXSSIZ		(1*256*1024*1024-PAGE_SIZE) /* maximum stack size */
#endif

#ifndef	DFLDSIZ
#define	DFLDSIZ		(256*1024*1024)		/* default data size */
#endif

#ifndef	DFLSSIZ
#define	DFLSSIZ		(2*1024*1024)		/* default stack size */
#endif

/*
 * Default number of pages in the user raw I/O map.
 */
#ifndef USRIOSIZE
#define	USRIOSIZE	1024
#endif

/*
 * The number of seconds for a process to be blocked before being
 * considered very swappable.
 */
#ifndef MAXSLP
#define	MAXSLP		20
#endif

/*
 * Some system constants
 */

#define	VM_MIN_ADDRESS		((vaddr_t) 0)
#define	VM_MAXUSER_ADDRESS	((vaddr_t) /* -32768 */ 0x7fff8000)
#define	VM_MAX_ADDRESS		VM_MAXUSER_ADDRESS
#define	VM_MIN_KERNEL_ADDRESS	((vaddr_t) 0xe4000000)
#define	VM_MAX_KERNEL_ADDRESS	((vaddr_t) 0xfefff000)

/*
 * The address to which unspecified mapping requests default
 * Put the stack in it's own segment and start mmaping at the
 * top of the next lower segment.
 */
#ifdef _KERNEL_OPT
#include "opt_uvm.h"
#endif
#define	__USE_TOPDOWN_VM
#define	VM_DEFAULT_ADDRESS(da, sz) \
	((VM_MAXUSER_ADDRESS - MAXSSIZ) - round_page(sz))

#ifndef VM_PHYSSEG_MAX
#define	VM_PHYSSEG_MAX		16
#endif
#define	VM_PHYSSEG_STRAT	VM_PSTRAT_BIGFIRST

#ifndef VM_PHYS_SIZE
#define	VM_PHYS_SIZE		(USRIOSIZE * PAGE_SIZE)
#endif

#define	VM_NFREELIST		2	/* 16 distinct memory segments */
#define	VM_FREELIST_DEFAULT	0
#define	VM_FREELIST_FIRST16	1
#define	VM_FREELIST_MAX		2

#ifndef VM_NFREELIST
#define	VM_NFREELIST		16	/* 16 distinct memory segments */
#define VM_FREELIST_DEFAULT	0
#define VM_FREELIST_MAX		1
#endif

#define	__HAVE_VM_PAGE_MD
#ifndef _LOCORE

typedef struct pv_entry {
	struct pv_entry *pv_next;
	struct pmap *pv_pmap;
	vaddr_t pv_va;
} *pv_entry_t;

#define	VM_PAGE_MD_REFERENCED	0x0001	/* page has been recently referenced */
#define	VM_PAGE_MD_MODIFIED	0x0002	/* page has been modified */
#define	VM_PAGE_MD_POOLPAGE	0x0004	/* page is used as a poolpage */
#define	VM_PAGE_MD_EXECPAGE	0x0008	/* page is exec mapped */
#if 0
#define	VM_PAGE_MD_UNCACHED	0x0010	/* page is mapped uncached */
#endif

#ifdef VM_PAGE_MD_UNCACHED
#define	VM_PAGE_MD_CACHED_P(pg)	(((pg)->mdpage.mdpg_attrs & VM_PAGE_MD_UNCACHED) == 0)
#define	VM_PAGE_MD_UNCACHED_P(pg)	(((pg)->mdpage.mdpg_attrs & VM_PAGE_MD_UNCACHED) != 0)
#endif
#define	VM_PAGE_MD_MODIFIED_P(pg)	(((pg)->mdpage.mdpg_attrs & VM_PAGE_MD_MODIFIED) != 0)
#define	VM_PAGE_MD_REFERENCED_P(pg)	(((pg)->mdpage.mdpg_attrs & VM_PAGE_MD_REFERENCED) != 0)
#define	VM_PAGE_MD_POOLPAGE_P(pg)	(((pg)->mdpage.mdpg_attrs & VM_PAGE_MD_POOLPAGE) != 0)
#define	VM_PAGE_MD_EXECPAGE_P(pg)	(((pg)->mdpage.mdpg_attrs & VM_PAGE_MD_EXECPAGE) != 0)

struct vm_page_md {
	struct pv_entry mdpg_first;	/* pv_entry first */
#ifdef MULTIPROCESSOR
	volatile u_int mdpg_attrs;	/* page attributes */
	kmutex_t *mdpg_lock;		/* pv list lock */
#define	VM_PAGE_PVLIST_LOCK_INIT(pg) 		\
	(pg)->mdpage.mdpg_lock = NULL
#define	VM_PAGE_PVLIST_LOCKED_P(pg)		\
	(mutex_owner((pg)->mdpage.mdpg_lock) != 0)
#define	VM_PAGE_PVLIST_LOCK(pg, list_change)	\
	pmap_pvlist_lock(pg, list_change)
#define	VM_PAGE_PVLIST_UNLOCK(pg)		\
	mutex_spin_exit((pg)->mdpage.mdpg_lock);
#define	VM_PAGE_PVLIST_GEN(pg)		((uint16_t)(pg->mdpage.mdpg_attrs >> 16))
#else
	u_int mdpg_attrs;		/* page attributes */
#define	VM_PAGE_PVLIST_LOCK_INIT(pg)	do { } while (/*CONSTCOND*/ 0)
#define	VM_PAGE_PVLIST_LOCKED_P(pg)	true
#define	VM_PAGE_PVLIST_LOCK(pg, lc)	(mutex_spin_enter(&pmap_pvlist_mutex), 0)
#define	VM_PAGE_PVLIST_UNLOCK(pg)	mutex_spin_exit(&pmap_pvlist_mutex)
#define	VM_PAGE_PVLIST_GEN(pg)		(0)
#endif
};

#define VM_MDPAGE_INIT(pg)						\
do {									\
	(pg)->mdpage.mdpg_first.pv_next = NULL;				\
	(pg)->mdpage.mdpg_first.pv_pmap = NULL;				\
	(pg)->mdpage.mdpg_first.pv_va = (pg)->phys_addr;			\
	VM_PAGE_PVLIST_LOCK_INIT(pg);					\
	(pg)->mdpage.mdpg_attrs = 0;					\
} while (/* CONSTCOND */ 0)

#endif /* _LOCORE */

#endif /* _POWERPC_BOOKE_VMPARAM_H_ */
