/*	$NetBSD: kickstart.S,v 1.1.2.1 1999/12/27 18:31:37 wrstuden Exp $	*/

/*
 * Copyright (C) 1998 Ignatios Souvatzis. All rights reserved.
 */

#define PPC_CACHESIZE 16384
#define PPC_LINESIZE 32

/*
Memst:	.long	0x08000000
Krnst:	.long	0x082f3400
Krnsz:	.long	0x00100000
 */
	.globl _start
_start:
	bl	L2	/* we start with this address @ 0xfff00100 */
L2:
	lis	13,0xf60000@ha
	li	14,0x08
	stb	14,0xf60000@l(13)
#ifdef DEBUG
	lis	13,0xdff180@h
	ori	13,13,0xdff180@l

	lis	12,1000000@h
	ori	12,12,1000000@l
	mtctr	12
L2a:
	li	14,0x00f
	sth	14,0(13)
	sync
	li	14,0xf0f
	sth	14,0(13)
	sync
	bdnz	L2a
#endif
	mflr	3
	lwz	4,-16(3)
	lwz	5,-12(3)
	lwz	6,-8(3)
	li	7,3
	addi	8,6,3
	cmplw	4,5
	andc	8,8,7
	srwi	9,8,2
	mtctr	9
	bgt	Lb0
/* here: copying downwards and forwards */
	lwz	9,0(5)
	stw	9,0(4)
	bdz	Lflush
Lf1:	lwzu	7,4(5)
	stwu	7,4(4)
	bdnz	Lf1
	b	Lflush

Lb0:	add	4,4,8	/* add the rounded up length */
	add	5,5,8	/* add the rounded up length */
Lb1:	lwzu	7,-4(5)
	stwu	7,-4(4)
	bdnz	Lb1
	
Lflush:
#ifdef DEBUG
	li	14,0xff0
	sth	14,0(13)
	sync
#endif
	li	10,PPC_CACHESIZE/PPC_LINESIZE
	li	11,PPC_LINESIZE
/* dcbst loop */
	mtctr	10
	lwz	12,-8(3)
Lfl1:	dcbst	0,12
	add	12,12,11
	bdnz	Lfl1
	sync		/* and wait for past instructions to finish */
/* icbi loop */
	mtctr	10
	lwz	12,-8(3)
Lfl2:	icbi	0,12
	add	12,12,11
	bdnz	Lfl2
	sync		/* and wait for past instructions to finish */
	isync		/* and flush instruction queue */
Ljump:
#ifdef DEBUG
	li	14,0x0f0
	sth	14,0(13)
	sync
#endif
	lwz	3,-16(3)
	mtlr	3
	blr
/* NOTREACHED */
