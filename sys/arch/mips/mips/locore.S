/*	$NetBSD: locore.S,v 1.122.2.3 2002/03/16 15:58:39 jdolecek Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
#include "opt_compat_ultrix.h"
#include "opt_compat_linux.h"
#include "opt_compat_irix.h"
#include "opt_ns.h"
#include "opt_ccitt.h"
#include "opt_iso.h"
#include "opt_lockdebug.h"
#include "fs_coda.h"

#include <sys/cdefs.h>

#include <sys/errno.h>
#include <sys/syscall.h>
#ifdef COMPAT_ULTRIX
#include <compat/ultrix/ultrix_syscall.h>
#endif
#ifdef COMPAT_LINUX
#include <compat/linux/linux_syscall.h>
#endif
#ifdef COMPAT_IRIX
#include <compat/irix/irix_syscall.h>
#endif

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
	and	v0, MIPS3_SR_DIAG_BEV
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
	COP0_SYNC
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
 * This code is copied the user's stack for returning from signal handlers
 * (see sendsig() and sigreturn()). We have to compute the address
 * of the sigcontext struct for the sigreturn call.
 *
 * NB: we cannot profile sigcode(), it executes from userspace.
 */
LEAF_NOPROFILE(sigcode)
	addu	a0, sp, 16		# address of sigcontext
	li	v0, SYS___sigreturn14	# sigreturn(scp)
	syscall
	break	0			# just in case sigreturn fails
END(sigcode)
XLEAF(esigcode)

#ifdef COMPAT_ULTRIX
LEAF_NOPROFILE(ultrix_sigcode)
	addu	a0, sp, 16		# address of sigcontext
	li	v0, ULTRIX_SYS_sigreturn	# sigreturn(scp)
	syscall
	break	0			# just in case sigreturn fails
END(ultrix_sigcode)
XLEAF(ultrix_esigcode)
#endif

#ifdef COMPAT_LINUX
#define SYSCALL_SHIFT 4000		# 4000 shift as in linux_syscall.c
LEAF_NOPROFILE(linux_sigcode)
	move	a0, sp
	li	v0, LINUX_SYS_sigreturn	+ SYSCALL_SHIFT # sigreturn(sf)
	syscall
	break	0			# just in case sigreturn fails
END(linux_sigcode)
XLEAF(linux_esigcode)
#endif

#ifdef COMPAT_IRIX
#define SYSCALL_SHIFT 1000		# 1000 shift as in irix_syscall.c
LEAF_NOPROFILE(irix_sigcode)
	move	a0, sp
	li	v0, IRIX_SYS_sigreturn	+ SYSCALL_SHIFT # sigreturn(sf)
	syscall
	break	0			# just in case sigreturn fails
END(irix_sigcode)
XLEAF(irix_esigcode)
#endif

/*
 * The following primitives manipulate the run queues.  whichqs tells which
 * of the 32 queues qs have processes in them.  Setrunqueue puts processes
 * into queues, remrunqueue removes them from queues.  The running process is
 * on no queue, other processes are on a queue related to p->p_priority,
 * divided by 4 actually to shrink the 0-127 range of priorities into the 32
 * available queues.
 */
/*
 * setrunqueue(struct proc *)
 *
 * Call should be made at splclock(), and p->p_stat should be SRUN.
 */
NESTED(setrunqueue, CALLFRAME_SIZ, ra)
	.mask	0x80000000, -4
	subu	sp, sp, CALLFRAME_SIZ
	lw	t0, P_BACK(a0)		# firewall: p->p_back must be 0
	sw	ra, CALLFRAME_RA(sp)
	beq	t0, zero, 1f
	lbu	t0, P_PRIORITY(a0)	# put on p->p_priority / 4 queue
	PANIC("setrunqueue")		#
