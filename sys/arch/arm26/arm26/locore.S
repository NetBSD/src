/* $NetBSD: locore.S,v 1.3 2000/06/08 23:26:10 bjh21 Exp $ */
/*
 * Copyright (c) 1998, 2000 Ben Harris
 * Copyright (C) 1994-1997 Mark Brinicombe
 * Copyright (C) 1994 Brini
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
 *	This product includes software developed by Brini.
 * 4. The name of Brini may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRINI ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL BRINI BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/* This file is part of NetBSD/arm26 -- a port of NetBSD to ARM2/3 machines. */
/*
 * locore.S - the (physical) start of the kernel and low-level handlers
 */
/* Note that the actual kernel startup is in start.c */

#include <machine/asm.h>

RCSID("$NetBSD: locore.S,v 1.3 2000/06/08 23:26:10 bjh21 Exp $")

#include <sys/syscall.h>
#include <machine/armreg.h>
#include "assym.h"

#include "opt_cputypes.h"

fp	.req	r11
ip	.req	r12
sp	.req	r13
lr	.req	r14
pc	.req	r15


/*
 * MODE_CHANGE_NOP should be inserted between a mode change and a
 * banked register (R8--R15) access.
 */
#if defined(CPU_ARM2) || defined(CPU_ARM250)
#define MODE_CHANGE_NOP	mov	r0, r0
#else
#define MODE_CHANGE_NOP /* Data sheet says ARM3 doesn't need it */
#endif

/*
 * USR_LDM_NOP should be inserted between an LDM to the user bank
 * and a read from a banked register. (ARM DS sec 6.6.4)
 */
#define USR_LDM_NOP	mov	r0, r0

	.text
	.balign	4

/*
 * This is for kvm_mkdb, and should be the address of the beginning
 * of the kernel text segment (not necessarily the same as kernbase).
 */

	.global _C_LABEL(kernel_text)
_C_LABEL(kernel_text):
	b	_C_LABEL(start)		/* Just in case the linker loses it */

/*
 * Trap handlers
 *
 * These take an exception and get up into a state to enter the kernel
 * proper.  This currently involves fixing up the return address (to
 * point to the faulting instruction in all cases), pushing a
 * trapframe onto the stack, calling the C handler restoring the
 * (possibly modified) trapframe and returning to userland.
 */
