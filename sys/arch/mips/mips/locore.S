/*	$NetBSD: locore.S,v 1.156 2003/08/07 16:28:32 agc Exp $	*/

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
#include "opt_ns.h"
#include "opt_ccitt.h"
#include "opt_iso.h"
#include "opt_lockdebug.h"
#include "opt_multiprocessor.h"
#include "fs_coda.h"

#include <sys/cdefs.h>

#include <machine/param.h>
#include <mips/asm.h>
#include <mips/cpuregs.h>
#include <mips/trap.h>

#include "assym.h"

	.set	noreorder

	.globl	start
	.globl	_C_LABEL(kernel_text)		# libkvm refers this
start:
_C_LABEL(kernel_text):
#if defined(MIPS3_PLUS) && !defined(MIPS1)
	/* keep firmware exception handler until we hook. */
	mfc0	v0, MIPS_COP_0_STATUS
	and	v0, MIPS_SR_BEV
	mtc0	v0, MIPS_COP_0_STATUS		# Disable interrupts
	COP0_SYNC
#else
	mtc0	zero, MIPS_COP_0_STATUS		# Disable interrupts
	COP0_SYNC
#endif
/*
 * Initialize stack and call machine startup.
 */
	la	v1, start
	slt	v0, v1, sp
	bne	v0, zero, 1f
	addu	v0, v1, -CALLFRAME_SIZ
	subu	v0, v1, sp
	slt	v0, v0, 4096			# within 4KB of _start
	beq	v0, zero, 2f
	addu	v0, v1, -CALLFRAME_SIZ
1:
	move	sp, v0
2:
#ifdef __GP_SUPPORT__
	la	gp, _C_LABEL(_gp)
#endif

#ifdef NOFPU /* No FPU; avoid touching FPU registers */
	li	t0, 0				# Disable interrupts and
	mtc0	t0, MIPS_COP_0_STATUS		# the fp coprocessor
	COP0_SYNC
#ifdef HPCMIPS_L1CACHE_DISABLE
	mfc0	t0, MIPS_COP_0_CONFIG
	li	t1, 0xfffffff8
	and	t0, t0, t1
	or	t0, 0x00000002			# XXX, KSEG0 is uncached
	mtc0	t0, MIPS_COP_0_CONFIG
	COP0_SYNC
#endif /* HPCMIPS_L1CACHE_DISABLE */
#else
	mfc0	t0, MIPS_COP_0_STATUS
	or	t0, MIPS_SR_COP_1_BIT		# Disable interrupts, and
	mtc0	t0, MIPS_COP_0_STATUS		# enable the fp coprocessor
	COP0_HAZARD_FPUENABLE
#endif
	nop
	nop
	mfc0	t0, MIPS_COP_0_PRID		# read product revision ID
	nop					# XXX r4000 pipeline:
	nop					# wait for new SR
	nop					# to be effective
	nop
#ifdef NOFPU /* No FPU; avoid touching FPU registers */
	add	t1, zero, zero
#else
	cfc1	t1, MIPS_FPU_ID			# read FPU ID register
#endif
	sw	t0, _C_LABEL(cpu_id)		# save PRID register
	sw	t1, _C_LABEL(fpu_id)		# save FPU ID register
	jal	_C_LABEL(mach_init)		# mach_init(a0, a1, a2, a3)
	nop

	lw	sp, _C_LABEL(proc0paddr)	# switch to proc0 stack
	nop
	addu	sp, sp, USPACE - FRAME_SIZ - CALLFRAME_SIZ
	jal	_C_LABEL(main)			# main(void)
	nop
	PANIC("main() returned")		# main never returns
	.set	at
	.globl _C_LABEL(verylocore)
_C_LABEL(verylocore):

/*
 * When no processes are on the runq, cpu_switch branches to idle
 * to wait for something to come ready.
 * Note: this is really a part of cpu_switch() but defined here for kernel
 * profiling.
 */
LEAF(mips_idle)
	sw	zero, _C_LABEL(curlwp)		# set curlwp NULL for stats
#if defined(LOCKDEBUG)
	jal	_C_LABEL(sched_unlock_idle)	# release sched_lock
	nop
#endif
#ifdef 	IPL_ICU_MASK
	# all interrupts enable.
	sw	zero, _C_LABEL(md_imask)
	jal	_C_LABEL(md_imask_update)
	nop
#endif
	li	t0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
	DYNAMIC_STATUS_MASK(t0,t1)		# machine dependent masking
	mtc0	t0, MIPS_COP_0_STATUS		# enable all interrupts
	COP0_SYNC
	nop

	/* Try to zero some free pages. */
	lw	t0, _C_LABEL(uvm) + UVM_PAGE_IDLE_ZERO
	nop
	beq	t0, zero, 1f
	nop
	jal	_C_LABEL(uvm_pageidlezero)
	nop
