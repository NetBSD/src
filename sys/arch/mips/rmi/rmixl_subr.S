/*	$NetBSD: rmixl_subr.S,v 1.1.2.5 2010/03/21 21:29:45 cliff Exp $	*/

#include "opt_cputype.h"
#include "opt_multiprocessor.h"

#include <sys/cdefs.h>

#include <mips/asm.h>
#include <mips/cpuregs.h>

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

#ifdef MULTIPROCESSOR
/*
 * rmixlfw_wakeup_cpu(func, args, mask, callback)
 */
NESTED(rmixlfw_wakeup_cpu, CALLFRAME_SIZ+4*SZREG, ra)
	PTR_ADDU	sp, sp, -(CALLFRAME_SIZ+4*SZREG)
	REG_S		ra, CALLFRAME_RA(sp)
	REG_S		s0, CALLFRAME_S0(sp)
	REG_S		gp, CALLFRAME_SIZ+0*SZREG(sp)
	REG_S		t8, CALLFRAME_SIZ+1*SZREG(sp)
	mfc0		t0, MIPS_COP_0_STATUS
	REG_S		t0, CALLFRAME_SIZ+2*SZREG(sp)

	move		s0, sp			/* save sp */
	srl		t0, sp, 0		/* nuke upper half */
	li		t1, MIPS_KSEG0_START
	jalr		a3			/* callback to firmware */
	 or		sp, t0, t1		/* set MIPS_KSEG0_START */
	move		sp, s0			/* restore sp */

	REG_L		t0, CALLFRAME_SIZ+2*SZREG(sp)
	mtc0		t0, MIPS_COP_0_STATUS
	REG_L		t8, CALLFRAME_SIZ+1*SZREG(sp)
	REG_L		gp, CALLFRAME_SIZ+0*SZREG(sp)
	REG_L		s0, CALLFRAME_S0(sp)
	REG_L		ra, CALLFRAME_RA(sp)
	jr		ra
	 PTR_ADDU	sp, sp, (CALLFRAME_SIZ+4*SZREG)
END(rmixlfw_wakeup_cpu)

/*
 * rmixl_cpu_trampoline - entry point for subordinate (non-#0) CPU wakeup
 */
NESTED(rmixl_cpu_trampoline, CALLFRAME_SIZ, ra)
#ifdef _LP64
	li		t0, MIPS_SR_KX
#else
	li		t0, 0
#endif

	mtc0		zero, $9, 7	/* disable all in MIPS_COP_0_EIMR */

	or		t0, MIPS_SR_INT_IE
	mtc0		t0, MIPS_COP_0_STATUS

	/* ensure COP_0_EBASE field 'EBASE' is 0 */
	mfc0		t0, $15, 1	/* MIPS_COP_0_EBASE */
	and		t0, t0, 0x3ff
	mtc0		t0, $15, 1	/* MIPS_COP_0_EBASE */

	/* XXX reconstruct presumed upper half of trampoline args addr */
	li		t1, MIPS_KSEG0_START
	or		a0, a0, t1

	/*
	 * load our stack pointer from trampoline args
	 */
	PTR_L		sp, 0*SZREG(a0)		/* XXX ta_sp */

	/*
	 * load our (idle) lwp from trampoline args
	 * save in t8 reg dedicated as 'mips_curlwp'
	 */
	PTR_L		t8, 1*SZREG(a0)		/* XXX ta_lwp */

	/*
	 * load our ta_cpuinfo from trampoline args and pass in a1
	 * jump to common mips cpu_trampoline
	 */
	PTR_L		a1, 2*SZREG(a0)		/* XXX ta_cpuinfo */
	j		cpu_trampoline
	 mtc0		a1, $22, 0		/* MIPS_COP_0_OSSCRATCH */

	/* NOTREACHED */

END(rmixl_cpu_trampoline)

#endif	/* MULTIPROCESSOR */

	.set	pop