1:
	li	t1, 1			# compute corresponding bit
	srl	t0, t0, 2		# compute index into 'whichqs'
	sll	t1, t1, t0
	lw	t2, _C_LABEL(sched_whichqs)	# set corresponding bit
	nop
	or	t2, t2, t1
	sw	t2, _C_LABEL(sched_whichqs)
	sll	t0, t0, 3		# compute index into 'qs'
	la	t1, _C_LABEL(sched_qs)
	addu	t0, t0, t1		# t0 = qp = &qs[pri >> 2]
	lw	t1, P_BACK(t0)		# t1 = qp->ph_rlink
	sw	t0, P_FORW(a0)		# p->p_forw = qp
	sw	t1, P_BACK(a0)		# p->p_back = qp->ph_rlink
	sw	a0, P_FORW(t1)		# p->p_back->p_forw = p;
	sw	a0, P_BACK(t0)		# qp->ph_rlink = p
	j	ra
	addu	sp, sp, CALLFRAME_SIZ
END(setrunqueue)

/*
 * remrunqueue(struct proc *)
 *
 * Call should be made at splclock().
 */
NESTED(remrunqueue, CALLFRAME_SIZ, ra)
	.mask	0x80000000, -4
	subu	sp, sp, CALLFRAME_SIZ
	lbu	t0, P_PRIORITY(a0)	# get from p->p_priority / 4 queue
	li	t1, 1			# compute corresponding bit
	srl	t0, t0, 2		# compute index into 'whichqs'
	lw	t2, _C_LABEL(sched_whichqs)	# check corresponding bit
	sll	t1, t1, t0
	and	v0, t2, t1
	sw	ra, CALLFRAME_RA(sp)
	bne	v0, zero, 1f
	lw	v0, P_BACK(a0)		# v0 = p->p_back
	PANIC("remrunqueue")		# it wasn't recorded to be on its q
1:
	lw	v1, P_FORW(a0)		# v1 = p->p_forw
	nop
	sw	v1, P_FORW(v0)		# p->p_back->p_forw = p->p_forw;
	sw	v0, P_BACK(v1)		# p->p_forw->p_back = p->r_rlink
	sll	t0, t0, 3		# compute index into 'qs'
	la	v0, _C_LABEL(sched_qs)
	addu	t0, t0, v0		# t0 = qp = &qs[pri >> 2]
	lw	v0, P_FORW(t0)		# check if queue empty
	nop
	bne	v0, t0, 2f		# No. qp->ph_link != qp
	nop
	xor	t2, t2, t1		# clear corresponding bit in 'whichqs'
	sw	t2, _C_LABEL(sched_whichqs)
2:
	sw	zero, P_BACK(a0)	# for firewall checking
	j	ra
	addu	sp, sp, CALLFRAME_SIZ
END(remrunqueue)

/*
 * When no processes are on the runq, cpu_switch branches to idle
 * to wait for something to come ready.
 * Note: this is really a part of cpu_switch() but defined here for kernel
 * profiling.
 */
LEAF(mips_idle)
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
	sw	zero, _C_LABEL(curproc)		# set curproc NULL for stats
#if defined(LOCKDEBUG)
	jal	_C_LABEL(sched_unlock_idle)	# release sched_lock
	nop
#endif

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
#if defined(LOCKDEBUG)
	mtc0	zero, MIPS_COP_0_STATUS		# disable all interrupts
	COP0_SYNC
	nop
	nop
	nop
	nop
	jal	_C_LABEL(sched_lock_idle)	# acquire sched_lock
	nop
	la	ra, cpu_switch_queuescan
	j	ra
	nop
#else
	mtc0	zero, MIPS_COP_0_STATUS		# disable all interrupts
	COP0_SYNC
	nop
	nop
	nop
	la	ra, cpu_switch_queuescan
	j	ra
	nop
#endif
END(mips_idle)

/*
 * cpu_switch(struct proc *)
 * Find the highest priority process and resume it.
 */
NESTED(cpu_switch, CALLFRAME_SIZ, ra)
	lw	a0, P_ADDR(a0)
	mfc0	t0, MIPS_COP_0_STATUS
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
	REG_S	t0, U_PCB_CONTEXT+SF_REG_SR(a0)
