/*	$NetBSD: locore.S,v 1.189.10.2 2017/12/03 11:36:28 jdolecek Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Digital Equipment Corporation and Ralph Campbell.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Copyright (C) 1989 Digital Equipment Corporation.
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appears in all copies.
 * Digital Equipment Corporation makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * from: Header: /sprite/src/kernel/mach/ds3100.md/RCS/loMem.s,
 *	v 1.1 89/07/11 17:55:04 nelson Exp  SPRITE (DECWRL)
 * from: Header: /sprite/src/kernel/mach/ds3100.md/RCS/machAsm.s,
 *	v 9.2 90/01/29 18:00:39 shirriff Exp  SPRITE (DECWRL)
 * from: Header: /sprite/src/kernel/vm/ds3100.md/vmPmaxAsm.s,
 *	v 1.1 89/07/10 14:27:41 nelson Exp  SPRITE (DECWRL)
 *
 *	@(#)locore.s	8.5 (Berkeley) 1/4/94
 */

#include "opt_cputype.h"	/* which mips CPU levels do we support? */
#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_lockdebug.h"
#include "opt_multiprocessor.h"

#include <sys/cdefs.h>
#include <sys/errno.h>

#include <mips/asm.h>
#include <mips/cpuregs.h>
#include <mips/trap.h>
#include <mips/locore.h>

RCSID("$NetBSD: locore.S,v 1.189.10.2 2017/12/03 11:36:28 jdolecek Exp $")

#include "assym.h"

	.set	noreorder

	.globl	start
	.globl	_C_LABEL(kernel_text)		# libkvm refers this
start:
_C_LABEL(kernel_text):
#if (defined(MIPS3_PLUS) && !defined(MIPS1)) || defined(emips)
	/* keep firmware exception handler until we hook. */
	mfc0	v0, MIPS_COP_0_STATUS
	MFC0_HAZARD
#if defined(emips)
	and	v0, MIPS_SR_TS | MIPS3_SR_RE
#else
	and	v0, MIPS_SR_BEV
#endif
	mtc0	v0, MIPS_COP_0_STATUS		# Disable interrupts
	COP0_SYNC
#else
	mtc0	zero, MIPS_COP_0_STATUS		# Disable interrupts
	COP0_SYNC
#endif
	mtc0	zero, MIPS_COP_0_CAUSE
	COP0_SYNC
#ifdef MIPS64_OCTEON
	//
	// U-boot on the erlite starts all cpus at the kernel entry point.
	// Use EBASE to find our CPU number and if it's not 0, call
	// octeon_cpu_spinup if MP or loop using the wait instruction since
	// non-primary CPUs can't do anything useful.
	//
	mfc0	a0, MIPS_COP_0_EBASE		# EBASE
	COP0_SYNC
	ext	a1, a0, 0, 10			# get CPU number
	beqz	a1, 2f				# normal startup if 0
	 nop
#ifdef MULTIPROCESSOR
	j	_C_LABEL(octeon_cpu_spinup)
	 nop
#else
1:	wait
	b	1b
	 nop
#endif /* MIPS64_OCTEON */
2:
#endif
/*
 * Initialize stack and call machine startup.
 */
	PTR_LA	v1, start
	slt	v0, v1, sp
	bne	v0, zero, 1f
	PTR_ADDU v0, v1, -CALLFRAME_SIZ
	PTR_SUBU v0, v1, sp
	slt	v0, v0, 4096			# within 4KB of _start
	beq	v0, zero, 2f
	PTR_ADDU v0, v1, -CALLFRAME_SIZ
1:
	move	sp, v0
2:
#ifdef __GP_SUPPORT__
	PTR_LA	gp, _C_LABEL(_gp)
#endif

#ifdef NOFPU /* No FPU; avoid touching FPU registers */
#if !defined(emips)   /*  XXX??? we have already disabled interrupts! */
#ifdef __mips_n32
	li	t0, MIPS_SR_KX			# turn on XKSEG and XKPHYS
#elif defined(_LP64)
	li	t0, MIPS_SR_KX | MIPS_SR_UX	# turn on XKSEG and XKPHYS
#else
	li	t0, 0				# Disable interrupts and
#endif /* n32 */
	mtc0	t0, MIPS_COP_0_STATUS		# the fp coprocessor
	COP0_SYNC
#endif /* !emips */
#ifdef HPCMIPS_L1CACHE_DISABLE
	mfc0	t0, MIPS_COP_0_CONFIG
	li	t1, 0xfffffff8
	and	t0, t0, t1
	or	t0, 0x00000002			# XXX, KSEG0 is uncached
	mtc0	t0, MIPS_COP_0_CONFIG
	COP0_SYNC
