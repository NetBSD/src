/*	$NetBSD: param.h,v 1.13.4.1 2012/04/17 00:06:57 yamt Exp $	*/

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
 *	from: Utah Hdr: machparam.h 1.16 92/12/20
 *	from: @(#)param.h	8.1 (Berkeley) 6/10/93
 */

#ifndef	_MACHINE_PARAM_H_
#define	_MACHINE_PARAM_H_

/*
 * Machine dependent constants for the Sun2 series.
 */
#define	_MACHINE	sun2
#define	MACHINE		"sun2"
#define	_MACHINE_ARCH	m68000
#define	MACHINE_ARCH	"m68000"
#define	MID_MACHINE	MID_M680002K

#define PGSHIFT		11		/* LOG2(NBPG) */

#ifdef MSGBUFSIZE
#error "MSGBUFSIZE is not user-adjustable for this arch"
#endif
#define MSGBUFOFF	0x0
#define MSGBUFSIZE	((NBPG * 4) - MSGBUFOFF)

/* This is needed by ps (actually USPACE). */
#define	UPAGES		(16384 / NBPG)		/* pages of u-area */

#if defined(_KERNEL) || defined(_STANDALONE)

#define	KERNBASE	0x00006000	/* start of kernel virtual */
#define	KERN_END	0x00E00000	/* end of kernel virtual */

#define	MAXBSIZE	0x4000		/* max FS block size */
#define	MAXPHYS		0xe000

/*
 * XXX fredette - we force a small cluster size to help me debug
 * this on my low-memory machine.  These should go away at some point.
 */
#define MCLSHIFT	10

/*
 * XXX fredette - we must define this, otw UBC consumes 8MB of kernel VM.
 * For now, we make it absurdly small, perhaps it can grow later.
 */
#define	UBC_NWINS	32

#define	MAXEXEC		1

#endif	/* _KERNEL */

#include <m68k/param.h>

/*
 * Minimum and maximum sizes of the kernel malloc arena in PAGE_SIZE-sized
 * logical pages.
 */
#define	NKMEMPAGES_MIN_DEFAULT	((8 * 1024 * 1024) >> PAGE_SHIFT)
#define	NKMEMPAGES_MAX_DEFAULT	((32 * 1024 * 1024) >> PAGE_SHIFT)

#if defined(_KERNEL) && !defined(_LOCORE)

#include <machine/intr.h>

extern void _delay(unsigned);
#define delay(us)	_delay((us)<<8)
#define	DELAY(n)	delay(n)

#endif	/* _KERNEL && !_LOCORE */
#endif	/* !_MACHINE_PARAM_H_ */
