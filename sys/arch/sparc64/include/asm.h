/*       $NetBSD: asm.h,v 1.20 2013/03/16 23:04:22 christos Exp $        */

#include <sparc/asm.h>

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

#if defined(_KERNEL) || defined(_RUMPKERNEL)
/* Give this real authority: reset the machine */
#define NOTREACHED	sir
#else
#define NOTREACHED
#endif

/* if < 32, copy by bytes, memcpy, kcopy, ... */
#define	BCOPY_SMALL	32

/* use as needed to align things on longword boundaries */
#define	_ALIGN	.align 8
#define ICACHE_ALIGN	.align	32
