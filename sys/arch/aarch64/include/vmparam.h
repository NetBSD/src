/* $NetBSD: vmparam.h,v 1.2 2014/08/11 22:08:34 matt Exp $ */

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

#ifndef _AARCH64_VMPARAM_H_
#define _AARCH64_VMPARAM_H_

#ifdef __aarch64__

/*
 * AARCH64 supports 3 page sizes: 4KB, 16KB, 64KB.  Each page table can
 * even have its own page size.
 */

#ifdef AARCH64_PAGE_SHIFT
#if (1 << AARCH64_PAGE_SHIFT) & ~0x141000
#error AARCH64_PAGE_SHIFT contains an unsupported value.
#endif
#define PAGE_SHIFT	AARCH64_PAGE_SHIFT
#else
#define PAGE_SHIFT	12
#endif
#define PAGE_SIZE	(1 << PAGE_SHIFT)
#define PAGE_MASK	(PAGE_SIZE - 1)

#if PAGE_SHIFT <= 14
#define USPACE		16384
#else
#define USPACE		65536
#endif
#define	UPAGES		(USPACE >> PAGE_SHIFT)

/*
 * USRSTACK is the top (end) of the user stack.  The user VA space is a
 * 48-bit address space starting at 0.  Place the stack at its top end.
 */
#define USRSTACK	((vaddr_t) 0x0000ffffffff0000)
#define USRSTACK32	((vaddr_t) 0x7ffff000)

#ifndef MAXTSIZ
#define	MAXTSIZ		(1L << 30)	/* max text size (1GB) */
#endif

#ifndef MAXTSIZ32
#define	MAXTSIZ32	(1L << 26)	/* 32bit max text size (64MB) */
#endif

#ifndef MAXDSIZ
#define	MAXDSIZ		(1L << 36)	/* max data size (64GB) */
#endif

#ifndef MAXSSIZ
#define	MAXSSIZ		(1L << 26)	/* max stack size (64MB) */
#endif

#ifndef DFLDSIZ
#define	DFLDSIZ		(1L << 32)	/* default data size (4GB) */
#endif

#ifndef DFLDSIZ32
#define	DFLDSIZ32	(1L << 27)	/* 32bit default data size (128MB) */
#endif

#ifndef DFLSSIZ
#define	DFLSSIZ		(1L << 23)	/* default stack size (8MB) */
#endif

#ifndef DFLSSIZ32
#define	DFLSSIZ32	(1L << 21)	/* 32bit default stack size (2MB) */
#endif

#define	VM_MIN_ADDRESS		((vaddr_t) 0x0)
#define	VM_MAXUSER_ADDRESS	((vaddr_t) (1L << 48) - PAGE_SIZE)
#define	VM_MAX_ADDRESS		VM_MAXUSER_ADDRESS

#define VM_MAXUSER_ADDRESS32	((vaddr_t) 0x7ffff000)

/*
 * Give ourselves 64GB of mappable kernel space.  That leaves the rest
 * to be user for directly mapped (block addressable) addresses. 
 */
#define VM_MIN_KERNEL_ADDRESS	((vaddr_t) 0xffffffc000000000L)
#define VM_MAX_KERNEL_ADDRESS	((vaddr_t) 0xffffffffffff0000L)

/* virtual sizes (bytes) for various kernel submaps */
#define USRIOSIZE		(PAGE_SIZE / 8)
#define VM_PHYS_SIZE		(USRIOSIZE * PAGE_SIZE)

/*
 * Since we have the address space, we map all of physical memory (RAM)
 * using block page table entries.
 */
#define AARCH64_KSEG_MASK	((vaddr_t) 0xffff000000000000L)
#define AARCH64_KSEG_START	AARCH64_KSEG_MASK
#define	AARCH64_KMEMORY_BASE	AARCH64_KSEG_MASK
#define AARCH64_KVA_P(va)	(((vaddr_t) (va) & AARCH64_KSEG_MASK) != 0)
#define AARCH64_PA_TO_KVA(pa)	((vaddr_t) ((pa) | AARCH64_KSEG_START))
#define AARCH64_KVA_TO_PA(va)	((paddr_t) ((pa) & ~AARCH64_KSEG_MASK))

/* */
#define VM_PHYSSEG_MAX		16              /* XXX */
#define VM_PHYSSEG_STRAT	VM_PSTRAT_BSEARCH

#define VM_NFREELIST		2
#define	VM_FREELIST_DEFAULT	0
#define VM_FREELIST_FIRST4GB	1

#elif defined(__arm__)

// These exist for building the RUMP libraries with MKCOMPAT

#define KERNEL_BASE		0x80000000
#define PGSHIFT			12
#define	NBPG			(1 << PGSHIFT)
#define VM_PHYSSEG_MAX		1
#define VM_NFREELIST		1
#define	VM_FREELIST_DEFAULT	0

#include <arm/vmparam.h>

#endif /* __aarch64__/__arm__ */

#endif /* _AARCH64_VMPARAM_H_ */
