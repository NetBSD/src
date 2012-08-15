/*	$NetBSD: vmparam.h,v 1.29.2.1 2012/08/15 15:22:55 riz Exp $	*/

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

#ifndef _X86_64_VMPARAM_H_
#define _X86_64_VMPARAM_H_

#ifdef __x86_64__

#include <sys/tree.h>
#include <sys/mutex.h>
#ifdef _KERNEL_OPT
#include "opt_xen.h"
#endif

/*
 * Machine dependent constants for 386.
 */

/*
 * Page size on the amd64 s not variable in the traditional sense.
 * We override the PAGE_* definitions to compile-time constants.
 */
#define	PAGE_SHIFT	12
#define	PAGE_SIZE	(1 << PAGE_SHIFT)
#define	PAGE_MASK	(PAGE_SIZE - 1)

/*
 * USRSTACK is the top (end) of the user stack. Immediately above the
 * user stack resides the user structure, which is UPAGES long and contains
 * the kernel stack.
 *
 * Immediately after the user structure is the page table map, and then
 * kernel address space.
 */
#define	USRSTACK	VM_MAXUSER_ADDRESS

#define USRSTACK32	VM_MAXUSER_ADDRESS32

/*
 * Virtual memory related constants, all in bytes
 */
#define	MAXTSIZ		(128*1024*1024)		/* max text size */
#ifndef DFLDSIZ
#define	DFLDSIZ		(256*1024*1024)		/* initial data size limit */
#endif
#ifndef MAXDSIZ
#define	MAXDSIZ		(8L*1024*1024*1024)	/* max data size */
#endif
#ifndef	DFLSSIZ
#define	DFLSSIZ		(4*1024*1024)		/* initial stack size limit */
#endif
#ifndef	MAXSSIZ
#define	MAXSSIZ		(128*1024*1024)		/* max stack size */
#endif

/*
 * 32bit memory related constants.
 */

#define MAXTSIZ32	(128*1024*1024)
#ifndef DFLDSIZ32
#define	DFLDSIZ32	(256*1024*1024)		/* initial data size limit */
#endif
#ifndef MAXDSIZ32
#define	MAXDSIZ32	(3U*1024*1024*1024)	/* max data size */
#endif
#ifndef	DFLSSIZ32
#define	DFLSSIZ32	(2*1024*1024)		/* initial stack size limit */
#endif
#ifndef	MAXSSIZ32
#define	MAXSSIZ32	(64*1024*1024)		/* max stack size */
#endif

/*
 * Size of User Raw I/O map
 */
#define	USRIOSIZE 	300

/*
 * Mach derived constants
 */

/* user/kernel map constants */
#define VM_MIN_ADDRESS		0
#define VM_MAXUSER_ADDRESS	0x00007f8000000000
#define VM_MAX_ADDRESS		0x00007fbfdfeff000
#ifndef XEN
#define VM_MIN_KERNEL_ADDRESS	0xffff800000000000
#else /* XEN */
#define VM_MIN_KERNEL_ADDRESS	0xffffa00000000000
#endif
#define VM_MAX_KERNEL_ADDRESS	0xfffffe8000000000

#define VM_MAXUSER_ADDRESS32	0xfffff000

/*
 * The address to which unspecified mapping requests default
 */
#ifdef _KERNEL_OPT
#include "opt_uvm.h"
#endif
#define __USE_TOPDOWN_VM
#define VM_DEFAULT_ADDRESS(da, sz) \
	trunc_page(USRSTACK - MAXSSIZ - (sz))
#define VM_DEFAULT_ADDRESS32(da, sz) \
	trunc_page(USRSTACK32 - MAXSSIZ32 - (sz))

/*
 * XXXfvdl we have plenty of KVM now, remove this.
 */
#ifndef VM_MAX_KERNEL_BUF
#define VM_MAX_KERNEL_BUF	(384 * 1024 * 1024)
#endif

/* virtual sizes (bytes) for various kernel submaps */
#define VM_PHYS_SIZE		(USRIOSIZE*PAGE_SIZE)

#define VM_PHYSSEG_MAX		16	/* 1 "hole" + 15 free lists */
#define VM_PHYSSEG_STRAT	VM_PSTRAT_BIGFIRST

#define	VM_NFREELIST		3
#define	VM_FREELIST_DEFAULT	0
#define	VM_FREELIST_FIRST4G	1
#define	VM_FREELIST_FIRST16	2

#else	/*	!__x86_64__	*/

#include <i386/vmparam.h>

#endif	/*	__x86_64__	*/

#endif /* _X86_64_VMPARAM_H_ */
