/*	$NetBSD: exception.S,v 1.17.2.3 2017/12/03 11:35:51 jdolecek Exp $	*/

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

#include "assym.h"

#include <arm/asm.h>

#include <arm/locore.h>

	RCSID("$NetBSD: exception.S,v 1.17.2.3 2017/12/03 11:35:51 jdolecek Exp $")

	.text	
	.align	0

AST_ALIGNMENT_FAULT_LOCALS

/*
 * reset_entry:
 *
 *	Handler for Reset exception.
 */
ARM_ASENTRY_NP(reset_entry)
	adr	r0, .Lreset_panicmsg
	mov	r1, lr
	bl	_C_LABEL(panic)
	/* NOTREACHED */
.Lreset_panicmsg:
	.asciz	"Reset vector called, LR = 0x%08x"
	.balign	4
ASEND(reset_entry)

/*
 * swi_entry
 *
 *	Handler for the Software Interrupt exception.
 */
ARM_ASENTRY_NP(swi_entry)
	PUSHFRAME
	ENABLE_ALIGNMENT_FAULTS

	mov	r0, sp			/* Pass the frame to any function */
	bl	_C_LABEL(swi_handler)	/* It's a SWI ! */

	DO_AST_AND_RESTORE_ALIGNMENT_FAULTS
	PULLFRAME
	movs	pc, lr			/* Exit */
ASEND(swi_entry)

/*
 * prefetch_abort_entry:
 *
 *	Handler for the Prefetch Abort exception.
 */
ARM_ASENTRY_NP(prefetch_abort_entry)
#ifdef __XSCALE__
	nop				/* Make absolutely sure any pending */
	nop				/* imprecise aborts have occurred. */
#endif
        sub     lr, lr, #0x00000004     /* Adjust the lr */

#ifdef _ARM_ARCH_7
	/*
	 * After taking a Data Abort exception, the state of the exclusive
	 * monitors is UNKNOWN. Therefore ARM strongly recommends that the
	 * abort handling software performs a CLREX instruction
	 */
	clrex
#endif
	PUSHFRAMEINSVC
	ENABLE_ALIGNMENT_FAULTS

	ldr	r1, .Lprefetch_abort_handler_address
	adr	lr, .Lexception_exit
 	mov	r0, sp			/* pass the stack pointer as r0 */
	ldr	pc, [r1]

.Labortprefetch:
        adr     r0, .Labortprefetchmsg
	b	_C_LABEL(panic)

.Lprefetch_abort_handler_address:
	.word	_C_LABEL(prefetch_abort_handler_address)

.Labortprefetchmsg:
        .asciz  "abortprefetch"
        .align  0
ASEND(prefetch_abort_entry)

	.data
	.p2align 2
	.global	_C_LABEL(prefetch_abort_handler_address)
_C_LABEL(prefetch_abort_handler_address):
	.word	.Labortprefetch

/*
 * data_abort_entry:
 *
 *	Handler for the Data Abort exception.
 */
ASENTRY_NP(data_abort_entry)
#ifdef __XSCALE__
	nop				/* Make absolutely sure any pending */
	nop				/* imprecise aborts have occurred. */
#endif
        sub     lr, lr, #0x00000008     /* Adjust the lr */

#ifdef _ARM_ARCH_7
	/*
	 * After taking a Data Abort exception, the state of the exclusive
	 * monitors is UNKNOWN. Therefore ARM strongly recommends that the
	 * abort handling software performs a CLREX instruction
	 */
	clrex
#endif
	PUSHFRAMEINSVC			/* Push trap frame and switch */
					/* to SVC32 mode */
	ENABLE_ALIGNMENT_FAULTS

	ldr	r1, .Ldata_abort_handler_address
	adr	lr, .Lexception_exit
	mov	r0, sp			/* pass the stack pointer as r0 */
	ldr	pc, [r1]

.Ldata_abort_handler_address:
	.word	_C_LABEL(data_abort_handler_address)

	.data
	.p2align 2
	.global	_C_LABEL(data_abort_handler_address)
_C_LABEL(data_abort_handler_address):
	.word	.Labortdata

	.text
.Labortdata:
        adr     r0, .Labortdatamsg
	b	_C_LABEL(panic)

.Labortdatamsg:
        .asciz  "abortdata"
        .align  0
ASEND(data_abort_entry)

/*
 * address_exception_entry:
 *
 *	Handler for the Address Exception exception.
 *
 *	NOTE: This exception isn't really used on arm32.  We
 *	print a warning message to the console and then treat
 *	it like a Data Abort.
 */
ASENTRY_NP(address_exception_entry)
#ifdef _ARM_ARCH_7
	/*
	 * After taking a Data Abort exception, the state of the exclusive
	 * monitors is UNKNOWN. Therefore ARM strongly recommends that the
	 * abort handling software performs a CLREX instruction
	 */
	clrex
#endif
	push	{r0-r3,ip,lr}
	mrs	r1, cpsr
	mrs	r2, spsr
	mov	r3, lr
	adr	r0, .Laddress_exception_msg
	bl	_C_LABEL(printf)	/* XXX CLOBBERS LR!! */
	pop	{r0-r3,ip,lr}
	b	_ASM_LABEL(data_abort_entry)
.Laddress_exception_msg:
	.asciz	"Address Exception CPSR=0x%08x SPSR=0x%08x LR=0x%08x\n"
	.balign	4

/*
 * General exception exit handler
 * (Placed here to be within range of all the references to it)
 *
 * It exits straight away if not returning to USR mode.
 * This loops around delivering any pending ASTs.
 * Interrupts are disabled at suitable points to avoid ASTs
 * being posted between testing and exit to user mode.
 *
 * This function uses PULLFRAMEFROMSVCANDEXIT and
 * DO_AST_AND_RESTORE_ALIGNMENT_FAULTS thus should
 * only be called if the exception handler used PUSHFRAMEINSVC
 * followed by ENABLE_ALIGNMENT_FAULTS.
 */

.Lexception_exit:
	DO_AST_AND_RESTORE_ALIGNMENT_FAULTS
	PULLFRAMEFROMSVCANDEXIT
ASEND(address_exception_entry)

/*
 * undefined_entry:
 *
 *	Handler for the Undefined Instruction exception.
 *
 *	We indirect the undefined vector via the handler address
 *	in the data area.  Entry to the undefined handler must
 *	look like direct entry from the vector.
 */
ASENTRY_NP(undefined_entry)
	str	r0, [sp, #-8]!
	GET_CURCPU(r0)
	ldr	r0, [r0, #CI_UNDEFSAVE+8]
	str	r0, [sp, #4]
	pop	{r0, pc}
ASEND(undefined_entry)

/*
 * assembly bounce code for calling the kernel
 * undefined instruction handler. This uses
 * a standard trap frame and is called in SVC mode.
 */

ENTRY_NP(undefinedinstruction_bounce)
	PUSHXXXREGSANDSWITCH
	PUSHDTRACEGAP
	PUSHTRAPFRAME(r2)
	ENABLE_ALIGNMENT_FAULTS

	mov	r0, sp
	adr	lr, .Lexception_exit
	b	_C_LABEL(undefinedinstruction)
END(undefinedinstruction_bounce)