1:
#ifdef MIPS_DYNAMIC_STATUS_MASK
	# Do this again since the mask may have changed.
	li	t3, (MIPS_INT_MASK | MIPS_SR_INT_IE)
	DYNAMIC_STATUS_MASK(t3,t1)		# machine dependent masking
	mtc0	t3, MIPS_COP_0_STATUS		# enable all interrupts
	COP0_SYNC
	nop
#endif
	lw	t0, _C_LABEL(sched_whichqs)	# look for non-empty queue
	nop
	beq	t0, zero, 1b
	nop
	mtc0	zero, MIPS_COP_0_STATUS		# disable all interrupts
	COP0_SYNC
	nop
	nop
	nop
#if defined(LOCKDEBUG)
	nop
	jal	_C_LABEL(sched_lock_idle)	# acquire sched_lock
	nop
#endif
	la	ra, cpu_switch_queuescan
	j	ra
	nop
END(mips_idle)

/*
 * cpu_switch(struct lwp *)
 * Find the highest priority process and resume it.
 * Arguments:
 *	a0	'struct lwp *' of the current LWP
 *
 * Returns:
 *      v0	0 = no context switch was done
 *              1 = switched to another context then restored
 */
NESTED(cpu_switch, CALLFRAME_SIZ, ra)
	lw	a2, L_ADDR(a0)			# a2 = l->l_addr
	mfc0	t0, MIPS_COP_0_STATUS
	REG_PROLOGUE
	REG_S	s0, U_PCB_CONTEXT+SF_REG_S0(a2)
	REG_S	s1, U_PCB_CONTEXT+SF_REG_S1(a2)
	REG_S	s2, U_PCB_CONTEXT+SF_REG_S2(a2)
	REG_S	s3, U_PCB_CONTEXT+SF_REG_S3(a2)
	REG_S	s4, U_PCB_CONTEXT+SF_REG_S4(a2)
	REG_S	s5, U_PCB_CONTEXT+SF_REG_S5(a2)
	REG_S	s6, U_PCB_CONTEXT+SF_REG_S6(a2)
	REG_S	s7, U_PCB_CONTEXT+SF_REG_S7(a2)
	REG_S	sp, U_PCB_CONTEXT+SF_REG_SP(a2)
	REG_S	s8, U_PCB_CONTEXT+SF_REG_S8(a2)
	REG_S	ra, U_PCB_CONTEXT+SF_REG_RA(a2)
	REG_S	t0, U_PCB_CONTEXT+SF_REG_SR(a2)
#ifdef IPL_ICU_MASK
	lw	t0, _C_LABEL(md_imask)
	sw	t0, U_PCB_PPL(a2)
#endif
	REG_EPILOGUE
	subu	sp, sp, CALLFRAME_SIZ
	sw	ra, CALLFRAME_RA(sp)
	.mask	0x80000000, -4

	move	s0, a0				# s0 = old proc
/*
 * Entered here from idle() and switch_exit().  Interrupts are
 * blocked, and sched_lock is held.
 */
	.globl	cpu_switch_queuescan
cpu_switch_queuescan:
	lw	t0, _C_LABEL(sched_whichqs)	# look for non-empty queue
	li	t2, -1				# t2 = lowest bit set
	bne	t0, zero, 1f
	lw	t3, _C_LABEL(mips_locoresw) + MIPSX_CPU_IDLE
	nop					# for r2000/r3000
	jal	ra, t3
	nop
1:
	move	t3, t0				# t3 = saved whichqs
1:
#if defined(MIPS3_5900)	/* work around for branch prediction miss. */
	nop
	nop
	nop
#endif
	addu	t2, t2, 1
	and	t1, t0, 1			# bit set?
	beq	t1, zero, 1b
	srl	t0, t0, 1			# try next bit
/*
 * Remove process from queue.
 */
	sll	t0, t2, 3
	la	t1, _C_LABEL(sched_qs)
	addu	t0, t0, t1			# t0 = qp = &qs[highbit]
	lw	a0, L_FORW(t0)			# a0 = l = highest pri process
	nop
	lw	v0, L_FORW(a0)			# v0 = l->l_forw
	bne	t0, a0, 2f			# make sure something in queue
	sw	v0, L_FORW(t0)			# qp->ph_link = p->p_forw;
	PANIC("cpu_switch")			# nothing in queue
2:
	sw	t0, L_BACK(v0)			# l->l_forw->l_back = qp
	bne	v0, t0, 3f			# queue still not empty
	sw	zero, L_BACK(a0)		## for firewall checking
	li	v1, 1				# compute bit in 'whichqs'
	sll	v1, v1, t2
	xor	t3, t3, v1			# clear bit in 'whichqs'
	sw	t3, _C_LABEL(sched_whichqs)
3:
	/* Squirrel away proc pointer into s7 */
	move	s7, a0
#if defined(LOCKDEBUG)
	/*
	 * Done mucking with the run queues, release the
	 * scheduler lock, but keep interrupts out.
	 */
	jal	_C_LABEL(sched_unlock_idle)
	nop