#ifdef IPL_ICU_MASK
	lw	t0, _C_LABEL(md_imask)	
	sw	t0, U_PCB_PPL(a0)
#endif	
	REG_EPILOGUE
	subu	sp, sp, CALLFRAME_SIZ
	sw	ra, CALLFRAME_RA(sp)
	.mask	0x80000000, -4
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
	lw	a0, P_FORW(t0)			# a0 = p = highest pri process
	nop
	lw	v0, P_FORW(a0)			# v0 = p->p_forw
	bne	t0, a0, 2f			# make sure something in queue
	sw	v0, P_FORW(t0)			# qp->ph_link = p->p_forw;
	PANIC("cpu_switch")			# nothing in queue
2:
	sw	t0, P_BACK(v0)			# p->p_forw->p_back = qp
	bne	v0, t0, 3f			# queue still not empty
	sw	zero, P_BACK(a0)		## for firewall checking
	li	v1, 1				# compute bit in 'whichqs'
	sll	v1, v1, t2
	xor	t3, t3, v1			# clear bit in 'whichqs'
	sw	t3, _C_LABEL(sched_whichqs)
3:
	/* Squirrel away proc pointer. */
	move	s7, a0
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
 * Switch to new context.
 */
#if defined(MULTIPROCESSOR)
	/*
	 * XXXSMP
	 * p->p_cpu = curcpu();
	 */
#endif
	li	v1, SONPROC			# p->p_stat = SONPROC
	sb	v1, P_STAT(a0)
	lw	t2, _C_LABEL(mips_locoresw) + MIPSX_CPU_SWITCH_RESUME
	sw	a0, _C_LABEL(curproc)
	jal	ra, t2
	nop

#if 1	/* XXX XXX XXX */
	REG_PROLOGUE
	lw	a0, P_ADDR(s7)
	nop
	REG_L	sp, U_PCB_CONTEXT+SF_REG_SP(a0)
	nop
	REG_EPILOGUE
#endif	/* XXX XXX XXX */

	jal	_C_LABEL(pmap_activate)
	move	a0, s7				# -BDSLOT-

	lw	a0, P_ADDR(s7)
	sw	zero, _C_LABEL(want_resched)	# we've context switched
	sw	a0, _C_LABEL(curpcb)

#ifdef IPL_ICU_MASK
	# restore ICU state
	lw	t0, U_PCB_PPL(a0)
	sw	t0, _C_LABEL(md_imask)	
	jal	_C_LABEL(md_imask_update)
	nop
	lw	a0, P_ADDR(s7)			# restore pcb_context pointer.
#endif /* IPL_ICU_MASK */
	REG_PROLOGUE
	REG_L	v0, U_PCB_CONTEXT+SF_REG_SR(a0)
	DYNAMIC_STATUS_MASK(v0,ra)		# machine dependent masking
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
	mtc0	v0, MIPS_COP_0_STATUS
	COP0_SYNC
	j	ra
	li	v0, 1				# possible return to 'savectx()'
END(cpu_switch)

/*
 * switch_exit(struct proc *)
 *
 * Make the named process exit.  Switch SP to proc0 stack, then
 * call exit2() to schedule exiting proc's vmspace and stack to be
 * released by the reaper thread.  MUST BE CALLED AT SPLHIGH.
 */
LEAF(switch_exit)
	lw	v0, _C_LABEL(proc0paddr)	# get proc0 p_addr
	nop
	sw	v0, _C_LABEL(curpcb)		# set current pcb
	REG_PROLOGUE
	REG_L	sp, U_PCB_CONTEXT+SF_REG_SP(v0)	# restore stack pointer
	REG_EPILOGUE
#if defined(LOCKDEBUG)
	jal	_C_LABEL(sched_lock_idle)	# acquire sched_lock
	nop
