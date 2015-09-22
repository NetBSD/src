/*	$NetBSD: vmparam.h,v 1.42.32.2 2015/09/22 12:05:36 skrll Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1986, 1990 The Regents of the University of California.
 * All rights reserved.
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
 *	@(#)vmparam.h	7.3 (Berkeley) 5/7/91
 */

#ifndef _MACHINE_VMPARAM_H_
#define _MACHINE_VMPARAM_H_

#include <machine/pte.h>

/*
 * Machine dependent constants for amiga
 */

/*
 * We use 8K pages on the Amiga.  Override the PAGE_* definitions
 * to be compie-time constants.
 */
#define	PAGE_SHIFT	PGSHIFT
#define	PAGE_SIZE	(1 << PAGE_SHIFT)
#define	PAGE_MASK	(PAGE_SIZE - 1)

/*
 * USRSTACK is the top (end) of the user stack.
 */

#ifndef USRSTACK
#define	USRSTACK	0x1E000000
#endif

/*
 * Virtual memory related constants, all in bytes
 */

#ifndef MAXTSIZ
#define	MAXTSIZ		(32*1024*1024)		/* max text size */
#endif
#ifndef DFLDSIZ
#define	DFLDSIZ		(64*1024*1024)		/* initial data size limit */
#endif
#ifndef MAXDSIZ
#define	MAXDSIZ		(416*1024*1024)		/* max data size */
#endif
#ifndef	DFLSSIZ
#define	DFLSSIZ		(2*1024*1024)		/* initial stack size limit */
#endif
#ifndef	MAXSSIZ
#define	MAXSSIZ		(32*1024*1024)		/* max stack size */
#endif

/*
 * PTEs for mapping user space into the kernel for phyio operations.
 * One page is enough to handle 16Mb of simultaneous raw IO operations.
 */
#ifndef USRIOSIZE
#define USRIOSIZE	(1 * NPTEPG)	/* 16mb */
#endif

/*
 * user/kernel map constants
 */
#define VM_MIN_ADDRESS		((vaddr_t)0)		/* user min */
#define VM_MAX_ADDRESS		((vaddr_t)(USRSTACK))	/* user max */
#define VM_MAXUSER_ADDRESS	((vaddr_t)(VM_MAX_ADDRESS))	/* same */
#define VM_MIN_KERNEL_ADDRESS	((vaddr_t)0)
#define VM_MAX_KERNEL_ADDRESS	((vaddr_t)-(NPTEPG * PAGE_SIZE))

/*
 * virtual sizes (bytes) for various kernel submaps
 */
#define VM_PHYS_SIZE		(USRIOSIZE*PAGE_SIZE)

/*
 * Our bootloader currently passes up to 16 segments (but this is variable)
 * Normally, the biggest of them is used for the kernel, and the kernel
 * segment is given to VM first.
 */
#define VM_PHYSSEG_MAX		(16)
#define VM_PHYSSEG_STRAT	VM_PSTRAT_RANDOM

/*
 * Allow supporting Zorro-II memory as lower priority:
 *
 *	- DEFAULT for Zorro-III memory (presumably 32 bit)
 *	- ZORROII for Zorro-II memory (16 bit, Zorro-II DMA)
 */

#define VM_NFREELIST		2
#define VM_FREELIST_DEFAULT	0
#define VM_FREELIST_ZORROII	1

#define	__HAVE_PMAP_PHYSSEG

/*
 * pmap-specific data stored in the vm_physmem[] array.
 */
struct pmap_physseg {
	struct pv_header *pvheader;	/* pv table for this seg */
};

/*
 * number of kernel PT pages (initial only, can grow dynamically)
 */
#define VM_KERNEL_PT_PAGES	((vm_size_t)10)
#endif /* !_MACHINE_VMPARAM_H_ */
