/*	$NetBSD: bcopyinout.S,v 1.15 1998/11/26 19:34:38 mycroft Exp $	*/

/*
 * Copyright (c) 1995-1998 Mark Brinicombe.
 * Copyright (c) 1995 Brini.
 * All rights reserved.
 *
 * This code is derived from software written for Brini by Mark Brinicombe
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
 *	This product includes software developed by Mark Brinicombe.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
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
 *
 * bcopyinout.S
 *
 * optimized and fault protected byte copy functions
 *
 * Created      : 16/05/95
 */

#include "opt_uvm.h"
#include "assym.h"
#include <machine/asm.h>
#include <sys/errno.h>

	.text
	.align	0
Lcurpcb:
	.word	_curpcb

Lvm_min_address:
	.word	VM_MIN_ADDRESS
Lvm_maxuser_address:
	.word	VM_MAXUSER_ADDRESS
Lvm_maxkern_address:
	.word	VM_MAXKERN_ADDRESS

#if defined(UVM)
ENTRY(kcopy)
	b	do_copyinout
#endif

/*
 * r0 = user space address
 * r1 = kernel space address
 * r2 = length
 *
 * Copies bytes from user space to kernel space
 */

ENTRY(copyin)
	/* Validate user and kernel addresses */
	ldr	r3, Lvm_min_address
	cmp	r0, r3
	bcc	Lbadaddress
	ldr	r3, Lvm_maxuser_address
	cmp	r0, r3
	bcs	Lbadaddress
	cmp	r1, r3
	bcc	Lbadaddress
	ldr	r3, Lvm_maxkern_address
	cmp	r1, r3
	bcs	Lbadaddress

	/* Do the actual copy */
	b	do_copyinout

/*
 * r0 = kernel space address
 * r1 = user space address
 * r2 = length
 *
 * Copies bytes from user space to kernel space
 */

ENTRY(copyout)
	/* Validate user and kernel addresses */
	ldr	r3, Lvm_min_address
	cmp	r1, r3
	bcc	Lbadaddress
	ldr	r3, Lvm_maxuser_address
	cmp	r1, r3
	bcs	Lbadaddress
	cmp	r0, r3
	bcc	Lbadaddress
	ldr	r3, Lvm_maxkern_address
	cmp	r0, r3
	bcs	Lbadaddress

	/* Do the actual copy */

do_copyinout:
	/* Quick exit if length is zero */	
	teq	r2, #0x00000000
	moveq	r0, #0x00000000
	moveq	pc, lr

	stmfd	sp!, {r4, r5}
	ldr	r4, Lcurpcb
	ldr	r4, [r4]

#ifdef DIAGNOSTIC
	teq	r4, #0x00000000
	beq	Lcopyinoutpcbfault
#endif	/* DIAGNOSTIC */

	ldr	r5, [r4, #PCB_ONFAULT]
	add	r3, pc, #Lcopyinoutfault - . - 8
	str	r3, [r4, #PCB_ONFAULT]

	/*
	 * If less than 4 bytes or the source or destination address is
	 * not 32 bit aligned then copy it slowly, byte at a time.
	 * Otherwise copy it 32 bites at a time.
	 */
	subs	r2, r2, #4
	bmi	Lslow_copyinout
	tst	r0, #3
	tsteq	r1, #3
	bne	Lslow_copyinout

Lcopyinout_loop:
	ldr	r3, [r0], #0x0004
	str	r3, [r1], #0x0004
	subs	r2, r2, #0x00000004
	bpl	Lcopyinout_loop

	tst	r2, #3
	beq	Lcopyinout_exit

Lslow_copyinout:
	add	r2, r2, #4

Lslow_copyinout_loop:
	ldrb	r3, [r0], #0x0001
	strb	r3, [r1], #0x0001
	subs	r2, r2, #0x00000001
	bne	Lslow_copyinout_loop

Lcopyinout_exit:
	mov	r0, #0x00000000
	str	r5, [r4, #PCB_ONFAULT]
	ldmfd	sp!, {r4, r5}
	mov	pc, lr

/* A fault occurred during the copy */
Lcopyinoutfault:
	str	r5, [r4, #PCB_ONFAULT]
	ldmfd	sp!, {r4, r5}

	/* FALLTHROUGH */

/* Source or Destination address was bad so fail */
Lbadaddress:
	/* Don't return EFAULT if legnth was zero */
	teq	r2, #0x00000000
	moveq	r0, #0x00000000
	movne	r0, #EFAULT
	mov	pc, lr

#ifdef DIAGNOSTIC
Lcopyinoutpcbfault:
	mov	r2, r1
	mov	r1, r0
	add	r0, pc, #Lcopyinouttext - . - 8
	b	_panic

Lcopyinouttext:
	.asciz	"No valid PCB during copyinout() addr1=%08x addr2=%08x\n"
	.align	0
#endif	/* DIAGNOSTIC */