#define TRAP_REGS r0-r14	
	.macro	HANDLER	handler, lr_fixup
	sub	lr, lr, #\lr_fixup	/* Compensate for prefetch */
	sub	sp, sp, #TF_SIZE	/* Adjust SP */
	str	lr, [sp, #TF_R15]	/* Push return address */
	stmia	sp, {TRAP_REGS}^	/* Push user mode registers */

	mov	r0, sp			/* Supply handler with argument */
	tst	lr, #R15_MODE		/* If the trap was from USR mode... */
	moveq	fp, #0			/* ... this is the end of the stack */
	bl	\handler

/*
 * AST handling stuff.  We have to disable IRQs between loading the
 * flag and returning to user mode, since otherwise an IRQ might
 * happen in the interim and set the flag again.  For simplicity, we
 * just leave IRQs off throughout (ast_handler will re-enable them).
 */
	ldr	lr, [sp, #TF_R15]	/* Pull return address and PSR */
	tst	lr, #R15_MODE		/* Was trap in USR mode? */
	bne	3f			/* If not, return */
	ldr	r4, Lastpending
1:
	ldr	r1, [r4]
	cmp	r1, #0			/* AST pending? */
	bne	2f			/* If not, return */
	mov	r0, sp			/* Reuse old trapframe */
	bl	_C_FUNC(ast_handler)	/* Handle AST */
	b	1b			/* Try again */

2:
	ldr	lr, [sp, #TF_R15]	/* Pull return address */
3:					/* ... which we may have done above */
	ldmia	sp, {TRAP_REGS}^	/* Restore USR mode registers */
	USR_LDM_NOP
	add	sp, sp, #TF_SIZE	/* Adjust SP */
	movs	pc, lr			
	.endm

Lastpending:
	.word	_C_LABEL(astpending)
	.global	reset_entry
reset_entry:
	adr	r0, Lreset_panicmsg
	bl	_C_LABEL(panic)
	/* NOTREACHED */
Lreset_panicmsg:
	.asciz	"Reset handler called.  Branch through zero?"
	.balign	4

	.global	undefined_entry
undefined_entry:
	HANDLER	_C_FUNC(undefined_handler), 4

	.global	swi_entry
swi_entry:
	HANDLER	_C_FUNC(swi_handler), 4

	.global	prefetch_abort_entry
prefetch_abort_entry:
	HANDLER	_C_FUNC(prefetch_abort_handler), 4

	.global	data_abort_entry
data_abort_entry:
	HANDLER	_C_FUNC(data_abort_handler), 8

	.global	address_exception_entry
address_exception_entry:
	HANDLER	_C_FUNC(address_exception_handler), 8

/*
 * The stack frame messing is different here since we're handling an
 * irqframe, not a trapframe.  IRQ handlers generally don't need to
 * get at the contents of user registers (except R15), so we leave
 * them to be saved by irq_handler's prologue.  On the other hand,
 * IRQs can happen in SVC mode, and we need to do a bit of juggling to
 * save R14_svc before we enter the IRQ handler.
 *
 * The "R15" slot in the irqframe hols the value of R15 to return to.
 * Conceptually, IRQs happen at the start of an instruction.
 */

#define IRQ_REGS_SVC r0-r3, r12, r14
#define IRQ_REGS_USR r0-r3, r11-r12
	.global irq_entry
irq_entry:
	sub	r14, r14, #4		/* Fix up return address */
	tst	r14, #(R15_MODE)
	beq	Lirq_from_usr
	teqp	r15, #(R15_IRQ_DISABLE | R15_MODE_SVC)
	MODE_CHANGE_NOP
	sub	sp, sp, #IF_SIZE	/* mark space for irqframe */
	stmia	sp, {IRQ_REGS_SVC}	/* save SVC mode regs */
	mov	r0, sp			/* copy frame pointer */
	teqp	r15, #(R15_IRQ_DISABLE | R15_MODE_IRQ)
	MODE_CHANGE_NOP
	str	lr, [r0, #IF_R15]	/* Finally, save return addr */
	teqp	r15, #(R15_IRQ_DISABLE | R15_MODE_SVC)
	MODE_CHANGE_NOP

	mov	r0, sp
	bl	_C_FUNC(irq_handler)

	mov	r0, sp
	teqp	r15, #(R15_IRQ_DISABLE | R15_MODE_IRQ)
	MODE_CHANGE_NOP
	ldr	lr, [r0, #IF_R15]	/* Retrieve return address */
	teqp	r15, #(R15_IRQ_DISABLE | R15_MODE_SVC)
	MODE_CHANGE_NOP
	ldmia	sp, {IRQ_REGS_SVC}	/* Restore SVC registers */
	add	sp, sp, #IF_SIZE	/* Free irqframe */
	teqp	r15, #(R15_IRQ_DISABLE | R15_MODE_IRQ)
	MODE_CHANGE_NOP
	movs	r15, lr			/* and return. */

Lirq_from_usr:
	teqp	r15, #(R15_IRQ_DISABLE | R15_MODE_SVC)
	MODE_CHANGE_NOP
	sub	sp, sp, #IF_SIZE	/* mark space for irqframe */
	stmia	sp, {IRQ_REGS_USR}	/* save SVC/USR mode regs */
	mov	r0, sp			/* copy frame pointer */
	teqp	r15, #(R15_IRQ_DISABLE | R15_MODE_IRQ)
	MODE_CHANGE_NOP
	str	lr, [r0, #IF_R15]	/* Finally, save return addr */
	teqp	r15, #(R15_IRQ_DISABLE | R15_MODE_SVC)
	MODE_CHANGE_NOP

	mov	fp, #0			/* This is the top of the stack */
	mov	r0, sp
	bl	_C_FUNC(irq_handler)

	ldr	r0, Lastpending
	ldr	r0, [r0]
	cmp	r0, #0
	bne	Lirq_to_ast
	mov	r0, sp
	teqp	r15, #(R15_IRQ_DISABLE | R15_MODE_IRQ)
	MODE_CHANGE_NOP
	ldr	lr, [r0, #IF_R15]	/* Retrieve return address */
	teqp	r15, #(R15_IRQ_DISABLE | R15_MODE_SVC)
	MODE_CHANGE_NOP
	ldmia	sp, {IRQ_REGS_USR}	/* Restore SVC/USR registers */
	add	sp, sp, #IF_SIZE	/* Free irqframe */
	teqp	r15, #(R15_IRQ_DISABLE | R15_MODE_IRQ)
	MODE_CHANGE_NOP
	movs	r15, lr			/* and return. */

Lirq_to_ast:
	ldmia	sp, {IRQ_REGS_USR}	/* Slurp up irqframe */
	sub	sp, sp, #(TF_SIZE - IF_SIZE)
	stmia	sp, {TRAP_REGS}^	/* Spit out trapframe */

	mov	fp, #0
Last_again:
	mov	r0, sp
	bl	_C_FUNC(ast_handler)

	teqp	r15, #(R15_IRQ_DISABLE | R15_MODE_SVC)
	ldr	r0, Lastpending
	ldr	r0, [r0]
	cmp	r0, #0
	bne	Last_again
	ldr	lr, [sp, #TF_R15]	/* Pull return address */
	ldmia	sp, {TRAP_REGS}^	/* Restore USR mode registers */
	USR_LDM_NOP
	add	sp, sp, #TF_SIZE	/* Adjust SP */
	movs	pc, lr			

ENTRY(atomic_set_bit)
	/* disable interrupts */
	/* Note that this is evil and only works due to prefetching */
	mov	r2, r15
	orrs	r15, r2, #(R15_IRQ_DISABLE | R15_FIQ_DISABLE)
	ldr	r2, [r0]
	orr	r2, r2, r1
	str	r2, [r0]
	/* restore interrupt state and return */
	movs	pc, lr
ENTRY(atomic_clear_bit)
	/* disable interrupts */
	/* Note that this is evil and only works due to prefetching */
	mov	r2, r15
	orrs	r15, r2, #(R15_IRQ_DISABLE | R15_FIQ_DISABLE)
	ldr	r2, [r0]
	bic	r2, r2, r1
	str	r2, [r0]
	/* restore interrupt state and return */
	movs	pc, lr

ENTRY(set_r13_irq)
	/* register_t set_r13_irq(register_t r13_irq) */
	/* XXX Is enabling FIQs OK? */
	teqp	r15, #(R15_IRQ_DISABLE | R15_MODE_IRQ)
	mov	r1, r0
	mov	r0, r13
	mov	r13, r1
	teqp	r15, #(R15_IRQ_DISABLE | R15_MODE_SVC)
	MODE_CHANGE_NOP
	movs	r15, lr

	/*
	 * These are grotty, and very non-APCS.  The CPU interrupt status
	 * is part of the state that's restored on function exit, so the
	 * effect of these only persists until then.
	 */
ENTRY(int_on)
	bics	r15, r14, #(R15_IRQ_DISABLE | R15_FIQ_DISABLE)
ENTRY(int_off)
	orrs	r15, r14, #(R15_IRQ_DISABLE)

	/*
	 * Low-level context-switch operation.  cpu_switch() is in C -- this
	 * just handles the bits that can't be done in C.
	 *
	 * void cpu_loswitch(struct switchframe **fromp,
	 *		     struct switchframe *to)
	 *
	 * We leave a switchframe on the stack pointed to by fromp,
	 * and return to the context in to.  This should be called in
	 * splhigh();
	 */
ENTRY(cpu_loswitch)
	mov	r2, sp			/* Temporary stack pointer */
	stmfd	r2!, {r4-r11, r13-r14}	/* Save all relevant registers */
	str	r2, [r0]		/* Save switchframe pointer */
	ldmfd	r1, {r4-r11, r13-r14}	/* Restore from old switchframe */
	movs	pc, r14			/* and return */

	/*
	 * This funny little routine implements starting a process.
	 * It's called by cpu_loswitch returning from a faked
	 * switchframe set up by cpu_fork() and cpu_set_kpc(), and gets
	 * the function it's meant to enter passed in R4 with its
	 * argument in R5.  If that function's NULL, or if it returns,
	 * we hope there's a trapframe on the stack that'll take us
	 * back to userland.
	 */
ENTRY(proc_trampoline)
	mov	fp, #0			/* Tie knot in top of stack */
	mov	r0, #0
	bl	_C_FUNC(lowerspl)		/* spl0() */
	cmp	r4, #0			/* Function to call? */
	beq	Lproc_trampoline_nofunc
	mov	r0, r5
	mov	r14, pc			/* Save return address */
	mov	pc, r4			/* Call function */
Lproc_trampoline_nofunc:
	ldr	lr, [sp, #TF_R15]	/* Pull return address */
	ldmia	sp, {r0-r14}^		/* Restore USR mode registers */
	USR_LDM_NOP
	add	sp, sp, #TF_SIZE	/* Adjust SP */
	movs	pc, lr			

/* int setjmp(label_t *); */
ENTRY(setjmp)
	stmia	r0, {r4-r11, r13-r14}	/* Save relevant registers */
	mov	r0, #0			/* Return 0 */
	movs	pc, lr

/* void longjmp(label_t *); */
ENTRY(longjmp)
	ldmia	r0, {r4-r11, r13-r14}	/* Restore registers */
	mov	r0, #1			/* Return 1 through longjmp */
	movs	pc, lr

/* Enter DDB -- shove registers in a trapframe first */
/* void cpu_Debugger(void); */
ENTRY(cpu_Debugger)
	stmfd	r13!, {r14}
	stmfd	r13!, {r0-r15}		/* Save all SVC-mode regs */
	mov	r1, r13
	mov	r0, #0
	bl	_C_FUNC(kdb_trap)
	add	r13, r13, #(4 * 16)
	ldmfd	r13!, {pc}^

/*
 * Signal trampoline; copied to top of user stack.
 */

ENTRY_NP(sigcode)
/*
 * r0-r2 are our signal handler parameters
 * r3 is the handler address
 */

	add	lr, pc, #0			/* Set return address */
	mov	pc, r3				/* Call the handler */

/*
 * Call sig_return with address of the signal context
 * Note: Don't use SIG_SCP as this make have been trashed by the program
 */
	add	r0, sp, #SF_SC
	swi	SYS___sigreturn14

/* Well if that failed we better exit quick ! */

	swi	SYS_exit
	b	. - 8

	.align	0
        .global _C_LABEL(esigcode)
_C_LABEL(esigcode):
