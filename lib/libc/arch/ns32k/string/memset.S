/*	$NetBSD: memset.S,v 1.2 1997/05/08 13:38:54 matthias Exp $	*/

/* 
 * Written by Matthias Pfaller, 1996
 * Public domain.
 */

#include <machine/asm.h>

#if defined(LIBC_SCCS)
	RCSID("$NetBSD: memset.S,v 1.2 1997/05/08 13:38:54 matthias Exp $")
# if defined(BZERO)
	RCSID("$Masqueraded: as bzero $")
# endif
#endif

#if !defined(BZERO)
/*
 * void *
 * memset (void *b, int c, size_t len)
 *	write len bytes of value c to the string b.
 */
#define SETMEMB(d)	movb	r4,d
#define SETMEMD(d)	movd	r4,d
ENTRY(memset)
	enter	[r3,r4],0

	movd	B_ARG0,r1		/* b */
	movzbd	B_ARG1,r0		/* c */
	movd	0x01010101,r4
	muld	r0,r4
	movd	B_ARG2,r2		/* len */
#else
/*
 * void
 * bzero (void *b, size_t len)
 *	write len zero bytes to the string b.
 */
#define SETMEMB(d)	movqb	0,d
#define SETMEMD(d)	movqd	0,d
ENTRY(bzero)
	enter	[r3],0

	movd	B_ARG0,r1		/* b */
	movd	B_ARG1,r2		/* len */
#endif
	cmpd	19,r2
	bhs	6f			/* Not worth the trouble. */

	/*
	 * Is address aligned?
	 */
	movqd	3,r0
	andd	r1,r0			/* r0 = b & 3 */
	cmpqd	0,r0
	beq	0f

	/*
	 * Align address (if necessary).
	 */
	SETMEMD(0(r1))
	addr	-4(r0)[r2:b],r2		/* len = len + (r0 - 4) */
	negd	r0,r0
	addr	4(r0)[r1:b],r1		/* b = b + (-r0 + 4) */

0:	/*
	 * Compute loop start address.
	 */
	movd	r2,r0
	addr	60(r2),r3
	andd	60,r0			/* r0 = len & 60 */
	lshd	-6,r3			/* r3 = (len + 60) >> 6 */
	andd	3,r2			/* len &= 3 */

	cmpqd	0,r0
	beq	1f

	addr	-64(r1)[r0:b],r1	/* b = b - 64 + r0 */
	lshd	-2,r0
	addr	0(r0)[r0:w],r0
	negd	r0,r0			/* r0 = -3 * r0 / 4 */

	jump	2f(pc)[r0:b]		/* Now enter the loop */

	/*
	 * Zero 64 bytes per loop iteration.
	 */
	.align	2
1:	SETMEMD(0(r1))
	SETMEMD(4(r1))
	SETMEMD(8(r1))
	SETMEMD(12(r1))
	SETMEMD(16(r1))
	SETMEMD(20(r1))
	SETMEMD(24(r1))
	SETMEMD(28(r1))
	SETMEMD(32(r1))
	SETMEMD(36(r1))
	SETMEMD(40(r1))
	SETMEMD(44(r1))
	SETMEMD(48(r1))
	SETMEMD(52(r1))
	SETMEMD(56(r1))
	SETMEMD(60(r1))
2:	addd	64,r1
	acbd	-1,r3,1b

3:	cmpqd	0,r2
	beq	5f

	/*
	 * Zero out blocks shorter then four bytes.
	 */
4:	SETMEMB(-1(r1)[r2:b])
	acbd	-1,r2,4b

#ifndef	BZERO
5:	movd	B_ARG0,r0
	exit	[r3,r4]
	ret	0
#else
5:	exit	[r3]
	ret	0
#endif

	/*
	 * For blocks smaller then 20 bytes
	 * this is faster.
	 */
	.align	2
6:	cmpqd	3,r2
	bhs	3b

	movd	r2,r0
	andd	3,r2
	lshd	-2,r0

7:	SETMEMD(0(r1))
	addqd	4,r1
	acbd	-1,r0,7b
	br	3b
