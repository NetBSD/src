/*-
 * Copyright (c) 2011, 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * r3 = fdt pointer (ignored)
 * r4 = 0
 * r5 = 0
 * r6 = EPAPR magic (0x45505150)
 * r7 = TLB1[0] entry size (64MB)
 * r8 = 0
 * r9 = 0
 */
	.p2align 5
ENTRY_NOPROFILE(e500_spinup_trampoline)

	lis	%r31, 0xdeadbeef@h
	ori	%r31, %r31, 0xdeadbeef@l
	mr	%r30, %r31
	mr	%r29, %r31
	mr	%r28, %r31
	mr	%r27, %r31
	mr	%r26, %r31
	mr	%r25, %r31
	mr	%r24, %r31
	mr	%r23, %r31
	mr	%r22, %r31
	mr	%r21, %r31
	mr	%r20, %r31
	mr	%r19, %r31
	mr	%r18, %r31
	mr	%r17, %r31
	mr	%r16, %r31
	mr	%r15, %r31
	mr	%r14, %r31
	mr	%r13, %r31
	mr	%r12, %r31
	mr	%r11, %r31
	mr	%r10, %r31
	mr	%r2, %r31

	/*
	 * First thing we need to do is to set SPRG0 with our cpu_info
	 * and get our initial stack pointer (this must be within the
	 * bounds of the TLB1[0] entry U-boot setup for us).
	 *
	 * cpu_hatch will return a new SP to use.
	 *
	 * All the caller-saved register are ours to use.  So we will.
	 */
	lis	%r20, _C_LABEL(cpu_hatch_data)@h
	ori	%r20, %r20, _C_LABEL(cpu_hatch_data)@l
	sync

	/*
	 * Ensure that the TLB entry we are using is memory coherent.
	 */
	lis	%r0, (MASX_TLBSEL_MAKE(1)|MAS0_ESEL_MAKE(0))@h
	mtspr	SPR_MAS0, %r0 			/* setup MAS0 */
	lis	%r3, (MAS1_V|MAS1_IPROT)@h	/* V | IPROT */
	ori	%r3, %r3, MASX_TSIZE_64MB	/* and 64MB */
	mtspr	SPR_MAS1, %r3 			/* save MAS1 */
	li	%r3, MAS2_M			/* set M bit */
	mtspr	SPR_MAS2, %r3 			/* save MAS2 */
	li	%r3, MAS3_SX|MAS3_SR|MAS3_SW	/* set kernel RWX */
	mtspr	SPR_MAS3, %r3 			/* save MAS3 */
	tlbwe					/* update entry */
	isync					/* flush i-stream */
	sync					/* sync memory. */

	li	%r0, 0
	stw	%r0, HATCH_RUNNING(%r20)	/* progress */
	sync
#if 0
	dcbf	0, %r20
#endif

	lwz	%r1, HATCH_SP(%r20)		/* get hatch SP */
	lwz	%r21, HATCH_CI(%r20)		/* get cpu_info */
	mtsprg0	%r21				/* save cpu_info */
	lwz	%r13, CI_CURLWP(%r21)		/* load r13 with curlwp */
	mtsprg2	%r13				/* save it in sprg2 */
	addi    %r0,%r21,CI_SAVELIFO		/* get SAVE area start */
	mtsprg3 %r0				/* save it in sprg3 */

	/*
	 * Now to synchronize timebase values.  First to make sure HID0 is
	 * set correctly, except with the timebase disabled.
	 */
	lwz	%r22, HATCH_HID0(%r20)		/* get HID0 */
	li	%r28, HID0_TBEN			/* HID0_TBEN */
	andc	%r0,%r22,%r28			/* clear TBEN from HID0 */
	mtspr	SPR_HID0, %r0			/* set HID0 (timebase off) */
	isync
	lwz	%r24, HATCH_TBL(%r20)		/* get lower timebase value */
	lwz	%r23, HATCH_TBU(%r20)		/* get upper timebase value */

	/*
	 * Figure out how much we are adjusting the timebase
	 */
	mftbl	%r4				/* get lower timebase */
	subfc	%r0, %r4, %r24			/* subtract from new value */
	stw	%r0, HATCH_TBL(%r20)		/* save it */
	mftbu	%r3				/* get upper timebase */
	subfe	%r0, %r3, %r23			/* subtract from new value */
	stw	%r0, HATCH_TBU(%r20)		/* save it */

	/*
	 * Since we've disabled timebase, we can set the timebase registers
	 * without fear of them changing.  Have to do this after we read the
	 * previous values.
	 */
	mttbu	%r23				/* set upper timebase */
	mttbl	%r24				/* set lower timebase */

	/*
	 * Now we loop until the boot cpu tells us to enable timebase
	 */