#endif

	/*
	 * regardless of whether a context switch is required, we need
	 * to update curlwp and other globals.  We must do this even
	 * if switching to ourselves because we might have re-entered
	 * cpu_switch() from idle() - in which case curlwp would be NULL
	 * This needs to be set before conext switch on MIPS
	 */
	li	v1, LSONPROC
	sw	v1, L_STAT(s7)			# l->l_stat = SONPROC
	sw	s7, _C_LABEL(curlwp)		# curlwp = l

	/*
	 * Check to see if we're switching to ourself.  If we are,
	 * don't bother loading the new context.
	 *
	 * Note that even if we re-enter cpu_switch() from idle(),
	 * s0 will still contain the old curlwp value because any
	 * users of that register between then and now must have
	 * saved it.  Also note that switch_exit() ensures that
	 * s0 is clear before jumping here to find a new process.
	 */
	beq	s0, s7, 4f			# oldproc == newproc ??
	li	s5, 0				# BDslot: didn't switch

/*
 * Switch to new context.
 */
#if defined(MULTIPROCESSOR)
	/*
	 * XXXSMP
	 * p->p_cpu = curcpu();
	 */
#endif
	lw	t2, _C_LABEL(mips_locoresw) + MIPSX_CPU_SWITCH_RESUME
	move	a0, s7
	jal	ra, t2
	nop

#if 1	/* XXX XXX XXX */
	REG_PROLOGUE
	lw	a0, L_ADDR(s7)
	nop
	REG_L	sp, U_PCB_CONTEXT+SF_REG_SP(a0)
	nop
	REG_EPILOGUE
#endif	/* XXX XXX XXX */

	jal	_C_LABEL(pmap_activate)
	move	a0, s7				# -BDSLOT-

	/*
	 * Check for restartable atomic sequences (RAS)
	 */
	lw	t1, L_PROC(s7)
	lw	a0, L_ADDR(s7)
	lw	v1, P_NRAS(t1)

	addu	t0, a0, USPACE - FRAME_SIZ
	beq	v1, zero, 1f
	nop
	move	a0, t1
	jal	_C_LABEL(ras_lookup)
	lw	a1, FRAME_EPC(t0)
	lw	a0, L_ADDR(s7)
	li	v1, -1
	beq	v1, v0, 1f
	addu	t0, a0, USPACE - FRAME_SIZ
	sw	v0, FRAME_EPC(t0)
1:
	li	s5, 1				# remember that we switched
4:
	/* New context is now active */
	lw	a0, L_ADDR(s7)
	sw	zero, _C_LABEL(want_resched)	# we've context switched
	sw	a0, _C_LABEL(curpcb)

#ifdef IPL_ICU_MASK
	# restore ICU state
	lw	t0, U_PCB_PPL(a0)
	sw	t0, _C_LABEL(md_imask)
	jal	_C_LABEL(md_imask_update)
	nop
	lw	a0, L_ADDR(s7)			# restore pcb_context pointer.
#endif /* IPL_ICU_MASK */
	move	v0, s5				# Save return value
	REG_PROLOGUE
	REG_L	t0, U_PCB_CONTEXT+SF_REG_SR(a0)
	DYNAMIC_STATUS_MASK(t0,ra)		# machine dependent masking
	REG_L	ra, U_PCB_CONTEXT+SF_REG_RA(a0)
	REG_L	s0, U_PCB_CONTEXT+SF_REG_S0(a0)
	REG_L	s1, U_PCB_CONTEXT+SF_REG_S1(a0)
	REG_L	s2, U_PCB_CONTEXT+SF_REG_S2(a0)
	REG_L	s3, U_PCB_CONTEXT+SF_REG_S3(a0)
	REG_L	s4, U_PCB_CONTEXT+SF_REG_S4(a0)
	REG_L	s5, U_PCB_CONTEXT+SF_REG_S5(a0)
	REG_L	s6, U_PCB_CONTEXT+SF_REG_S6(a0)
	REG_L	s7, U_PCB_CONTEXT+SF_REG_S7(a0)
	REG_L	sp, U_PCB_CONTEXT+SF_REG_SP(a0)
	REG_L	s8, U_PCB_CONTEXT+SF_REG_S8(a0)
	REG_EPILOGUE
	mtc0	t0, MIPS_COP_0_STATUS
	COP0_SYNC
	j	ra
	nop
END(cpu_switch)


/*
 * cpu_switchto(struct lwp *cur, struct lwp *next)
 * Switch to the specified next LWP
 * Arguments:
 *	a0	'struct lwp *' of the current LWP
 *	a1	'struct lwp *' of the LWP to switch to
 */
