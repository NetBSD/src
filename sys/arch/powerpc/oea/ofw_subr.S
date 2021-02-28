/*	$NetBSD: ofw_subr.S,v 1.20 2021/02/28 19:01:11 thorpej Exp $	*/

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

#ifdef _KERNEL_OPT
#include "opt_ppcarch.h"
#endif

	/* Stack used to call into OpenFirmware. */
	.lcomm	firmstk,NBPG,16

	/* Buffer used to pass data to/from OpenFirmware. */
	.lcomm	OF_buffer,NBPG + 36,4

	/* The OpenFirmware entry point, provided by OpenFirmware. */
	.lcomm	ofentry,4,4

	/* Entry trampoline used by openfirmware(). */
	.lcomm	oftramp,4,4

	/* Client SR save area */
	.lcomm	clsrsave,64,4

	/* MSR used in OpenFirmware */
	.globl	ofwmsr
	.comm	ofwmsr,4,4

#ifdef FIRMWORKSBUGS
	.lcomm	ofwreal_incharge,4,4
#endif

/*
 * Called by start to save the initial OFW state so we can restore it
 * when call back to OFW.
 *
 * We expect the registers to be as for the entry point into the kernel:
 *
 *	%r1	Stack provided by OpenFirmware / boot loader
 *	%r5	OpenFirmware client entry point
 *
 * (others -- don't care)
 */
ENTRY_NOPROFILE(ofwinit)
	/* Save return address, Push a stack frame. */
	mflr	%r0
	stw	%r0,4(%r1)
	stwu	%r1,-16(%r1)

#ifdef FIRMWORKSBUGS
	mfmsr	%r0
	andi.	%r0,%r0,PSL_IR|PSL_DR
	beq	1f

	li	%r8,1
	lis	%r9,ofwreal_incharge@ha
	stw	%r8,ofwreal_incharge@l(%r9)

	bl	_C_LABEL(ofwr_init)
1:
#endif

	lis	%r8,ofentry@ha
	stw	%r5,ofentry@l(%r8)	/* save client interface handler */

	/*
	 * Call directly into OpenFirmware until we're ready to use
	 * the trampoline.
	 */
	lis	%r8,oftramp@ha
	stw	%r5,oftramp@l(%r8)

	/* Save the MSR that OpenFirmware is using. */
	mfmsr	%r0
	lis	%r9,ofwmsr@ha
	stw	%r0,ofwmsr@l(%r9)

	lis	%r8,OF_buffer@ha
	addi	%r8,%r8,OF_buffer@l
	lis	%r9,_C_LABEL(OF_buf)@ha
	stw	%r8,_C_LABEL(OF_buf)@l(%r9)

	/*
	 * Call into C code to perform additional early initialization.
	 */
	lis	%r8,_C_LABEL(ofw_bootstrap)@ha
	addi	%r8,%r8,_C_LABEL(ofw_bootstrap)@l
	mtctr	%r8
	bctrl

	/*
	 * Jump off the trampoline for all subsequent calls
	 * into OpenFirmware.
	 */
	lis	%r5,_C_LABEL(openfirmware_trampoline)@ha
	addi	%r5,%r5,_C_LABEL(openfirmware_trampoline)@l
	lis	%r8,oftramp@ha
	stw	%r5,oftramp@l(%r8)

	/* Retrieve LR, pop stack frame. */
	addi	%r1,%r1,16
	lwz	%r0,4(%r1)
	mtlr	%r0

	blr

/*
 * OpenFirmware trampoline.  We are already on the OpenFirmware stack.
 */
ENTRY_NOPROFILE(openfirmware_trampoline)
	mflr	%r0
	stw	%r0,4(%r1)		/* save return address */

	/*
	 * Push stack frame and save area:
	 *
	 * [SP+8 save area]
	 * [SP+4 slot for saved LR in callee]
	 * [SP+0 saved SP]
	 */
	stwu	%r1,-48(%r1)

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

	li	%r0,0			/* disable MMU */
	mtmsr	%r0
	isync

#if defined (PPC_OEA) || defined (PPC_OEA64_BRIDGE)
	/* clear BAT translations */
	mtdbatu	2,%r0
	mtdbatu	3,%r0
	mtibatu	2,%r0
	mtibatu	3,%r0
#endif /* PPC_OEA */

	lis	%r4,clsrsave@ha		/* save current SRs */
	addi	%r4,%r4,clsrsave@l
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
	/* curcpu()->ci_battable = &ofw_battable */
	GET_CPUINFO(%r4)
	lis	%r5,_C_LABEL(ofw_battable)@ha
	addi	%r5,%r5,_C_LABEL(ofw_battable)@l
	stw	%r5,CI_BATTABLE(%r4)

	lis	%r4,ofentry@ha		/* get firmware entry point */
	lwz	%r4,ofentry@l(%r4)
	mtlr	%r4

	lis	%r4,ofwmsr@ha		/* load Open Firmware MSR */
	lwz	%r5,ofwmsr@l(%r4)
	mtmsr	%r5
	isync

	blrl				/* call Open Firmware */

	li	%r0,0			/* ensure disable MMU is disabled */
	mtmsr	%r0
	isync

	/* curcpu()->ci_battable = &battable */
	GET_CPUINFO(%r4)
	lis	%r5,_C_LABEL(battable)@ha
	addi	%r5,%r5,_C_LABEL(battable)@l
	stw	%r5,CI_BATTABLE(%r4)

	lis	%r4,clsrsave@ha		/* restore saved SRs */
	addi	%r4,%r4,clsrsave@l
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
	addi	%r1,%r1,48		/* pop stack frame and save area */
	lwz	%r0,4(%r1)		/* return address */
	mtlr	%r0
	blr

/*
 * Call into OpenFirmware.
 */
ENTRY_NOPROFILE(openfirmware)
	mflr	%r0
	stw	%r0,4(%r1)		/* save return address */

	/* Switch to OpenFirmware stack. */
	lis	%r7,firmstk+NBPG-16@ha
	addi	%r7,%r7,firmstk+NBPG-16@l
	stw	%r1,0(%r7)		/* stash away prev stack pointer */
	mr	%r1,%r7

	lis	%r4,oftramp@ha
	lwz	%r4,oftramp@l(%r4)

	mtctr	%r4
	bctrl

	lwz	%r1,0(%r1)		/* restore previous stack pointer */
	lwz	%r0,4(%r1)		/* return address */
	mtlr	%r0
	blr
