/*	$NetBSD: vmparam.h,v 1.4 2026/04/30 05:46:13 thorpej Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1986, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 * from: Utah $Hdr: vmparam.h 1.16 91/01/18$
 *
 *	@(#)vmparam.h	8.2 (Berkeley) 4/19/94
 */

#ifndef _M68K_VMPARAM_H_
#define	_M68K_VMPARAM_H_

/*
 * Common constants for m68k ports
 */

/*
 * hp300 pmap derived m68k ports can use 4K or 8K pages.
 * (except HPMMU machines, that support only 4K page)
 * sun3 and sun3x use 8K pages.
 * sun2 has 2K pages.
 * The page size is specified by PGSHIFT in <machine/param.h>.
 * Override the PAGE_* definitions to be compile-time constants.
 */
#define	PAGE_SHIFT	PGSHIFT
#define	PAGE_SIZE	(1 << PAGE_SHIFT)
#define	PAGE_MASK	(PAGE_SIZE - 1)

/* Some implemantations like jemalloc(3) require physical page size details. */
/*
 * XXX:
 * <uvm/uvm_param.h> assumes PAGE_SIZE is not a constant macro
 * but a variable (*uvmexp_pagesize) on MODULE builds in case of
 * (MIN_PAGE_SIZE != MAX_PAGE_SIZE).  For now we define these macros
 * for m68k ports only on !_KERNEL (currently just for jemalloc) builds.
 */
#if !defined(_KERNEL)
#if defined(__mc68010__)
/*
 * As of now, there is only a single PAGE_SHIFT value here (11),
 * so don't define anything differently.
 */
#else
#define	MIN_PAGE_SHIFT	12
#define	MAX_PAGE_SHIFT	13
#define	MIN_PAGE_SIZE	(1 << MIN_PAGE_SHIFT)
#define	MAX_PAGE_SIZE	(1 << MAX_PAGE_SHIFT)
#endif /* __mc68010__ */
#endif /* !_KERNEL */

/*
 * Virtual memory related constants, all in bytes
 *
 * Note: the 68010 has only 16MB of user virtual address space,
 * so we need to be extremely conservative about those limits
 * on those systems.
 */
#if defined(__mc68010__)
#define	__M68K_DFLT_MAXTSIZ	(5*1024*1024)
#define	__M68K_DFLT_DFLDSIZ	(4*1024*1024)
#define	__M68K_DFLT_MAXDSIZ	(6*1024*1024)
#define	__M68K_DFLT_DFLSSIZ	(512*1024)
#define	__M68K_DFLT_MAXSSIZ	(4*1024*1024)
#else
#define	__M68K_DFLT_MAXTSIZ	(32*1024*1024)
#define	__M68K_DFLT_DFLDSIZ	(32*1024*1024)
#define	__M68K_DFLT_MAXDSIZ	(256*1024*1024)
#define	__M68K_DFLT_DFLSSIZ	(2*1024*1024)
#define	__M68K_DFLT_MAXSSIZ	(64*1024*1024)
#endif

#ifndef MAXTSIZ
#define	MAXTSIZ		__M68K_DFLT_MAXTSIZ	/* max text size */
#endif
#ifndef	DFLDSIZ
#define	DFLDSIZ		__M68K_DFLT_DFLDSIZ	/* initial data size limit */
#endif
#ifndef	MAXDSIZ
#define	MAXDSIZ		__M68K_DFLT_MAXDSIZ	/* max data size */
#endif
#ifndef	DFLSSIZ
#define	DFLSSIZ		__M68K_DFLT_DFLSSIZ	/* initial stack size limit */
#endif
#ifndef	MAXSSIZ
#define	MAXSSIZ		__M68K_DFLT_MAXSSIZ	/* max stack size */
#endif

/*
 * User VM map constraints.
 *
 * N.B. For the 68851 and 68030+, user-space and kernel-space are
 * completely separated by separate root pointers and they have
 * completely separate page tables.  So, for those systems, the
 * entire 4GB virtual address space is available for user-space,
 * although we skip the very last page to avoid overflow confusion.
 *
 * On the 68010, the basic constraint is the 24-bit address space.
 * Note that there's no integer overflow concern in this case, so
 * we don't skip the last virtual page since the address space is
 * at such a premium.
 *
 * Platforms with other constrains: define these values before including
 * this file.
 */
#define	VM_MIN_ADDRESS		((vaddr_t)0)
#ifndef VM_MAX_ADDRESS
#if defined(__mc68010__)
#define	VM_MAX_ADDRESS		((vaddr_t)0x01000000)
#else
#define	VM_MAX_ADDRESS		((vaddr_t)0-PAGE_SIZE)
#endif
#endif
#define	VM_MAXUSER_ADDRESS	VM_MAX_ADDRESS

/*
 * USRSTACK is the top (end) of the user stack.  The m68k stack
 * is pre-decrement, so the first address touched in a stack access
 * is USRSTACK-1.
 *
 * We simply put the user stack at the very top of the user address space.
 */
#define	USRSTACK		VM_MAX_ADDRESS

/*
 * Kernel map constraints.
 *
 * Comment above about 68851/68030/etc. applies here, as well.  The
 * limit of the kernel address space is dictated by any static mappings
 * that might be placed near the top of the address space.
 *
 * Other platforms (e.g. Sun2/Sun3) have different constraints, and
 * define them before including this file.
 */
#ifndef VM_MIN_KERNEL_ADDRESS
#define	VM_MIN_KERNEL_ADDRESS	((vaddr_t)0)
#endif
#ifndef VM_MAX_KERNEL_ADDRESS
extern vaddr_t kernel_virtual_max;
#define	VM_MAX_KERNEL_ADDRESS	(kernel_virtual_max)
#endif

#endif /* _M68K_VMPARAM_H_ */