NESTED(cpu_switchto, CALLFRAME_SIZ, ra)
	lw	a2, L_ADDR(a0)
	mfc0	t0, MIPS_COP_0_STATUS
	REG_PROLOGUE
	REG_S	s0, U_PCB_CONTEXT+SF_REG_S0(a2)
	REG_S	s1, U_PCB_CONTEXT+SF_REG_S1(a2)
	REG_S	s2, U_PCB_CONTEXT+SF_REG_S2(a2)
	REG_S	s3, U_PCB_CONTEXT+SF_REG_S3(a2)
	REG_S	s4, U_PCB_CONTEXT+SF_REG_S4(a2)
	REG_S	s5, U_PCB_CONTEXT+SF_REG_S5(a2)
	REG_S	s6, U_PCB_CONTEXT+SF_REG_S6(a2)
	REG_S	s7, U_PCB_CONTEXT+SF_REG_S7(a2)
	REG_S	sp, U_PCB_CONTEXT+SF_REG_SP(a2)
	REG_S	s8, U_PCB_CONTEXT+SF_REG_S8(a2)
	REG_S	ra, U_PCB_CONTEXT+SF_REG_RA(a2)
	REG_S	t0, U_PCB_CONTEXT+SF_REG_SR(a2)
#ifdef IPL_ICU_MASK
	lw	t0, _C_LABEL(md_imask)
	sw	t0, U_PCB_PPL(a2)
#endif
	REG_EPILOGUE
	subu	sp, sp, CALLFRAME_SIZ
	sw	ra, CALLFRAME_RA(sp)
	.mask	0x80000000, -4

	move	s0, a0				# s0 = old proc
	move	s7, a1				# s7 = newproc

#if defined(LOCKDEBUG)
	/*
	 * Done mucking with the run queues, release the
	 * scheduler lock, but keep interrupts out.
	 */
	jal	_C_LABEL(sched_unlock_idle)
	nop
	move	a0, s7				# restore proc
#endif
	/*
	 * regardless of whether a context switch is required, we need
	 * to update curlwp and other globals.  We must do this even
	 * if switching to ourselves because we might have re-entered
	 * cpu_switch() from idle() - in which case curlwp would be NULL
	 * This needs to be set before conext switch on MIPS
	 */
	li	v1, LSONPROC
	sw	v1, L_STAT(s7)			# l->l_stat = SONPROC
	sw	s7, _C_LABEL(curlwp)		# curlwp = l

	/*
	 * Check to see if we're switching to ourself.  If we are,
	 * don't bother loading the new context.
	 *
	 * Note that even if we re-enter cpu_switch() from idle(),
	 * s0 will still contain the old curlwp value because any
	 * users of that register between then and now must have
	 * saved it.  Also note that switch_exit() ensures that
	 * s0 is clear before jumping here to find a new process.
	 */
	beq	s0, s7, 4f			# oldproc == newproc ??
	li	s5, 0				# BDslot: didn't switch

/*
 * Switch to new context.
 */
#if defined(MULTIPROCESSOR)
	/*
	 * XXXSMP
	 * p->p_cpu = curcpu();
	 */
#endif
	lw	t2, _C_LABEL(mips_locoresw) + MIPSX_CPU_SWITCH_RESUME
	move	a0, s7
	jal	ra, t2
	nop

#if 1	/* XXX XXX XXX */
	REG_PROLOGUE
	lw	a0, L_ADDR(s7)
	nop
	REG_L	sp, U_PCB_CONTEXT+SF_REG_SP(a0)
	nop
	REG_EPILOGUE
#endif	/* XXX XXX XXX */

	jal	_C_LABEL(pmap_activate)
	move	a0, s7				# -BDSLOT-
	li	s5, 1				# remember that we switched

4:
	/* New context is now active */
	lw	a0, L_ADDR(s7)
	sw	zero, _C_LABEL(want_resched)	# we've context switched
	sw	a0, _C_LABEL(curpcb)

#ifdef IPL_ICU_MASK
	# restore ICU state
	lw	t0, U_PCB_PPL(a0)
	sw	t0, _C_LABEL(md_imask)
	jal	_C_LABEL(md_imask_update)
	nop
	lw	a0, L_ADDR(s7)			# restore pcb_context pointer.
#endif /* IPL_ICU_MASK */
	move	v0, s5				# Save return value
	REG_PROLOGUE
	REG_L	t0, U_PCB_CONTEXT+SF_REG_SR(a0)
	DYNAMIC_STATUS_MASK(t0,ra)		# machine dependent masking
	REG_L	ra, U_PCB_CONTEXT+SF_REG_RA(a0)
	REG_L	s0, U_PCB_CONTEXT+SF_REG_S0(a0)
	REG_L	s1, U_PCB_CONTEXT+SF_REG_S1(a0)
	REG_L	s2, U_PCB_CONTEXT+SF_REG_S2(a0)
	REG_L	s3, U_PCB_CONTEXT+SF_REG_S3(a0)
	REG_L	s4, U_PCB_CONTEXT+SF_REG_S4(a0)
	REG_L	s5, U_PCB_CONTEXT+SF_REG_S5(a0)
	REG_L	s6, U_PCB_CONTEXT+SF_REG_S6(a0)
	REG_L	s7, U_PCB_CONTEXT+SF_REG_S7(a0)
	REG_L	sp, U_PCB_CONTEXT+SF_REG_SP(a0)
	REG_L	s8, U_PCB_CONTEXT+SF_REG_S8(a0)
	REG_EPILOGUE
	mtc0	t0, MIPS_COP_0_STATUS
	COP0_SYNC
	j	ra
	nop
