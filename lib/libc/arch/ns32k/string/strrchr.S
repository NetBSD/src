/*	$NetBSD: strrchr.S,v 1.2 1997/05/08 13:39:00 matthias Exp $	*/

/* 
 * Written by Matthias Pfaller, 1996
 * Public domain.
 */

#include <machine/asm.h>

#if defined(LIBC_SCCS)
	RCSID("$NetBSD: strrchr.S,v 1.2 1997/05/08 13:39:00 matthias Exp $")
# if defined(RINDEX)
	RCSID("$Masqueraded: as rindex $")
# endif
#endif

#if !defined(RINDEX)
/*
 * char *
 * strrchr (const char *s, int c)
 *	locate the last occurrence of c in the string s.
 */
ENTRY(strrchr)
#else
/*
 * char *
 * rindex (const char *s, int c)
 *	locate the last occurrence of c in the string s.
 */
ENTRY(rindex)
#endif
	enter	[r3,r4,r5,r6,r7],0
	movd	B_ARG0,r0
	movzbd	B_ARG1,r1
	movd	0x01010101,r5
	movqd	0,r6
	muld	r1,r5

	/*
	 * First begin by seeing if we can doubleword align the
	 * pointer. The following code aligns the pointer in r0.
	 */

	movqd	3,r3
	andd	r0,r3

0:	casew	1f(pc)[r3:w]
1:	.word	5f-0b
	.word	2f-0b
	.word	3f-0b
	.word	4f-0b

	.align	2,0xa2
2:	cmpb	r5,0(r0) ; bne 0f ; movd r0,r6 ; 0: cmpqb 0,0(r0) ; beq 8f
	cmpb	r5,1(r0) ; bne 0f ; movd r0,r6 ; 0: cmpqb 0,1(r0) ; beq 8f
	cmpb	r5,2(r0) ; bne 0f ; movd r0,r6 ; 0: cmpqb 0,2(r0) ; beq 8f
	addqd	3,r0
	br	5f

	.align	2,0xa2
3:	cmpb	r5,0(r0) ; bne 0f ; movd r0,r6 ; 0: cmpqb 0,0(r0) ; beq 8f
	cmpb	r5,1(r0) ; bne 0f ; movd r0,r6 ; 0: cmpqb 0,1(r0) ; beq 8f
	addqd	2,r0
	br	5f

	.align	2,0xa2
4:	cmpb	r5,0(r0) ; bne 0f ; movd r0,r6 ; 0: cmpqb 0,0(r0) ; beq 8f
	addqd	1,r0

	/*
	 * Okay, when we get down here r0 points at a double word
	 * algined source block of bytes.
	 * This guy processes four bytes at a time and checks for the
	 * zero terminating byte amongst the bytes in the double word.
	 * This algorithm is de to Dave Rand.
	 *
	 * Sneaky test for zero amongst four bytes:
	 *
	 *	 xxyyzztt
	 *	-01010101
	 *	---------
	 *	 aabbccdd
	 *  bic  xxyyzztt
	 *	---------
	 *       eeffgghh   ee=0x80 if xx=0, ff=0x80 if yy=0, etc. 
	 *
	 *		This whole result will be zero if there
	 *		was no zero byte, it will be non-zero if
	 *		there is a zero byte present.
	 */
   
5:	movd	0x01010101,r2		/* Magic number to use */
	movd	0x80808080,r3		/* Another magic number. */

	addqd	-4,r0
	br	1f

	.align	2,0xa2
0:	movd	r4,r1
	subd	r2,r1			/* Gets borrow if byte = 0. */
	bicd	r4,r1			/* Clear original bits. */
	andd	r3,r1			/* See if borrow occurred. */
	cmpqd	0,r1
	bne	8f			/* See if this DWORD contained a 0. */
1:	movd	4(r0),r4		/* Get next double word. */
	addd	4,r0			/* Advance pointer */
	xord	r5,r4			/* Convert any 'c' to 0. */
	movd	r4,r1
	subd	r2,r1
	bicd	r4,r1
	andd	r3,r1
	xord	r5,r4
	cmpqd	0,r1
	beq	0b
	movd	r6,r7
	movd	r0,r6
	br	0b

	/*
	 * At this point, r6 points at a double word which
	 * contains c or r6 is zero.
	 */

8:	movqd	0,r0
9:	cmpqd	0,r6
	beq	2f

	cmpb	r5,0(r6) ; bne 0f ; movd    r6,r0 ; 0: cmpqb 0,0(r6) ; beq 1f
	cmpb	r5,1(r6) ; bne 0f ; addr 1(r6),r0 ; 0: cmpqb 0,1(r6) ; beq 1f
	cmpb	r5,2(r6) ; bne 0f ; addr 2(r6),r0 ; 0: cmpqb 0,2(r6) ; beq 1f
	cmpb	r5,3(r6) ; bne 1f ; addr 3(r6),r0 ; 1: cmpqd 0,r0    ; bne 2f

	/* 
	 * Maybe the last match was behind the end of the string.
	 */
	movd	r7,r6
	br	9b

2:	exit	[r3,r4,r5,r6,r7]
	ret	0
