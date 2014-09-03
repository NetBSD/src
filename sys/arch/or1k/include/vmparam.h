/* $NetBSD: vmparam.h,v 1.1 2014/09/03 19:34:26 matt Exp $ */

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

#ifndef _OR1K_VMPARAM_H_
#define _OR1K_VMPARAM_H_

/*
 * OR1K supports 1 page size: 8KB.
 */

#define PAGE_SHIFT	13
#define PAGE_SIZE	(1 << PAGE_SHIFT)
#define PAGE_MASK	(PAGE_SIZE - 1)

#define USPACE		16384
#define	UPAGES		(USPACE >> PAGE_SHIFT)

/*
 * USRSTACK is the top (end) of the user stack.  The user VA space is a
 * 32-bit address space starting at 0.  Place the stack at its top end.
 */
#define USRSTACK	((vaddr_t) 0x80000000U - PAGE_SIZE)

#ifndef MAXTSIZ
#define	MAXTSIZ		(1UL << 26)	/* 32bit max text size (64MB) */
#endif

#ifndef MAXDSIZ
#define	MAXDSIZ		(1UL << 30)	/* max data size (1024MB) */
#endif

#ifndef MAXSSIZ
#define	MAXSSIZ		(1UL << 26)	/* max stack size (64MB) */
#endif

#ifndef DFLDSIZ
#define	DFLDSIZ		(1UL << 27)	/* 32bit default data size (128MB) */
#endif

#ifndef DFLSSIZ
#define	DFLSSIZ		(1UL << 21)	/* 32bit default stack size (2MB) */
#endif

#define	VM_MIN_ADDRESS		((vaddr_t) 0x0)
#define	VM_MAXUSER_ADDRESS	((vaddr_t) 0x80000000 - PAGE_SIZE)
#define	VM_MAX_ADDRESS		VM_MAXUSER_ADDRESS

/*
 * Give ourselves 64GB of mappable kernel space.  That leaves the rest
 * to be user for directly mapped (block addressable) addresses. 
 */
#define VM_MIN_KERNEL_ADDRESS	((vaddr_t) 0x80000000L)
#define VM_MAX_KERNEL_ADDRESS	((vaddr_t) -PAGE_SIZE)

/* virtual sizes (bytes) for various kernel submaps */
#define USRIOSIZE		(PAGE_SIZE / 8)
#define VM_PHYS_SIZE		(USRIOSIZE * PAGE_SIZE)

/* */
#define VM_PHYSSEG_MAX		16              /* XXX */
#define VM_PHYSSEG_STRAT	VM_PSTRAT_BSEARCH

#define VM_NFREELIST		1
#define	VM_FREELIST_DEFAULT	0

#endif /* _OR1K_VMPARAM_H_ */
