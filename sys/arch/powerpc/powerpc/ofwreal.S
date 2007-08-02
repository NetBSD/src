/*	$NetBSD: ofwreal.S,v 1.9.14.2 2007/08/02 05:34:22 macallan Exp $	*/

/*
 * Copyright (C) 1996 Wolfgang Solfrank.
 * Copyright (C) 1996 TooLs GmbH.
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * This file provides a real-mode client interface on machines, that
 * (incorrectly) only implement virtual mode client interface.
 *
 * It assumes though, that any actual memory in the machine is
 * mapped 1:1 even by the virtual mode OpenFirmware.
 * Furthermore it assumes that addresses returned by OpenFirmware are not
 * accessed by the client.
 *
 * TODO: handle set-callback specially
 */
#include <machine/psl.h>
#include <machine/trap.h>
#define _NOREGNAMES
#include <machine/asm.h>

#define	CACHELINE	32		/* Note that this value is really
					   hardwired */

	.data
ofentry:
	.long	0			/* actual entry to firmware in
					   virtual mode */

#define	BATSIZE		(8*8)
#define	SRSIZE		(16*4)
#define	SPRGSIZE	(4*4)
#define	SDR1SIZE	4
#define	SI1SIZE		(2*256)
#define	SI2SIZE		(3*256)
#define	SVSIZE		(BATSIZE+SRSIZE+SPRGSIZE+SDR1SIZE+SI1SIZE+SI2SIZE)

	.local	fwsave
	.comm	fwsave,SVSIZE,8

	.local	clsave
	.comm	clsave,SVSIZE,8

ENTRY(ofwr_init)
	mflr	%r31			/* save return address */

	mr	%r13,%r6		/* save args */
	mr	%r12,%r7		/* save argslen */
	lis	%r8,ofentry@ha
	stw	%r5,ofentry@l(%r8)	/* save virtual mode firmware entry */

	lis	%r3,fwsave@ha		/* save the mmu values of the
					   firmware */
	addi	%r3,%r3,fwsave@l
	bl	savemmu

	lis	%r5,fwentry@ha		/* get new firmware entry */
	addi	%r5,%r5,fwentry@l

	mr	%r6,%r13		/* restore args pointer */
	mr	%r7,%r12		/* restore args length */
	mtlr	%r31			/* restore return address */
	blr

/*
 * Emulated firmware entry.
 */
fwentry:
	mflr	%r0			/* save return address */
	stw	%r0,4(%r1)
	stwu	%r1,-16(%r1)		/* setup stack frame */
	stw	%r3,8(%r1)		/* save arg */

	lis	%r3,clsave@ha		/* save mmu values of client */
	addi	%r3,%r3,clsave@l
	bl	savemmu

	lis	%r3,fwsave@ha		/* restore mmu values of firmware */
	addi	%r3,%r3,fwsave@l
	bl	restoremmu

	lis	%r3,ofentry@ha
	lwz	%r3,ofentry@l(%r3)	/* get actual firmware entry */
	mtlr	%r3

	mfmsr	%r4
	stw	%r4,12(%r1)		/* save MSR */
	ori	%r4,%r4,PSL_IR|PSL_DR	/* turn on MMU */
	andi.	%r4,%r4,~PSL_EE@l	/* turn off interrupts */
	mtmsr	%r4
	isync

	lwz	%r3,8(%r1)		/* restore arg */
	blrl				/* do actual firmware call */
	stw	%r3,8(%r1)		/* save return value */

	lwz	%r4,12(%r1)		/* get saved MSR */
	mtmsr	%r4
	isync

	lis	%r3,fwsave@ha		/* save mmu values of firmare */
	addi	%r3,%r3,fwsave@l	/* (might not be necessary, but... */
	bl	savemmu

	lis	%r3,clsave@ha		/* restore mmu values of client */
	addi	%r3,%r3,clsave@l
	bl	restoremmu

	lwz	%r3,8(%r1)		/* restore return value */
	lwz	%r1,0(%r1)		/* and return */
	lwz	%r0,4(%r1)
	mtlr	%r0
	blr

/*
 * Save everything related to the mmu to the saveare pointed to by r3.
 */
savemmu:

	mfibatl	%r4,0			/* save BATs */
	stw	%r4,0(%r3)
	mfibatu	%r4,0
	stw	%r4,4(%r3)
	mfibatl	%r4,1
	stw	%r4,8(%r3)
	mfibatu	%r4,1
	stw	%r4,12(%r3)
	mfibatl	%r4,2
	stw	%r4,16(%r3)
	mfibatu	%r4,2
	stw	%r4,20(%r3)
	mfibatl	%r4,3
	stw	%r4,24(%r3)
	mfibatu	%r4,3
	stw	%r4,28(%r3)
	mfdbatl	%r4,0
	stw	%r4,32(%r3)
	mfdbatu	%r4,0
	stw	%r4,36(%r3)
	mfdbatl	%r4,1
	stw	%r4,40(%r3)
	mfdbatu	%r4,1
	stw	%r4,44(%r3)
	mfdbatl	%r4,2
	stw	%r4,48(%r3)
	mfdbatu	%r4,2
	stw	%r4,52(%r3)
	mfdbatl	%r4,3
	stw	%r4,56(%r3)
	mfdbatu	%r4,3
	stwu	%r4,60(%r3)

	li	%r4,0			/* save SRs */