#endif
	la	ra, cpu_switch_queuescan	# rathole to cpu_switch()
	j	_C_LABEL(exit2)			# proc already in a0
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


/*
 * int copystr(void *kfaddr, void *kdaddr, size_t maxlen, size_t *lencopied)
 * Copy a NIL-terminated string, at most maxlen characters long.  Return the
 * number of characters copied (including the NIL) in *lencopied.  If the
 * string is too long, return ENAMETOOLONG; else return 0.
 */
LEAF(copystr)
	move	t0, a2
	beq	a2, zero, 4f
1:
	lbu	v0, 0(a0)
	subu	a2, a2, 1
	beq	v0, zero, 2f
	sb	v0, 0(a1)			# each byte until NIL
	addu	a0, a0, 1
	bne	a2, zero, 1b			# less than maxlen
	addu	a1, a1, 1
4:
	li	v0, ENAMETOOLONG		# run out of space
2:
	beq	a3, zero, 3f			# return num. of copied bytes
	subu	a2, t0, a2			# if the 4th arg was non-NULL
	sw	a2, 0(a3)
3:
	j	ra				# v0 is 0 or ENAMETOOLONG
	nop
END(copystr)

/*
 * int copyinstr(void *uaddr, void *kaddr, size_t maxlen, size_t *lencopied)
 * Copy a NIL-terminated string, at most maxlen characters long, from the
 * user's address space.  Return the number of characters copied (including
 * the NIL) in *lencopied.  If the string is too long, return ENAMETOOLONG;
 * else return 0 or EFAULT.
 */
LEAF(copyinstr)
	lw	v1, _C_LABEL(curpcb)
	la	v0, _C_LABEL(copystrerr)
	blt	a0, zero, _C_LABEL(copystrerr)
	sw	v0, U_PCB_ONFAULT(v1)
	move	t0, a2
	beq	a2, zero, 4f
1:
	lbu	v0, 0(a0)
	subu	a2, a2, 1
	beq	v0, zero, 2f
	sb	v0, 0(a1)
	addu	a0, a0, 1
	bne	a2, zero, 1b
	addu	a1, a1, 1
4:
	li	v0, ENAMETOOLONG
2:
	beq	a3, zero, 3f
	subu	a2, t0, a2
	sw	a2, 0(a3)
3:
	j	ra				# v0 is 0 or ENAMETOOLONG
	sw	zero, U_PCB_ONFAULT(v1)
END(copyinstr)

/*
 * int copyoutstr(void *uaddr, void *kaddr, size_t maxlen, size_t *lencopied);
 * Copy a NIL-terminated string, at most maxlen characters long, into the
 * user's address space.  Return the number of characters copied (including
 * the NIL) in *lencopied.  If the string is too long, return ENAMETOOLONG;
 * else return 0 or EFAULT.
 */
LEAF(copyoutstr)
	lw	v1, _C_LABEL(curpcb)
	la	v0, _C_LABEL(copystrerr)
	blt	a1, zero, _C_LABEL(copystrerr)
	sw	v0, U_PCB_ONFAULT(v1)
	move	t0, a2
	beq	a2, zero, 4f
1:
	lbu	v0, 0(a0)
	subu	a2, a2, 1
	beq	v0, zero, 2f
	sb	v0, 0(a1)
	addu	a0, a0, 1
	bne	a2, zero, 1b
	addu	a1, a1, 1
4:
	li	v0, ENAMETOOLONG
2:
	beq	a3, zero, 3f
	subu	a2, t0, a2
	sw	a2, 0(a3)
3:
	j	ra				# v0 is 0 or ENAMETOOLONG
	sw	zero, U_PCB_ONFAULT(v1)
END(copyoutstr)

LEAF(copystrerr)
	sw	zero, U_PCB_ONFAULT(v1)
	j	ra
	li	v0, EFAULT			# return EFAULT
END(copystrerr)