#endif /* HPCMIPS_L1CACHE_DISABLE */
#else /* NOFPU */
	mfc0	t0, MIPS_COP_0_STATUS
	MFC0_HAZARD
#if defined(_LP64) || defined(__mips_n32)
	or	t0, MIPS_SR_KX			# turn on XKSEG and XKPHYS
#endif
	or	t0, MIPS_SR_COP_1_BIT		# Disable interrupts, and
	mtc0	t0, MIPS_COP_0_STATUS		# enable the fp coprocessor
	COP0_HAZARD_FPUENABLE
#endif
	mfc0	t0, MIPS_COP_0_PRID		# read product revision ID
	COP0_SYNC
	nop
#ifdef NOFPU /* No FPU; avoid touching FPU registers */
	add	t1, zero, zero
#else
	cfc1	t1, MIPS_FPU_ID			# read FPU ID register
#endif
	INT_S	t0, _C_LABEL(mips_options)+MO_CPU_ID # save PRID register
	INT_S	t1, _C_LABEL(mips_options)+MO_FPU_ID # save FPU ID register
	PTR_LA	MIPS_CURLWP, _C_LABEL(lwp0)	# set curlwp
	jal	_C_LABEL(mach_init)		# mach_init(a0, a1, a2, a3)
	 nop

	# XXXuvm_lwp_getuarea
	PTR_L	sp, L_PCB(MIPS_CURLWP)		# switch to lwp0 stack
	nop
	PTR_ADDU sp, USPACE - TF_SIZ - CALLFRAME_SIZ
	/*
	 * Raise to IPLHIGH
	 */
	jal	_C_LABEL(splhigh_noprof)	# go to splhigh
	 nop
	/*
	 * Now enable interrupts (but they are all masked).
	 */
#if __mips_isa_rev >= 2
	ei
#else
	mfc0	v0, MIPS_COP_0_STATUS
	MFC0_HAZARD
	or	v0, MIPS_SR_INT_IE
	mtc0	v0, MIPS_COP_0_STATUS
#endif
	COP0_SYNC

	jal	_C_LABEL(main)			# main(void)
	nop
	PANIC("main() returned")		# main never returns
	.set	at
	.globl _C_LABEL(verylocore)
_C_LABEL(verylocore):

/*
 * struct lwp *cpu_switchto(struct lwp *cur, struct lwp *next)
 * Switch to the specified next LWP
 * Arguments:
 *	a0	the current LWP
 *	a1	the LWP to switch to
 * Returns:
 *	v0	the LWP we have switched from
 *
 * called at IPL_SCHED
 */
NESTED(cpu_switchto, CALLFRAME_SIZ, ra)
#if defined(PARANOIA)
	/*
	 * Make sure we are at IPL_SCHED
	 */
	PTR_L	v0, L_CPU(MIPS_CURLWP)
	INT_L	v1, CPU_INFO_CPL(v0)
#if __mips >= 32
	tnei	v1, IPL_SCHED
#else
	li	v0, IPL_SCHED
10:	bne	v0, v1, 10b
	 nop
#endif

	mfc0	t0, MIPS_COP_0_STATUS
	MFC0_HAZARD
	and	t0, MIPS_SR_INT_IE
#if __mips >= 32
	teqi	t0, 0
#else
11:	beqz	t0, 11b
	 nop
#endif
#endif /* PARANOIA */
	/*
	 * Save old context, unless the LWP is exiting.
	 */
	beq	a0, zero, 1f
	 nop
	PTR_L	a2, L_PCB(a0)			# a2 = pcb of old lwp
	mfc0	t0, MIPS_COP_0_STATUS
	REG_PROLOGUE
	REG_S	s0, PCB_CONTEXT+SF_REG_S0(a2)
	REG_S	s1, PCB_CONTEXT+SF_REG_S1(a2)
	REG_S	s2, PCB_CONTEXT+SF_REG_S2(a2)
	REG_S	s3, PCB_CONTEXT+SF_REG_S3(a2)
	REG_S	s4, PCB_CONTEXT+SF_REG_S4(a2)
	REG_S	s5, PCB_CONTEXT+SF_REG_S5(a2)
	REG_S	s6, PCB_CONTEXT+SF_REG_S6(a2)
	REG_S	s7, PCB_CONTEXT+SF_REG_S7(a2)
	#REG_S	t8, PCB_CONTEXT+SF_REG_T8(a2)	# no reason to save MIPS_CURLWP
	REG_S	sp, PCB_CONTEXT+SF_REG_SP(a2)
	REG_S	s8, PCB_CONTEXT+SF_REG_S8(a2)
	REG_S	ra, PCB_CONTEXT+SF_REG_RA(a2)
	REG_S	t0, PCB_CONTEXT+SF_REG_SR(a2)
