/* $NetBSD: entry.S,v 1.5.6.1 2012/02/18 07:33:05 mrg Exp $ */

#include <powerpc/psl.h>
#include <powerpc/spr.h>
#include <powerpc/oea/spr.h>
#include <powerpc/oea/bat.h>
#include <powerpc/oea/hid.h>

	.text
	.globl _start
_start:
	/*
	 * Save possible argc and argv values from the firmware, usually
	 * passed in r3 and r4.
	 * When started with "bootm", as a Linux kernel module, r6 and r7
	 * point to the start and end address of the bootargs.
	 */
	mr	30,3
	mr	31,4
	mr	28,6
	mr	29,7

	/* Disable interrupts and everything except the MMU. */
	mfmsr	3
	andi.	3,3,PSL_DR|PSL_IR
	mtmsr	3
	isync

	/*
	 * U-Boot/PPCBoot forgets to flush the cache when using the "bootm"
	 * command, so we have to do that now.
	 */
	lis	11,_start@ha
	addi	11,11,_start@l
	li	10,-32
	and	11,11,10
	lis	12,(_edata+31)@ha
	addi	12,12,(_edata+31)@l
	bl	syncicache

	mfspr	11,SPR_HID0
	andi.	0,11,HID0_DCE
	ori	11,11,HID0_ICE
	ori	8,11,HID0_ICFI
	bne	1f			/* don't invalidate the D-cache */
	ori	8,8,HID0_DCFI		/* unless it wasn't enabled */
1:
	mfmsr	0
	andi.	0,0,PSL_DR
	beq	2f
	lis	5,0xfec00000@ha		/* CONFIG_ADDR of PCI */
	lis	6,0xfee00000@ha		/* CONFIG_DATA of PCI */
	mfspr	3,SPR_DBAT0U
	mfspr	4,SPR_DBAT0L
	bl	dbat_sanity_check
	beq	3f
	mfspr	3,SPR_DBAT1U
	mfspr	4,SPR_DBAT1L
	bl	dbat_sanity_check
	beq	3f
	mfspr	3,SPR_DBAT2U
	mfspr	4,SPR_DBAT2L
	bl	dbat_sanity_check
	beq	3f
	mfspr	3,SPR_DBAT3U
	mfspr	4,SPR_DBAT3L
	bl	dbat_sanity_check
	beq	3f

2:	/* Disable D-cache */
	li	0,HID0_DCE
	andc	11,11,0
	b	4f

3:	/* Enable D-cache */
	ori	11,11,HID0_DCE

4:
	lis	1,BAT123@ha
	addi	1,1,BAT123@l
	lwz	3,0(1)
	lwz	4,4(1)
	mtdbatl	1,3
	mtdbatu	1,4
	lwz	3,8(1)
	lwz	4,12(1)
	mtdbatl	2,3
	mtdbatu	2,4
	lwz	3,16(1)
	lwz	4,20(1)
	mtdbatl	3,3
	mtdbatu	3,4

	sync
	mtspr	SPR_HID0,8		/* enable and invalidate caches */
	sync
	mtspr	SPR_HID0,11		/* enable caches */
	sync
	isync

	/* make sure .bss gets zeroed. */
	li	0,0
	lis	8,edata@ha
	addi	8,8,edata@l
	lis	9,end@ha
	addi	9,9,end@l
5:	cmpw	0,8,9			/* edata & end are >= word aligned */
	bge	6f
	stw	0,0(8)
	addi	8,8,4
	b	5b

6:
	/* prepare stack at +1MB from _start, 16-byte aligned */
	lis	1,_start@ha
	addi	1,1,_start@l
	addis	1,1,0x100000@ha
	li	10,-16
	and	1,1,10
	stw	0,0(1)

	bl	brdsetup
#ifdef DEBUG
	bl	init_vectors
#endif
	mr	3,30
	mr	4,31
	mr	5,28
	mr	6,29
	bl	main

hang:	b	hang
	/* NOTREACHED */

dbat_sanity_check:
	andi.	0,3,BAT_Vs
	beq	2f
	andi.	0,4,BAT_I|BAT_PP_RW
	cmpwi	0,0,BAT_I|BAT_PP_RW
	bnelr
	rlwinm	0,3,15,4,14
	andis.	3,3,0xfffe0000@ha	/* BAT_EPI */
	andis.	4,4,BAT_RPN@ha
	cmplw	0,3,4
	bnelr
	add	4,4,0
	oris	4,4,0x0001ffff@ha
	ori	4,4,0x0001ffff@l
	cmplw	0,3,5
	bgt	1f
	cmplw	0,5,4
	bgt	1f
	li	5,0
1:	cmplw	0,3,6
	bgt	2f
	cmplw	0,6,4
	bgt	2f
	li	6,0
2:	cmplw	0,5,6
	blr

/*
 * run(startsym, endsym, howto, bootinfo, entry)
 */
	.globl	run
run:
	mtctr	7			/* hat trick jump to entry point */
	bctr

