/*	$NetBSD: ofw_subr.S,v 1.8.36.1 2011/06/23 14:19:32 cherry Exp $	*/

/*
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
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


	.local	firmstk
	.globl	openfirmware_entry
	.local	ofwsrsave
	.local	OF_buffer

	.lcomm	firmstk,NBPG,16
	.lcomm	OF_buffer,NBPG,4
	.comm	openfirmware_entry,4,4	/* openfirmware entry point */
	.lcomm	ofwsrsave,64,4		/* openfirmware SR savearea */
	.lcomm	ofwmsr,20,4		/* msr & sprg[0-3] used in OF */
	.comm	ofwreal_incharge,4,4

/*
 * Called by start to save the initial OFW state so we can restore it
 * when call back to OFW.
 */
ENTRY_NOPROFILE(ofwinit)
#ifdef	FIRMWORKSBUGS
	mfmsr	%r0
	andi.	%r0,%r0,PSL_IR|PSL_DR
	beq	1f

	li	%r8,1
	lis	%r9,ofwreal_incharge@ha
	stw	%r8,ofwreal_incharge@l(%r9)

	mflr	%r30
	bl	_C_LABEL(ofwr_init)
	mtlr	%r30
1:
#endif
	lis	%r8,openfirmware_entry@ha
	stw	%r5,openfirmware_entry@l(%r8) /* save client interface handler*/

	mfmsr	%r0
	li	%r8,PSL_IP
	andc	%r0,%r0,%r8			/* make sure PSL_IP is off */
	lis	%r9,ofwmsr@ha
	stwu	%r0,ofwmsr@l(%r9)		/* save initial MSR value */

	mfsprg0	%r0				/* save SPRGs */
	stw	%r0,4(%r9)
	mfsprg1	%r0
	stw	%r0,8(%r9)
	mfsprg2	%r0
	stw	%r0,12(%r9)
	mfsprg3	%r0
	stw	%r0,16(%r9)

	lis	%r8,OF_buffer@ha
	addi	%r8,%r8,OF_buffer@l
	lis	%r9,_C_LABEL(OF_buf)@ha
	stw	%r8,_C_LABEL(OF_buf)@l(%r9)

	blr

/*
 * OpenFirmware entry point
 */
	.text
ENTRY(openfirmware)
	mflr	%r0			/* save return address */
	stw	%r0,4(%r1)
	stwu	%r1,-32(%r1)		/* setup stack frame */

	lis	%r4,openfirmware_entry@ha	/* get firmware entry point */
	lwz	%r4,openfirmware_entry@l(%r4)
	mtlr	%r4

	mfsprg0	%r5			/* save current sprg0 (curcpu) */
	stw	%r5,16(%r1)
	mfsprg1	%r5			/* save current sprg1 */
	stw	%r5,20(%r1)
	mfsprg2	%r5			/* save current sprg1 */
	stw	%r5,24(%r1)
	mfsprg3	%r5			/* save current sprg3 */
	stw	%r5,28(%r1)

#ifdef FIRMWORKSBUGS
	lis	%r4,ofwreal_incharge@ha
	lwz	%r4,ofwreal_incharge@l(%r4)
	cmpwi	%r4,1
	bne	1f
	blrl
	b	4f
1:
#endif
	mfmsr	%r4			/* save msr */
	stw	%r4,8(%r1)

	li	%r0,0			/* clear battable translations */
	mtmsr	%r0
#if defined (PPC_OEA) || defined (PPC_OEA64_BRIDGE)
	mtdbatu	2,%r0
	mtdbatu	3,%r0
	mtibatu	2,%r0
	mtibatu	3,%r0
#endif /* PPC_OEA */

	lis	%r4,ofwsrsave@ha		/* save current SRs */
	addi	%r4,%r4,ofwsrsave@l
	li	%r5,0