END(cpu_switchto)

/*
 * switch_exit(struct lwp *, void (*exit)(struct lwp *))
 *
 * Make the named process exit.  Switch SP to proc0 stack, then
 * call exit2() to schedule exiting proc's vmspace and stack to be
 * released by the reaper thread.  MUST BE CALLED AT SPLHIGH.
 */
LEAF(switch_exit)
	lw	v0, _C_LABEL(proc0paddr)		# get proc0 l_addr
	nop
	sw	v0, _C_LABEL(curpcb)		# set current pcb
	REG_PROLOGUE
	REG_L	sp, U_PCB_CONTEXT+SF_REG_SP(v0)	# restore stack pointer
	REG_EPILOGUE
	jal	ra, a1				# lwp already in a0
	nop
#if defined(LOCKDEBUG)
	jal	_C_LABEL(sched_lock_idle)	# acquire sched_lock
	nop
#endif
	/*
	 * Now jump back into the middle of cpu_switch().  Note that
	 * we must clear s0 to guarantee that the check for switching
	 * to ourselves in cpu_switch() will fail.  This is safe since
	 * s0 will be restored when a new process is resumed.
	 */
	li	s0, 0

	la	ra, cpu_switch_queuescan	# rathole to cpu_switch()
	j	ra
	sub	sp, sp, CALLFRAME_SIZ		#BDSlot: set stack call frame
END(switch_exit)

/*
 * savectx(struct user *up)
 */
LEAF(savectx)
	mfc0	v0, MIPS_COP_0_STATUS
	REG_PROLOGUE
	REG_S	s0, U_PCB_CONTEXT+SF_REG_S0(a0)
	REG_S	s1, U_PCB_CONTEXT+SF_REG_S1(a0)
	REG_S	s2, U_PCB_CONTEXT+SF_REG_S2(a0)
	REG_S	s3, U_PCB_CONTEXT+SF_REG_S3(a0)
	REG_S	s4, U_PCB_CONTEXT+SF_REG_S4(a0)
	REG_S	s5, U_PCB_CONTEXT+SF_REG_S5(a0)
	REG_S	s6, U_PCB_CONTEXT+SF_REG_S6(a0)
	REG_S	s7, U_PCB_CONTEXT+SF_REG_S7(a0)
	REG_S	sp, U_PCB_CONTEXT+SF_REG_SP(a0)
	REG_S	s8, U_PCB_CONTEXT+SF_REG_S8(a0)
	REG_S	ra, U_PCB_CONTEXT+SF_REG_RA(a0)
	REG_S	v0, U_PCB_CONTEXT+SF_REG_SR(a0)
	REG_EPILOGUE
	j	ra
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
	REG_S	sp, SF_REG_SP(a0)
	REG_S	s8, SF_REG_S8(a0)
	REG_S	ra, SF_REG_RA(a0)
	REG_S	v0, SF_REG_SR(a0)
	REG_EPILOGUE
	j	ra
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
	REG_L	sp, SF_REG_SP(a0)
	REG_L	s8, SF_REG_S8(a0)
	REG_EPILOGUE
	mtc0	v0, MIPS_COP_0_STATUS
	COP0_SYNC
	j	ra
	li	v0, 1
END(longjmp)
#endif


/*
 * MIPS processor interrupt control
 *
 * Used as building blocks for spl(9) kernel interface.
 */
LEAF(_splraise)
XLEAF(_splraise_noprof)				# does not get mcount hooks
	mfc0	v0, MIPS_COP_0_STATUS		# fetch status register
	and	a0, a0, MIPS_INT_MASK		# extract INT bits
	nor	a0, zero, a0			# bitwise inverse of A0
	and	a0, a0, v0			# disable retaining other bits
	DYNAMIC_STATUS_MASK(a0,t0)		# machine dependent masking
	mtc0	a0, MIPS_COP_0_STATUS		# store back
	COP0_SYNC
	and	v0, v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
	j	ra
	nop
END(_splraise)

LEAF(_spllower)
	mfc0	v0, MIPS_COP_0_STATUS		# fetch status register
	li	v1, ~MIPS_INT_MASK
	and	v1, v0, v1			# turn off INT bit
	nor	a0, zero, a0			# bitwise inverse of A0
	and	a0, a0, MIPS_INT_MASK		# extract INT bits
	or	a0, a0, v1			# disable making other bits on
	DYNAMIC_STATUS_MASK(a0,t0)		# machine dependent masking
	mtc0	a0, MIPS_COP_0_STATUS		# store back
	COP0_SYNC
	and	v0, v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
	j	ra
	nop
