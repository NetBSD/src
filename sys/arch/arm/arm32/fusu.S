/*	$NetBSD: fusu.S,v 1.1.2.2 2001/08/03 04:10:58 lukem Exp $	*/

/*
 * Copyright (c) 1996-1998 Mark Brinicombe.
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
 *	This product includes software developed by Mark Brinicombe
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
 */

#include "assym.h"
#include <machine/asm.h>

/*
 * Reads a word but traps failed faults
 *
 * This is similar to fuword() but rather than returnin
 * the word, the second argument points to the location
 * to store the word in. The function returns 0 for
 * suceess and -1 for failure.
 * This routine is used to fetch a word from user space
 * when the value being fetched could be -1. By separating
 * error flagging from the return value this is achieved.
 * This function is used by the prefetch abort handler
 *
 *  r0 - address to read
 *  r1 - address to store result
 */

ENTRY(fetchuserword)
	ldr	r2, Lvm_min_address
	cmp	r0, r2
	bcc	fusuaddrfault
	ldr	r2, Lvm_maxuser_address
	cmp	r0, r2
	bcs	fusuaddrfault

	ldr	r2, Lcurpcb
	ldr	r2, [r2]

#ifdef DIAGNOSTIC
	teq	r2, #0x00000000
	beq	fusupcbfault
