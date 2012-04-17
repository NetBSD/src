/* $NetBSD: param.h,v 1.38.4.1 2012/04/17 00:05:55 yamt Exp $ */

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department and Ralph Campbell.
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
 * from: Utah $Hdr: machparam.h 1.11 89/08/14$
 *
 *	@(#)param.h	8.1 (Berkeley) 6/10/93
 */

/*
 * Machine dependent constants for the Alpha.
 */
#define	_MACHINE	alpha
#define	MACHINE		"alpha"
#define	_MACHINE_ARCH	alpha
#define	MACHINE_ARCH	"alpha"
#define	MID_MACHINE	MID_ALPHA

#include <machine/cpu.h>

#define	NBPG		(1 << ALPHA_PGSHIFT)		/* bytes/page */
#define	PGOFSET		(NBPG-1)			/* byte off. into pg */
#define	PGSHIFT		ALPHA_PGSHIFT			/* LOG2(NBPG) */

#define	KERNBASE	0xfffffc0000230000	/* start of kernel virtual */
#define	BTOPKERNBASE	((u_long)KERNBASE >> PGSHIFT)

#define	DEV_BSIZE	512
#define	DEV_BSHIFT	9		/* log2(DEV_BSIZE) */
#define	BLKDEV_IOSIZE	2048
#ifndef	MAXPHYS
#define	MAXPHYS		(64 * 1024)	/* max raw I/O transfer size */
#endif

#define	SSIZE		1		/* initial stack size/NBPG */
#define	SINCR		1		/* increment of stack/NBPG */

#define	UPAGES		2			/* pages of u-area */
#define	USPACE		(UPAGES * NBPG)		/* total size of u-area */

#ifndef MSGBUFSIZE
#define MSGBUFSIZE	NBPG		/* default message buffer size */
#endif

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
 * No enforced maximum on alpha
 */
#define	NKMEMPAGES_MIN_DEFAULT	((16 * 1024 * 1024) >> PAGE_SHIFT)
#define	NKMEMPAGES_MAX_UNLIMITED	1

/*
 * Mach derived conversion macros
 */
#define	alpha_round_page(x)	((((unsigned long)(x)) + NBPG - 1) & ~(NBPG-1))
#define	alpha_trunc_page(x)	((unsigned long)(x) & ~(NBPG-1))
#define	alpha_btop(x)		((unsigned long)(x) >> PGSHIFT)
#define	alpha_ptob(x)		((unsigned long)(x) << PGSHIFT)

#ifdef _KERNEL
#ifndef _LOCORE

#include <machine/intr.h>

void	delay(unsigned long);
#define	DELAY(n)	delay(n)

/* XXX THE FOLLOWING PROTOTYPE SHOULD BE A BUS.H INTERFACE */
paddr_t alpha_XXX_dmamap(vaddr_t);
/* XXX END BUS.H */

#endif
#endif /* !_KERNEL */
