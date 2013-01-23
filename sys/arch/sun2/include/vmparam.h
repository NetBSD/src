/*	$NetBSD: vmparam.h,v 1.14.8.1 2013/01/23 00:05:58 yamt Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
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

#define __USE_TOPDOWN_VM

/*
 * Machine dependent constants for Sun2
 *
 * The Sun2 has limited total kernel virtual space (14MB) and
 * can not use main memory for page tables.  (All active PTEs
 * must be installed in special translation RAM in the MMU).
 * Therefore, parameters that would normally configure the
 * size of various page tables are irrelevant.  Only things
 * that consume portions of kernel virtual (KV) space matter,
 * and those things should be chosen to conserve KV space.
 */

/*
 * The Sun2 has 2K pages.  Override PAGE_* to be compile-time constants.
 */
#define	PAGE_SHIFT	11
#define	PAGE_SIZE	(1 << PAGE_SHIFT)
#define	PAGE_MASK	(PAGE_SIZE - 1)

/*
 * We definitely need a small pager map.
 */
#define	PAGER_MAP_DEFAULT_SIZE (1 * 1024 * 1024)

/*
 * USRSTACK is the top (end) of the user stack.
 */
#define	USRSTACK	0x1000000	/* High end of user stack */

/*
 * Virtual memory related constants, all in bytes.
 * The Sun2 has only 16 MB of user-virtual space,
 * so we need to be conservative with these limits.
 */
#ifndef MAXTSIZ
#define	MAXTSIZ		(5*1024*1024)		/* max text size */
#endif
#ifndef DFLDSIZ
#define	DFLDSIZ		(4*1024*1024)		/* initial data size limit */
#endif
#ifndef MAXDSIZ
#define	MAXDSIZ		(6*1024*1024)		/* max data size */
#endif
#ifndef	DFLSSIZ
#define	DFLSSIZ		(512*1024)		/* initial stack size limit */
#endif
#ifndef	MAXSSIZ
#define	MAXSSIZ		(4*1024*1024)		/* max stack size */
#endif

/*
 * PTEs for mapping user space into the kernel for phyio operations.
 * The actual limitation for physio requests will be the DVMA space,
 * and that is fixed by hardware design at 256K.  We could make the
 * physio map larger than that, but it would not buy us much.
 */
#ifndef USRIOSIZE
#define USRIOSIZE	128		/* 256K */
#endif

/*
 * Mach-derived constants:
 */

/* user/kernel map constants */
#define VM_MIN_ADDRESS		((vaddr_t)0)
#define VM_MAX_ADDRESS		((vaddr_t)USRSTACK)
#define VM_MAXUSER_ADDRESS	((vaddr_t)USRSTACK)
#define VM_MIN_KERNEL_ADDRESS	((vaddr_t)KERNBASE)
#define VM_MAX_KERNEL_ADDRESS	((vaddr_t)KERN_END)

/* virtual sizes (bytes) for various kernel submaps */
#define VM_PHYS_SIZE		(USRIOSIZE*PAGE_SIZE)

#define VM_PHYSSEG_STRAT	VM_PSTRAT_BSEARCH

#define	VM_NFREELIST		1
#define	VM_FREELIST_DEFAULT	0

#ifdef	_LKM
#undef	KERNBASE
extern	char KERNBASE[];
#endif	/* _LKM */

/* This is needed by some LKMs. */
#define VM_PHYSSEG_MAX		4
