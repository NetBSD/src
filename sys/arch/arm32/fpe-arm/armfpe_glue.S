/* $NetBSD: armfpe_glue.S,v 1.13 1999/10/26 06:53:43 cgd Exp $ */

/*
 * Copyright (c) 1996 Mark Brinicombe
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
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
 * arm_fpe_glue.S
 *
 * Glue code for calling the ARM FPE core code
 *
 * Created      : 21/12/95
 */

#define CHECK_BEFORE_USERRET
#include "assym.h"
#include <machine/asm.h>
#include <machine/cpu.h>

/*
 * Glue for calling the core entry points
 */

ENTRY_NP(arm_fpe_core_disable)
	stmfd	sp!, {r0-r7, lr}
	bl	_fpe_arm_core_disable
	ldmfd	sp!, {r0-r7, pc}

ENTRY_NP(arm_fpe_core_enable)
	stmfd	sp!, {r0-r7, lr}
	bl	_fpe_arm_core_enable
	ldmfd	sp!, {r0-r7, pc}

ENTRY_NP(arm_fpe_core_initws)
	stmfd	sp!, {r10, lr}
	mov	r10, r0
	bl	_fpe_arm_core_initws
	ldmfd	sp!, {r10, pc}

ENTRY_NP(arm_fpe_core_abort)
	stmfd	sp!, {r1-r7, r10, lr}
	mov	r10, r0
	mov	r0, r1
	mov	r1, r2
	bl	_fpe_arm_core_abort
	ldmfd	sp!, {r1-r7, r10, pc}

/* Only needs to preserve r10 */

ENTRY_NP(arm_fpe_core_initcontext)
	stmfd	sp!, {r0-r7, r10, lr}
	mov	r10, r0
	bl	_fpe_arm_core_initcontext
	ldmfd	sp!, {r0-r7, r10, pc}

/* Only needs to preserve r10 */

ENTRY_NP(arm_fpe_core_changecontext)
	stmfd	sp!, {r1-r7, r10, lr}
	mov	r10, r0
	bl	_fpe_arm_core_changecontext
	ldmfd	sp!, {r1-r7, r10, pc}

/* All regs preserved */

ENTRY_NP(arm_fpe_core_shutdown)
	stmfd	sp!, {r0-r7, r10, lr}
	bl	_fpe_arm_core_shutdown
	ldmfd	sp!, {r0-r7, r10, pc}

/* Preserve r10 */

ENTRY_NP(arm_fpe_core_savecontext)
	stmfd	sp!, {r1-r7, r10, lr}
	mov	r10, r0
	mov	r0, r1
	mov	r1, r2
	bl	_fpe_arm_core_savecontext
	ldmfd	sp!, {r1-r7, r10, pc}

/* Preserve r10 */

ENTRY_NP(arm_fpe_core_loadcontext)
	stmfd	sp!, {r0-r7, r10, lr}
	mov	r10, r0
	mov	r0, r1
	bl	_fpe_arm_core_loadcontext
	ldmfd	sp!, {r0-r7, r10, pc}


/* Only needs to preserve r10 */

ENTRY_NP(arm_fpe_core_activatecontext)
	stmfd	sp!, {r0-r7, r10, lr}
	mov	r10, r0
	bl	_fpe_arm_core_activatecontext
	ldmfd	sp!, {r0-r7, r10, pc}

/* Only needs to preserve r10 */

ENTRY_NP(arm_fpe_core_deactivatecontext)
	stmfd	sp!, {r1-r7, r10, lr}
	bl	_fpe_arm_core_deactivatecontext
	ldmfd	sp!, {r1-r7, r10, pc}

/*
 * Call back functions from the core
 */

ENTRY_NP(arm_fpe_newhandler)
	stmfd	sp!, {r0, lr}
	ldr	r0, Llocal_handler_addr
	str	r1, [r0]
	ldmfd	sp!, {r0, pc}

Llocal_handler_addr:
	.word	_undefined_handler_address

ENTRY_NP(arm_fpe_restorehandler)
	stmfd	sp!, {r0-r1, lr}
	ldr	r0, Llocal_handler_addr
	ldr	r1, Lold_handler_addr
	ldr	r1, [r1]
	str	r1, [r0]
	ldmfd	sp!, {r0-r1, pc}

Lold_handler_addr:
	.word	_arm_fpe_old_handler_address

ENTRY_NP(arm_fpe_handle_exception)
	b	_arm_fpe_exception_glue