#if defined(__mips_n32) || defined(__mips_n64)
	REG_S	gp, PCB_CONTEXT+SF_REG_GP(a2)
#endif
	REG_EPILOGUE
1:
#if defined(PARANOID_SPL)
	/*
	 * Verify interrupt configuration matches IPL_SCHED
	 */
	jal	_C_LABEL(splcheck)
	 nop
#endif /* PARANOID_SPL */

	move	s6, a0				# s6 = old lwp
	move	MIPS_CURLWP, a1			# t8 = new lwp
	PTR_SUBU sp, CALLFRAME_SIZ
	REG_S	ra, CALLFRAME_RA(sp)
	.mask	0x80000000, -4
/*
 * Switch to new context.
 */
	jal	_C_LABEL(mips_cpu_switch_resume)
	 move	a0, MIPS_CURLWP

	PTR_L	t2, L_CPU(MIPS_CURLWP)
	nop					# patchable load delay slot
	PTR_S	MIPS_CURLWP, CPU_INFO_CURLWP(t2)

	/* Check for restartable atomic sequences (RAS) */
	PTR_L	a0, L_PROC(MIPS_CURLWP)		# argument to ras_lookup
	PTR_L	s5, L_PCB(MIPS_CURLWP)		# XXXuvm_lwp_getuarea
	PTR_L	v1, P_RASLIST(a0)		# get raslist
	NOP_L					# load delay
	beqz	v1, 1f				#   skip call if empty
	 nop
	jal	_C_LABEL(ras_lookup)		# ras_lookup(p, pc)
	 PTR_L	a1, (USPACE - TF_SIZ - CALLFRAME_SIZ + TF_REG_EPC)(s5)
	PTR_ADDU v1, v0, 1
	beqz	v1, 1f				# branch if v0 + 1 == 0
	 nop
	PTR_S	v0, (USPACE - TF_SIZ - CALLFRAME_SIZ + TF_REG_EPC)(s5)
1:
	/* New context is now active */
	move	v0, s6				# Save return value (old lwp)
	REG_PROLOGUE
	REG_L	t0, PCB_CONTEXT+SF_REG_SR(s5)
#if defined(PARANOIA) && __mips >= 32
	and	t1, t0, MIPS_SR_INT_IE
	teqi	t1, 0
#elif defined(PARANOID_LOOP)
	and	t1, t0, MIPS_SR_INT_IE
2:	beqz	t1, 2b			# status reg should not differ
	 nop
#endif /* PARANOID_LOOP */
	DYNAMIC_STATUS_MASK(t0,ra)
	move	a0, s5
	REG_L	ra, PCB_CONTEXT+SF_REG_RA(a0)
	REG_L	s0, PCB_CONTEXT+SF_REG_S0(a0)
	REG_L	s1, PCB_CONTEXT+SF_REG_S1(a0)
	REG_L	s2, PCB_CONTEXT+SF_REG_S2(a0)
	REG_L	s3, PCB_CONTEXT+SF_REG_S3(a0)
	REG_L	s4, PCB_CONTEXT+SF_REG_S4(a0)
	REG_L	s5, PCB_CONTEXT+SF_REG_S5(a0)
	REG_L	s6, PCB_CONTEXT+SF_REG_S6(a0)
	REG_L	s7, PCB_CONTEXT+SF_REG_S7(a0)
	#REG_L	t8, PCB_CONTEXT+SF_REG_T8(a0)	# no reason to load MIPS_CURLWP
#if defined(__mips_n32) || defined(__mips_n64)
	REG_L	gp, PCB_CONTEXT+SF_REG_GP(a0)
#endif
	REG_L	sp, PCB_CONTEXT+SF_REG_SP(a0)
	REG_L	s8, PCB_CONTEXT+SF_REG_S8(a0)
	REG_EPILOGUE
	mtc0	t0, MIPS_COP_0_STATUS
#if defined(PARANOID_SPL)
	COP0_SYNC
	/*
	 * Verify interrupt configuration still matches IPL_SCHED
	 */
	j	_C_LABEL(splcheck)
	 nop
#else
	JR_HB_RA
#endif /* PARANOID_SPL */
END(cpu_switchto)

