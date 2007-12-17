/*	$NetBSD: srt0.s,v 1.1.2.2 2007/12/17 19:09:55 garbled Exp $	*/

/*
 * Copyright (C) 1996-1999 Cort Dougan (cort@fsmlasb.com).
 * Copyright (C) 1996-1999 Gary Thomas (gdt@osf.org).
 * Copyright (C) 1996-1999 Paul Mackeras (paulus@linuxcare.com).
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

#define MSR_IP		(1<<6)

#define HID0_DCI	(1<<10)
#define HID0_ICFI	(1<<11)
#define	HID0_DCE	(1<<14)
#define HID0_ICE	(1<<15)

/* when we start, the IPLCB pointer is in r3, and the extended one is in r4 */

	.text

	.globl	_start
_start:
	bl	start
start:
	mr	11,3		/* save the IPLCB pointer in r11 */
	mr	12,4		/* save the extended IPLCB in r12 */
	li	3,MSR_IP	/* Establish default MSR value */
	mtmsr	3
	isync

	mflr	7
	bl	flush_icache
	mfspr	3,1008
	lis	4,~(HID0_ICE|HID0_DCE)@h
	ori	4,4,~(HID0_ICE|HID0_DCE)@l
	andc	3,3,4
	mtspr	1008,3
	mtlr	7

/* We aren't passed our loadaddr, so leave it alone */
#if 0
/*
 * check if we need to relocate ourselves to the link addr or were we
 * loaded there to begin with -- Cort
 */
	lis	4,_start@h
	ori	4,4,_start@l
	mflr	3
	subi	3,3,4		/* we get the nip, not the ip of the branch */
	mr	8,3
	cmpw	3,4
	bne	relocate
	b	start_ldr

/* 
 * no matter where we're loaded, move ourselves to -Ttext address
 */
relocate:
	lis	4,_start@h
	ori	4,4,_start@l
	lis	5,end@h
	ori	5,5,end@l
	addi	5,5,3		/* Round up - just in case */
	sub	5,5,4		/* Compute # longwords to move */
	srwi	5,5,2
	mtctr	5
	subi	3,3,4		/* Set up for loop */
	subi	4,4,4
2:
	lwzu	5,4(3)
	stwu	5,4(4)
	bdnz	2b
  	lis	3,start_ldr@h
	ori	3,3,start_ldr@l
	mtlr	3		/* Easiest way to do an absolute jump */
	blr
#endif

start_ldr:
	mr	9,1		/* Save old stack pointer */
	lis	1,.stack@h
	ori	1,1,.stack@l
	addi	1,1,4096
	li	2,0x000F
	andc	1,1,2
	mr	3,11		/* arg1: IPLCB pointer */
	mr	4,12		/* arg2: IPLCB extended block pointer */
	bl	boot
hang:
	b	hang

/*
 * Execute
 * run(startsym, endsym, args, bootinfo, entry)
 */
	.globl  run
run:
	mtctr   7                       /* Entry point */
	bctr

/*
 * Flush instruction cache
 */
	.globl flush_icache
flush_icache:
	mflr	5
	bl	flush_dcache
	mfspr	4,1008
	li	4,0
	ori	4,4,HID0_ICE|HID0_ICFI
	or	3,3,4
	mtspr	1008,3
	andc	3,3,4
	ori	3,3,HID0_ICE
	mtspr	1008,3
	mtlr	5
	blr

/*
 * Flush data cache
 */
	.globl flush_dcache
flush_dcache:
	lis	3,0x1000@h
	ori	3,3,0x1000@l
	li	4,1024
	mtctr	4
1:
	lwz	4,0(3)
	addi	3,3,32
	bdnz	1b
	blr

#if 0
/*
 * write LEDS
 */

	.globl led
led:
	mfmsr	5	/* save MSR to r5 */
	lis	7,0xffff
	ori	7,7,~(MSR_DR)
	and	6,5,7
	mtmsr	6
	isync
	
#endif
/*
 * local stack
 */
	.comm	.stack,8192,4
