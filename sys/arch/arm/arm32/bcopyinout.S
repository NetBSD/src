/*	$NetBSD: bcopyinout.S,v 1.4 2002/03/03 11:22:59 chris Exp $	*/

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

#include "assym.h"

#include <machine/asm.h>
#include <sys/errno.h>

	.text
	.align	0
Lcurpcb:
	.word	_C_LABEL(curpcb)

Lvm_min_address:
	.word	VM_MIN_ADDRESS
Lvm_maxuser_address:
	.word	VM_MAXUSER_ADDRESS
Lvm_maxkern_address:
	.word	_C_LABEL(pmap_curmaxkvaddr)

ENTRY(kcopy)
	b	do_copyinout

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
	ldr	ip, [r3]
	cmp	r1, ip
	bcs	Lbadaddress

	/* Quick exit if length is zero */	
	teq	r2, #0
	moveq	r0, #0
	moveq	pc, lr

	/* Do the actual copy */
	b	do_copyinout

/*
 * r0 = kernel space address
 * r1 = user space address
 * r2 = length
 *
 * Copies bytes from user space to kernel space
 */

Lpgbase:
	.long	PROCESS_PAGE_TBLS_BASE
	
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
	ldr	ip, [r3]
	cmp	r0, ip
	bcs	Lbadaddress

	/* Quick exit if length is zero */	
	teq	r2, #0
	moveq	r0, #0
	moveq	pc, lr

	/* Check the page protection for copy-on-write */
	stmfd	sp!, {r4-r7}			/* stack is 8 byte aligned */
	ldr	r4, Lcurpcb
	ldr	r4, [r4]

	ldr	r5, [r4, #PCB_ONFAULT]
	add	r3, pc, #do_cowfault - . - 8
	str	r3, [r4, #PCB_ONFAULT]

	ldr	r3, Lpgbase
	add	r6, r3, r1, lsr #(PGSHIFT-2)
	bic	r6, r6, #3			/* beginning PTE */

	mov	r7, r1, lsl #(32-PGSHIFT)
	add	r7, r2, r7, lsr #(32-PGSHIFT)
	sub	r7, r7, #1
	mov	r7, r7, lsr #(PGSHIFT)		/* number of pages -1 */

do_ptecheck:
	ldr	r3, [r6]			/* grab PTE */
	tst	r3, #1				/* check writable bit */
	beq	do_cowfault			/* if clear, do COW fault */
do_ptecheck_next:
	add	r6, r6, #4
	subs	r7, r7, #1
	bpl	do_ptecheck
	str	r5, [r4, #PCB_ONFAULT]
	ldmfd	sp!, {r4-r7}			/* stack is 8 byte aligned */

	/* Do the actual copy */
	b	do_copyinout

do_cowfault:
	stmfd	sp!, {r0-r2, lr}		/* stack is 8 byte aligned */
	ldr	r3, Lpgbase
	sub	r0, r6, r3
	mov	r0, r0, lsl #(PGSHIFT-2)	/* calculate VA of page */
	bl	_C_LABEL(cowfault)
	mov	r3, r0
	ldmfd	sp!, {r0-r2, lr}		/* stack is 8 byte aligned */
	teq	r3, #0				/* check for error return */
	beq	do_ptecheck_next
	mov	r0, r3
	str	r5, [r4, #PCB_ONFAULT]
	ldmfd	sp!, {r4-r7}			/* stack is 8 byte aligned */
	mov	pc, lr

do_copyinout:
	stmfd	sp!, {r4, r5}			/* stack is 8 byte aligned */
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
	 * Otherwise copy it 32 bits at a time.
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
	ldmfd	sp!, {r4, r5}			/* stack is 8 byte aligned */
	mov	pc, lr

/* A fault occurred during the copy */
Lcopyinoutfault:
	str	r5, [r4, #PCB_ONFAULT]
	ldmfd	sp!, {r4, r5}			/* stack is 8 byte aligned */

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
	b	_C_LABEL(panic)

Lcopyinouttext:
	.asciz	"No valid PCB during copyinout() addr1=%08x addr2=%08x\n"
	.align	0
#endif	/* DIAGNOSTIC */