END(_spllower)

LEAF(_splrestore)
	mfc0	v0, MIPS_COP_0_STATUS		# fetch status register
	and	a0, a0, MIPS_INT_MASK
	li	v1, ~MIPS_INT_MASK
	and	v1, v1, v0			# turn off every INT bit
	or	v1, v1, a0			# set old INT bits
	DYNAMIC_STATUS_MASK(v1,t0)		# machine dependent masking
	mtc0	v1, MIPS_COP_0_STATUS		# store back
	COP0_SYNC
	and	v0, v0, MIPS_INT_MASK
	j	ra
	nop
END(_splrestore)

LEAF(_splset)
XLEAF(_splset_noprof)				# does not get mcount hooks
	mfc0	v0, MIPS_COP_0_STATUS		# fetch status register
	and	a0, a0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
	li	v1, ~(MIPS_INT_MASK | MIPS_SR_INT_IE)
	and	v1, v1, v0			# turn off every INT bit
	or	v1, v1, a0			# set old INT bits
	DYNAMIC_STATUS_MASK(v1,t0)		# machine dependent masking
	mtc0	v1, MIPS_COP_0_STATUS		# store back
	COP0_SYNC
	and	v0, v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
	j	ra
	nop
END(_splset)

LEAF(_splget)
	mfc0	v0, MIPS_COP_0_STATUS		# fetch status register
	and	v0, v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
	j	ra
	nop
END(_splget)

LEAF(_setsoftintr)
	mfc0	v1, MIPS_COP_0_STATUS		# save status register
	mtc0	zero, MIPS_COP_0_STATUS		# disable interrupts (2 cycles)
	COP0_SYNC
	nop
	nop
	mfc0	v0, MIPS_COP_0_CAUSE		# fetch cause register
	nop
	or	v0, v0, a0			# set soft intr. bits
	mtc0	v0, MIPS_COP_0_CAUSE		# store back
	COP0_SYNC
	mtc0	v1, MIPS_COP_0_STATUS		# enable interrupts
	COP0_SYNC
	j	ra
	nop
END(_setsoftintr)

LEAF(_clrsoftintr)
	mfc0	v1, MIPS_COP_0_STATUS		# save status register
	mtc0	zero, MIPS_COP_0_STATUS		# disable interrupts (2 cycles)
	COP0_SYNC
	nop
	nop
	mfc0	v0, MIPS_COP_0_CAUSE		# fetch cause register
	nor	a0, zero, a0			# bitwise inverse of A0
	and	v0, v0, a0			# clear soft intr. bits
	mtc0	v0, MIPS_COP_0_CAUSE		# store back
	COP0_SYNC
	mtc0	v1, MIPS_COP_0_STATUS		# enable interrupts
	COP0_SYNC
	j	ra
	nop
END(_clrsoftintr)

LEAF(_splnone)
	mtc0	zero, MIPS_COP_0_CAUSE		# clear SOFT_INT bits
	COP0_SYNC
	li	v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
	DYNAMIC_STATUS_MASK(v0,t0)		# machine dependent masking
	mtc0	v0, MIPS_COP_0_STATUS		# enable all sources
	COP0_SYNC
	nop
	j	ra
	nop
END(_splnone)

#if defined(NS) || defined(ISO) || defined(CCITT) || defined(CODA)
/*
 * Insert 'p' after 'q'.
 *	_insque(p, q)
 *		caddr_t p, q;
 */
LEAF(_insque)
	lw	v0, 0(a1)		# v0 = q->next
	sw	a1, 4(a0)		# p->prev = q
	sw	v0, 0(a0)		# p->next = q->next
	sw	a0, 4(v0)		# q->next->prev = p
	j	ra
	sw	a0, 0(a1)		# q->next = p
END(_insque)

/*
 * Remove item 'p' from queue.
 *	_remque(p)
 *		caddr_t p;
 */
LEAF(_remque)
	lw	v0, 0(a0)		# v0 = p->next
	lw	v1, 4(a0)		# v1 = p->prev
	nop
	sw	v0, 0(v1)		# p->prev->next = p->next
	j	ra
	sw	v1, 4(v0)		# p->next->prev = p->prev
END(_remque)
#endif


/*
 * u_int32_t mips_cp0_cause_read(void)
 *
 *	Return the current value of the CP0 Cause register.
 *
 *	Note: Not profiled, skews CPU-clock measurement (mips_mcclock.c)
 *	to uselessness.
 */
LEAF_NOPROFILE(mips_cp0_cause_read)
	mfc0	v0, MIPS_COP_0_CAUSE
	j	ra
	nop
END(mips_cp0_cause_read)

/*
 * void mips_cp0_cause_write(u_int32_t)
 *
 *	Set the value of the CP0 Cause register.
 */
