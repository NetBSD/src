/*	$NetBSD: locore.h,v 1.3 2010/03/07 01:52:44 mrg Exp $	*/

/*
 * Copyright (c) 1996-2002 Eduardo Horvath
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR  ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR  BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#undef	CURLWP
#undef	CPCB
#undef	FPLWP

#define	CURLWP	(CPUINFO_VA + CI_CURLWP)
#define	CPCB	(CPUINFO_VA + CI_CPCB)
#define	FPLWP	(CPUINFO_VA + CI_FPLWP)

/*
 * Here are some defines to try to maintain consistency but still
 * support 32-and 64-bit compilers.
 */
#ifdef _LP64
/* reg that points to base of data/text segment */
#define	BASEREG	%g4
/* first constants for storage allocation */
#define LNGSZ		8
#define LNGSHFT		3
#define PTRSZ		8
#define PTRSHFT		3
#define POINTER		.xword
#define ULONG		.xword
/* Now instructions to load/store pointers & long ints */
#define LDLNG		ldx
#define LDULNG		ldx
#define STLNG		stx
#define STULNG		stx
#define LDPTR		ldx
#define LDPTRA		ldxa
#define STPTR		stx
#define STPTRA		stxa
#define	CASPTR		casxa
/* Now something to calculate the stack bias */
#define STKB		BIAS
#define	CCCR		%xcc
#else
#define	BASEREG		%g0
#define LNGSZ		4
#define LNGSHFT		2
#define PTRSZ		4
#define PTRSHFT		2
#define POINTER		.word
#define ULONG		.word
/* Instructions to load/store pointers & long ints */
#define LDLNG		ldsw
#define LDULNG		lduw
#define STLNG		stw
#define STULNG		stw
#define LDPTR		lduw
#define LDPTRA		lduwa
#define STPTR		stw
#define STPTRA		stwa
#define	CASPTR		casa
#define STKB		0
#define	CCCR		%icc
#endif

/* Give this real authority: reset the machine */
#define NOTREACHED	sir

/* if < 32, copy by bytes, memcpy, kcopy, ... */
#define	BCOPY_SMALL	32

/* use as needed to align things on longword boundaries */
#define	_ALIGN	.align 8
#define ICACHE_ALIGN	.align	32
