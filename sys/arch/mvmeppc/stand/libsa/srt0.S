/*	$NetBSD: srt0.S,v 1.1 2002/02/27 21:02:27 scw Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Steve C. Woodford.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <machine/asm.h>
#include <machine/psl.h>
#include <powerpc/spr.h>

#define	STACK_SIZE	8192

/*
 * The main entry point when loaded by PPC-Bug.
 *
 * There are two possible entry conditions here:
 *
 *   1) We were booted in `PReP' mode, either from disk or the network.
 *      In this case, we have no control over the load address so we
 *      have to relocate ourselves to the appropriate place.
 *      The firmware passes us the following registers:
 *
 *        r1  -> Temporary stack
 *        r3  -> Residual Data
 *        r4  -> The address we were loaded to
 *        r5  -> Zero
 *
 *   2) We were booted over the network in Non-PReP mode. In this case,
 *      the load address is usually set using PPC-Bug's "niot" command,
 *      but we won't depend on it so relocation may be required. The
 *      firmware passes us the following registers:
 *
 *        r1  -> Temporary stack
 *        r3  -> CLUN of the network device we booted from
 *        r4  -> DLUN of the network device we booted from
 *        r5  -> Non-zero
 *        r6  -> Base address of network device
 *        r7  -> Execution address of loaded program
 *        r8  -> Address of IP-address data structure
 *        r9  -> Pointer to start of filename string
 *        r10 -> Pointer to end+1 of filename string
 *        r11 -> Pointer to start of argument string
 *        r12 -> Pointer to end+1 of argument string
 *
 * The obvious way to distinguish between the two boot modes is by
 * checking the value of r5.
 */
ENTRY(_start)
	bl	1f
1:	xor	r0,r0,r0

	/* First, switch off Instruction and Data caches. */
	mfspr	r13,SPR_HID0
	LDCONST(r14, HID0_DCE|HID0_ICE)
	andc	r13,r13,r14
	sync
	mtspr	SPR_HID0,r13


	/*
	 * All registers now available. Let's see if we need to relocate
	 */
	LDCONST(r13,_C_LABEL(_start))	/* Where we'd like to be */
	LDCONST(r14,_C_LABEL(edata))	/* End of data section */
	LDCONST(r15,0x3)
	add	r14,r14,r15
	andc	r14,r14,r15		/* Rounded up to the nearest 32-bits */
	sub	r15,r14,r13		/* Our size, in bytes */
	mflr	r16			/* Get address we were loaded to */
	subi	r16,r16,0x4		/* Correct for branch */
	cmp	cr0,r13,r16		/* Do we need to relocate? */
	beq	_ASM_LABEL(clrbss)	/* No relocation necessary */
	li	r17,0x4
	bgt	1f			/* Relocate using forward copy? */

	/* Nope. Need to copy in reverse in case of overlap */
	mr	r13,r14			/* dest -> end */
	add	r16,r16,r15		/* src + size */
	subi	r17,r17,0x8		/* Increment is -4 */

	/*
	 * Do the relocation
	 *  r13  -> dest
	 *  r15  -> number of bytes
	 *  r16  -> src
	 *  r17  -> Increment (+4 or -4)
	 */
1:	srwi	r15,r15,0x2		/* Convert length to 32-bit words */
	mtctr	r15			/* Save in counter register */

2:	lwz	r15,0(r16)
	stw	r15,0(r13)
	add	r16,r16,r17
	add	r13,r13,r17
	bdnz	2b

	/* Now do an absolute jump to the relocated code */
	LDCONST(r13,_ASM_LABEL(clrbss))
	mtlr	r13
	blr

ASENTRY(clrbss)
	LDCONST(r13,_C_LABEL(edata))	/* End of the data section */
	LDCONST(r14,_C_LABEL(end))	/* End of BSS */
	LDCONST(r15,0x3)
	add	r14,r14,r15
	andc	r14,r14,r15		/* Round-up end of BSS to 32-bits */
	sub	r15,r14,r13		/* r15 == length of BSS */
	srwi	r15,r15,0x2
	mtctr	r15			/* CTR == # of 32-bit words in BSS */
1:	stw	r0,0(r13)		/* Clear BSS */
	addi	r13,r13,4
	bdnz	1b

	/* Fix up our own stack */
	LDCONST(r1,stack)
	addi	r1,r1,STACK_SIZE-0x10
	LDCONST(r13,0x0f)
	andc	r1,r1,r13

	/*
	 * Copy the arguments passed in from Bug into bug_bootinfo
	 *
	 * See bugsyscalls.h for details.
	 */
	LDCONST(r13,_C_LABEL(bug_bootinfo))
	stw	r5,0x00(r13)
	stw	r3,0x04(r13)
	stw	r4,0x08(r13)
	stw	r6,0x0c(r13)
	stw	r7,0x10(r13)
	stw	r8,0x14(r13)
	stw	r9,0x18(r13)
	stw	r10,0x1c(r13)
	stw	r11,0x20(r13)
	stw	r12,0x24(r13)

	mr	r3,r13
	bl	_C_LABEL(main)		/* void main(void) */
	/* FALLTHROUGH */

/*
 * Return to the debugger, either because main() returned or via panic().
 */
ENTRY(_rtt)
	addi	r10,0,0x0063
	sc
1:	nop
	b	1b

	/*
	 * C code runs on this stack.
	 */
	.comm	stack,STACK_SIZE,4
	.comm	errno,4,4
	.comm	debug,4,4