LEAF(mips_cp0_cause_write)
	mtc0	a0, MIPS_COP_0_CAUSE
	COP0_SYNC
	nop
	nop
	j	ra
	nop
END(mips_cp0_cause_write)


/*
 * u_int32_t mips_cp0_status_read(void)
 *
 *	Return the current value of the CP0 Status register.
 */
LEAF(mips_cp0_status_read)
	mfc0	v0, MIPS_COP_0_STATUS
	j	ra
	nop
END(mips_cp0_status_read)

/*
 * void mips_cp0_status_write(u_int32_t)
 *
 *	Set the value of the CP0 Status register.
 *
 *	Note: This is almost certainly not the way you want to write a
 *	"permanent" value to to the CP0 Status register, since it gets
 *	saved in trap frames and restores.
 */
LEAF(mips_cp0_status_write)
	mtc0	a0, MIPS_COP_0_STATUS
	COP0_SYNC
	nop
	nop
	j	ra
	nop
END(mips_cp0_status_write)


#if !defined(NOFPU) && !defined(SOFTFLOAT)
/*----------------------------------------------------------------------------
 *
 * MachFPInterrupt --
 * MachFPTrap --
 *
 *	Handle a floating point interrupt (r3k) or trap (r4k).
 *	the handlers are indentical, only the reporting mechanisms differ.
 *
 *	MachFPInterrupt(status, cause, pc, frame)
 *		unsigned status;
 *		unsigned cause;
 *		unsigned pc;
 *		int *frame;
 *
 *	MachFPTrap(status, cause, pc, frame)
 *		unsigned status;
 *		unsigned cause;
 *		unsigned pc;
 *		int *frame;
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------------
 */
NESTED(MachFPInterrupt, CALLFRAME_SIZ, ra)
XNESTED(MachFPTrap)
	.mask	0x80000000, -4
	subu	sp, sp, CALLFRAME_SIZ
	mfc0	t0, MIPS_COP_0_STATUS
	sw	ra, CALLFRAME_RA(sp)
	or	t0, t0, MIPS_SR_COP_1_BIT
	mtc0	t0, MIPS_COP_0_STATUS
	COP0_HAZARD_FPUENABLE

	cfc1	t0, MIPS_FPU_CSR	# stall til FP done
	cfc1	t0, MIPS_FPU_CSR	# now get status
	nop
	sll	t2, t0, (31 - 17)	# unimplemented operation?
	bgez	t2, 3f			# no, normal trap
	nop
/*
 * We got an unimplemented operation trap so
 * We received an unimplemented operation trap.
 *
 * We check whether it's an unimplemented FP instruction here rather
 * than invoking MachEmulateInst(), since it is faster.
 *
 * fetch the instruction and emulate the instruction.
 */
	bgez	a1, 1f			# Check the branch delay bit.
	nop
/*
 * The instruction is in the branch delay slot.
 */
	b	2f
	lw	a0, 4(a2)			# a0 = coproc instruction
/*
 * This is not in the branch delay slot so calculate the resulting
 * PC (epc + 4) into v0 and continue to MachEmulateFP().
 */
1:
	lw	a0, 0(a2)			# a0 = coproc instruction
2:
	move	a2, a1

/*
 * Check to see if the instruction to be emulated is a floating-point
 * instruction.
 */
	srl	t0, a0, MIPS_OPCODE_SHIFT
	beq	t0, MIPS_OPCODE_C1, 4f
	nop

/*
 * Send a floating point exception signal to the current process.
 */
	li	t0, 0xFFFFFF00
	and	a1, a1, t0
	ori	a1, a1, T_RES_INST << MIPS_CR_EXC_CODE_SHIFT
	REG_PROLOGUE
	REG_S	a1, FRAME_CAUSE(a3)
	REG_EPILOGUE

	move	a2, a0				# code = instruction
	lw	a0, _C_LABEL(curlwp)		# get current process
	jal	_C_LABEL(trapsignal)
	li	a1, SIGILL

	b	FPReturn
	nop

/*
 * Send a FPE signal to the current process if it tripped the any of
 * the VZOUI bits.
 */
3:
	REG_PROLOGUE
	REG_S	a1, FRAME_CAUSE(a3)
	REG_EPILOGUE

	move	a2, a0				# code = instruction
	lw	a0, _C_LABEL(curlwp)		# get current process
	jal	_C_LABEL(trapsignal)
	li	a1, SIGFPE			# BDSLOT

	b	FPReturn
	nop

/*
 * Finally, we can call MachEmulateFP() where a0 is the instruction to emulate.
 */
4:
	jal	_C_LABEL(MachEmulateFP)
	move	a1, a3

/*
 * Turn off the floating point coprocessor and return.
 */
FPReturn:
	mfc0	t0, MIPS_COP_0_STATUS
	lw	ra, CALLFRAME_RA(sp)
	and	t0, t0, ~MIPS_SR_COP_1_BIT
	mtc0	t0, MIPS_COP_0_STATUS
	COP0_SYNC
	j	ra
	addu	sp, sp, CALLFRAME_SIZ
