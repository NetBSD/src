/*	$NetBSD: param.h,v 1.15.14.1 1999/12/21 23:15:55 wrstuden Exp $	*/

/*
 * Copyright (c) 1994,1995 Mark Brinicombe.
 * All rights reserved.
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
 *	This product includes software developed by the RiscBSD team.
 * 4. The name "RiscBSD" nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY RISCBSD ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL RISCBSD OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef	_ARM32_PARAM_H_
#define	_ARM32_PARAM_H_

#ifdef _KERNEL
#  include <machine/cpu.h>
#endif

/*
 * Machine dependent constants for ARM6+ processors
 */

#define	_MACHINE	arm32
#define	MACHINE		"arm32"
#define	_MACHINE_ARCH	arm32
#define	MACHINE_ARCH	"arm32"
#define	MID_MACHINE	MID_ARM6

/*
 * Round p (pointer or byte index) up to a correctly-aligned value
 * for all data types (int, long, ...).   The result is u_int and
 * must be cast to any desired pointer type.
 *
 * ALIGNED_POINTER is a boolean macro that checks whether an address
 * is valid to fetch data elements of type t from on this architecture.
 * This does not reflect the optimal alignment, just the possibility
 * (within reasonable limits). 
 *
 */
#define ALIGNBYTES		(sizeof(int) - 1)
#define ALIGN(p)		(((u_int)(p) + ALIGNBYTES) &~ ALIGNBYTES)
#define ALIGNED_POINTER(p,t)	((((u_long)(p)) & (sizeof(t)-1)) == 0)

#define	PGSHIFT		12		/* LOG2(NBPG) */
#define	NBPG		(1 << PGSHIFT)	/* bytes/page */
#define	PGOFSET		(NBPG-1)	/* byte offset into page */
#define	NPTEPG		(NBPG/(sizeof (pt_entry_t)))


#define	DEF_BSHIFT	9		/* log2(DEF_BSIZE) */
#define	DEF_BSIZE	(1 << DEF_BSHIFT)
#define	BLKDEV_IOSIZE	2048
#define	MAXPHYS		(64 * 1024)	/* max raw I/O transfer size */

#define	CLSIZELOG2	0
#define	CLSIZE		(1 << CLSIZELOG2)

/* NOTE: SSIZE, SINCR and UPAGES must be multiples of CLSIZE */
#define SSIZE           1               /* initial stack size/NBPG */
#define SINCR           1               /* increment of stack/NBPG */
#define UPAGES          2               /* pages of u-area */
#define USPACE          (UPAGES * NBPG) /* total size of u-area */

#ifndef MSGBUFSIZE
#define MSGBUFSIZE	NBPG		/* default message buffer size */
#endif

/*
 * Constants related to network buffer management.
 * MCLBYTES must be no larger than CLBYTES (the software page size), and,
 * on machines that exchange pages of input or output buffers with mbuf
 * clusters (MAPPED_MBUFS), MCLBYTES must also be an integral multiple
 * of the hardware page size.
 */
#define	MSIZE		128		/* size of an mbuf */
#define	MCLSHIFT	11		/* convert bytes to m_buf clusters */
#define	MCLBYTES	(1 << MCLSHIFT)	/* size of a m_buf cluster */
#define	MCLOFSET	(MCLBYTES - 1)	/* offset within a m_buf cluster */

#ifndef NMBCLUSTERS

#if defined(_KERNEL) && !defined(_LKM)
#include "opt_gateway.h"
#include "opt_non_po2_blocks.h"
#endif /* _KERNEL && ! _LKM */

#ifdef GATEWAY
#define	NMBCLUSTERS	512		/* map size, max cluster allocation */
#else
#define	NMBCLUSTERS	256		/* map size, max cluster allocation */
#endif
#endif

/*
 * Size of kernel malloc arena in CLBYTES-sized logical pages
 */ 
#ifndef NKMEMCLUSTERS
#define	NKMEMCLUSTERS	(6 * 1024 * 1024 / CLBYTES)
#endif

/* pages ("clicks") (4096 bytes) to disk blocks */
#define	ctod(x, sh)	((x) << (PGSHIFT - (sh))
#define	dtoc(x, sh)	((x) >> (PGSHIFT - (sh))
/*#define	dtob(x)	((x) << DEV_BSHIFT)*/

#define	ctob(x)	((x) << PGSHIFT)

/* bytes to pages */
#define	btoc(x)	(((x) + PGOFSET) >> PGSHIFT)

#if defined(_LKM) || defined(NON_PO2_BLOCKS)
#define	dbtob(x, sh)	(((sh) >= 0) ? ((x) << (sh)) : ((x) * (-sh)))
#define	btodb(x, sh)	(((sh) >= 0) ? ((x) >> (sh)) : ((x) / (-sh)))
#define	blocksize(sh)	(((sh) >= 0) ? (1 << (sh))   : (-sh))
#else
#define	dbtob(x, sh)	((x) << (sh))
#define	btodb(x, sh)	((x) >> (sh))
#define	blocksize(sh)	(((sh) >= 0) ? (1 << (sh))   : (0))
#endif

/*
 * Map a ``block device block'' to a file system block.
 * This should be device dependent, and should use the bsize
 * field from the disk label.
 * For now though just use DEV_BSIZE.
 */
#define	bdbtofsb(bn)	((bn) / (BLKDEV_IOSIZE / DEF_BSIZE))

/* Constants used to divide the USPACE area */

/*
 * The USPACE area contains :
 * 1. the user structure for the process
 * 2. the fp context for FP emulation
 * 3. the kernel (svc) stack
 * 4. the undefined instruction stack
 *
 * The layout of the area looks like this
 *
 * | user area | FP context | undefined stack | kernel stack |
 *
 * The size of the user area is known.
 * The size of the FP context is variable depending of the FP emulator
 * in use and whether there is hardware FP support. However we can put
 * an upper limit on it.
 * The undefined stack needs to be at least 512 bytes. This is a requirement
 * if the FP emulators
 * The kernel stack should be at least 4K is size.
 *
 * The stack top addresses are used to set the stack pointers. The stack bottom
 * addresses at the addresses monitored by the diagnostic code for stack overflows
 *
 */

#define FPCONTEXTSIZE			(0x100)
#define USPACE_SVC_STACK_TOP		(USPACE)
#define USPACE_SVC_STACK_BOTTOM		(USPACE_SVC_STACK_TOP - 0x1000)
#define	USPACE_UNDEF_STACK_TOP		(USPACE_SVC_STACK_BOTTOM - 0x10)
#define USPACE_UNDEF_STACK_BOTTOM	(sizeof(struct user) + FPCONTEXTSIZE + 10)

#define arm_byte_to_page(x) ((x) >> PGSHIFT)
#define arm_page_to_byte(x) ((x) << PGSHIFT)

#ifdef _KERNEL
#ifndef _LOCORE
void	delay __P((unsigned));
#define DELAY(x)	delay(x)
#endif
#endif

#endif	/* _ARM_PARAM_H_ */
