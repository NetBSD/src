/*	$NetBSD: exception.S,v 1.1.2.2 2001/08/03 04:10:58 lukem Exp $	*/

/*
 * Copyright (c) 1994-1997 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
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
 *	This product includes software developed by Brini.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRINI ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL BRINI OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
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
 * exception.S
 *
 * Low level handlers for exception vectors
 *
 * Created      : 24/09/94
 *
 * Based on kate/display/abort.s
 */

#include "opt_ipkdb.h"
#include <machine/asm.h>
#include <machine/cpu.h>
#include <machine/frame.h>
#include "assym.h"

	.text	
	.align	0

Lastpending:
        .word   _C_LABEL(astpending)

/*
 * General exception exit handler
 *
 * It exits straight away if not returning to USR mode.
 * This loops around delivering any pending ASTs.
 * Interrupts are disabled at suitable points to avoid ASTs
 * being posted between testing and exit to user mode.
 *
 * This function uses PULLFRAMEFROMSVCANDEXIT thus should
 * only be called if the exception handler used PUSHFRAMEINSVC
 */

exception_exit:
	mrs     r4, cpsr_all		/* Get CPSR */

	ldr	r0, [sp]		/* Get the SPSR from stack */
	and	r0, r0, #(PSR_MODE)	/* Test for USR32 mode before the AST */
	teq	r0, #(PSR_USR32_MODE)
	bne	do_exit			/* Not USR mode so no AST delivery */

	ldr	r5, Lastpending		/* Get address of astpending */

Lexception_exit_loop:
	orr     r0, r4, #(I32_bit)	/* Block IRQs */
	msr     cpsr_all, r0

	ldr	r1, [r5]		/* Do we have an AST pending */
	teq	r1, #0x00000000
	bne	do_ast

	PULLFRAMEFROMSVCANDEXIT		/* No AST so exit */

do_ast:
	msr     cpsr_all, r4		/* Restore interrupts */

	mov	r1, #0x00000000		/* Clear ast pending */
	str	r1, [r5]

	mov	r0, sp			/* arg 0 = trap frame */
	bl	_C_LABEL(ast)		/* call the AST handler */
	b	Lexception_exit_loop	/* Try and exit again */

do_exit:
	orr     r0, r4, #(I32_bit)	/* Disable interupts */
	msr     cpsr_all, r0

	PULLFRAMEFROMSVCANDEXIT		/* Restore the trap frame and exit */


/* entry point for CPU data abort */

ASENTRY_NP(data_abort_entry)
        sub     lr, lr, #0x00000008     /* Adjust the lr */

	PUSHFRAMEINSVC			/* Push trap frame and switch */
					/* to SVC32 mode */

	mov	r0, sp			/* pass the stack pointer as r0 */

/*	add	lr, pc, #exception_exitLdata_abort_return - . - 8*/
	add	lr, pc, #exception_exit - . - 8
	ldr	r1, Ldata_abort_handler_address
	ldr	pc, [r1]
	
Ldata_abort_return:	/* XXX - could just straight to exception_exit */
	b	exception_exit

Ldata_abort_handler_address:
	.word	_C_LABEL(data_abort_handler_address)

	.data
	.global	_C_LABEL(data_abort_handler_address)
_C_LABEL(data_abort_handler_address):
	.word	abortdata

	.text
abortdata:
        add     r0, pc, #abortdatamsg - . - 8
	b	_C_LABEL(panic)

abortdatamsg:
        .asciz  "abortdata"
        .align  0


ASENTRY_NP(prefetch_abort_entry)
        sub     lr, lr, #0x00000004     /* Adjust the lr */

	PUSHFRAMEINSVC

 	mov	r0, sp			/* pass the stack pointer as r0 */

#	add	lr, pc, #Lprefetch_abort_return - . - 8
	add	lr, pc, #exception_exit - . - 8
	ldr	r1, Lprefetch_abort_handler_address
	ldr	pc, [r1]
	
Lprefetch_abort_return:	/* XXX - could just straight to exception_exit */
	b	exception_exit

Lprefetch_abort_handler_address:
	.word	_C_LABEL(prefetch_abort_handler_address)

	.data
	.global	_C_LABEL(prefetch_abort_handler_address)

_C_LABEL(prefetch_abort_handler_address):
	.word	abortprefetch

	.text
abortprefetch:
        add     r0, pc, #abortprefetchmsg - . - 8
	b	_C_LABEL(panic)

abortprefetchmsg:
        .asciz  "abortprefetch"
        .align  0


/*
 * swi_entry
 *
 * Main entry point for the SWI vector
 */

ASENTRY_NP(swi_entry)
	PUSHFRAME

 	sub	r0, lr, #0x00000004	/* Get the address of the SWI */
	ldr	r4, [r0]		/* Get the instruction */

	bic	r1, r4, #0xff000000	/* Extract the comment field */

	mov	r0, sp			/* Pass the frame to any function */

	bl	_C_LABEL(syscall)	/* It's a syscall ! */

	ldr	r5, Lastpending		/* Get address of astpending */
	mrs     r4, cpsr_all		/* Get CPSR */

swi_exit_loop:
	orr     r0, r4, #(I32_bit)	/* Disable IRQs */
	msr     cpsr_all, r0

	ldr	r1, [r5]		/* Do we have an AST pending */
	teq	r1, #0x00000000
	bne	do_swi_ast

	PULLFRAME
	movs	pc, lr			/* Exit */