/*
 * kcopy(const void *src, void *dst, size_t len);
 *
 * Copy len bytes from src to dst, aborting if we encounter a fatal
 * page fault.
 *
 * kcopy() _must_ save and restore the old fault handler since it is
 * called by uiomove(), which may be in the path of servicing a non-fatal
 * page fault.
 */
NESTED(kcopy, 48, ra)
	subu	sp, sp, 48			# set up stack frame
	/* Frame contains RA (31) and S0 (16). */
	.mask	0x80010000, -4
	sw	ra, 44(sp)			# save ra
	sw	s0, 32(sp)			# save s0
	move	v0, a0				# swap a0, a1 for call to memcpy
	move	a0, a1
	move	a1, v0
	lw	v1, _C_LABEL(curpcb)		# set up fault handler
	la	v0, _C_LABEL(kcopyerr)
	lw	s0, U_PCB_ONFAULT(v1)		# save old handler
	jal	memcpy
	sw	v0, U_PCB_ONFAULT(v1)

	lw	v1, _C_LABEL(curpcb)		# restore the old handler
	lw	ra, 44(sp)			# restore ra
	sw	s0, U_PCB_ONFAULT(v1)
	lw	s0, 32(sp)			# restore s0
	addu	sp, sp, 48			# kill stack frame
	j	ra
	move	v0, zero			# success!
END(kcopy)

LEAF(kcopyerr)
	lw	v1, _C_LABEL(curpcb)		# restore the old handler
	lw	ra, 44(sp)			# restore ra
	sw	s0, U_PCB_ONFAULT(v1)
	lw	s0, 32(sp)			# restore s0
	addu	sp, sp, 48			# kill stack frame
	j	ra
	li	v0, EFAULT			# return EFAULT
END(kcopyerr)

/*
 * int copyin(void *uaddr, void *kaddr, size_t len)
 * Copies len bytes of data from the user-space address uaddr to the
 * kernel-space address kaddr.  copyin returns 0 on success or EFAULT
 * if a bad address is encountered.
 */
NESTED(copyin, CALLFRAME_SIZ, ra)
	subu	sp, sp, CALLFRAME_SIZ
	.mask	0x80000000, -4
	sw	ra, CALLFRAME_RA(sp)
	blt	a0, zero, _C_LABEL(copyerr)
	move	v0, a0				# swap a0, a1 for call to memcpy
	move	a0, a1
	move	a1, v0
	lw	v1, _C_LABEL(curpcb)
	la	v0, _C_LABEL(copyerr)
	jal	memcpy
	sw	v0, U_PCB_ONFAULT(v1)

	lw	v1, _C_LABEL(curpcb)
	lw	ra, CALLFRAME_RA(sp)
	addu	sp, sp, CALLFRAME_SIZ
	sw	zero, U_PCB_ONFAULT(v1)
	j	ra
	move	v0, zero
END(copyin)

/*
 * int copyout(void *kaddr, void *uaddr, size_t len)
 * Copies len bytes of data from the kernel-space address kaddr to the
 * user-space address uaddr.  copyout returns 0 on success or EFAULT
 * if a bad address is encountered.
 */
NESTED(copyout, CALLFRAME_SIZ, ra)
	subu	sp, sp, CALLFRAME_SIZ
	.mask	0x80000000, -4
	sw	ra, CALLFRAME_RA(sp)
	blt	a1, zero, _C_LABEL(copyerr)
	move	v0, a0				# swap a0, a1 for call to memcpy
	move	a0, a1
	move	a1, v0
	lw	v1, _C_LABEL(curpcb)
	la	v0, _C_LABEL(copyerr)
	jal	memcpy
	sw	v0, U_PCB_ONFAULT(v1)

	lw	v1, _C_LABEL(curpcb)
	lw	ra, CALLFRAME_RA(sp)
	addu	sp, sp, CALLFRAME_SIZ
	sw	zero, U_PCB_ONFAULT(v1)
	j	ra
	move	v0, zero
END(copyout)

