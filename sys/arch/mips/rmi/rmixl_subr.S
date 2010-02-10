/*	$NetBSD: rmixl_subr.S,v 1.1.2.4 2010/02/10 19:45:48 cliff Exp $	*/

#include "opt_cputype.h"

#include <sys/cdefs.h>

#include <mips/cpuregs.h>
#include <mips/asm.h>

#include "assym.h"

	.set	push
        .set    noreorder
        .set    mips64
        .text

/*
 * read XLS Processor Control register
 *
 * uint64_t rmixl_mfcr(u_int cr);
 */
LEAF(rmixl_mfcr)
#if defined(__mips_o32)
#error O32 not supported
#endif
	j	ra
	mfcr	v0, a0
END(rmixl_mfcr)

/*
 * write XLS Processor Control register
 *
 * void rmixl_mtcr(u_int cr, uint64_t val);
 */
LEAF(rmixl_mtcr)
#if defined(__mips_o32)
#error O32 not supported
#endif
	j	ra
	mtcr	a1, a0
END(rmixl_mtcr)

/*
 * rmixlfw_wakeup_cpu(func, args, mask, callback)
 */
NESTED(rmixlfw_wakeup_cpu, CALLFRAME_SIZ+2*SZREG, ra)
	PTR_ADDU	sp, sp, -(CALLFRAME_SIZ+2*SZREG)
	REG_S		ra, CALLFRAME_RA(sp)
	REG_S		s0, CALLFRAME_S0(sp)
	REG_S		gp, CALLFRAME_SIZ+0*SZREG(sp)
	REG_S		t8, CALLFRAME_SIZ+1*SZREG(sp)

	move		s0, sp			/* save sp */
	srl		t0, sp, 0		/* nuke upper half */
	li		t1, MIPS_KSEG0_START
	jalr		a3			/* callback to firmware */
	 or		sp, t0, t1		/* delay slot */
	move		sp, s0			/* restore sp */

	REG_L		t8, CALLFRAME_SIZ+1*SZREG(sp)
	REG_L		gp, CALLFRAME_SIZ+0*SZREG(sp)
	REG_L		s0, CALLFRAME_S0(sp)
	REG_L		ra, CALLFRAME_RA(sp)
	jr		ra
	 PTR_ADDU	sp, sp, (CALLFRAME_SIZ+2*SZREG)	/* delay slot */
END(rmixlfw_wakeup_cpu)

/*
 * cpu_wakeup_trampoline
 */
NESTED(cpu_wakeup_trampoline, CALLFRAME_SIZ, ra)
	j	.
	nop
END(cpu_wakeup_trampoline)

	.set	pop