#ifdef __HAVE_FAST_SOFTINTS
/*
 * void softint_fast_dispatch(struct lwp *l, int s);
 *
 * called at IPL_HIGH
 *
 */
softint_cleanup:
#ifdef PARANOIA
	mfc0	t1, MIPS_COP_0_STATUS
	MFC0_HAZARD
	and	v0, t1, MIPS_SR_INT_IE
#if __mips >= 32
	teqi	v0, 0
#else
1:	beqz	v0, 1b
	 nop
#endif
#endif /* PARANOIA */
	PTR_L	t0, L_CPU(MIPS_CURLWP)
	NOP_L					# load delay
	INT_L	t1, CPU_INFO_MTX_COUNT(t0)
	NOP_L					# load delay
	INT_ADDU t1, 1
	INT_S	t1, CPU_INFO_MTX_COUNT(t0)
	REG_L	ra, CALLFRAME_RA(sp)
	REG_L	v0, CALLFRAME_S0(sp)		# get softint lwp
	NOP_L					# load delay
	PTR_S	zero, L_CTXSWITCH(v0)		# clear l_ctxswtch
#if IPL_SCHED != IPL_HIGH
	j	_C_LABEL(splhigh_noprof)
#else
	jr	ra
#endif
	 PTR_ADDU sp, CALLFRAME_SIZ

/*
 * Arguments:
 *	a0	the LWP to switch to
 *	a1	IPL to execute at
 */
NESTED(softint_fast_dispatch, CALLFRAME_SIZ, ra)
	PTR_SUBU sp, CALLFRAME_SIZ
	REG_S	a0, CALLFRAME_S0(sp)		# save softint lwp
	REG_S	ra, CALLFRAME_RA(sp)		# save return address
	.mask	0x80000000, -4
	PTR_L	t0, L_PCB(MIPS_CURLWP)		# t0 = curlwp->l_addr

	/*
	 * Save our state in case softint_dispatch blocks and get switched back
	 * to.
 	 */
	mfc0	t1, MIPS_COP_0_STATUS
#if defined(PARANOIA) && __mips >= 32
	MFC0_HAZARD
	and	v0, t1, MIPS_SR_INT_IE		# assert interrupts are on
	teqi	v0, 0
#elif defined(PARANOID_LOOP)
	MFC0_HAZARD
	and	v0, t1, MIPS_SR_INT_IE		# assert interrupts are on
1:	beqz	v0, 1b
	 nop
#endif /* PARANOID_LOOP */
	PTR_LA	t2, softint_cleanup		# if softint blocks, return here
	REG_PROLOGUE
	REG_S	s0, PCB_CONTEXT+SF_REG_S0(t0)
	REG_S	s1, PCB_CONTEXT+SF_REG_S1(t0)
	REG_S	s2, PCB_CONTEXT+SF_REG_S2(t0)
	REG_S	s3, PCB_CONTEXT+SF_REG_S3(t0)
	REG_S	s4, PCB_CONTEXT+SF_REG_S4(t0)
	REG_S	s5, PCB_CONTEXT+SF_REG_S5(t0)
	REG_S	s6, PCB_CONTEXT+SF_REG_S6(t0)
	REG_S	s7, PCB_CONTEXT+SF_REG_S7(t0)
	#REG_S	t8, PCB_CONTEXT+SF_REG_T8(t0)	# no reason to save MIPS_CURLWP
	REG_S	sp, PCB_CONTEXT+SF_REG_SP(t0)
	REG_S	s8, PCB_CONTEXT+SF_REG_S8(t0)
	REG_S	t2, PCB_CONTEXT+SF_REG_RA(t0)
	REG_S	t1, PCB_CONTEXT+SF_REG_SR(t0)
#if defined(__mips_n32) || defined(__mips_n64)
	REG_S	gp, PCB_CONTEXT+SF_REG_GP(t0)
