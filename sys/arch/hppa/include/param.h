/*	$NetBSD: param.h,v 1.20.6.1 2015/12/27 12:09:36 skrll Exp $	*/

/*	$OpenBSD: param.h,v 1.12 2001/07/06 02:07:41 provos Exp $	*/

/* 
 * Copyright (c) 1988-1994, The University of Utah and
 * the Computer Systems Laboratory at the University of Utah (CSL).
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software is hereby
 * granted provided that (1) source code retains these copyright, permission,
 * and disclaimer notices, and (2) redistributions including binaries
 * reproduce the notices in supporting documentation, and (3) all advertising
 * materials mentioning features or use of this software display the following
 * acknowledgement: ``This product includes software developed by the
 * Computer Systems Laboratory at the University of Utah.''
 *
 * THE UNIVERSITY OF UTAH AND CSL ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSL DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 *
 * 	Utah $Hdr: param.h 1.18 94/12/16$
 */

#ifdef _KERNEL
#include <machine/cpu.h>
#endif

/*
 * Machine dependent constants for PA-RISC.
 */

#define	_MACHINE	hppa
#define	MACHINE		"hppa"
#define	_MACHINE_ARCH	hppa
#define	MACHINE_ARCH	"hppa"
#define	MID_MACHINE	MID_HPPA

#define	PGSHIFT		12		/* LOG2(NBPG) */
#define	NBPG		(1 << PGSHIFT)	/* bytes/page */
#define	PGOFSET		(NBPG-1)	/* byte offset into page */

#define	SEGSHIFT	(PGSHIFT + (PGSHIFT-PTESHIFT))	/* LOG2(NBSEG) */
#define NBSEG		(1 << SEGSHIFT)	/* bytes/segment (quadrant) */
#define	SEGOFSET	(NBSEG-1)	/* byte offset into segment */

#define	KERNBASE	0x00000000	/* start of kernel virtual */
#define	BTOPKERNBASE	((u_long)KERNBASE >> PGSHIFT)

#define	DEV_BSIZE	512
#define	DEV_BSHIFT	9		/* log2(DEV_BSIZE) */
#define BLKDEV_IOSIZE	2048
#define	MAXPHYS		(64 * 1024)	/* max raw I/O transfer size */

#define	SSIZE		(1)		/* initial stack size/NBPG */
#define	SINCR		(1)		/* increment of stack/NBPG */

#ifdef DIAGNOSTIC
#define	UPAGES		5		/* pages of u-area + redzone */
#else
#define	UPAGES		4		/* pages of u-area */
#endif
#define	USPACE		(UPAGES * NBPG)	/* pages for user struct and kstack */

#ifndef	MSGBUFSIZE
#define	MSGBUFSIZE	(2*NBPG)	/* default message buffer size */
#endif

/*
 * Constants related to network buffer management.
 * MCLBYTES must be no larger than the software page size, and,
 * on machines that exchange pages of input or output buffers with mbuf
 * clusters (MAPPED_MBUFS), MCLBYTES must also be an integral multiple
 * of the hardware page size.
 */
#define	MSIZE		256		/* size of an mbuf */
#define	MCLSHIFT	11
#define	MCLBYTES	(1 << MCLSHIFT)	/* large enough for ether MTU */
#define	MCLOFSET	(MCLBYTES - 1)

/*
 * Size of kernel malloc arena in logical pages
 */
#define NKMEMPAGES_MIN_DEFAULT  ((16 * 1024 * 1024) >> PAGE_SHIFT)
#define NKMEMPAGES_MAX_DEFAULT  ((256 * 1024 * 1024) >> PAGE_SHIFT) 

/*
 * Mach derived conversion macros
 */

#define btop(x)		((unsigned long)(x) >> PGSHIFT)
#define ptob(x)		((unsigned long)(x) << PGSHIFT)