1:	lwz	%r0, HATCH_RUNNING(%r20)	/* is it time? */
	cmplwi	%r0, 0
	beq+	1b

	mtspr	SPR_HID0, %r22			/* start timebase running */
	isync

	li	%r0, 2
	stw	%r0, HATCH_RUNNING(%r20)	/* progress */
	sync

	/*
	 * We have to setup the IVOR SPRs since the ones u-boot setup
	 * don't work for us.
	 */
	bl	_C_LABEL(exception_init)	/* setup IVORs */

	li	%r0, 3
	stw	%r0, HATCH_RUNNING(%r20)	/* progress */

	/*
	 * U-boot has mapped the bottom 64MB in TLB1[0].  We are going to need
	 * to change this entry and it's not safe to do so while running out
	 * of it.  So we copy TLB1[0] to TLB1[1] but set it for AS1.  We then
	 * switch to AS1 and reload TLB1[0] with its correct value, and then we
	 * switch back to AS0.  After that, we can load the rest of the TLB1
	 * entries.
	 */

	/*
	 * Fetch TLB1[0]
	 */
	lis	%r16, (MASX_TLBSEL_MAKE(1)|MAS0_ESEL_MAKE(0))@h
	mtspr	SPR_MAS0, %r16
	tlbre

	li	%r0, 4
	stw	%r0, HATCH_RUNNING(%r20)	/* progress */

	/*
	 * Copy TLB1[0] to TLB[1] and set it to use AS1
	 */
	mfspr	%r3, SPR_MAS0
	addis	%r3, %r3, MAS0_ESEL@h		/* advance to next TLB entry */
	mtspr	SPR_MAS0, %r3			/* place into SPR */
	mfspr	%r4, SPR_MAS1
	ori	%r4, %r4, MAS1_TS@l		/* Make it use AS1 */
	mtspr	SPR_MAS1, %r4
	tlbwe					/* write the TLB entry */

	li	%r0, 5
	stw	%r0, HATCH_RUNNING(%r20)	/* progress */

	/*
	 * Let's find out what TLB1 entry we are supposed to use.
	 */
	lwz	%r3, HATCH_TLBIDX(%r20)
	bl	_C_LABEL(e500_tlb1_fetch)
	lmw	%r28, 0(%r3)			/* load the saved TLB1 entry */

	li	%r0, 6
	stw	%r0, HATCH_RUNNING(%r20)	/* progress */

	/*
	 * Now to switch to running in AS1
	 */
	mfmsr	%r3
	ori	%r4,%r3,(PSL_DS|PSL_IS)@l
	mtsrr1	%r4

	bl	1f
1:	mflr	%r11
	addi	%r4,%r11,.Las1start-1b
	addi	%r5,%r11,.Las1end-1b
	mtsrr0	%r4
	li	%r0, 7
	stw	%r0, HATCH_RUNNING(%r20)	/* progress */
	rfi			/* switch to AS1, context synchronizing */

.Las1start:
	/*
	 * We are now running in AS1, update TLB1[0]
	 */
	li	%r0, 8
	stw	%r0, HATCH_RUNNING(%r20)	/* progress */

	/*
	 * Let's clear TBL1[0] and TBL1[1]
	 */
	li	%r8, 0
	mtspr	SPR_MAS1, %r8
	mtspr	SPR_MAS2, %r8
	mtspr	SPR_MAS3, %r8
	mtspr	SPR_MAS7, %r8

	lis	%r8, (MASX_TLBSEL_MAKE(1)|MAS0_ESEL_MAKE(0))@h
	mtspr	SPR_MAS0, %r8
	tlbwe

	lis	%r8, (MASX_TLBSEL_MAKE(1)|MAS0_ESEL_MAKE(1))@h
	mtspr	SPR_MAS0, %r8
	tlbwe

	/*
	 * Now load the new TLB data into the MAS registers.
	 */
	mtspr	SPR_MAS0, %r28			/* place into SPRs */
	mtspr	SPR_MAS1, %r29
	mtspr	SPR_MAS2, %r30
	mtspr	SPR_MAS3, %r31
	tlbwe

	mtsrr0	%r5
	mtsrr1	%r3

	li	%r0, 9
	stw	%r0, HATCH_RUNNING(%r20)	/* progress */

	rfi			/* switch back to AS0, context synchronizing */

.Las1end:
	li	%r0, 10
	stw	%r0, HATCH_RUNNING(%r20)	/* progress */

	/*
	 * We now have our TLB1[0] in place.  Now we need to load the rest of
	 * TLB1 with our entries.  After this is done, we should have access
	 * to everything.
	 */
	bl	_C_LABEL(e500_tlb1_sync)

	li	%r0, 11
	stw	%r0, HATCH_RUNNING(%r20)	/* progress */

	/*
	 * Now we can use our stack...
	 */
	lwz	%r0, CI_CURPCB(%r21)
	lwz	%r1, PCB_SP(%r0)

	li	%r0, 12
	stw	%r0, HATCH_RUNNING(%r20)	/* progress */

	/*
	 * Tell spinup code we are done with the hatch stack.
	 */
	li	%r0, 0
	stw	%r0, HATCH_SP(%r20)

	li	%r0, 13
	stw	%r0, HATCH_RUNNING(%r20)	/* progress */

	/*
	 * We've gotten the low level stuff done.
	 * Now to do more advanced stuff.
	 */
	mr	%r3, %r21
	bl	_C_LABEL(e500_cpu_hatch)	

	li	%r0, 14
	stw	%r0, HATCH_RUNNING(%r20)	/* progress */

	/*
	 * Now wait to become runnable
	 */
	bl	_C_LABEL(cpu_hatch)

	wrteei	1				/* allow interrupts */
	bl	_C_LABEL(spl0)			/* unblock interrupts */

	b	_C_LABEL(idle_loop)