#endif
	REG_EPILOGUE

	/*
	 * Switch to a fast softint thread.  We don't care about its existing
	 * state and we use a private KSEG0/XKPHYS mapped stack so don't have
	 * to do TLB manipulation.
	 */
	move	s0, MIPS_CURLWP				# remember current lwp
	move	MIPS_CURLWP, a0				# switch to softint lwp
	PTR_L	s1, L_CPU(MIPS_CURLWP)			# get curcpu()
	nop					# patchable load delay slot
	PTR_S	MIPS_CURLWP, CPU_INFO_CURLWP(s1)	#    ...
	move	s2, sp					# remember sp
	move	s3, t0					# remember curpcb

	PTR_L	t2, L_PCB(MIPS_CURLWP)
	move	a0, s0					# wants the pinned lwp
	jal	_C_LABEL(softint_dispatch)
	 PTR_ADDU sp, t2, USPACE - TF_SIZ - CALLFRAME_SIZ

	move	sp, s2					# restore stack
	move	MIPS_CURLWP, s0				# restore curlwp
	PTR_S	MIPS_CURLWP, CPU_INFO_CURLWP(s1)	#    ....

	REG_L	ra, CALLFRAME_RA(sp)		# load early since we use it

	REG_PROLOGUE
	REG_L	s0, PCB_CONTEXT+SF_REG_S0(s3)		# restore the saved
	REG_L	s1, PCB_CONTEXT+SF_REG_S1(s3)		#    registers that we
	REG_L	s2, PCB_CONTEXT+SF_REG_S2(s3)		#    used
	REG_L	s3, PCB_CONTEXT+SF_REG_S3(s3)
	REG_EPILOGUE

	/*
	 * Almost everything (all except sp) is restored so we can return.
	 */
	jr	ra
	 PTR_ADDU sp, CALLFRAME_SIZ
END(softint_fast_dispatch)
#endif /* __HAVE_FAST_SOFTINTS */

/*
 * int lwp_oncpu(lwp_t *);
 */
LEAF(lwp_oncpu)
	PTR_L	t0, L_PCB(MIPS_CURLWP)		# get curpcb
	li	v0, EFAULT			# assume failure
	PTR_LA	t1, 1f				# load addr of cleanup
	PTR_S	t1, PCB_ONFAULT(t0)		# save onfault handler
	PTR_L	t2, L_CPU(a0)			# grab cpu of supplied lwp
	NOP_L					# load delay
	PTR_L	t3, CPU_INFO_CURLWP(t2)		# grab curlwp of that cpu
	li	v0, ESRCH			# assume the lwp isn't curlwp
	bne	a0, t3, 1f			# branch if true (not equal)
	 nop
	PTR_S	t2, 0(a1)			# return the cpu_info
	li	v0, 0				# load success
1:
	PTR_S	zero, PCB_ONFAULT(t0)		# reset fault handler
	jr	ra				# and return.
	 nop
END(lwp_oncpu)


/*
 * void savectx(struct pcb *)
 */
LEAF(savectx)
	mfc0	v0, MIPS_COP_0_STATUS
#ifdef PARANOIA
	MFC0_HAZARD
	and	t0, v0, MIPS_SR_INT_IE
#if __mips >= 32
	teqi	t0, 0
#else
1:	beqz	t0, 1b
	 nop
#endif
#endif /* PARANOIA */
	REG_PROLOGUE
	REG_S	s0, PCB_CONTEXT+SF_REG_S0(a0)
	REG_S	s1, PCB_CONTEXT+SF_REG_S1(a0)
	REG_S	s2, PCB_CONTEXT+SF_REG_S2(a0)
	REG_S	s3, PCB_CONTEXT+SF_REG_S3(a0)
	REG_S	s4, PCB_CONTEXT+SF_REG_S4(a0)
	REG_S	s5, PCB_CONTEXT+SF_REG_S5(a0)
	REG_S	s6, PCB_CONTEXT+SF_REG_S6(a0)
	REG_S	s7, PCB_CONTEXT+SF_REG_S7(a0)
	REG_S	t8, PCB_CONTEXT+SF_REG_T8(a0)	# MIPS_CURLWP
#if defined(__mips_n32) || defined(__mips_n64)
	REG_S	gp, PCB_CONTEXT+SF_REG_GP(a0)
#endif
	REG_S	sp, PCB_CONTEXT+SF_REG_SP(a0)
	REG_S	s8, PCB_CONTEXT+SF_REG_S8(a0)
	REG_S	ra, PCB_CONTEXT+SF_REG_RA(a0)
	REG_S	v0, PCB_CONTEXT+SF_REG_SR(a0)
	REG_EPILOGUE
	jr	ra
	move	v0, zero
END(savectx)

#if defined(DDB) || defined(KGDB)
/*
 * setjmp(label_t *)
 * longjmp(label_t *)
 */
