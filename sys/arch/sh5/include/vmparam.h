/*	$NetBSD: vmparam.h,v 1.5 2003/04/02 02:44:06 thorpej Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas <matt@3am-softwre.com> of Allegro Networks, Inc.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#ifndef _SH5_VMPARAM_H
#define _SH5_VMPARAM_H

/*
 * SH5 VM contants
 */

#include <sys/queue.h>

/*
 * Virtual Memory constants.
 *
 * The kernel always occupies the top 1GB of virtual address space,
 * regardless of whether it's running in 32- or 64-bit mode.
 *
 * User VM space stretches to fill what's left.
 */

/*
 * We use 4K pages on the sh5.  Override the PAGE_* definitions
 * to be compile-time constants.
 */
#define	PAGE_SHIFT	12
#define	PAGE_SIZE	(1 << PAGE_SHIFT)
#define	PAGE_MASK	(PAGE_SIZE - 1)

#ifdef _LP64
#define	VM_MIN_KERNEL_ADDRESS	(0xffffffffe0000000UL)
#define	VM_MAX_KERNEL_ADDRESS	(0xfffffffffffff000UL)
#else
#define	VM_MIN_KERNEL_ADDRESS	(0xe0000000UL)
#define	VM_MAX_KERNEL_ADDRESS	(0xfffff000UL)
#endif

#define	VM_MIN_ADDRESS		(0)
#ifdef _LP64
#define	VM_MAXUSER_ADDRESS	(0xffffffffc0000000UL - NBPG)
#else
#define	VM_MAXUSER_ADDRESS	(0xc0000000UL - NBPG)
#endif
#define	VM_MAX_ADDRESS		VM_MAXUSER_ADDRESS

/*
 * USRSTACK is the top of user stack. 
 */
#define USRSTACK	VM_MAXUSER_ADDRESS
#define USRSTACK32	USRSTACK

/* Virtual memory resoruce limit. */
#define MAXTSIZ                 (64 * 1024 * 1024)      /* max text size */
#ifndef MAXDSIZ
#define MAXDSIZ                 (512 * 1024 * 1024)     /* max data size */
#endif
#ifndef MAXSSIZ
#define MAXSSIZ                 (32 * 1024 * 1024)      /* max stack size */
#endif

/* initial data size limit */
#ifndef DFLDSIZ
#define DFLDSIZ                 (128 * 1024 * 1024)
#endif
/* initial stack size limit */
#ifndef DFLSSIZ
#define DFLSSIZ                 (4 * 1024 * 1024)
#endif

/*
 * Size of shared memory map
 */
#ifndef SHMMAXPGS
#define SHMMAXPGS               1024
#endif

/* Size of user raw I/O map */
#ifndef USRIOSIZE
#define USRIOSIZE               (MAXBSIZE / NBPG * 8)
#endif

#define VM_PHYS_SIZE            (USRIOSIZE * NBPG)

/*
 * Physical memory segments
 * Alloc port-specific vmparam.h to override
 */
#ifndef VM_PHYSSEG_MAX
#define VM_PHYSSEG_MAX		1
#endif
#ifndef VM_PHYSSEG_STRAT
#define VM_PHYSSEG_STRAT        VM_PSTRAT_BSEARCH
#endif
#ifndef VM_PHYSSEG_NOADD
#define VM_PHYSSEG_NOADD
#endif
#ifndef VM_NFREELIST
#define VM_NFREELIST		1
#define VM_FREELIST_DEFAULT	0
#endif

#ifndef _LP64
#define sh5_round_page(x)       ((((u_int32_t)(x)) + PGOFSET) & ~PGOFSET)
#define sh5_trunc_page(x)       ((u_int32_t)(x) & ~PGOFSET)
#define sh5_page_offset(x)      ((u_int32_t)(x) & PGOFSET)
#define sh5_btop(x)             ((u_int32_t)(x) >> PGSHIFT)
#define sh5_ptob(x)             ((u_int32_t)(x) << PGSHIFT)
#else
#define sh5_round_page(x)       ((((u_int64_t)(x)) + PGOFSET) & ~PGOFSET)
#define sh5_trunc_page(x)       ((u_int64_t)(x) & ~PGOFSET)
#define sh5_page_offset(x)      ((u_int64_t)(x) & PGOFSET)
#define sh5_btop(x)             ((u_int64_t)(x) >> PGSHIFT)
#define sh5_ptob(x)             ((u_int64_t)(x) << PGSHIFT)
#endif

/* pmap-specific data store in the vm_page structure. */
#define __HAVE_VM_PAGE_MD

#ifndef _LOCORE

LIST_HEAD(pvo_head, pvo_entry);

struct vm_page_md {
	struct pvo_head mdpg_pvoh;
	unsigned int mdpg_attrs;
};

#define VM_MDPAGE_INIT(pg)                                              \
do {                                                                    \
	LIST_INIT(&(pg)->mdpage.mdpg_pvoh);                             \
	(pg)->mdpage.mdpg_attrs = 0;                                    \
} while (/*CONSTCOND*/0)

#endif /* _LOCORE */

#endif /* _SH5_VMPARAM_H */
