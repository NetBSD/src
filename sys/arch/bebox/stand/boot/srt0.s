/*	$NetBSD: srt0.s,v 1.3 1999/06/28 01:20:45 sakamoto Exp $	*/

/*-
 * Copyright (C) 1995-1997 Gary Thomas (gdt@linuxppc.org)
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
 *      This product includes software developed by Gary Thomas.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define	MSR_EE	0x8000

	.text

/*
 * This code is loaded by the ROM loader at location 0x3100.
 * Move it to high memory so that it can load the kernel at 0x0000.
 */
#define PEF_LOAD_ADDRESS	0x3100

	.globl	_start
_start:
	mfmsr	3		/* Turn off interrupts */
	li	4,0x3002
	li	4,0
	ori	4,4,MSR_EE
	andc	3,3,4
	mtmsr	3
	bl	whichCPU
	cmpi	0,3,0
	bne	90f
/* CPU 0 runs here */
/* Relocate code to final resting spot */
	li	3,PEF_LOAD_ADDRESS
	lis	4,_start@h
	ori	4,4,_start@l
	lis	5,edata@h
	ori	5,5,edata@l
	addi	5,5,3			/* Round up - just in case */
	sub	5,5,4			/* Compute # longwords to move */
	srwi	5,5,2
	mtctr	5
	subi	3,3,4			/* Set up for loop */
	subi	4,4,4
00:	lwzu	5,4(3)
	stwu	5,4(4)
	bdnz	00b
	lis	3,start_ldr@h
	ori	3,3,start_ldr@l
	mtlr	3			/* Easiest way to do an absolute jump */
	blr
start_ldr:	
/* Clear all of BSS */
	lis	3,edata@h
	ori	3,3,edata@l
	lis	4,end@h
	ori	4,4,end@l
	subi	3,3,4
	subi	4,4,4
	li	0,0
50:	stwu	0,4(3)
	cmp	0,3,4
	bne	50b
90:
	bl	main
hang:
	b	hang	

/*
 * end address
 */
	.globl endaddr
endaddr:
	lis	3,end@h
	ori	3,3,end@l
	lis	4,_start@h
	ori	4,4,_start@l
	sub	3,3,4
	blr

/*
 * Execute
 * run(startsym, endsym, args, bootinfo, entry)
 */
 	.globl	run
run:
	mtctr	7			/* Entry point */
	bctr
