/* $NetBSD: copystr.S,v 1.5 1996/08/29 22:18:09 mark Exp $ */

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
 * RiscBSD kernel project
 *
 * copystr.S
 *
 * optimised and fault protected copystr functions
 *
 * Created      : 16/05/95
 */

#include "assym.h"
#include <sys/errno.h>

sp	.req	r13
lr	.req	r14
pc	.req	r15

	.text
Lcurpcb:
	.word	_curpcb

Lcurproc:
	.word	_curproc

/*
 * r0 - from
 * r1 - to
 * r2 - maxlens
 * r3 - lencopied
 *
 * Copy string from r0 to r1
 */

	.global _copystr

_copystr:
	stmfd	sp!, {r4-r5}
	teq	r2, #0x00000000
	moveq	r4, #0x00000000
	moveq	r5, #0x00000000
	beq	copystrexit

	mov 	r5, #0x00000000

copystr_loop:
	ldrb	r4, [r0], #0x0001
	strb	r4, [r1], #0x0001
	add	r5, r5, #0x00000001
	teq	r4, #0x00000000
	teqne	r5, r2
	bne	copystr_loop

copystrexit:
	teq	r3, #0x00000000
	strne	r5, [r3]

	teq	r4, #0x00000000
	moveq	r0, #0x00000000
	movne	r0, #ENAMETOOLONG

	ldmfd	sp!, {r4-r5}
	mov	pc, lr


/*
 * r0 - user space address
 * r1 - kernel space address
 * r2 - maxlens
 * r3 - lencopied
 *
 * Copy string from user space to kernel space
 */

	.global _copyinstr

_copyinstr:
	stmfd	sp!, {r4-r6}
	teq	r2, #0x00000000
	moveq	r5, #0x00000000
	moveq	r6, #0x00000000
	beq	copyinstrexit

/* Validate addresses */

	ldr	r4, Lvm_min_address
	cmp	r0, r4
	bcc	Lbadaddress
	ldr	r4, Lvm_maxuser_address
	cmp	r0, r4
	bcs	Lbadaddress
	cmp	r1, r4
	bcc	Lbadaddress
	ldr	r4, Lvm_maxkern_address
	cmp	r1, r4
	bcs	Lbadaddress

	ldr	r4, Lcurpcb
	ldr	r4, [r4]

#ifdef DIAGNOSTIC
	teq	r4, #0x00000000
	beq	copystrpcbfault
#endif

	add	r5, pc, #copystrfault - . - 8
	str	r5, [r4, #PCB_ONFAULT]
	mov 	r6, #0x00000000

copyinstr_loop:
	ldrb	r5, [r0], #0x0001
	strb	r5, [r1], #0x0001
	add	r6, r6, #0x00000001
	teq	r5, #0x00000000
	teqne	r6, r2
	bne	copyinstr_loop

copyinstrexit:
	teq	r3, #0x00000000
	strne	r6, [r3]

	teq	r5, #0x00000000
	moveq	r0, #0x00000000
	movne	r0, #ENAMETOOLONG

	mov	r6, #0x00000000
	str	r6, [r4, #PCB_ONFAULT]
	ldmfd	sp!, {r4-r6}
	mov	pc, lr

/*
 * r0 - kernel space address
 * r1 - user space address
 * r2 - maxlens
 * r3 - lencopied
 *
 * Copy string from kernel space to user space
 */

	.global _copyoutstr

_copyoutstr:
	stmfd	sp!, {r4-r6}
	teq	r2, #0x00000000
	moveq	r5, #0x00000000
	moveq	r6, #0x00000000
	beq	copyinstrexit

/* Validate addresses */

	ldr	r4, Lvm_min_address
	cmp	r1, r4
	bcc	Lbadaddress
	ldr	r4, Lvm_maxuser_address
	cmp	r1, r4
	bcs	Lbadaddress
	cmp	r0, r4
	bcc	Lbadaddress
	ldr	r4, Lvm_maxkern_address
	cmp	r0, r4
	bcs	Lbadaddress

	ldr	r4, Lcurpcb
	ldr	r4, [r4]

#ifdef DIAGNOSTIC
	teq	r4, #0x00000000
	beq	copystrpcbfault
#endif

	add	r5, pc, #copystrfault - . - 8
	str	r5, [r4, #PCB_ONFAULT]
	mov 	r6, #0x00000000

copyoutstr_loop:
	ldrb	r5, [r0], #0x0001
	strb	r5, [r1], #0x0001
	add	r6, r6, #0x00000001
	teq	r5, #0x00000000
	teqne	r6, r2
	bne	copyoutstr_loop

copyoutstrexit:
	teq	r3, #0x00000000
	strne	r6, [r3]

	teq	r5, #0x00000000
	moveq	r0, #0x00000000
	movne	r0, #ENAMETOOLONG

	mov	r6, #0x00000000
	str	r6, [r4, #PCB_ONFAULT]
	ldmfd	sp!, {r4-r6}
	mov	pc, lr

Lvm_min_address:
	.word	VM_MIN_ADDRESS
Lvm_maxuser_address:
	.word	VM_MAXUSER_ADDRESS
Lvm_maxkern_address:
	.word	VM_MAXKERN_ADDRESS

copystrfault:
	mov	r0, #0x00000000
	str	r0, [r4, #PCB_ONFAULT]
	mov	r0, #EFAULT
	ldmfd	sp!, {r4-r6}
	mov	pc, lr

Lbadaddress:
	mov	r0, #EFAULT
	ldmfd	sp!, {r4-r6}
	mov	pc, lr

#ifdef DIAGNOSTIC
copystrpcbfault:
	mov	r2, r1
	mov	r1, r0
	ldr	r2, Lcurproc
	ldr	r2, [r2]
	add	r0, pc, #copystrpcbfaulttext - . - 8
	b	_panic

copystrpcbfaulttext:
	.asciz	"Yikes - no valid PCB during copystr() addr1=%08x addr2=%08x curproc=%08x\n"
	.align	0
#endif