/*
 * newaltboot(argc, argv, altboot_base, altboot_len)
 * To be executed in a safe memory region. Copies the new altboot from
 * altboot_base to 0x1000000 and starts it there.
 */
	.globl	newaltboot
newaltboot:
	lis	7,0x1000000@h
	mr	11,7
	subi	7,7,4
	subi	5,5,4
	add	12,11,6
	addi	6,6,3
	srawi	6,6,2
	mtctr	6
1:	lwzu	8,4(5)
	stwu	8,4(7)
	bdnz+	1b
	mtctr	11
	addi	12,12,31
	bl	syncicache
	bctr
syncicache:
/* r11=start, r12=end, r10=scratch */
	mr	10,11
2:	dcbst	0,10
	addi	10,10,32
	cmplw	10,12
	ble	2b
	sync
3:	icbi	0,11
	addi	11,11,32
	cmplw	11,12
	ble	3b
	sync
	isync
	blr
	.globl	newaltboot_end
newaltboot_end:


/* 8-bit i/o access */
	.globl	out8
out8:
	stb	4,0(3)
	eieio
	blr

	.globl	in8
in8:
	lbz	3,0(3)
	eieio
	blr

/*
 * reverse endian access to mimic outw/outl/inw/inl
 */
	.globl	out16rb
	.globl	iohtole16
out16rb:
iohtole16:
	sthbrx	4,0,3
	eieio
	blr 

	.globl	out32rb
	.globl	iohtole32
out32rb:
iohtole32:
	stwbrx	4,0,3
	eieio
	blr

	.globl	in16rb
	.globl	iole16toh
in16rb:
iole16toh:
	lhbrx	3,0,3
	eieio
	blr

	.globl	in32rb
	.globl	iole32toh
in32rb:
iole32toh:
	lwbrx	3,0,3
	eieio
	blr

#ifdef DEBUG
	/*
	 * Call an exception handler, which prints out all information
	 * about the type of exception, cpu registers, stack frame
	 * backtrace, etc.
	 * Use a new stack at 0x2000 and make room for 32 GPRs, and 15
	 * special registers. The layout will be:
	 * 0x00: link area
	 * 0x10: R0
	 * ...
	 * 0x8c: R31
	 * 0x90: CR, XER, LR, CTR
	 * 0xa0: SRR0, SRR1, DAR, DSISR
	 * 0xb0: DMISS, DCMP, HASH1, HASH2
	 * 0xc0: IMISS, ICMP, RPA
	 *
	 */
	.globl	trap
trap:
	mtsprg1	1
	mfmsr	1
	andis.	1,1,PSL_TGPR@h
	beq	1f
	andi.	1,1,0xffff		/* make sure TGPR is disabled */
	mtmsr	1
	isync
	mtsprg1	1			/* and save the real r1 again */
1:	li	1,0x2000-16-(32*4+15*4)
	stmw	2,24(1)			/* save r2..r31 */
	stw	0,16(1)			/* save r0 */
	mfsprg1	3
	stw	3,20(1)			/* and finally r1 */
	mfcr	3
	stw	3,0x90(1)
	mfxer	3
	stw	3,0x94(1)
	mflr	3
	stw	3,0x98(1)
	mfctr	3
	stw	3,0x9c(1)
	mfsrr0	3
	stw	3,0xa0(1)
	mfsrr1	3
	stw	3,0xa4(1)
	mfdar	3
	stw	3,0xa8(1)
	mfdsisr	3
	stw	3,0xac(1)
	mfspr	3,976
	stw	3,0xb0(1)
	mfspr	3,977
	stw	3,0xb4(1)
	mfspr	3,978
	stw	3,0xb8(1)
	mfspr	3,979
	stw	3,0xbc(1)
	mfspr	3,980
	stw	3,0xc0(1)
	mfspr	3,981
	stw	3,0xc4(1)
	mfspr	3,982
	stw	3,0xc8(1)
	bl	call_handler
call_handler:
	lis	11,exception_handler@ha
	addi	11,11,exception_handler@l
	mtsrr0	11
	li	0,PSL_DR|PSL_IR
	mtsrr1	0
	mflr	3
	subi	3,3,call_handler-trap
	addi	4,1,16
	rfi
	.globl	trap_end
trap_end:
#endif

	.data
#define	xBATL(pa, wimg, pp)						\
	((pa) | (wimg) | (pp))
#define	xBATU(va, len, v)						\
	((va) | ((len) & BAT_BL) | ((v) & BAT_V))
BAT123:
	.long xBATL(0x80000000, BAT_I|BAT_G, BAT_PP_RW)
	.long xBATU(0x80000000, BAT_BL_256M, BAT_Vs)
	.long xBATL(0xfc000000, BAT_I|BAT_G, BAT_PP_RW)
	.long xBATU(0xfc000000, BAT_BL_64M, BAT_Vs)
	.long xBATL(0x70000000, BAT_I|BAT_G, BAT_PP_RW)
	.long xBATU(0x70000000, BAT_BL_128K, BAT_Vs)
