/*	$NetBSD: param.h,v 1.3 2001/05/30 12:28:39 mrg Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *	@(#)param.h	7.8 (Berkeley) 6/28/91
 */

#ifndef	_MACHINE_PARAM_H_
#define	_MACHINE_PARAM_H_

#ifdef	_KERNEL
#ifndef	_LOCORE
#include <machine/cpu.h>
#endif	/* _LOCORE */
#endif

/*
 * Machine dependent constants for Amiga PowerPC (32-bit only currently)
 */
#define	MACHINE		"amigappc"
#define MACHINE_ARCH	"powerpc"
#define	MID_MACHINE	MID_POWERPC

#define	ALIGNBYTES		(sizeof(double) - 1)
#define	ALIGN(p)		(((u_int)(p) + ALIGNBYTES) & ~ALIGNBYTES)
#define ALIGNED_POINTER(p,t)	((((u_long)(p)) & (sizeof(t)-1)) == 0)

#define	PGSHIFT		12
#define	NBPG		(1 << PGSHIFT)	/* Page size */
#define	PGOFSET		(NBPG - 1)

#define	DEV_BSHIFT	9		/* log2(DEV_BSIZE) */
#define	DEV_BSIZE	(1 << DEV_BSHIFT)
#define	BLKDEV_IOSIZE	NBPG
#define	MAXPHYS		(64 * 1024)	/* max raw I/O transfer size */

#define	UPAGES		4
#define	USPACE		(UPAGES * NBPG)

#ifndef	MSGBUFSIZE
#define	MSGBUFSIZE	NBPG		/* default message buffer size */
#endif

#define	KERNBASE	0x00000000	/* start of kernel virtual */

/*
 * Constants related to network buffer management.
 * MCLBYTES must be no larger than NBPG (the software page size), and,
 * on machines that exchange pages of input or output buffers with mbuf
 * clusters (MAPPED_MBUFS), MCLBYTES must also be an integral multiple
 * of the hardware page size.
 */
#define	MSIZE		256		/* size of an mbuf */
#define	MCLSHIFT	11		/* convert bytes to m_buf clusters */
#define	MCLBYTES	(1 << MCLSHIFT)	/* size of a m_buf cluster */

#ifndef NMBCLUSTERS

#if defined(_KERNEL_OPT)
#include "opt_gateway.h"
#endif

#ifdef GATEWAY
#define	NMBCLUSTERS	512		/* map size, max cluster allocation */
#else
#define	NMBCLUSTERS	256		/* map size, max cluster allocation */
#endif
#endif

/*
 * Minimum and maximum sizes of the kernel malloc arena in PAGE_SIZE-sized
 * logical pages.
 */
#define NKMEMPAGES_MIN_DEFAULT  ((128 * 1024 * 1024) >> PAGE_SHIFT)
#define NKMEMPAGES_MAX_DEFAULT  ((128 * 1024 * 1024) >> PAGE_SHIFT)

/*
 * pages ("clicks") to disk blocks
 */
#define	ctod(x)		((x) << (PGSHIFT - DEV_BSHIFT))
#define	dtoc(x)		((x) >> (PGSHIFT - DEV_BSHIFT))
/*
 * bytes to pages
 */
#define	ctob(x)		((x) << PGSHIFT)
#define	btoc(x)		(((x) + PGOFSET) >> PGSHIFT)

/*
 * bytes to disk blocks
 */
#define	dbtob(x)	((x) << DEV_BSHIFT)
#define	btodb(x)	((x) >> DEV_BSHIFT)

/*
 * Segment handling stuff
 */
#define	SEGMENT_LENGTH	0x10000000
#define	SEGMENT_MASK	0xf0000000

/*
 * Fixed segments
 */
#define	USER_SR		13
#define	KERNEL_SR	14
#define	KERNEL_SEGMENT	(0xf0000 + KERNEL_SR)
#define	EMPTY_SEGMENT	0xfffff0
#define	USER_ADDR	((void *)(USER_SR << ADDR_SR_SHFT))

/*
 * Some system constants
 */
#ifndef	NPMAPS
#define	NPMAPS		32768	/* Number of pmaps in system */
#endif

#ifdef	_KERNEL
/*
 * point to the custom.intenar and custom.intenaw respectively.
extern volatile unsigned short *amiga_intena_read, *amiga_intena_write;
*/

/* ADAM: redefiniton of cpu.h */
/*
#ifndef	_LOCORE
void delay __P((int));
void DELAY __P((int));
#endif*/	/* !_LOCORE */
#endif	/* _KERNEL */

#endif /* def _MACHINE_PARAM_H_ */