LEAF(copyerr)
	lw	v1, _C_LABEL(curpcb)
	lw	ra, CALLFRAME_RA(sp)
	addu	sp, sp, CALLFRAME_SIZ
	sw	zero, U_PCB_ONFAULT(v1)
	j	ra
	li	v0, EFAULT			# return EFAULT
END(copyerr)

/*
 * int fuswintr(void *)
 * Fetches a short word of data from the user-space address.
 * This function is safe to call during an interrupt context.
 */
LEAF(fuswintr)
	lw	v1, _C_LABEL(curpcb)
	la	v0, _C_LABEL(fswintrberr)
	lw	a2, U_PCB_ONFAULT(v1)
	blt	a0, zero, _C_LABEL(fswintrberr)
	sw	v0, U_PCB_ONFAULT(v1)
	lhu	v0, 0(a0)			# fetch short
	j	ra
	sw	a2, U_PCB_ONFAULT(v1)
END(fuswintr)

/*
 * int suswintr(void *, short);
 * Stores a short word of data to the user-space address.
 * This function is safe to call during an interrupt context.
 */
LEAF(suswintr)
	lw	v1, _C_LABEL(curpcb)
	la	v0, _C_LABEL(fswintrberr)
	lw	a2, U_PCB_ONFAULT(v1)
	blt	a0, zero, _C_LABEL(fswintrberr)
	sw	v0, U_PCB_ONFAULT(v1)
	sh	a1, 0(a0)			# store short
	sw	a2, U_PCB_ONFAULT(v1)
	j	ra
	move	v0, zero
END(suswintr)

/*
 * int fuword(void *)
 * Fetches a word of data from the user-space address.
 */
LEAF(fuword)
XLEAF(fuiword)
	lw	v1, _C_LABEL(curpcb)
	la	v0, _C_LABEL(fswberr)
	blt	a0, zero, _C_LABEL(fswberr)
	sw	v0, U_PCB_ONFAULT(v1)
	lw	v0, 0(a0)			# fetch word
	j	ra
	sw	zero, U_PCB_ONFAULT(v1)
END(fuword)

/*
 * int fusword(void *)
 * Fetches a short word of data from the user-space address.
 */
LEAF(fusword)
XLEAF(fuisword)
	lw	v1, _C_LABEL(curpcb)
	la	v0, _C_LABEL(fswberr)
	blt	a0, zero, _C_LABEL(fswberr)
	sw	v0, U_PCB_ONFAULT(v1)
	lhu	v0, 0(a0)			# fetch short
	j	ra
	sw	zero, U_PCB_ONFAULT(v1)
END(fusword)

/*
 * int fubyte(void *)
 * Fetch a byte from the user's address space.
 */
LEAF(fubyte)
XLEAF(fuibyte)
	lw	v1, _C_LABEL(curpcb)
	la	v0, _C_LABEL(fswberr)
	blt	a0, zero, _C_LABEL(fswberr)
	sw	v0, U_PCB_ONFAULT(v1)
	lbu	v0, 0(a0)			# fetch byte
	j	ra
	sw	zero, U_PCB_ONFAULT(v1)
END(fubyte)

/*
 * int suword(void *, int)
 * Stores a word of data to the user-space address.
 */
LEAF(suword)
	lw	v1, _C_LABEL(curpcb)
	la	v0, _C_LABEL(fswberr)
	blt	a0, zero, _C_LABEL(fswberr)
	sw	v0, U_PCB_ONFAULT(v1)
	sw	a1, 0(a0)			# store word
	sw	zero, U_PCB_ONFAULT(v1)
	j	ra
	move	v0, zero
END(suword)

/*
 * int suiword(void *, int)
 * Have to flush instruction cache afterwards.
 */
