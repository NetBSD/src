/*	$NetBSD: vectors.S,v 1.27 2022/05/29 23:39:59 ryo Exp $	*/

#include <aarch64/asm.h>
#include <aarch64/locore.h>

#include "assym.h"

#include "opt_compat_netbsd32.h"
#include "opt_cpuoptions.h"
#include "opt_ddb.h"
#include "opt_dtrace.h"
#include "opt_gic.h"

RCSID("$NetBSD: vectors.S,v 1.27 2022/05/29 23:39:59 ryo Exp $")

	ARMV8_DEFINE_OPTIONS

#ifdef KDTRACE_HOOKS
/*
 * dtrace needs to emulate  stp x29,x30,[sp,#-FRAMESIZE]!   where
 * FRAMESIZE can be as large as 512, so create a 512-byte buffer
 * between the interrupted code's frame and our struct trapframe.
 */
#define	TRAP_FRAMESIZE	(TF_SIZE + 512)
#else
#define	TRAP_FRAMESIZE	TF_SIZE
#endif

/*
 * Template for the handler functions.
 */
.macro	vector_func, func, el, intr_p, label, tpidr
	.align 7	/* cacheline-aligned */

ENTRY_NBTI(\func)

	.if \el == 1
	/* need to allocate stack on el1 */
	sub	sp, sp, #TRAP_FRAMESIZE
	.endif

	stp	x0, x1, [sp, #TF_X0]
	stp	x2, x3, [sp, #TF_X2]
	stp	x4, x5, [sp, #TF_X4]
	stp	x6, x7, [sp, #TF_X6]
	stp	x8, x9, [sp, #TF_X8]
	stp	x10, x11, [sp, #TF_X10]
	stp	x12, x13, [sp, #TF_X12]
	stp	x14, x15, [sp, #TF_X14]
	stp	x16, x17, [sp, #TF_X16]
	str	x18, [sp, #TF_X18]
	stp	x19, x20, [sp, #TF_X19]
	stp	x21, x22, [sp, #TF_X21]
	stp	x23, x24, [sp, #TF_X23]
	stp	x25, x26, [sp, #TF_X25]
	stp	x27, x28, [sp, #TF_X27]
	stp	x29, x30, [sp, #TF_X29]

	/* get sp and elr */
	.if \el == 0
	mrs	x20, sp_el0
	.else
	/* sp was already adjusted, so adjust x20 back */
	add	x20, sp, #TRAP_FRAMESIZE
	.endif
	mrs	x21, elr_el1

	/* store sp and elr */
	.if TF_SP + 8 == TF_PC
	stp	x20, x21, [sp, #TF_SP]
	.else
	str	x20, [sp, #TF_SP]
	str	x21, [sp, #TF_PC]
	.endif

	mrs	x22, spsr_el1
	str	x22, [sp, #TF_SPSR]

	.if \intr_p == 1
	mov	x23, #-1
	mov	x24, xzr
	.else
	mrs	x23, esr_el1
	mrs	x24, far_el1
	.endif

	.if TF_ESR + 8 == TF_FAR
	stp	x23, x24, [sp, #TF_ESR]
	.else
	str	x23, [sp, #TF_ESR]
	str	x24, [sp, #TF_FAR]
	.endif

	.if \el == 0
	/* curlwp->l_private = tpidr{,ro}_el0 */
	mrs	x1, tpidr_el1		/* x1 = curlwp */
	mrs	x0, tpidr\tpidr\()_el0
	str	x0, [x1, #L_PRIVATE]	/* curlwp->l_private = tpidr{,ro}_el0 */

#ifdef ARMV83_PAC
	/* Switch to the kern PAC key. */
	adrl	x4, _C_LABEL(aarch64_pac_enabled)
	ldr	w4, [x4]
	cbz	w4, 1f
	ldp	x5, x6, [x1, #L_MD_IA_KERN]
	msr	APIAKeyLo_EL1, x5
	msr	APIAKeyHi_EL1, x6
	isb
1:
#endif
	.endif

	adr	x30, el\el\()_trap_exit	/* el[01]_trap_exit */
	mov	x0, sp
#ifdef DDB
	mov	x29, sp			/* for backtrace */
#endif
	b	\label
END(\func)
.endm

/*
 * The vector_entry macro must be small enough to fit 0x80 bytes! We just jump
 * into the proper function, so this constraint is always respected.
 */
.macro	vector_entry, func
	.align 7	/* aligned 0x80 */
	b	\func
.endm

/*
 * The functions.
 */
vector_func	el1t_sync_handler,  1, 0, trap_el1t_sync
vector_func	el1t_irq_handler,   1, 1, trap_el1t_irq
vector_func	el1t_fiq_handler,   1, 1, trap_el1t_fiq
vector_func	el1t_error_handler, 1, 0, trap_el1t_error

vector_func	el1h_sync_handler,  1, 0, trap_el1h_sync
vector_func	el1h_intr_handler,  1, 1, cpu_irq
vector_func	el1h_fiq_handler,   1, 1, cpu_fiq
vector_func	el1h_error_handler, 1, 0, trap_el1h_error

vector_func	el0_sync_handler,  0, 0, trap_el0_sync
vector_func	el0_intr_handler,  0, 1, cpu_irq
vector_func	el0_fiq_handler,   0, 1, cpu_fiq
vector_func	el0_error_handler, 0, 0, trap_el0_error

vector_func	el0_32sync_handler,  0, 0, trap_el0_32sync, ro
vector_func	el0_32intr_handler,  0, 1, cpu_irq, ro
vector_func	el0_32fiq_handler,   0, 1, cpu_fiq, ro
vector_func	el0_32error_handler, 0, 0, trap_el0_32error, ro

/*
 * The vector table. Must be aligned to 2048.
 */
	.align 11
ENTRY_NBTI(el1_vectors)
	/*
	 * Exception taken from current Exception Level with SP_EL0.
	 * (These shouldn't happen)
	 */
	vector_entry	el1t_sync_handler
	vector_entry	el1t_irq_handler
	vector_entry	el1t_fiq_handler
	vector_entry	el1t_error_handler

	/*
	 * Exception taken from current Exception Level with SP_EL1.
	 * There are entries for exceptions caused in EL1 (kernel exceptions).
	 */
	vector_entry	el1h_sync_handler
	vector_entry	el1h_intr_handler
	vector_entry	el1h_fiq_handler
	vector_entry	el1h_error_handler

	/*
	 * Exception taken from lower Exception Level which is using AArch64.
	 * There are entries for exceptions caused in EL0 (native user exceptions).
	 */
	vector_entry	el0_sync_handler
	vector_entry	el0_intr_handler
	vector_entry	el0_fiq_handler
	vector_entry	el0_error_handler

	/*
	 * Exception taken from lower Exception Level which is using AArch32.
	 * There are entries for exceptions caused in EL0 (compat user exceptions).
	 */
	vector_entry	el0_32sync_handler
	vector_entry	el0_32intr_handler
	vector_entry	el0_32fiq_handler
	vector_entry	el0_32error_handler
END(el1_vectors)

	.macro unwind_x0_x2
	ldp	x0, x1, [sp, #TF_X0]
	ldr	x2, [sp, #TF_X2]
	.endm

	.macro unwind_x3_x30
	ldp	x3, x4, [sp, #TF_X3]
	ldp	x5, x6, [sp, #TF_X5]
	ldp	x7, x8, [sp, #TF_X7]
	ldp	x9, x10, [sp, #TF_X9]
	ldp	x11, x12, [sp, #TF_X11]
	ldp	x13, x14, [sp, #TF_X13]
	ldp	x15, x16, [sp, #TF_X15]
	ldp	x17, x18, [sp, #TF_X17]
	ldp	x19, x20, [sp, #TF_X19]
	ldp	x21, x22, [sp, #TF_X21]
	ldp	x23, x24, [sp, #TF_X23]
	ldp	x25, x26, [sp, #TF_X25]
	ldp	x27, x28, [sp, #TF_X27]
	ldp	x29, x30, [sp, #TF_X29]
	.endm

/*
 * EL1 exception return for trap and interrupt.
 */
#ifdef DDB
ENTRY_NP(el1_trap)
	nop				/* dummy for DDB backtrace (for lr-4) */
#endif
ENTRY_NP(el1_trap_exit)
	DISABLE_INTERRUPT		/* make sure I|F marked */

	unwind_x3_x30

#ifdef GIC_SPLFUNCS
	mrs	x0, tpidr_el1		/* get curlwp */
	ldr	x0, [x0, #L_CPU]	/* get curcpu */

	/*
	 * If ci_intr_depth == 0 and ci_splx_restart != 0, return
	 * to splx restart. Otherwise return to exception pc.
	 */
	ldr	w1, [x0, #CI_INTR_DEPTH]
	cbnz	w1, 1f
	ldr	x0, [x0, #CI_SPLX_RESTART]
	cbnz	x0, 2f
1:
	ldr	x0, [sp, #TF_PC]
2:
	ldr	x1, [sp, #TF_SPSR]
#elif TF_PC + 8 == TF_SPSR
	ldp	x0, x1, [sp, #TF_PC]
#else
	ldr	x0, [sp, #TF_PC]
	ldr	x1, [sp, #TF_SPSR]
#endif
	msr	elr_el1, x0		/* exception pc */
	msr	spsr_el1, x1		/* exception pstate */

	/*
	 * cpu_jump_onfault() modify tf->tf_sp, therefore
	 * we need to restore sp from trapframe,
	 * and unwind x0-x2 without sp.
	 */
	mov	x0, sp
	ldr	x1, [x0, #TF_SP]
	mov	sp, x1
	ldp	x1, x2, [x0, #TF_X1]
	ldr	x0, [x0, #TF_X0]

	ERET
END(el1_trap_exit)
#ifdef DDB
END(el1_trap)
#endif

/*
 * EL0 exception return for trap, interrupt and syscall with
 * possible AST processing.
 */
#ifdef DDB
ENTRY_NP(el0_trap)
	nop				/* dummy for DDB backtrace (for lr-4) */
#endif
ENTRY_NP(el0_trap_exit)

	adr	lr, 1f			/* return address from trap_doast */
1:
	/* while (curlwp->l_md.md_astpending != 0) { */
	DISABLE_INTERRUPT		/* make sure I|F marked */
	mrs	x9, tpidr_el1
	ldr	w8, [x9, #L_MD_ASTPENDING]
	cbz	w8, 9f

	/* curlwp->l_md.md_astpending = 0; */
	str	wzr, [x9, #L_MD_ASTPENDING]

	/*  trap_doast(tf); */
	ENABLE_INTERRUPT
	mov	x0, sp
	b	_C_LABEL(trap_doast)	/* tail call (return to 1b) */
	/* } */
9:

	/* x9 is tpidr_el1 */
	ldr	x23, [x9, #L_MD_CPACR]
	msr	cpacr_el1, x23		/* FP unit EL0 handover */
	isb				/* necessary? */

	ldr	x0, [x9, #L_PRIVATE]	/* tpidr_el0 = curlwp->l_private */
	msr	tpidr_el0, x0
#ifdef COMPAT_NETBSD32
	msr	tpidrro_el0, x0
#endif

#ifdef ARMV83_PAC
	/* Switch to the user PAC key. */
	adrl	x4, _C_LABEL(aarch64_pac_enabled)
	ldr	w4, [x4]
	cbz	w4, 1f
	ldp	x5, x6, [x9, #L_MD_IA_USER]
	msr	APIAKeyLo_EL1, x5
	msr	APIAKeyHi_EL1, x6
	isb
1:
#endif

	unwind_x3_x30

#if TF_PC + 8 == TF_SPSR
	ldp	x0, x1, [sp, #TF_PC]
#else
	ldr	x0, [sp, #TF_PC]
	ldr	x1, [sp, #TF_SPSR]
#endif
	ldr	x2, [sp, #TF_SP]
	msr	elr_el1, x0		/* exception pc */
	msr	spsr_el1, x1		/* exception pstate */
	msr	sp_el0, x2		/* restore EL0 stack */

	/* if the process is traced, enable MDSCR_EL1.SS */
	tbz	x1, #SPSR_SS_SHIFT, 1f
	mrs	x0, mdscr_el1
	orr	x0, x0, #MDSCR_SS
#ifdef DDB
	bic	x0, x0, #MDSCR_KDE
#endif
	msr	mdscr_el1, x0
1:
	unwind_x0_x2

	/* leave sp at l_md.md_utf, return back to EL0 user process */
	ERET
END(el0_trap_exit)
#ifdef DDB
END(el0_trap)
#endif