END(MachFPInterrupt)
#endif /* !defined(NOFPU) && !defined(SOFTFLOAT) */

LEAF(mips_pagecopy)
#if defined(__mips_n32) || defined(_LP64)
	.set	push
	.set	mips3
	li	a2, PAGE_SIZE >> 6

1:	ld	t0, 0(a1)
	ld	ta0, 32(a1)
	ld	t2, 16(a1)
	ld	ta2, 48(a1)
	subu	a2, 1
	ld	t1, 8(a1)
	ld	t3, 24(a1)
	ld	ta1, 40(a1)
	ld	ta3, 56(a1)

	sd	t0, 0(a0)
	sd	ta0, 32(a0)
	sd	t2, 16(a0)
	sd	ta2, 48(a0)
	addu	a1, 64
	sd	t1, 8(a0)
	sd	t3, 24(a0)
	sd	ta1, 40(a0)
	sd	ta3, 56(a0)
	bgtz	a2,1b
	addu	a0, 64
	.set	pop
#else
	/* o32 */
	li	a2, PAGE_SIZE >> 5

1:	lw	t0, 0(a1)
	lw	ta0, 16(a1)
	subu	a2, 1
	lw	t1, 4(a1)
	lw	t2, 8(a1)
	lw	t3, 12(a1)
	lw	ta1, 20(a1)
	lw	ta2, 24(a1)
	lw	ta3, 28(a1)

	sw	t0, 0(a0)
	sw	ta0, 16(a0)
	addu	a1, 32
	sw	t1, 4(a0)
	sw	t2, 8(a0)
	sw	t3, 12(a0)
	sw	ta1, 20(a0)
	sw	ta2, 24(a0)
	sw	ta3, 28(a0)
	bgtz	a2,1b
	addu	a0, 32
#endif /* __mips_n32 || _LP64 */
	j	ra
	nop
END(mips_pagecopy)

LEAF(mips_pagezero)
/* We can always safely store a 64-bit zero on MIPS3,4,64 */
#if !defined(MIPS1) && !defined(MIPS32)
	.set	push
	.set	mips3
	li	a1, PAGE_SIZE >> 6

1:	sd	zero, 0(a0)			# try to miss cache first
	sd	zero, 32(a0)
	subu	a1, 1
	sd	zero, 16(a0)
	sd	zero, 48(a0)
	sd	zero, 8(a0)			# fill in cache lines
	sd	zero, 40(a0)
	sd	zero, 24(a0)
	sd	zero, 56(a0)
	bgtz	a1,1b
	addu	a0, 64
	.set	pop
#else
	/* o32 */
	li	a1, PAGE_SIZE >> 5

1:	sw	zero, 0(a0)
	sw	zero, 16(a0)			# try to miss cache first
	subu	a1, 1
	sw	zero, 4(a0)
	sw	zero, 8(a0)
	sw	zero, 12(a0)
	sw	zero, 20(a0)
	sw	zero, 24(a0)
	sw	zero, 28(a0)
	bgtz	a1,1b
	addu	a0, 32
#endif /* __mips_n32 || _LP64 */
	j	ra
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
NESTED(stacktrace, CALLFRAME_SIZ+24, ra)
XNESTED(logstacktrace)
	subu	sp, sp, CALLFRAME_SIZ+24	# four arg-passing slots

	move	t0, ra				# save caller's PC
	addu	t1, sp, CALLFRAME_SIZ+24	# compute caller's SP
	move	t2, s8				# non-virtual frame pointer

	la	v0, _C_LABEL(printf)

	sw	ra, 36(sp)			# save return address

	/* a0-a3 are still caller's a0-a3, pass in-place as given. */
	sw	t0, 16(sp)			# push caller's PC
	sw	t1, 20(sp)			# push caller's SP
	sw	t2, 24(sp)			# push caller's FP, in case
	sw	zero, 28(sp)			# caller's RA on stack
	jal	_C_LABEL(stacktrace_subr)
	sw	v0, 32(sp)			# push printf

	lw	ra, 36(sp)
	addu	sp, sp, CALLFRAME_SIZ+24
	j	ra
	nop
END(stacktrace)
#endif	/* DEBUG || DDB */
#endif	/* DDB_TRACE */

	.sdata
	.globl	_C_LABEL(esym)
_C_LABEL(esym):
	.word 0

	.globl	_C_LABEL(cpu_id)
	.globl	_C_LABEL(fpu_id)
_C_LABEL(cpu_id):
	.word	0
_C_LABEL(fpu_id):
	.word	0

#ifdef MIPS_DYNAMIC_STATUS_MASK
	.globl	_C_LABEL(mips_dynamic_status_mask)
_C_LABEL(mips_dynamic_status_mask):
	.word	0xffffffff
#endif