1:
	addis	%r4,%r4,-0x10000000@ha
	or.	%r4,%r4,%r4
	mfsrin	%r5,%r4
	stwu	%r5,4(%r3)
	bne	1b

	mfsprg0	%r4			/* save SPRGs */
	stw	%r4,4(%r3)
	mfsprg1	%r4
	stw	%r4,8(%r3)
	mfsprg2	%r4
	stw	%r4,12(%r3)
	mfsprg3	%r4
	stw	%r4,16(%r3)

	mfsdr1	%r4			/* save SDR1 */
	stw	%r4,20(%r3)

	addi	%r4,%r3,24
	mflr	%r11
	li	%r3,EXC_DSI		/* save DSI/ISI trap vectors */
	li	%r5,SI1SIZE
	bl	copy

	mtlr	%r11
	li	%r3,EXC_IMISS		/* save MISS trap vectors */
	li	%r5,SI2SIZE

copy:
	li	%r6,CACHELINE
1:
	lwz	%r7,0(%r3)
	lwz	%r8,4(%r3)
	lwz	%r9,8(%r3)
	lwz	%r10,12(%r3)
	stw	%r7,0(%r4)
	stw	%r8,4(%r4)
	stw	%r9,8(%r4)
	stw	%r10,12(%r4)
	lwz	%r7,16(%r3)
	lwz	%r8,20(%r3)
	lwz	%r9,24(%r3)
	lwz	%r10,28(%r3)
	stw	%r7,16(%r4)
	stw	%r8,20(%r4)
	stw	%r9,24(%r4)
	stw	%r10,28(%r4)
	dcbst	%r0,%r4
	icbi	%r0,%r4
	add	%r3,%r3,%r6
	add	%r4,%r4,%r6
	subf.	%r5,%r6,%r5
	bgt	1b

	dcbst	%r0,%r4
	icbi	%r0,%r4

	sync
	isync

	blr

/*
 * Restore everyting related to the mmu from the saveare pointed to by r3.
 */
restoremmu:
	mfmsr	%r12
	andi.	%r4,%r12,~(PSL_IR|PSL_DR)@l
	mtmsr	%r4			/* Disable MMU */
	isync

	li	%r4,0			/* first, invalidate BATs */
	mtibatu	0,%r4
	mtibatu	1,%r4
	mtibatu	2,%r4
	mtibatu	3,%r4
	mtdbatu	0,%r4
	mtdbatu	1,%r4
	mtdbatu	2,%r4
	mtdbatu	3,%r4

	lwz	%r4,0(%r3)
	mtibatl	0,%r4			/* restore BATs */
	lwz	%r4,4(%r3)
	mtibatu	0,%r4
	lwz	%r4,8(%r3)
	mtibatl	1,%r4
	lwz	%r4,12(%r3)
	mtibatu	1,%r4
	lwz	%r4,16(%r3)
	mtibatl	2,%r4
	lwz	%r4,20(%r3)
	mtibatu	2,%r4
	lwz	%r4,24(%r3)
	mtibatl	3,%r4
	lwz	%r4,28(%r3)
	mtibatu	3,%r4
	lwz	%r4,32(%r3)
	mtdbatl	0,%r4
	lwz	%r4,36(%r3)
	mtdbatu	0,%r4
	lwz	%r4,40(%r3)
	mtdbatl	1,%r4
	lwz	%r4,44(%r3)
	mtdbatu	1,%r4
	lwz	%r4,48(%r3)
	mtdbatl	2,%r4
	lwz	%r4,52(%r3)
	mtdbatu	2,%r4
	lwz	%r4,56(%r3)
	mtdbatl	3,%r4
	lwzu	%r4,60(%r3)
	mtdbatu	3,%r4

	li	%r4,0			/* restore SRs */
1:
	lwzu	%r5,4(%r3)
	addis	%r4,%r4,-0x10000000@ha
	or.	%r4,%r4,%r4
	mtsrin	%r5,%r4
	bne	1b

	lwz	%r4,4(%r3)
	mtsprg0	%r4			/* restore SPRGs */
	lwz	%r4,8(%r3)
	mtsprg1	%r4
	lwz	%r4,12(%r3)
	mtsprg2	%r4
	lwz	%r4,16(%r3)
	mtsprg3	%r4

	sync				/* remove everything from tlb */
	lis	%r4,0x40000@ha
	li	%r5,0x1000
1:
	subf.	%r4,%r5,%r4
	tlbie	%r4
	bne	1b

	sync
	tlbsync
	sync

	lwz	%r4,20(%r3)
	sync
	mtsdr1	%r4			/* restore SDR1 */

	addi	%r3,%r3,24
	mflr	%r11
	li	%r4,EXC_DSI		/* restore DSI/ISI trap vectors */
	li	%r5,SI1SIZE
	bl	copy

	li	%r4,EXC_IMISS		/* restore MISS trap vectors */
	li	%r5,SI2SIZE
	bl	copy

	/* tlbia */
	sync
	li	%r3,0x40
	mtctr	%r3
	li	%r4,0
    1:
	tlbie	%r4
	addi	%r4,%r4,0x1000
	bdnz	1b
	sync
	tlbsync
	sync

	mtmsr	%r12			/* restore MMU */
	mtlr	%r11
	blr