LEAF(setjmp)
	mfc0	v0, MIPS_COP_0_STATUS
	REG_PROLOGUE
	REG_S	s0, SF_REG_S0(a0)
	REG_S	s1, SF_REG_S1(a0)
	REG_S	s2, SF_REG_S2(a0)
	REG_S	s3, SF_REG_S3(a0)
	REG_S	s4, SF_REG_S4(a0)
	REG_S	s5, SF_REG_S5(a0)
	REG_S	s6, SF_REG_S6(a0)
	REG_S	s7, SF_REG_S7(a0)
	#REG_S	t8, SF_REG_T8(a0)		# no reason to save MIPS_CURLWP
#if defined(__mips_n32) || defined(__mips_n64)
	REG_S	gp, SF_REG_GP(a0)
#endif
	REG_S	sp, SF_REG_SP(a0)
	REG_S	s8, SF_REG_S8(a0)
	REG_S	ra, SF_REG_RA(a0)
	REG_S	v0, SF_REG_SR(a0)
	REG_EPILOGUE
	jr	ra
	move	v0, zero
END(setjmp)

LEAF(longjmp)
	REG_PROLOGUE
	REG_L	v0, SF_REG_SR(a0)
	DYNAMIC_STATUS_MASK(v0,ra)		# machine dependent masking
	REG_L	ra, SF_REG_RA(a0)
	REG_L	s0, SF_REG_S0(a0)
	REG_L	s1, SF_REG_S1(a0)
	REG_L	s2, SF_REG_S2(a0)
	REG_L	s3, SF_REG_S3(a0)
	REG_L	s4, SF_REG_S4(a0)
	REG_L	s5, SF_REG_S5(a0)
	REG_L	s6, SF_REG_S6(a0)
	REG_L	s7, SF_REG_S7(a0)
	#REG_L	t8, SF_REG_T8(a0)		# no reason to load MIPS_CURLWP
#if defined(__mips_n32) || defined(__mips_n64)
	REG_L	gp, SF_REG_GP(a0)
#endif
	REG_L	sp, SF_REG_SP(a0)
	REG_L	s8, SF_REG_S8(a0)
	REG_EPILOGUE
	mtc0	v0, MIPS_COP_0_STATUS
	COP0_SYNC
	jr	ra
	 li	v0, 1
END(longjmp)
#endif

/*
 * uint32_t mips_cp0_cause_read(void)
 *
 *	Return the current value of the CP0 Cause register.
 *
 *	Note: Not profiled, skews CPU-clock measurement (mips_mcclock.c)
 *	to uselessness.
 */
LEAF_NOPROFILE(mips_cp0_cause_read)
	mfc0	v0, MIPS_COP_0_CAUSE
	jr	ra
	 nop
END(mips_cp0_cause_read)

/*
 * void mips_cp0_cause_write(uint32_t)
 *
 *	Set the value of the CP0 Cause register.
 */
LEAF(mips_cp0_cause_write)
	mtc0	a0, MIPS_COP_0_CAUSE
	JR_HB_RA
END(mips_cp0_cause_write)


/*
 * uint32_t mips_cp0_status_read(void)
 *
 *	Return the current value of the CP0 Status register.
 */
LEAF(mips_cp0_status_read)
	mfc0	v0, MIPS_COP_0_STATUS
	jr	ra
	 nop
END(mips_cp0_status_read)

/*
 * void mips_cp0_status_write(uint32_t)
 *
 *	Set the value of the CP0 Status register.
 *
 *	Note: This is almost certainly not the way you want to write a
 *	"permanent" value to to the CP0 Status register, since it gets
 *	saved in trap frames and restores.
 */
LEAF(mips_cp0_status_write)
	mtc0	a0, MIPS_COP_0_STATUS
	JR_HB_RA
END(mips_cp0_status_write)

#if !defined(NOFPU) || defined(FPEMUL)
/*----------------------------------------------------------------------------
 *
 * mips_fpu_intr --
 * mips_fpu_trap --
 *
 *	Handle a floating point interrupt (r3k) or trap (r4k).
 *	the handlers are indentical, only the reporting mechanisms differ.
 *
 *	mips_fpu_intr(vaddr_t pc, struct trapframe *tf)
 *
 *	mips_fpu_trap(vaddr_t pc, struct trapframe *tf)
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------------
 */
NESTED(mips_fpu_intr, CALLFRAME_SIZ, ra)
XNESTED(mips_fpu_trap)
	.mask	0x80000000, -4
	PTR_SUBU	sp, CALLFRAME_SIZ
	mfc0		t0, MIPS_COP_0_STATUS
	REG_S		ra, CALLFRAME_RA(sp)
	or		t0, t0, MIPS_SR_COP_1_BIT
	mtc0		t0, MIPS_COP_0_STATUS
	COP0_HAZARD_FPUENABLE

	REG_PROLOGUE
	REG_L		a2, TF_REG_CAUSE(a1)
	REG_EPILOGUE

	cfc1		t0, MIPS_FPU_CSR	# stall til FP done
	cfc1		t0, MIPS_FPU_CSR	# now get status
	nop
	sll		t2, t0, (31 - 17)	# unimplemented operation?
	bgez		t2, 3f			# no, normal trap
	 nop