#endif

	add	r3, pc, #fusufault - . - 8
	str	r3, [r2, #PCB_ONFAULT]

	ldr	r0, [r0]
	str	r0, [r1]

	mov	r0, #0x00000000
	str	r0, [r2, #PCB_ONFAULT]
	mov	pc, lr

Lcurpcb:
	.word	_C_LABEL(curpcb)

Lvm_min_address:
	.word	VM_MIN_ADDRESS
Lvm_maxuser_address:
	.word	VM_MAXUSER_ADDRESS
Lvm_maxkern_address:
	.word	VM_MAXKERN_ADDRESS


/*
 * fuword(caddr_t uaddr);
 * Fetch an int from the user's address space.
 */

ENTRY(fuword)
	ldr	r2, Lvm_min_address
	cmp	r0, r2
	bcc	fusuaddrfault
	ldr	r2, Lvm_maxuser_address
	cmp	r0, r2
	bcs	fusuaddrfault

	ldr	r2, Lcurpcb
	ldr	r2, [r2]

#ifdef DIAGNOSTIC
	teq	r2, #0x00000000
	beq	fusupcbfault
#endif

	add	r1, pc, #fusufault - . - 8
	str	r1, [r2, #PCB_ONFAULT]

	ldr	r0, [r0]

	mov	r1, #0x00000000
	str	r1, [r2, #PCB_ONFAULT]
	mov	pc, lr

/*
 * fusword(caddr_t uaddr);
 * Fetch a short from the user's address space.
 */

ENTRY(fusword)
	ldr	r2, Lvm_min_address
	cmp	r0, r2
	bcc	fusuaddrfault
	ldr	r2, Lvm_maxuser_address
	cmp	r0, r2
	bcs	fusuaddrfault

	ldr	r2, Lcurpcb
	ldr	r2, [r2]

#ifdef DIAGNOSTIC
	teq	r2, #0x00000000
	beq	fusupcbfault
#endif

	add	r1, pc, #fusufault - . - 8
	str	r1, [r2, #PCB_ONFAULT]

	ldr	r0, [r0]
	bic	r0, r0, #0xff000000
	bic	r0, r0, #0x00ff0000

	mov	r1, #0x00000000
	str	r1, [r2, #PCB_ONFAULT]
	mov	pc, lr

/*
 * fuswintr(caddr_t uaddr);
 * Fetch a short from the user's address space.  Can be called during an
 * interrupt.
 */

ENTRY(fuswintr)
	ldr	r2, Lblock_userspace_access
	ldr	r2, [r2]
	teq	r2, #0
	mvnne	r0, #0x00000000
	movne	pc, lr

	ldr	r2, Lvm_min_address
	cmp	r0, r2
	bcc	fusuaddrfault
	ldr	r2, Lvm_maxuser_address
	cmp	r0, r2
	bcs	fusuaddrfault

	ldr	r2, Lcurpcb
	ldr	r2, [r2]

#ifdef DIAGNOSTIC
	teq	r2, #0x00000000
	beq	fusupcbfault
#endif

	add	r1, pc, #_C_LABEL(fusubailout) - . - 8
	str	r1, [r2, #PCB_ONFAULT]

	ldr	r0, [r0]
	bic	r0, r0, #0xff000000
	bic	r0, r0, #0x00ff0000

	mov	r1, #0x00000000
	str	r1, [r2, #PCB_ONFAULT]
	mov	pc, lr

Lblock_userspace_access:
	.word	_C_LABEL(block_userspace_access)

	.data
	.align	0
	.global	_C_LABEL(block_userspace_access)
_C_LABEL(block_userspace_access):
	.word	0
	.text

/*
 * fubyte(caddr_t uaddr);
 * Fetch a byte from the user's address space.
 */

ENTRY(fubyte)
	ldr	r2, Lvm_min_address
	cmp	r0, r2
	bcc	fusuaddrfault
	ldr	r2, Lvm_maxuser_address
	cmp	r0, r2
	bcs	fusuaddrfault

	ldr	r2, Lcurpcb
	ldr	r2, [r2]

#ifdef DIAGNOSTIC
	teq	r2, #0x00000000
	beq	fusupcbfault
#endif

	add	r1, pc, #fusufault - . - 8
	str	r1, [r2, #PCB_ONFAULT]

	ldrb	r0, [r0]

	mov	r1, #0x00000000
	str	r1, [r2, #PCB_ONFAULT]
	mov	pc, lr

/*
 * Handle faults from [fs]u*().  Clean up and return -1.
 */

fusufault:
	mov	r0, #0x00000000
	str	r0, [r2, #PCB_ONFAULT]
	mvn	r0, #0x00000000
	mov	pc, lr

/*
 * Handle faults from [fs]u*().  Clean up and return -1.  This differs from
 * fusufault() in that trap() will recognise it and return immediately rather
 * than trying to page fault.
 */

/* label must be global as fault.c references it */
	.global	_C_LABEL(fusubailout)
_C_LABEL(fusubailout):
	mov	r0, #0x00000000
	str	r0, [r2, #PCB_ONFAULT]
	mvn	r0, #0x00000000
	mov	pc, lr

/*
 * Handle earlier faults from [fs]u*(), due to our of range addresses.
 */

fusuaddrfault:
	mvn	r0, #0x00000000
	mov	pc, lr

#ifdef DIAGNOSTIC
/*
 * Handle earlier faults from [fs]u*(), due to no pcb
 */

fusupcbfault:
	mov	r1, r0
	add	r0, pc, #fusupcbfaulttext - . - 8
	b	_C_LABEL(panic)

fusupcbfaulttext:
	.asciz	"Yikes - no valid PCB during fusuxxx() addr=%08x\n"
	.align	0
#endif

/*
 * suword(caddr_t uaddr, int x);
 * Store an int in the user's address space.
 */

ENTRY(suword)
	ldr	r2, Lvm_min_address
	cmp	r0, r2
	bcc	fusuaddrfault
	ldr	r2, Lvm_maxuser_address
	cmp	r0, r2
	bcs	fusuaddrfault

	ldr	r2, Lcurpcb
	ldr	r2, [r2]

#ifdef DIAGNOSTIC
	teq	r2, #0x00000000
	beq	fusupcbfault
#endif

	add	r3, pc, #fusufault - . - 8
	str	r3, [r2, #PCB_ONFAULT]

	str	r1, [r0]

	mov	r0, #0x00000000
	str	r0, [r2, #PCB_ONFAULT]
	mov	pc, lr

/*
 * suswintr(caddr_t uaddr, short x);
 * Store a short in the user's address space.  Can be called during an
 * interrupt.
 */

ENTRY(suswintr)
	ldr	r2, Lblock_userspace_access
	ldr	r2, [r2]
	teq	r2, #0
	mvnne	r0, #0x00000000
	movne	pc, lr

	ldr	r2, Lvm_min_address
	cmp	r0, r2
	bcc	fusuaddrfault
	ldr	r2, Lvm_maxuser_address
	cmp	r0, r2
	bcs	fusuaddrfault

	ldr	r2, Lcurpcb
	ldr	r2, [r2]

#ifdef DIAGNOSTIC
	teq	r2, #0x00000000
	beq	fusupcbfault
#endif

	add	r3, pc, #_C_LABEL(fusubailout) - . - 8
	str	r3, [r2, #PCB_ONFAULT]

	strb	r1, [r0, #0x0000]
	mov	r1, r1, lsr #8
	strb	r1, [r0, #0x0001]

	mov	r0, #0x00000000
	str	r0, [r2, #PCB_ONFAULT]
	mov	pc, lr

/*
 * susword(caddr_t uaddr, short x);
 * Store a short in the user's address space.
 */

ENTRY(susword)
	ldr	r2, Lvm_min_address
	cmp	r0, r2
	bcc	fusuaddrfault
	ldr	r2, Lvm_maxuser_address
	cmp	r0, r2
	bcs	fusuaddrfault

	ldr	r2, Lcurpcb
	ldr	r2, [r2]

#ifdef DIAGNOSTIC
	teq	r2, #0x00000000
	beq	fusupcbfault
#endif

	add	r3, pc, #fusufault - . - 8
	str	r3, [r2, #PCB_ONFAULT]

	strb	r1, [r0, #0x0000]
	mov	r1, r1, lsr #8
	strb	r1, [r0, #0x0001]

	mov	r0, #0x00000000
	str	r0, [r2, #PCB_ONFAULT]
	mov	pc, lr

/*
 * subyte(caddr_t uaddr, char x);
 * Store a byte in the user's address space.
 */

ENTRY(subyte)
	ldr	r2, Lvm_min_address
	cmp	r0, r2
	bcc	fusuaddrfault
	ldr	r2, Lvm_maxuser_address
	cmp	r0, r2
	bcs	fusuaddrfault

	ldr	r2, Lcurpcb
	ldr	r2, [r2]


#ifdef DIAGNOSTIC
	teq	r2, #0x00000000
	beq	fusupcbfault
#endif

	add	r3, pc, #fusufault - . - 8
	str	r3, [r2, #PCB_ONFAULT]

	strb	r1, [r0]

	mov	r0, #0x00000000
	str	r0, [r2, #PCB_ONFAULT]
	mov	pc, lr
