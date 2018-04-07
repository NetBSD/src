/*	$NetBSD: vmparam.h,v 1.50.28.1 2018/04/07 04:12:14 pgoyette Exp $	*/

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
 *
 * Slightly modified for the VAX port /IC
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

/*
 * Machine dependent constants for VAX.
 */

/*
 * We use 4K VM pages on the VAX.  Override the PAGE_* definitions
 * to be compile-time constants.
 */
#define	PAGE_SHIFT	12
#define	PAGE_SIZE	(1 << PAGE_SHIFT)
#define	PAGE_MASK	(PAGE_SIZE - 1)

/*
 * USRSTACK is the top (end) of the user stack. Immediately above the
 * user stack resides kernel.
 */

#define USRSTACK	KERNBASE

/*
 * Virtual memory related constants, all in bytes
 */

#ifndef MAXTSIZ
#define MAXTSIZ		(32*1024*1024)		/* max text size */
#endif
#ifndef DFLDSIZ
#define DFLDSIZ		(128*1024*1024)		/* initial data size limit */
#endif
#ifndef MAXDSIZ
#define MAXDSIZ		(512*1024*1024)		/* max data size */
#endif
#ifndef DFLSSIZ
#define DFLSSIZ		(512*1024)		/* initial stack size limit */
#endif
#ifndef MAXSSIZ
#define MAXSSIZ		(8*1024*1024)		/* max stack size */
#endif

#define VM_PHYSSEG_MAX		1
#define VM_PHYSSEG_STRAT	VM_PSTRAT_BSEARCH /* XXX */

#define	VM_NFREELIST		1
#define	VM_FREELIST_DEFAULT	0

/* MD round macros */
#define	vax_round_page(x) (((vaddr_t)(x) + VAX_PGOFSET) & ~VAX_PGOFSET)
#define	vax_trunc_page(x) ((vaddr_t)(x) & ~VAX_PGOFSET)

/*
 * Mach derived constants
 */

/* user/kernel map constants */
#define VM_MIN_ADDRESS		((vaddr_t)0)
#define VM_MAXUSER_ADDRESS	((vaddr_t)KERNBASE)
#define VM_MAX_ADDRESS		((vaddr_t)KERNBASE)
#define VM_MIN_KERNEL_ADDRESS	((vaddr_t)KERNBASE)
#define VM_MAX_KERNEL_ADDRESS	((vaddr_t)(0xC0000000))

#define __USE_TOPDOWN_VM

#define	USRIOSIZE		(8 * VAX_NPTEPG)	/* 512MB */
#define	VM_PHYS_SIZE		(USRIOSIZE*VAX_NBPG)

#endif