do_swi_ast:
	msr     cpsr_all, r4		/* Restore interrupts */

	mov	r1, #0x00000000		/* Clear ast pending */
	str	r1, [r5]

	mov	r0, sp			/* arg 0 = trap frame */
	bl	_C_LABEL(ast)		/* call the AST handler */
	b	swi_exit_loop		/* Try and exit again */


/*
 * We indirect the undefined vector via the handler address
 * in the data area.
 * Entry to the undefined handler must look like direct
 * entry from the vector.
 */

ASENTRY_NP(undefined_entry)
#ifdef IPKDB
/*
 * IPKDB must be hooked in at the earliest possible entry point.
 *
 */
/*
 * Make room for all registers saving real r0-r7 and r15.
 * The remaining registers are updated later.
 */
	stmfd	sp!, {r0,r1}		/* psr & spsr */
	stmfd	sp!, {lr}		/* pc */
	stmfd	sp!, {r0-r14}		/* r0-r7, r8-r14 */
/*
 * Get previous psr.
 */
	mrs	r7, cpsr_all
	mrs	r0, spsr_all
	str	r0, [sp, #(16*4)]
/*
 * Test for user mode.
 */
	tst	r0, #0xf
	bne	Lprenotuser_push
	add	r1, sp, #(8*4)
	stmia	r1,{r8-r14}^		/* store user mode r8-r14*/
	b	Lgoipkdb
/*
 * Switch to previous mode to get r8-r13.
 */
Lprenotuser_push:
	orr	r0, r0, #(I32_bit) /* disable interrupts */
	msr	cpsr_all, r0
	mov	r1, r8
	mov	r2, r9
	mov	r3, r10
	mov	r4, r11
	mov	r5, r12
	mov	r6, r13
	msr	cpsr_all, r7		/* back to undefined mode */
	add	r8, sp, #(8*4)
	stmia	r8, {r1-r6}		/* r8-r13 */
/*
 * Now back to previous mode to get r14 and spsr.
 */
	msr	cpsr_all, r0
	mov	r1, r14
	mrs	r2, spsr
	msr	cpsr_all, r7		/* back to undefined mode */
	str	r1, [sp, #(14*4)]	/* r14 */
	str	r2, [sp, #(17*4)]	/* spsr */
/*
 * Now to IPKDB.
 */
Lgoipkdb:
	mov	r0, sp
	bl	_C_LABEL(ipkdb_trap_glue)
	ldr	r1, Lipkdb_trap_return
	str	r0,[r1]
/*
 * Have to load all registers from the stack.
 *
 * Start with spsr and pc.
 */
	ldr	r0, [sp, #(16*4)]	/* spsr */
	ldr	r1, [sp, #(15*4)]	/* r15 */
	msr	spsr_all, r0
	mov	r14, r1
/*
 * Test for user mode.
 */
	tst	r0, #0xf
	bne	Lprenotuser_pull
	add	r1, sp, #(8*4)
	ldmia	r1, {r8-r14}^		/* load user mode r8-r14 */
	b	Lpull_r0r7
Lprenotuser_pull:
/*
 * Now previous mode spsr and r14.
 */
	ldr	r1, [sp, #(17*4)]		/* spsr */
	ldr	r2, [sp, #(14*4)]		/* r14 */
	orr	r0, r0, #(I32_bit)
	msr	cpsr_all, r0			/* switch to previous mode */
	msr	spsr_all, r1
	mov	r14, r2
	msr	cpsr_all, r7			/* back to undefined mode */
/*
 * Now r8-r13.
 */
	add	r8, sp, #(8*4)
	ldmia	r8, {r1-r6}		/* r8-r13 */
	msr	cpsr_all, r0
	mov	r8, r1
	mov	r9, r2
	mov	r10, r3
	mov	r11, r4
	mov	r12, r5
	mov	r13, r6
	msr	cpsr_all, r7
Lpull_r0r7:
/*
 * Now the rest of the registers.
 */
	ldr	r1,Lipkdb_trap_return
	ldr	r0,[r1]
	tst	r0,r0
	ldmfd	sp!, {r0-r7}		/* r0-r7 */
	add	sp, sp, #(10*4)		/* adjust sp */

/*
 * Did IPKDB handle it?
 */
	movnes	pc, lr			/* return */

#endif
	stmfd	sp!, {r0, r1}
	ldr	r0, Lundefined_handler_indirection
	ldr	r1, [sp], #0x0004
	str	r1, [r0, #0x0000]
	ldr	r1, [sp], #0x0004
	str	r1, [r0, #0x0004]
	ldmia	r0, {r0, r1, pc}

#ifdef IPKDB
Lipkdb_trap_return:
	.word	Lipkdb_trap_return_data
#endif

Lundefined_handler_indirection:
	.word	Lundefined_handler_indirection_data

/*
 * assembly bounce code for calling the kernel
 * undefined instruction handler. This uses
 * a standard trap frame and is called in SVC mode.
 */

ENTRY_NP(undefinedinstruction_bounce)
	PUSHFRAMEINSVC
	mov	r0, sp
	bl	_C_LABEL(undefinedinstruction)

	b	exception_exit

	.data
	.align	0

#ifdef IPKDB
Lipkdb_trap_return_data:
	.word	0
#endif

/*
 * Indirection data
 * 2 words use for preserving r0 and r1
 * 3rd word contains the undefined handler address.
 */

Lundefined_handler_indirection_data:
	.word	0
	.word	0

	.global	_C_LABEL(undefined_handler_address)
_C_LABEL(undefined_handler_address):
	.word	_C_LABEL(undefinedinstruction_bounce)

/* End of exception.S */