LEAF(suiword)
	lw	v1, _C_LABEL(curpcb)
	la	v0, _C_LABEL(fswberr)
	blt	a0, zero, _C_LABEL(fswberr)
	sw	v0, U_PCB_ONFAULT(v1)
	sw	a1, 0(a0)			# store word
	sw	zero, U_PCB_ONFAULT(v1)
	move	v0, zero
	lw	v1, _C_LABEL(mips_cache_ops) + MIPSX_FLUSHICACHE
	j	v1				# NOTE: must not clobber v0!
	li	a1, 4				# size of word
END(suiword)

/*
 * int susword(void *, short)
 * Stores a short word of data to the user-space address.
 */
LEAF(susword)
XLEAF(suisword)
	lw	v1, _C_LABEL(curpcb)
	la	v0, _C_LABEL(fswberr)
	blt	a0, zero, _C_LABEL(fswberr)
	sw	v0, U_PCB_ONFAULT(v1)
	sh	a1, 0(a0)			# store short
	sw	zero, U_PCB_ONFAULT(v1)
	j	ra
	move	v0, zero
END(susword)

/*
 * int subyte(void *, int)
 * Stores a byte of data to the user-space address.
 */
LEAF(subyte)
XLEAF(suibyte)
	lw	v1, _C_LABEL(curpcb)
	la	v0, _C_LABEL(fswberr)
	blt	a0, zero, _C_LABEL(fswberr)
	sw	v0, U_PCB_ONFAULT(v1)
	sb	a1, 0(a0)			# store byte
	sw	zero, U_PCB_ONFAULT(v1)
	j	ra
	move	v0, zero
END(subyte)

/*
 * int badaddr(void addr, int len)
 * See if access to addr with a len type instruction causes a machine check.
 * len is length of access (1=byte, 2=short, 4=long)
 */
LEAF(badaddr)
	lw	v1, _C_LABEL(curpcb)
	la	v0, _C_LABEL(baderr)
	bne	a1, 1, 2f
	sw	v0, U_PCB_ONFAULT(v1)
	b	5f
	lbu	v0, (a0)
2:
	bne	a1, 2, 4f
	nop
	b	5f
	lhu	v0, (a0)
4:
	lw	v0, (a0)
5:
	sw	zero, U_PCB_ONFAULT(v1)
	j	ra
	move	v0, zero		# made it w/o errors
END(badaddr)

/*
 * Error routine for {f,s}uswintr.  The fault handler in trap.c
 * checks for pcb_onfault set to this fault handler and
 * "bails out" before calling the VM fault handler.
 * (We can not call VM code from interrupt level.)
 */
LEAF(fswintrberr)
	nop
	sw	a2, U_PCB_ONFAULT(v1)
	j	ra
	li	v0, -1
END(fswintrberr)

LEAF(fswberr)
XLEAF(baderr)
	sw	zero, U_PCB_ONFAULT(v1)
	j	ra
	li	v0, -1
END(fswberr)

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


#if !defined(NOFPU) || defined(SOFTFLOAT)
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
#ifndef SOFTFLOAT
	mfc0	t0, MIPS_COP_0_STATUS
#endif
	sw	ra, CALLFRAME_RA(sp)
#ifndef SOFTFLOAT
	or	t0, t0, MIPS_SR_COP_1_BIT
	mtc0	t0, MIPS_COP_0_STATUS
	COP0_SYNC
	nop
	nop
	nop				# 1st extra nop for r4k
	nop				# 2nd extra nop for r4k

	cfc1	t0, MIPS_FPU_CSR	# stall til FP done
	cfc1	t0, MIPS_FPU_CSR	# now get status
	nop
	sll	t2, t0, (31 - 17)	# unimplemented operation?
	bgez	t2, 3f			# no, normal trap
	nop
