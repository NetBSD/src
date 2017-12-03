/*	$NetBSD: param.h,v 1.8.6.1 2017/12/03 11:36:20 jdolecek Exp $	*/

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
 *	@(#)param.h	5.8 (Berkeley) 6/28/91
 */

#ifndef _IA64_PARAM_H_
#define _IA64_PARAM_H_

/*
 * Machine dependent constants for Intel Itanium.
 */

#ifdef _KERNEL
#include <machine/cpu.h>
#endif

#define	_MACHINE	ia64
#define	MACHINE		"ia64"
#define	_MACHINE_ARCH	ia64
#define	MACHINE_ARCH	"ia64"
#define	MID_MACHINE	MID_IA64

#ifdef MULTIPROCESSOR
#define	MAXCPU		16
#else
#define MAXCPU		1
#endif

#define	DEV_BSHIFT	9		/* log2(DEV_BSIZE) */
#define	DEV_BSIZE	(1<<DEV_BSHIFT)
#define	BLKDEV_IOSIZE	2048

#ifndef MAXPHYS
#define MAXPHYS		(64 * 1024)	/* max raw I/O transfer size */
#endif

#define UPAGES		4
#define	USPACE		(UPAGES * NBPG)	/* total size of u-area */

#ifndef MSGBUFSIZE
#define MSGBUFSIZE	NBPG		/* default message buffer size */
#endif

#ifndef	KSTACK_PAGES
#define	KSTACK_PAGES	4		/* pages of kernel stack */
#endif
#define	KSTACK_GUARD_PAGES 0		/* pages of kstack guard; 0 disables */

#define ALIGNBYTES32		(sizeof(int) - 1)
#define ALIGN32(p)		(((u_long)(p) + ALIGNBYTES32) &~ALIGNBYTES32)

#ifndef LOG2_PAGE_SIZE
#define LOG2_PAGE_SIZE          14              /* 16K pages by default. */
#endif
#define	PGSHIFT		14		/* LOG2(NBPG) */
#define	NBPG		(1 << PGSHIFT)	/* bytes/page */
#define	PGOFSET		(NBPG-1)	/* byte offset into page */
#define	NPTEPG		(NBPG/(sizeof (pt_entry_t)))
/*
 * Constants related to network buffer management.
 * MCLBYTES must be no larger than NBPG (the software page size), and,
 * on machines that exchange pages of input or output buffers with mbuf
 * clusters (MAPPED_MBUFS), MCLBYTES must also be an integral multiple
 * of the hardware page size.
 */
#define	MSIZE		256		/* size of an mbuf */

#ifndef MCLSHIFT
#define	MCLSHIFT	11		/* convert bytes to m_buf clusters */
					/* 2K cluster can hold Ether frame */
#endif	/* MCLSHIFT */

#define	MCLBYTES	(1 << MCLSHIFT)	/* size of a m_buf cluster */

/*
 * Minimum and maximum sizes of the kernel malloc arena in PAGE_SIZE-sized
 * logical pages.
 * No enforced maxmimum an ia64
 */
#define	NKMEMPAGES_MIN_DEFAULT	((32 * 1024 * 1024) >> PAGE_SHIFT)
#define	NKMEMPAGES_MAX_UNLIMITED 1

/* The default size of identity mappings in region 6 & 7. */
#ifndef LOG2_ID_PAGE_SIZE
#define LOG2_ID_PAGE_SIZE       16
#endif

/*
 * Mach derived conversion macros
 */

#define ia64_round_page(x)   ((((unsigned long)(x)) + NBPG - 1) & ~(NBPG - 1))
#define ia64_trunc_page(x)   ((unsigned long)(x) & ~(NBPG - 1))
                
#define ia64_btop(x)            ((unsigned long)(x) >> PGSHIFT)
#define ia64_ptob(x)            ((unsigned long)(x) << PGSHIFT) 
                
#ifdef _KERNEL
#ifndef _LOCORE

#include <machine/intr.h>

static __inline void delay(unsigned int us) {}	/* XXXXX */

#endif /* _LOCORE */
#endif /* _KERNEL */

#endif /* _IA64_PARAM_H_ */
