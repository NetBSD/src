/*	$NetBSD: mips_param.h,v 1.33.6.2 2017/12/03 11:36:27 jdolecek Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * No reason this can't be common
 */
#if defined(__MIPSEB__)
# if defined(__mips_n32) || defined(__mips_n64)
#  define	_MACHINE_ARCH	mips64eb
#  define	MACHINE_ARCH	"mips64eb"
#  define	_MACHINE32_ARCH	mipseb
#  define	MACHINE32_ARCH	"mipseb"
# else
#  define	_MACHINE_ARCH	mipseb
#  define	MACHINE_ARCH	"mipseb"
# endif
#elif defined(__MIPSEL__)
# if defined(__mips_n32) || defined(__mips_n64)
#  define	_MACHINE_ARCH	mips64el
#  define	MACHINE_ARCH	"mips64el"
#  define	_MACHINE32_ARCH	mipsel
#  define	MACHINE32_ARCH	"mipsel"
# else
#  define	_MACHINE_ARCH	mipsel
#  define	MACHINE_ARCH	"mipsel"
#endif
#else
#error neither __MIPSEL__ nor __MIPSEB__ are defined.
#endif

/*
 * Userland code should be using uname/sysctl to get MACHINE so simply
 * export a generic MACHINE of "mips"
 */
#ifndef _KERNEL
#undef MACHINE
#define MACHINE "mips"
#endif

#define ALIGNBYTES32		(sizeof(double) - 1)
#define ALIGN32(p)		(((uintptr_t)(p) + ALIGNBYTES32) &~ALIGNBYTES32)

/*
 * On mips, UPAGES is fixed by sys/arch/mips/mips/locore code
 * to be the number of per-process-wired kernel-stack pages/PTES.
 */

#define	SSIZE		1		/* initial stack size/NBPG */
#define	SINCR		1		/* increment of stack/NBPG */

#if (ENABLE_MIPS_16KB_PAGE + ENABLE_MIPS_8KB_PAGE + ENABLE_MIPS_4KB_PAGE) > 1
#error only one of ENABLE_MIPS_{4,8,16}KB_PAGE can be defined.
#endif

#ifndef MSGBUFSIZE
#define MSGBUFSIZE	NBPG		/* default message buffer size */
#endif

#ifndef COHERENCY_UNIT
#define COHERENCY_UNIT	32	/* MIPS cachelines are usually 32 bytes */
#endif

#ifdef ENABLE_MIPS_16KB_PAGE
#define	PGSHIFT		14		/* LOG2(NBPG) */
#elif defined(ENABLE_MIPS_8KB_PAGE) \
    || (!defined(ENABLE_MIPS_4KB_PAGE) && __mips >= 3)
#define	PGSHIFT		13		/* LOG2(NBPG) */
#else
#define	PGSHIFT		12		/* LOG2(NBPG) */
#endif
#define	NBPG		(1 << PGSHIFT)	/* bytes/page */
#define	PGOFSET		(NBPG-1)	/* byte offset into page */
#define	PTPSHIFT	2
#define	PTPLENGTH	(PGSHIFT-PTPSHIFT)
#define	NPTEPG		(1 << PTPLENGTH)

#define	SEGSHIFT	(PGSHIFT+PTPLENGTH)	/* LOG2(NBSEG) */
#define NBSEG		(1 << SEGSHIFT)	/* bytes/segment */
#define	SEGOFSET	(NBSEG-1)	/* byte offset into segment */

#ifdef _LP64
#define SEGLENGTH	(PGSHIFT-3)
#define	XSEGSHIFT	(SEGSHIFT+SEGLENGTH)	/* LOG2(NBXSEG) */
#define NBXSEG		(1UL << XSEGSHIFT)	/* bytes/xsegment */
#define	XSEGOFSET	(NBXSEG-1)	/* byte offset into xsegment */
#define XSEGLENGTH	(PGSHIFT-3)
#define	NXSEGPG		(1 << XSEGLENGTH)
#else
#define	SEGLENGTH	(31-SEGSHIFT)
#endif
#define	NSEGPG		(1 << SEGLENGTH)

#if PGSHIFT >= 13
#define	UPAGES		1		/* pages of u-area */
#else
#define	UPAGES		2		/* pages of u-area */
#endif
#define	USPACE		(UPAGES*NBPG)	/* size of u-area in bytes */
#define	USPACE_ALIGN	USPACE		/* make sure it starts on a even VA */

/*
 * Minimum and maximum sizes of the kernel malloc arena in PAGE_SIZE-sized
 * logical pages.
 */
#define	NKMEMPAGES_MIN_DEFAULT	((8 * 1024 * 1024) >> PAGE_SHIFT)
#define	NKMEMPAGES_MAX_DEFAULT	((128 * 1024 * 1024) >> PAGE_SHIFT)

/*
 * Mach derived conversion macros
 */
#define mips_round_page(x)	((((uintptr_t)(x)) + NBPG - 1) & ~(NBPG-1))
#define mips_trunc_page(x)	((uintptr_t)(x) & ~(NBPG-1))
#define mips_btop(x)		((paddr_t)(x) >> PGSHIFT)
#define mips_ptob(x)		((paddr_t)(x) << PGSHIFT)

#ifdef __MIPSEL__
#define	MID_MACHINE	MID_PMAX	/* MID_PMAX (little-endian) */
#endif
#ifdef __MIPSEB__
#define	MID_MACHINE	MID_MIPS	/* MID_MIPS (big-endian) */
#endif

/*
 * Constants related to network buffer management.
 * MCLBYTES must be no larger than NBPG (the software page size), and,
 * on machines that exchange pages of input or output buffers with mbuf
 * clusters (MAPPED_MBUFS), MCLBYTES must also be an integral multiple
 * of the hardware page size.
 */
#ifndef MSIZE
#ifdef _LP64
#define	MSIZE		512		/* size of an mbuf */
#else
#define	MSIZE		256		/* size of an mbuf */
#endif

#ifndef MCLSHIFT
# define MCLSHIFT	11		/* convert bytes to m_buf clusters */
#endif	/* MCLSHIFT */

#define	MCLBYTES	(1 << MCLSHIFT)	/* size of a m_buf cluster */

#endif