1:	mfsrin	%r0,%r5
	stw	%r0,0(%r4)
	addi	%r4,%r4,4
	addis	%r5,%r5,0x10000000@h
	cmpwi	%r5,0
	bne	1b

	lis	%r4,_C_LABEL(ofw_pmap)@ha	/* load OFW SR */
	addi	%r4,%r4,_C_LABEL(ofw_pmap)@l
	lwz	%r0,PM_KERNELSR(%r4)
	cmpwi	%r0,0			/* pm_sr[KERNEL_SR] == 0? */
	beq	2f			/* then skip (not initialized yet) */
	li	%r5,0
1:	lwz	%r0,0(%r4)
	mtsrin	%r0,%r5
	addi	%r4,%r4,4
	addis	%r5,%r5,0x10000000@h
	cmpwi	%r5,0
	bne	1b
2:
	lis	%r4,ofwmsr+16@ha	/* Open Firmware msr + sprg[0-3] */
	lwzu	%r5,ofwmsr+16@l(%r4)
	mtsprg3	%r5
	lwz	%r5,-4(%r4)
	mtsprg2	%r5
	lwz	%r5,-8(%r4)
	mtsprg1	%r5
	lwz	%r5,-12(%r4)
	mtsprg0	%r5
	lwz	%r5,-16(%r4)
	mtmsr	%r5
	isync

	blrl				/* call Open Firmware */

	lis	%r4,ofwsrsave@ha	/* restore saved SRs */
	addi	%r4,%r4,ofwsrsave@l
	li	%r5,0
1:	lwz	%r0,0(%r4)
	mtsrin	%r0,%r5
	addi	%r4,%r4,4
	addis	%r5,%r5,0x10000000@h
	cmpwi	%r5,0
	bne	1b

	lwz	%r4,8(%r1)		/* restore msr */
	mtmsr	%r4
	isync
4:	
	lwz	%r5,16(%r1)		/* restore saved sprgs (curcpu) */
	mtsprg0	%r5
	lwz	%r5,20(%r1)
	mtsprg1	%r5
	lwz	%r5,24(%r1)
	mtsprg2	%r5
	lwz	%r5,28(%r1)
	mtsprg3	%r5

	lwz	%r1,0(%r1)		/* and return */
	lwz	%r0,4(%r1)
	mtlr	%r0
	blr

/*
 * Switch to/from OpenFirmware real mode stack
 *
 * Note: has to be called as the very first thing in OpenFirmware interface
 * routines.
 * E.g.:
 * int
 * OF_xxx(arg1, arg2)
 * type arg1, arg2;
 * {
 *	static struct {
 *		char *name;
 *		int nargs;
 *		int nreturns;
 *		char *method;
 *		int arg1;
 *		int arg2;
 *		int ret;
 *	} args = {
 *		"xxx",
 *		2,
 *		1,
 *	};
 *
 *	ofw_stack();
 *	args.arg1 = arg1;
 *	args.arg2 = arg2;
 *	if (openfirmware(&args) < 0)
 *		return -1;
 *	return args.ret;
 * }
 */

ENTRY(ofw_stack)
	mfmsr	%r8			/* turn off interrupts */
	andi.	%r0,%r8,~(PSL_EE|PSL_RI)@l
	mtmsr	%r0
	stw	%r8,4(%r1)		/* abuse return address slot */

	lwz	%r5,0(%r1)		/* get length of stack frame */
	subf	%r5,%r1,%r5

	lis	%r7,firmstk+NBPG-8@ha
	addi	%r7,%r7,firmstk+NBPG-8@l
	lis	%r6,ofw_back@ha
	addi	%r6,%r6,ofw_back@l
	subf	%r4,%r5,%r7		/* make room for stack frame on
					   new stack */
	stw	%r6,-4(%r7)		/* setup return pointer */
	stwu	%r1,-8(%r7)

	stw	%r7,-8(%r4)

	addi	%r3,%r1,8
	addi	%r1,%r4,-8
	subi	%r5,%r5,8

	b	_C_LABEL(ofbcopy)	/* and copy it */

ofw_back:
	lwz	%r1,0(%r1)		/* get callers original stack pointer */

	lwz	%r0,4(%r1)		/* get saved msr from abused slot */
	mtmsr	%r0

	lwz	%r1,0(%r1)		/* return */
	lwz	%r0,4(%r1)
	mtlr	%r0
	blr
