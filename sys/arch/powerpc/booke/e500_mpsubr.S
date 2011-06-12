
/*
 * r3 = fdt pointer (ignored)
 * r4 = 0
 * r5 = 0
 * r6 = EPAPR magic (0x45505150)
 * r7 = TLB1[0] entry size (64MB)
 * r8 = 0
 * r9 = 0
 */
_ENTRY(e500_spinup_trampoline)

	/*
	 * First thing we need to do is to set SPRG0 with our cpu_info
	 * and get our initial stack pointer (this must be within the
	 * bounds of the TLB1[0] entry U-boot setup for us).
	 *
	 * cpu_hatch will return a new SP to use.
	 *
	 * All the caller-saved register are ours to use.  So we will.
	 */
	lis	%r20, _C_LABEL(cpu_hatch_data)@ha
	addi	%r20, %r20, _C_LABEL(cpu_hatch_data)@l

	lwz	%r21, HATCH_CI(%r20)		/* get cpu_info */
	mtsprg0	%r21				/* save cpu_info */
	lwz	%r1, HATCH_SP(%r20)		/* get hatch SP */
	
	/*
	 * We have to setup the IVOR SPRs since the ones u-boot setup
	 * don't work for us.
	 */
	bl	_C_LABEL(exception_init)	/* setup IVORs */

	/*
	 * U-boot has mapped the bottom 64MB in TLB1[0].  We are going to need
	 * change this entry and it's not safe to do so while running out of it.
	 * So we copy TLB1[0] to TLB1[1] but set it for AS1.  We then switch
	 * to AS1 and reload TLB1[0] with its correct value, and we switch
	 * back to AS0.  Then we can load the rest of the TLB1 entries.
	 */

	/*
	 * Fetch TLB1[0]
	 */
	lis	%r16, (MASX_TLBSEL_MAKE(1)|MAS0_ESEL_MAKE(0))@h
	mtspr	SPR_MAS0, %r16
	tlbre

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

	/*
	 * Let's find out what TLB1[0] entry we are supposed to use.
	 */
	li	%r3, 0
	bl	_C_LABEL(e500_tlb1_fetch)
	lwz	%r28, 0(%r3)			/* load the saved TLB1 entry */
	mtspr	SPR_MAS0, %r28			/* place into SPRs */
	mtspr	SPR_MAS1, %r29
	mtspr	SPR_MAS2, %r30
	mtspr	SPR_MAS3, %r31

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
	rfi			/* switch to AS1, context synchronizing */

.Las1start:
	/*
	 * We are now running in AS1, update TLB1[0]
	 */
	tlbwe

	mtsrr0	%r5
	mtsrr1	%r3
	rfi			/* switch back to AS0, context synchronizing */

.Las1end:
	/*
	 * We now have our TLB1[0] in place.  Now we need to load the rest of
	 * TLB1 with our entries.  After this is done, we should have access
	 * to everything.
	 */
	bl	_C_LABEL(e500_tlb1_sync)

	/*
	 * We've gotten the low level stuff done.
	 * Now to do more advanced stuff.
	 */
	bl	_C_LABEL(cpu_hatch)	
	mr	%r1, %r3			/* our new SP */

	wrteei	1				/* allow interrupts */

	b	_C_LABEL(idle_loop)