/*
 * We received an unimplemented operation trap so
 * fetch the instruction and emulate the instruction.
 *
 * We check whether it's an unimplemented FP instruction here rather
 * than invoking mips_emul_inst(), since it is faster.
 */
	srl		v1, a2, 31		# move branch delay bit to LSB
	sll		v1, 2			# shift it left by 2 (mult by 4)
	PTR_ADDU	a0, v1			# add to a0
	INT_L		a0, 0(a0)		# a0 = coproc instruction
	NOP_L					# load delay

/*
 * Check to see if the instruction to be emulated is a floating-point
 * instruction.
 */
	srl		t2, a0, MIPS_OPCODE_SHIFT
	beq		t2, MIPS_OPCODE_C1, 4f
	 nop

/*
 * Send an ILL signal to the current LWP if the instruction can't be emulated.
 */
	srl		a2, 8
	sll		a2, 8
	ori		a2, T_RES_INST << MIPS_CR_EXC_CODE_SHIFT
	REG_PROLOGUE
	REG_S		a2, TF_REG_CAUSE(a1)
	REG_EPILOGUE

	and		t2, t0, ~MIPS_FPU_EXCEPTION_BITS
	ctc1		t2, MIPS_FPU_CSR

	move		a1, a0				# code = instruction
	jal		_C_LABEL(mips_fpuillinst)
	 move		a0, MIPS_CURLWP			# get current LWP

	b		FPReturn
	 nop

/*
 * Send a FPE signal to the current LWP if it tripped the any of
 * the VZOUI bits.
 */
3:
	REG_PROLOGUE
	REG_S		a2, TF_REG_CAUSE(a1)
	REG_EPILOGUE

	and		a0, t0, ~MIPS_FPU_EXCEPTION_BITS
	ctc1		a0, MIPS_FPU_CSR

	move		a1, t0			# FPU status
	jal		_C_LABEL(mips_fpuexcept)
	 move		a0, MIPS_CURLWP		# get current LWP

	b		FPReturn
	 nop

/*
 * Finally, we can call
 * mips_emul_fp(uint32_t insn, struct trapframe *tf, uint32_t cause).
 */
4:
	jal		_C_LABEL(mips_emul_fp)
	 nop

/*
 * Turn off the floating point coprocessor and return.
 */
FPReturn:
	mfc0		t0, MIPS_COP_0_STATUS
	REG_L		ra, CALLFRAME_RA(sp)
	and		t0, ~MIPS_SR_COP_1_BIT
	mtc0		t0, MIPS_COP_0_STATUS
	COP0_SYNC
	j		ra
	 PTR_ADDU	sp, CALLFRAME_SIZ
END(mips_fpu_intr)
#endif /* !defined(NOFPU) || defined(FPEMUL) */

LEAF(mips_pagecopy)
	.set	push
#if defined(__mips_n32) || defined(_LP64)
	.set	mips3
#endif
	li		a2, PAGE_SIZE / (8 * SZREG)

1:	REG_L		t0,  (0*SZREG)(a1)
	REG_L		ta0, (4*SZREG)(a1)
	PTR_SUBU	a2, 1
	REG_L		t1,  (1*SZREG)(a1)
	REG_L		t2,  (2*SZREG)(a1)
	REG_L		t3,  (3*SZREG)(a1)
	REG_L		ta1, (5*SZREG)(a1)
	REG_L		ta2, (6*SZREG)(a1)
	REG_L		ta3, (7*SZREG)(a1)

	REG_S		t0,  (0*SZREG)(a0)
	REG_S		ta0, (4*SZREG)(a0)
	PTR_ADDU	a1, 8*SZREG
	REG_S		t1,  (1*SZREG)(a0)
	REG_S		t2,  (2*SZREG)(a0)
	REG_S		t3,  (3*SZREG)(a0)
	REG_S		ta1, (5*SZREG)(a0)
	REG_S		ta2, (6*SZREG)(a0)
	REG_S		ta3, (7*SZREG)(a0)
	bgtz		a2, 1b
	PTR_ADDU	a0, 8*SZREG
	.set	pop
	jr	ra
	nop
END(mips_pagecopy)

