/*	$NetBSD: copystr.S,v 1.3 2002/03/03 11:22:59 chris Exp $	*/

/*
 * Copyright (c) 1995 Mark Brinicombe.
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
 *	This product includes software developed by Mark Brinicombe.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * copystr.S
 *
 * optimised and fault protected copystr functions
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

/*
 * r0 - from
 * r1 - to
 * r2 - maxlens
 * r3 - lencopied
 *
 * Copy string from r0 to r1
 */

ENTRY(copystr)
	stmfd	sp!, {r4-r5}			/* stack is 8 byte aligned */
	teq	r2, #0x00000000
	mov	r5, #0x00000000
	moveq	r0, #ENAMETOOLONG
	beq	Lcopystrexit

Lcopystr_loop:
	ldrb	r4, [r0], #0x0001
	strb	r4, [r1], #0x0001
	add	r5, r5, #0x00000001
	teq	r4, #0x00000000
	teqne	r5, r2
	bne	Lcopystr_loop

	teq	r4, #0x00000000
	moveq	r0, #0x00000000
	movne	r0, #ENAMETOOLONG

Lcopystrexit:
	teq	r3, #0x00000000
	strne	r5, [r3]

	ldmfd	sp!, {r4-r5}			/* stack is 8 byte aligned */
	mov	pc, lr


/*
 * r0 - user space address
 * r1 - kernel space address
 * r2 - maxlens
 * r3 - lencopied
 *
 * Copy string from user space to kernel space
 */

ENTRY(copyinstr)
	stmfd	sp!, {r4-r6}

	/* Validate user and kernel addresses */
	ldr	r4, Lvm_min_address
	cmp	r0, r4
	bcc	Lbadaddress
	ldr	r4, Lvm_maxuser_address
	cmp	r0, r4
	bcs	Lbadaddress
	cmp	r1, r4
	bcc	Lbadaddress
	ldr	r4, Lvm_maxkern_address
	ldr	r5, [r4]
	cmp	r1, r5
	bcs	Lbadaddress

	/* Do the actual copy */
	b	do_copyinoutstr

/*
 * r0 - kernel space address
 * r1 - user space address
 * r2 - maxlens
 * r3 - lencopied
 *
 * Copy string from kernel space to user space
 */

ENTRY(copyoutstr)
	stmfd	sp!, {r4-r6}		/* stack isn't 8 byte aligned */

	/* Validate user and kernel addresses */
	ldr	r4, Lvm_min_address
	cmp	r1, r4
	bcc	Lbadaddress
	ldr	r4, Lvm_maxuser_address
	cmp	r1, r4
	bcs	Lbadaddress
	cmp	r0, r4
	bcc	Lbadaddress
	ldr	r4, Lvm_maxkern_address
	ldr	r5, [r4]
	cmp	r0, r5
	bcs	Lbadaddress

	/* Do the actual copy */
do_copyinoutstr:
	teq	r2, #0x00000000
	mov	r6, #0x00000000
	moveq	r0, #ENAMETOOLONG
	beq	Lcopyinoutstrexit

	ldr	r4, Lcurpcb
	ldr	r4, [r4]

#ifdef DIAGNOSTIC
	teq	r4, #0x00000000
	beq	Lcopystrpcbfault
#endif

	add	r5, pc, #Lcopystrfault - . - 8
	str	r5, [r4, #PCB_ONFAULT]

Lcopyinoutstr_loop:
	ldrb	r5, [r0], #0x0001
	strb	r5, [r1], #0x0001
	add	r6, r6, #0x00000001
	teq	r5, #0x00000000
	teqne	r6, r2
	bne	Lcopyinoutstr_loop

	mov	r0, #0x00000000
	str	r0, [r4, #PCB_ONFAULT]

	teq	r5, #0x00000000
	moveq	r0, #0x00000000
	movne	r0, #ENAMETOOLONG

Lcopyinoutstrexit:
	teq	r3, #0x00000000
	strne	r6, [r3]

	ldmfd	sp!, {r4-r6}			/* stack is 8 byte aligned */
	mov	pc, lr

/* A fault occurred during the copy */
Lcopystrfault:
	mov	r0, #0x00000000
	str	r0, [r4, #PCB_ONFAULT]

	/* FALLTHROUGH */

/* Source or Destination address was bad so fail */
Lbadaddress:
	ldmfd	sp!, {r4-r6}			/* stack is 8 byte aligned */
	mov	r0, #EFAULT
	mov	pc, lr

#ifdef DIAGNOSTIC
Lcopystrpcbfault:
	mov	r2, r1
	mov	r1, r0
	add	r0, pc, #Lcopystrpcbfaulttext - . - 8
	bic	sp, sp, #7			/* align stack to 8 bytes */
	b	_C_LABEL(panic)

Lcopystrpcbfaulttext:
	.asciz	"No valid PCB during copyinoutstr() addr1=%08x addr2=%08x\n"
	.align	0
#endif
