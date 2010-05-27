/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry, LLC.
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
#define	MAXDSIZ		(13*256*1024*1024)	/* maximum data size */
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
#define	VM_MAXUSER_ADDRESS	((vaddr_t) -PAGE_SIZE)
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
	(((VM_MAXUSER_ADDRESS - MAXSSIZ) - round_page(sz))

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

#ifndef _LOCORE

#define	__HAVE_VM_PAGE_MD

struct pv_entry {
	struct pv_entry *pv_next;
	struct pmap *pv_pmap;
	vaddr_t pv_va;
};

struct vm_page_md {
	struct pv_entry mdpg_pvlist;
	unsigned int mdpg_attrs; 
};

#define	VM_MDPAGE_INIT(pg) do {				\
	(pg)->mdpage.mdpg_pvlist.pv_next = NULL;	\
	(pg)->mdpage.mdpg_pvlist.pv_pmap = NULL;	\
	(pg)->mdpage.mdpg_pvlist.pv_va = 0;		\
	(pg)->mdpage.mdpg_attrs = 0;			\
} while (/*CONSTCOND*/0)

#endif	/* _LOCORE */

#endif /* _POWERPC_BOOKE_VMPARAM_H_ */
