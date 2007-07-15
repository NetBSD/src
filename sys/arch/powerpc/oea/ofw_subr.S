/*	$NetBSD: ofw_subr.S,v 1.3.14.1 2007/07/15 13:16:49 ad Exp $	*/

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

	.data
GLOBAL(ofmsr)
	.long	0,0,0,0,0		/* msr & sprg[0-3] used in OF */

GLOBAL(ofwsprg0save)
	.long	0

GLOBAL(ofwreal_incharge)
	.long	0

	.comm	firmstk,NBPG,8
	.comm	OF_buffer,NBPG,4
	.comm	openfirmware_entry,4,4	/* openfirmware entry point */
	.comm	ofwsrsave,64,4		/* openfirmware SR savearea */

/*
 * Called by start to save the initial OFW state so we can restore it
 * when call back to OFW.
 */
ENTRY_NOPROFILE(ofwinit)
#ifdef	FIRMWORKSBUGS
	mfmsr	0
	andi.	0,0,PSL_IR|PSL_DR
	beq	1f

	li	8,1
	lis	9,ofwreal_incharge@ha
	stw	8,ofwreal_incharge@l(9)

	mflr	30
	bl	_C_LABEL(ofwr_init)
	mtlr	30
1:
#endif
	lis	8,openfirmware_entry@ha
	stw	5,openfirmware_entry@l(8) /* save client interface handler*/

	mfmsr	0
	lis	9,ofmsr@ha
	stwu	0,ofmsr@l(9)		/* save initial MSR value */

        mfsprg  0,0			/* save SPRGs */
       	stwu	0,4(9)
        mfsprg  0,1
       	stwu	0,4(9)
        mfsprg  0,2
       	stwu	0,4(9)
        mfsprg  0,3
       	stw	0,4(9)

	lis	8,OF_buffer@ha
	addi	8,8,OF_buffer@l
	lis	9,_C_LABEL(OF_buf)@ha
	stw	8,_C_LABEL(OF_buf)@l(9)

	blr

/*
 * OpenFirmware entry point
 */
	.text
ENTRY(openfirmware)
	mflr	0			/* save return address */
	stw	0,4(1)
	stwu	1,-16(1)		/* setup stack frame */

	lis	4,openfirmware_entry@ha	/* get firmware entry point */
	lwz	4,openfirmware_entry@l(4)
	mtlr	4

	mfsprg	5,0			/* save current sprg0 (curcpu) */
	lis	4,ofwsprg0save@ha
	addi	4,4,ofwsprg0save@l
	stw	5,0(4)

#ifdef FIRMWORKSBUGS
	lis	4,ofwreal_incharge@ha
	lwz	4,ofwreal_incharge@l(4)
	cmpwi	4,1
	bne	1f
	blrl
1:
#else
	mfmsr	4			/* save msr */
	stw	4,8(1)

	li	0,0			/* clear battable translations */
#if defined (PPC_OEA) && !defined (PPC_OEA64) && !defined (PPC_OEA64_BRIDGE)
	mtdbatu	2,0
	mtdbatu	3,0
	mtibatu	2,0
	mtibatu	3,0
#endif /* PPC_OEA */

	lis	4,ofwsrsave@ha		/* save current SRs */
	addi	4,4,ofwsrsave@l
	li	5,0
1:	mfsrin	0,5
	stw	0,0(4)
	addi	4,4,4
	addis	5,5,0x10000000@h
	cmpwi	5,0
	bne	1b

	lis	4,_C_LABEL(ofw_pmap)@ha	/* load OFW SR */
	addi	4,4,_C_LABEL(ofw_pmap)@l
	lwz	0,PM_KERNELSR(4)
	cmpwi	0,0			/* pm_sr[KERNEL_SR] == 0? */
	beq	2f			/* then skip (not initialized yet) */
	li	5,0
1:	lwz	0,0(4)
	mtsrin	0,5
	addi	4,4,4
	addis	5,5,0x10000000@h
	cmpwi	5,0
	bne	1b
2:
	lis	4,ofmsr@ha		/* Open Firmware msr + sprg[0-3] */
	lwzu	5,ofmsr+16@l(4)
	mtsprg	3,5
	lwzu	5,-4(4)
	mtsprg	2,5
	lwzu	5,-4(4)
	mtsprg	1,5
	lwzu	5,-4(4)
	mtsprg	0,5
	lwz	5,-4(4)
	mtmsr	5
	isync

	blrl				/* call Open Firmware */

	lis	4,ofwsrsave@ha		/* restore saved SRs */
	addi	4,4,ofwsrsave@l
	li	5,0
1:	lwz	0,0(4)
	mtsrin	0,5
	addi	4,4,4
	addis	5,5,0x10000000@h
	cmpwi	5,0
	bne	1b

	lwz	4,8(1)			/* restore msr */
	mtmsr	4
	isync
#endif /* FIRMWORKSBUGS */

	lis	4,ofwsprg0save@ha	/* restore saved sprg0 (curcpu) */
	addi	4,4,ofwsprg0save@l
	lwz	5,0(4)
	mtsprg	0,5

	lwz	1,0(1)			/* and return */
	lwz	0,4(1)
	mtlr	0
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
	mfmsr	8			/* turn off interrupts */
	andi.	0,8,~(PSL_EE|PSL_RI)@l
	mtmsr	0
	stw	8,4(1)			/* abuse return address slot */

	lwz	5,0(1)			/* get length of stack frame */
	subf	5,1,5

	lis	7,firmstk+NBPG-8@ha
	addi	7,7,firmstk+NBPG-8@l
	lis	6,ofw_back@ha
	addi	6,6,ofw_back@l
	subf	4,5,7			/* make room for stack frame on
					   new stack */
	stw	6,-4(7)			/* setup return pointer */
	stwu	1,-8(7)

	stw	7,-8(4)

	addi	3,1,8
	addi	1,4,-8
	subi	5,5,8

	b	_C_LABEL(ofbcopy)	/* and copy it */

ofw_back:
	lwz	1,0(1)			/* get callers original stack pointer */

	lwz	0,4(1)			/* get saved msr from abused slot */
	mtmsr	0

	lwz	1,0(1)			/* return */
	lwz	0,4(1)
	mtlr	0
	blr