ENTRY_NP(arm_fpe_get_ws)
	sub	sp, sp, #8
	str	r0, [sp]
	ldr	r0, Larm_fpe_core_workspace
	ldr	r0, [r0]
	str	r0, [sp, #4]
	ldr	r0, [sp], #4
	mov	pc, lr

Larm_fpe_core_workspace:
	.word	_arm_fpe_core_workspace

ENTRY_NP(arm_fpe_post_proc)
	b	_arm_fpe_post_proc_glue


/* Simple call back function that panics */

ENTRY_NP(arm_fpe_panic)
	adr	r0, Lfpe_panic_text
	b	_C_LABEL(panic)

Lfpe_panic_text:
	.asciz	"armfpe: we are panicing"
	.align	0

/*
 * Call back routine from FPE on completion of an instruction
 */

#ifdef CHECK_BEFORE_USERRET

	.global	_userret_count0
	.global	_userret_count1
	.data
_userret_count0:
	.word	0
_userret_count1:
	.word	0

	.text

Luserret_count0:
	.word	_userret_count0
Luserret_count1:
	.word	_userret_count1

Lwant_resched:
	.word	_want_resched

Lcurproc:
	.word	_curproc

Lcurpriority:
	.word	_curpriority

#endif

ENTRY_NP(arm_fpe_post_proc_glue)
	stmfd	sp!, {r0-r3, lr}

#ifdef CHECK_BEFORE_USERRET

	/* Call userret if we need a reschedule */

	/* Debugging */
	ldr	r0, Luserret_count0
	ldr	r1, [r0]
	add	r1, r1, #1
	str	r1, [r0]

	/* Do we need a reschedule */
	ldr	r0, Lwant_resched
	ldr	r0, [r0]
	teq	r0, #0x00000000
	bne	Lwe_need_userret

	/* All other userret requirement conditions come from curproc */
	ldr	r0, Lcurproc
	ldr	r0, [r0]

	/* Remember the flags field */
	ldr	r3, [r0, #(P_FLAG)]

	/* Get the signal list */
	ldr	r1, [r0, #(P_SIGLIST)]
	teq	r1, #0x00000000
	beq	Lno_signals_pending

	tst	r3, #(P_TRACED)
	bne	Lwe_need_userret

	ldr	r1, [r0, #(P_SIGLIST)]
	ldr	r2, [r0, #(P_SIGMASK)]
	bic	r1, r1, r2
	teq	r1, #0x00000000
	bne	Lwe_need_userret

Lno_signals_pending:
	/* Are we profiling ? */
	tst	r3, #(P_PROFIL)
	bne	Lwe_need_userret

	/* Update the current priority */
	ldrb	r1, [r0, #(P_USRPRI)]
	strb	r1, [r0, #(P_PRIORITY)]
	ldr	r0, Lcurpriority
	strb	r1, [r0]

	/* Fast return */
	ldmfd	sp!, {r0-r3, pc}

Lwe_need_userret:
	/* Ok we need to call userret() */

	stmfd	sp!, {r4-r6, r10-r12}

	/* Debugging */
	ldr	r0, Luserret_count1
	ldr	r1, [r0]
	add	r1, r1, #1
	str	r1, [r0]

#endif

/* This could be optimised as we are going from UND32->SVC32 mode */

        mrs     r4, cpsr_all
	bic	r3, r4, #(PSR_MODE)
        orr     r3, r3, #(PSR_SVC32_MODE)
        msr     cpsr_all, r3

	mov	r0, r12

/* Reserve a trapframe on the SVC stack */

	sub	sp, sp, #(TRAPFRAMESIZE)
	mov	r1, sp

	ldr	r2, [r0, #-0x0008]	/* Copy spsr */
	str	r2, [r1], #0x0004

	ldmia	r0!, {r2, r3, r5, r6}	/* copy r0-r5 */
	stmia	r1!, {r2, r3, r5, r6}

	ldmia	r0!, {r2, r3, r5, r6}	/* copy r6-r11 */
	stmia	r1!, {r2, r3, r5, r6}

	ldmia	r0!, {r2, r3, r5, r6}	/* copy r6-r11 */
	stmia	r1!, {r2, r3, r5, r6}

	ldmia	r0!, {r2, r3, r5, r6}	/* copy r12, r13, r14, r15 */
	stmia	r1!, {r2, r3, r5, r14}
	str	r6, [r1, #0x0000]

	mov	r0, r12
	mov	r1, sp

/*
 * OK Question Time ...
 *
 * Do I need to save SVC r14 ? 
 * It only needs saving if this routine can interrupt something already
 * running in SVC mode. Since FP is only valid from USR32 mode this
 * should not happen.
 */

	mov	r5, r14
	mov	r6, r12

/* More optimisation ... Need to code an assembly version of userret() */

	bl	_arm_fpe_postproc

/* Release the trapframe on the SVC stack */
	mov	r14, r5

	mov	r0, sp

	ldr	r2, [r0], #0x0004	/* Copy spsr */
	str	r2, [r6, #-0x0008]

	ldmia	r0!, {r1, r2, r3, r5, r10, r11}	/* copy r0-r5 */
	stmia	r6!, {r1, r2, r3, r5, r10, r11}

	ldmia	r0!, {r1, r2, r3, r5, r10, r11}	/* copy r6-r11 */
	stmia	r6!, {r1, r2, r3, r5, r10, r11}

	ldmia	r0!, {r1, r2, r3}	/* copy r12, r13, r14 */
	stmia	r6!, {r1, r2, r3}

	ldr	r1, [r0, #0x0004]
	str	r1, [r6]

	add	sp, sp, #(TRAPFRAMESIZE)

	msr	cpsr_all, r4

	ldmfd	sp!, {r4-r6, r10-r12}
	ldmfd	sp!, {r0-r3, pc}


/*
 * Call back routine from FPE when the an exception occurs
 */

ENTRY_NP(arm_fpe_exception_glue)
	stmfd	sp!, {r0-r6, r10-r12, lr}

	mov	r10, r0

/* This could be optimised as we are going from UND32->SVC32 mode */

        mrs     r4, cpsr_all
	bic	r3, r4, #(PSR_MODE)
        orr     r3, r3, #(PSR_SVC32_MODE)
        msr     cpsr_all, r3

	mov	r0, r12

/* Reserve a trapframe on the SVC stack */

	sub	sp, sp, #(TRAPFRAMESIZE)
	mov	r1, sp

	ldr	r2, [r0, #-0x0008]	/* Copy spsr */
	str	r2, [r1], #0x0004

	ldmia	r0!, {r2, r3, r5, r6}	/* copy r0-r5 */
	stmia	r1!, {r2, r3, r5, r6}

	ldmia	r0!, {r2, r3, r5, r6}	/* copy r6-r11 */
	stmia	r1!, {r2, r3, r5, r6}

	ldmia	r0!, {r2, r3, r5, r6}	/* copy r6-r11 */
	stmia	r1!, {r2, r3, r5, r6}

	ldmia	r0!, {r2, r3, r5, r6}	/* copy r12, r13, r14, r15 */
	stmia	r1!, {r2, r3, r5, r14}
	str	r6, [r1, #0x0000]

	mov	r0, r10		/* exception */
	mov	r1, r12		/* fpframe */
	mov	r2, sp		/* trapframe */

/*
 * OK Question Time ...
 *
 * Do I need to save SVC r14 ? 
 * It only needs saving if this routine can interrupt something already
 * running in SVC mode. Since FP is only valid from USR32 mode this
 * should not happen.
 */

	mov	r5, r14
	mov	r6, r12

/* More optimisation ... Need to code an assembly version of userret() */

	bl	_arm_fpe_exception

/* Release the trapframe on the SVC stack */
	mov	r14, r5

	mov	r0, sp

	ldr	r2, [r0], #0x0004	/* Copy spsr */
	str	r2, [r6, #-0x0008]

	ldmia	r0!, {r1, r2, r3, r5, r10, r11}	/* copy r0-r5 */
	stmia	r6!, {r1, r2, r3, r5, r10, r11}

	ldmia	r0!, {r1, r2, r3, r5, r10, r11}	/* copy r6-r11 */
	stmia	r6!, {r1, r2, r3, r5, r10, r11}

	ldmia	r0!, {r1, r2, r3}	/* copy r12, r13, r14 */
	stmia	r6!, {r1, r2, r3}

	ldr	r1, [r0, #0x0004]
	str	r1, [r6]

	add	sp, sp, #(TRAPFRAMESIZE)

	msr	cpsr_all, r4

	ldmfd	sp!, {r0-r6, r10-r12, lr}

/* Now pull the original trapframe that the FPE pushed off the stack */

	ldmdb	r12, {r0, r1}

	msr	cpsr_all, r1
	msr	spsr_all, r0

	mov	sp, r12

	ldmia	sp, {r0-r14}^
	mov	r0, r0
	add	sp, sp, #15*4
	ldmfd	sp!, {pc}^


ENTRY_NP(arm_fpe_set_exception_mask)
	rfs	r1			/* Get FP status */
	bic	r1, r1, #0x001f0000	/* Zero exception mask */
	and	r0, r0, #0x0000001f	/* Mask new bits */
	orr	r0, r1, r0, lsl #16	/* Merge */
	wfs	r0			/* Set status */
	mov	r0, r1, lsr #16		/* Return old mask */
	mov	pc, lr			/* return */ 


	.global _fpe_nexthandler
_fpe_nexthandler:
	.word	_undefinedinstruction_bounce