LEAF(mips_pagezero)
/* We can always safely store a 64-bit zero on MIPS3,4,64 */
	.set	push
#if (MIPS1 + MIPS32 + MIPS32R2) == 0
	.set	mips3
#endif
	li		a1, PAGE_SIZE / (8*SZREG)

1:	REG_S		zero, (0*SZREG)(a0)	# try to miss cache first
	REG_S		zero, (4*SZREG)(a0)
	subu		a1, 1
	REG_S		zero, (1*SZREG)(a0)	# fill in cache lines
	REG_S		zero, (2*SZREG)(a0)
	REG_S		zero, (3*SZREG)(a0)
	REG_S		zero, (5*SZREG)(a0)
	REG_S		zero, (6*SZREG)(a0)
	REG_S		zero, (7*SZREG)(a0)
	bgtz		a1,1b
	PTR_ADDU	a0, 8*SZREG
	.set	pop
	jr	ra
	nop
END(mips_pagezero)


#ifndef DDB_TRACE

#if defined(DEBUG) || defined(DDB) || defined(KGDB) || defined(geo)
/*
 * Stacktrace support hooks which use type punnign to access
 * the caller's registers.
 */


/*
 * stacktrace() -- print a stack backtrace to the console.
 *	implicitly accesses caller's a0-a3.
 */
#if defined(__mips_o32) || defined(__mips_o64)
#define	XCALLFRAME_SIZ		(CALLFRAME_SIZ + 6*SZREG)
#define	XCALLFRAME_RA		(CALLFRAME_RA  + 4*SZREG)
#endif
#if defined(__mips_n32) || defined(__mips_n64)
#define	XCALLFRAME_SIZ		(CALLFRAME_SIZ + 2*SZREG)
#define	XCALLFRAME_RA		(CALLFRAME_RA  + 2*SZREG)
#endif
NESTED(stacktrace, XCALLFRAME_SIZ, ra)
XNESTED(logstacktrace)
	PTR_SUBU sp, XCALLFRAME_SIZ		# four arg-passing slots
	move	t0, ra				# save caller's PC
	PTR_ADDU t1, sp, XCALLFRAME_SIZ		# save caller's SP
	move	t2, s8				# non-virtual frame pointer

	PTR_LA	v0, _C_LABEL(printf)

	REG_S	ra, XCALLFRAME_RA(sp)		# save return address
#if defined(__mips_o32) || defined(__mips_o64)
	/* a0-a3 are still caller's a0-a3, pass in-place as given. */
	REG_S	t0, 4*SZREG(sp)			# push caller's PC
	REG_S	t1, 5*SZREG(sp)			# push caller's SP
	REG_S	t2, 6*SZREG(sp)			# push caller's FP, in case
	REG_S	zero, 7*SZREG(sp)		# caller's RA on stack
	/* this uses the slot used for saving s0 in the callframe */
	jal	_C_LABEL(stacktrace_subr)
	 REG_S	v0, 8*SZREG(sp)			# push printf
#endif
#if defined(__mips_n32) || defined(__mips_n64)
	move	a4, t0				# pass caller's PC
	move	a5, t1				# pass caller's SP
	move	a6, t2				# pass caller's FP, in case
	move	a7, zero			# caller's RA on stack
	/* this uses the slot used for saving s0 in the callframe */
	jal	_C_LABEL(stacktrace_subr)
	 REG_S	v0, 0(sp)			# push printf
#endif

	REG_L	ra, XCALLFRAME_RA(sp)
	PTR_ADDU sp, XCALLFRAME_SIZ
	jr	ra
	nop
#undef XCALLFRAME_RA
#undef XCALLFRAME_SIZ
END(stacktrace)
#endif	/* DEBUG || DDB */
#endif	/* DDB_TRACE */

	.section .stub, "ax"
NESTED_NOPROFILE(tlb_update_addr, 0, ra)
	lui	v0,%hi(_C_LABEL(mips_locore_jumpvec)+LJV_TLB_UPDATE_ADDR)
	PTR_L	t9,%lo(_C_LABEL(mips_locore_jumpvec)+LJV_TLB_UPDATE_ADDR)(v0)
	jr	t9
	 nop
END(tlb_update_addr)

	.sdata
	.globl	_C_LABEL(esym)
_C_LABEL(esym):
	.word 0

#ifdef MIPS_DYNAMIC_STATUS_MASK
	.globl	_C_LABEL(mips_dynamic_status_mask)
_C_LABEL(mips_dynamic_status_mask):
	.word	0xffffffff
#endif
