/*	$NetBSD: vmparam.h,v 1.7.36.2 2017/08/28 17:51:42 skrll Exp $	*/

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
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
 *	@(#)vmparam.h	5.9 (Berkeley) 5/12/91
 */

#ifndef _VMPARAM_H_
#define _VMPARAM_H_

#define	USRSTACK	VM_MAX_ADDRESS /* XXX: Revisit vm address space. */

/*
 * Virtual memory related constants, all in bytes
 */
#ifndef MAXTSIZ
#define	MAXTSIZ		(1<<30)			/* max text size (1G) */
#endif
#ifndef DFLDSIZ
#define	DFLDSIZ		(1<<27)			/* initial data size (128M) */
#endif
#ifndef MAXDSIZ
#define	MAXDSIZ		(1<<30)			/* max data size (1G) */
#endif
#ifndef	DFLSSIZ
#define	DFLSSIZ		(1<<21)			/* initial stack size (2M) */
#endif
#ifndef	MAXSSIZ
#define	MAXSSIZ		(1<<28)			/* max stack size (256M) */
#endif
#ifndef SGROWSIZ
#define SGROWSIZ	(128UL*1024)		/* amount to grow stack */
#endif

#define	IA64_VM_MINKERN_REGION	4

/*
 * PTEs for mapping user space into the kernel for phyio operations.
 * 64 pte's are enough to cover 8 disks * MAXBSIZE.
 */
#ifndef USRIOSIZE
#define USRIOSIZE	64
#endif

/*
 * Manipulating region bits of an address.
 */
#define IA64_RR_BASE(n)         (((uint64_t) (n)) << 61)
#define IA64_RR_MASK(x)         ((x) & ((1L << 61) - 1))

#define IA64_PHYS_TO_RR6(x)     ((x) | IA64_RR_BASE(6))
#define IA64_PHYS_TO_RR7(x)     ((x) | IA64_RR_BASE(7))

/*
 * The Itanium architecture defines that all implementations support at
 * least 51 virtual address bits (i.e. IMPL_VA_MSB=50). The unimplemented
 * bits are sign-extended from VA{IMPL_VA_MSB}. As such, there's a gap in
 * the virtual address range, which extends at most from 0x0004000000000000
 * to 0x1ffbffffffffffff. We define the top half of a region in terms of
 * this worst-case gap.
 */
#define IA64_REGION_GAP_START   0x0004000000000000
#define IA64_REGION_GAP_EXTEND  0x1ffc000000000000

/*
 * Parameters for Pre-Boot Virtual Memory (PBVM).
 * The kernel, its modules and metadata are loaded in the PBVM by the loader.
 * The PBVM consists of pages for which the mapping is maintained in a page
 * table. The page table is at least 1 EFI page large (i.e. 4KB), but can be
 * larger to accommodate more PBVM. The maximum page table size is 1MB. With
 * 8 bytes per page table entry, this means that the PBVM has at least 512
 * pages and at most 128K pages.
 * The GNU toolchain (in particular GNU ld) does not support an alignment
 * larger than 64K. This means that we cannot guarantee page alignment for
 * a page size that's larger than 64K. We do want to have text and data in
 * different pages, which means that the maximum usable page size is 64KB.
 * Consequently:
 * The maximum total PBVM size is 8GB -- enough for a DVD image. A page table
 * of a single EFI page (4KB) allows for 32MB of PBVM.
 *
 * The kernel is given the PA and size of the page table that provides the
 * mapping of the PBVM. The page table itself is assumed to be mapped at a
 * known virtual address and using a single translation wired into the CPU.
 * As such, the page table is assumed to be a power of 2 and naturally aligned.
 * The kernel also assumes that a good portion of the kernel text is mapped
 * and wired into the CPU, but does not assume that the mapping covers the
 * whole of PBVM.
 */
#define IA64_PBVM_RR            IA64_VM_MINKERN_REGION
#define IA64_PBVM_BASE          \
                (IA64_RR_BASE(IA64_PBVM_RR) + IA64_REGION_GAP_EXTEND)

#define IA64_PBVM_PGTBL_MAXSZ   1048576
#define IA64_PBVM_PGTBL         \
                (IA64_RR_BASE(IA64_PBVM_RR + 1) - IA64_PBVM_PGTBL_MAXSZ)

#define IA64_PBVM_PAGE_SHIFT    16      /* 64KB */
#define IA64_PBVM_PAGE_SIZE     (1 << IA64_PBVM_PAGE_SHIFT)
#define IA64_PBVM_PAGE_MASK     (IA64_PBVM_PAGE_SIZE - 1)
  
#define	IA64_ID_PAGE_SHIFT	28		/* 256M */
#define	IA64_ID_PAGE_SIZE	(1 << IA64_ID_PAGE_SHIFT)
#define	IA64_ID_PAGE_MASK	(IA64_ID_PAGE_SIZE-1)

/* XXX freebsd uses
#define IA64_BACKINGSTORE       (USRSTACK - (2 * MAXSSIZ) - PAGE_SIZE)
*/
#define	IA64_BACKINGSTORE	IA64_RR_BASE(4)

#define	PAGE_SHIFT	14		/* 16K pages by default. */
#define	PAGE_SIZE	(1 << PAGE_SHIFT)
#define	PAGE_MASK	(PAGE_SIZE - 1)

/* user/kernel map constants */
#define VM_MIN_ADDRESS		((vaddr_t)0)
#define	VM_MAX_ADDRESS		((vaddr_t) IA64_RR_BASE(5))
#define	VM_GATEWAY_SIZE		PAGE_SIZE
#define	VM_MAXUSER_ADDRESS	(VM_MAX_ADDRESS + VM_GATEWAY_SIZE)
#define	VM_MIN_KERNEL_ADDRESS	VM_MAXUSER_ADDRESS
#define VM_INIT_KERNEL_ADDRESS  IA64_RR_BASE(IA64_VM_MINKERN_REGION + 1)
#define VM_MAX_KERNEL_ADDRESS	((vaddr_t) (IA64_RR_BASE(6) - 1))

#define VM_PHYSSEG_MAX		16		/* XXX: */
#define VM_PHYSSEG_STRAT	VM_PSTRAT_BSEARCH

#define	VM_NFREELIST		1 /* XXX: */
#define	VM_FREELIST_DEFAULT	0 /* XXX: */

/* virtual sizes (bytes) for various kernel submaps */
#define VM_PHYS_SIZE		(USRIOSIZE*PAGE_SIZE)

#endif /* _VMPARAM_H_ */
