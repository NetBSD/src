/*	$NetBSD: strcpy.S,v 1.4 1999/06/06 19:49:11 matthias Exp $	*/

/* 
 * Written by Randy Hyde, 1993
 * Public domain.
 */

#include <machine/asm.h>

#if defined(LIBC_SCCS)
	RCSID("$NetBSD: strcpy.S,v 1.4 1999/06/06 19:49:11 matthias Exp $")
#endif

/*
 * char *
 * strcpy (char *d, const char *s)
 *	copy string s to d.
 */

KENTRY(strcpy, 8)
	enter	[r3,r4,r5],0
	movd	B_ARG1,r1
	movd	B_ARG0,r2

	/*
	 * We need to double word align the src (R1). When we read
	 * by double words we can read potentially up to 3 bytes
	 * past the null terminating byte. We can avoid spurious page
	 * faults by double word aligning the source. It also runs
	 * faster.  IWD
	 */

	movqd	3,r3
	andd	r1,r3

0:	casew	1f(pc)[r3:w]
1:	.word	5f-0b
	.word	2f-0b
	.word	3f-0b
	.word	4f-0b

	.align	2,0xa2
2:	movb	0(r1),0(r2) ; cmpqb 0,0(r1) ; beq 9f
	movb 	1(r1),1(r2) ; cmpqb 0,1(r1) ; beq 9f
	movb	2(r1),2(r2) ; cmpqb 0,2(r1) ; beq 9f
	addqd	3,r1
	addqd	3,r2
	br	5f

	.align	2,0xa2
3:	movb	0(r1),0(r2) ; cmpqb 0,0(r1) ; beq 9f
	movb 	1(r1),1(r2) ; cmpqb 0,1(r1) ; beq 9f
	addqd	2,r1
	addqd	2,r2
	br	5f

	.align	2,0xa2
4:	movb	0(r1),0(r2) ; cmpqb 0,0(r1) ; beq 9f
	addqd	1,r1
	addqd	1,r2

	/*
	 * Okay, when we get down here R1 points at a double word
	 * aligned source block of bytes, R2 points at the destination
	 * block of bytes (typically, though not always double word
	 * aligned).
	 * This guy processes four bytes at a time and checks for the
	 * zero terminating byte amongst the bytes in the double word.
	 * This algorithm is due to Dave Rand.
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
   
5:	movd	0x01010101,r3		/* Magic number to use */
	movd	0x80808080,r4		/* Another magic number. */
	
	movd	0(r1),r5		/* Get 1st double word. */
	movd	r5,r0			/* Save for storage later. */
	subd	r3,r0			/* Gets borrow if byte = 0. */
	bicd	r5,r0			/* Clear original bits. */
	andd	r4,r0			/* See if borrow occurred. */
	cmpqd	0,r0
	bne	1f			/* See if this DWORD contained a 0. */

	.align	2,0xa2
0:	movd	4(r1),r0		/* Get next 4 bytes to process */
	movd	r5,0(r2)		/* Save away prev four bytes. */
	addd	4,r1			/* addd is faster then addqd here */
	addd	4,r2
	movd	r0,r5			/* Save for storage if no zeros. */
	subd	r3,r0
	bicd	r5,r0
	andd	r4,r0
	cmpqd	0,r0
	beq	0b

	/*
	 * At this point, R1 points at a double word which
	 * contains a zero byte. Copy the characters up to
	 * (and including) the zero byte and quit.
	 */

1:	movb	0(r1),0(r2) ; cmpqb 0,0(r1) ; beq 9f
	movb	1(r1),1(r2) ; cmpqb 0,1(r1) ; beq 9f
	movb	2(r1),2(r2) ; cmpqb 0,2(r1) ; beq 9f
	movb	3(r1),3(r2)		/* This one must be a zero byte */
9:	movd	B_ARG0,r0
	exit	[r3,r4,r5]
	ret	ARGS
