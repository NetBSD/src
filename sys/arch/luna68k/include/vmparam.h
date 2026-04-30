/*	$NetBSD: vmparam.h,v 1.32 2026/04/30 03:44:44 thorpej Exp $	*/

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

#ifndef _MACHINE_VMPARAM_H_
#define _MACHINE_VMPARAM_H_

/*
 * Use common m68k definitions to define PAGE_SIZE and related constants.
 */
#include <m68k/vmparam.h>

/*
 * PTEs for mapping user space into the kernel for phyio operations.
 * One page is enough to handle 4Mb of simultaneous raw IO operations.
 */
#ifndef USRIOSIZE
#define USRIOSIZE	(1 * NPTEPG)	/* 4mb */
#endif

/*
 * Mach derived constants
 */

/*
 * kernel map constants
 *
 * TT registers are used to map the I/O space (starting at 0x4000.0000), so
 * the kernel virtual address space needs to end before that (with room for
 * the Sysmap, because that's where the Hibler pmap puts it).
 */
#define VM_MIN_KERNEL_ADDRESS	((vaddr_t)0)
#ifdef __HAVE_NEW_PMAP_68K
extern vaddr_t kernel_virtual_max;
#define	VM_MAX_KERNEL_ADDRESS	(kernel_virtual_max)
#else
#define VM_MAX_KERNEL_ADDRESS	((vaddr_t)(0x40000000-PAGE_SIZE*NPTEPG))
#endif

/* virtual sizes (bytes) for various kernel submaps */
#define VM_PHYS_SIZE		(USRIOSIZE*PAGE_SIZE)

/*
 * Constants which control the way the VM system deals with memory segments.
 * Only one physical contiguous memory segment.
 */
#define VM_PHYSSEG_MAX		1
#define VM_PHYSSEG_STRAT	VM_PSTRAT_BSEARCH

#define VM_NFREELIST		1
#define VM_FREELIST_DEFAULT	0

#endif /* _MACHINE_VMPARAM_H_ */