#endif
/*
 * We got an unimplemented operation trap so
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

#ifdef SOFTFLOAT
	REG_PROLOGUE
	REG_S	zero, FRAME_ZERO(a3)		# ensure zero has value 0
	REG_EPILOGUE

	beq	t0, MIPS_OPCODE_LWC1, 5f
	nop
	beq	t0, MIPS_OPCODE_LDC1, 6f
	nop
	beq	t0, MIPS_OPCODE_SWC1, 7f
	nop
	beq	t0, MIPS_OPCODE_SDC1, 8f
	nop
#endif

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
	lw	a0, _C_LABEL(curproc)		# get current process
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
	lw	a0, _C_LABEL(curproc)		# get current process
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
#ifndef SOFTFLOAT
	mfc0	t0, MIPS_COP_0_STATUS
#endif
	lw	ra, CALLFRAME_RA(sp)
#ifndef SOFTFLOAT
	and	t0, t0, ~MIPS_SR_COP_1_BIT
	mtc0	t0, MIPS_COP_0_STATUS
	COP0_SYNC
#else
	nop
#endif
	j	ra
	addu	sp, sp, CALLFRAME_SIZ

#ifdef SOFTFLOAT
5:	# lwc1
	jal	_C_LABEL(MachEmulateLWC1)
	move	a1, a3
	b	FPReturn
	nop

6:	# ldc1
	jal	_C_LABEL(MachEmulateLDC1)
	move	a1, a3
	b	FPReturn
	nop

7:	# swc1
	jal	_C_LABEL(MachEmulateSWC1)
	move	a1, a3
	b	FPReturn
	nop

8:	# sdc1
	jal	_C_LABEL(MachEmulateSDC1)
	move	a1, a3
	b	FPReturn
	nop

#endif
END(MachFPInterrupt)
#endif /* !defined(NOFPU) || defined(SOFTFLOAT) */

LEAF(mips_pagecopy)
#if !defined(_MIPS_BSD_API) || _MIPS_BSD_API == _MIPS_BSD_API_LP32
	li	a2, NBPG >> 5

1:	lw	t0, 0(a1)
	lw	t4, 16(a1)
	subu	a2, 1
	lw	t1, 4(a1)
	lw	t2, 8(a1)
	lw	t3, 12(a1)
	lw	t5, 20(a1)
	lw	t6, 24(a1)
	lw	t7, 28(a1)

	sw	t0, 0(a0)
	sw	t4, 16(a0)
	addu	a1, 32
	sw	t1, 4(a0)
	sw	t2, 8(a0)
	sw	t3, 12(a0)
	sw	t5, 20(a0)
	sw	t6, 24(a0)
	sw	t7, 28(a0)
	bgtz	a2,1b
	addu	a0, 32
#else
	li	a2, NBPG >> 6

	REG_PROLOGUE
1:	ld	t0, 0(a1)
	ld	t4, 32(a1)
	ld	t2, 16(a1)
	ld	t6, 48(a1)
	subu	a2, 1
	ld	t1, 8(a1)
	ld	t3, 24(a1)
	ld	t5, 40(a1)
	ld	t7, 56(a1)

	sd	t0, 0(a0)
	sd	t4, 32(a0)
	sd	t2, 16(a0)
	sd	t6, 48(a0)
	addu	a1, 64
	sd	t1, 8(a0)
	sd	t3, 24(a0)
	sd	t5, 40(a0)
	sd	t7, 56(a0)
	bgtz	a2,1b
	addu	a0, 64
	REG_EPILOGUE
#endif

	j	ra
	nop
END(mips_pagecopy)

LEAF(mips_pagezero)
#if !defined(_MIPS_BSD_API) || _MIPS_BSD_API == _MIPS_BSD_API_LP32
	li	a1, NBPG >> 5

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
#else
	li	a1, NBPG >> 6

	REG_PROLOGUE
1:	sd	zero, 0(a0)			# try to miss cache first
	sd	zero, 32(a0)
	subu	a1, 1
	sd	zero, 16(a0)
	sd	zero, 48(a0)
	sd	zero, 8(a0)			# fill in cache lines
	sd	zero, 40(a0)
	sd	zero, 24(a0)
	sd	zero, 56(a0)
	REG_EPILOGUE
	bgtz	a1,1b
	addu	a0, 64
#endif

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
