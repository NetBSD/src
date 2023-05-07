/* $NetBSD: cpu_switch.S,v 1.5 2023/05/07 12:41:48 skrll Exp $ */

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "opt_ddb.h"

#include <machine/asm.h>
#include "assym.h"

/*
 * struct lwp *
 * cpu_switchto(struct lwp *oldl, struct lwp *newl, bool returning);
 */
ENTRY_NP(cpu_switchto)
	addi	sp, sp, -TF_LEN		// allocate trapframe

	REG_S	ra, TF_RA(sp)		// save return address
	REG_S	s0, TF_S0(sp)		// save callee saved address
	REG_S	s1, TF_S1(sp)		// save callee saved address
	REG_S	s2, TF_S2(sp)		// save callee saved address
	REG_S	s3, TF_S3(sp)		// save callee saved address
	REG_S	s4, TF_S4(sp)		// save callee saved address
	REG_S	s5, TF_S5(sp)		// save callee saved address
	REG_S	s6, TF_S6(sp)		// save callee saved address
	REG_S	s7, TF_S7(sp)		// save callee saved address
	REG_S	s8, TF_S8(sp)		// save callee saved address
	REG_S	s9, TF_S9(sp)		// save callee saved address
	REG_S	s10, TF_S10(sp)		// save callee saved address
	REG_S	s11, TF_S11(sp)		// save callee saved address

	REG_S	sp, L_MD_KTF(a0)	// record trapframe pointer

	csrrci	t0, sstatus, SR_SIE	// # disable interrupts

	mv	tp, a1			// # put the new lwp in thread pointer

	PTR_L	t1, L_CPU(tp)		// # get curcpu

	/*
	 * Issue barriers to coordinate mutex_exit on this CPU with
	 * mutex_vector_enter on another CPU.
	 *
	 * 1. Any prior mutex_exit by oldlwp must be visible to other
	 *    CPUs before we set ci_curlwp := newlwp on this one,
	 *    requiring a store-before-store barrier.
	 *
	 * 2. ci_curlwp := newlwp must be visible on all other CPUs
	 *    before any subsequent mutex_exit by newlwp can even test
	 *    whether there might be waiters, requiring a
	 *    store-before-load barrier.
	 *
	 * See kern_mutex.c for details -- this is necessary for
	 * adaptive mutexes to detect whether the lwp is on the CPU in
	 * order to safely block without requiring atomic r/m/w in
	 * mutex_exit.
	 */
	fence	w,w
	PTR_S	tp, CI_CURLWP(t1)	// # update curcpu with the new curlwp
	fence	w,r

	REG_L	sp, L_MD_KTF(tp)	// # load its kernel stack pointer

	csrw	sstatus, t0		// enable interrupts
	REG_L	s0, TF_S0(sp)		// restore callee saved
	REG_L	s1, TF_S1(sp)		// restore callee saved
	REG_L	s2, TF_S2(sp)		// restore callee saved
	REG_L	s3, TF_S3(sp)		// restore callee saved
	REG_L	s4, TF_S4(sp)		// restore callee saved
	REG_L	s5, TF_S5(sp)		// restore callee saved
	REG_L	s6, TF_S6(sp)		// restore callee saved
	REG_L	s7, TF_S7(sp)		// restore callee saved
	REG_L	s8, TF_S8(sp)		// restore callee saved
	REG_L	s9, TF_S9(sp)		// restore callee saved
	REG_L	s10, TF_S10(sp)		// restore callee saved
	REG_L	s11, TF_S11(sp)		// restore callee saved

	REG_L	ra, TF_RA(sp)		// restore return address

	addi	sp, sp, TF_LEN		// remove trapframe

	//	a0 = oldl
	//	a1 = newl
	//	tp = newl
	//	t1 = curcpu()

	ret
END(cpu_switchto)

/*
 * Called at IPL_SCHED:
 *	a0 = old lwp (from cpu_switchto)
 *	a1 = new lwp (from cpu_switchto)
 *	s0 = func
 *	s1 = arg
 */
ENTRY_NP(lwp_trampoline)
	call	_C_LABEL(lwp_startup)	// call lwp startup

	// If the saved func returns, we are returning to user land.
	PTR_LA	ra, exception_userexit
	mv	a0, s2			// get saved arg
	jr	s1			// call saved func
END(lwp_trampoline)


ENTRY_NP(cpu_fast_switchto_cleanup)
//	PTR_L	t0, L_CPU(tp)		// Get curcpu()
//	INT_L	t1, CI_MTX_COUNT(t0)	// get mutex count
//	addi	t1, t1, 1		// increment mutex count
//	INT_S	t1, CI_MTX_COUNT(t0)	// save it
	mv	ra, a1			// Restore real RA
#if IPL_SCHED != IPL_HIGH
	tail	_C_LABEL(splhigh)	// go back to IPL HIGH
#else
	ret				// just return
#endif
END(cpu_fast_switchto_cleanup)


/*
 * void
 * cpu_fast_switchto(struct lwp *, int s);
 */
ENTRY_NP(cpu_fast_switchto)
	addi	sp, sp, -(TF_LEN + CALLFRAME_SIZ)
	REG_S	a0, (TF_LEN + CALLFRAME_S0)(sp)
	REG_S	ra, (TF_LEN + CALLFRAME_RA)(sp)

	PTR_LA	t2, _C_LABEL(cpu_fast_switchto_cleanup)

	REG_S	t2, TF_RA(sp)		// return to someplace else
	REG_S	s0, TF_S0(sp)		// save callee saved register
	REG_S	s1, TF_S1(sp)		// save callee saved register
	REG_S	s2, TF_S2(sp)		// save callee saved register
	REG_S	s3, TF_S3(sp)		// save callee saved register
	REG_S	s4, TF_S4(sp)		// save callee saved register
	REG_S	s5, TF_S5(sp)		// save callee saved register
	REG_S	s6, TF_S6(sp)		// save callee saved register
	REG_S	s7, TF_S7(sp)		// save callee saved register
	REG_S	s8, TF_S8(sp)		// save callee saved register
	REG_S	s9, TF_S9(sp)		// save callee saved register
	REG_S	s10, TF_S10(sp)		// save callee saved register
	REG_S	s11, TF_S11(sp)		// save callee saved register
	csrr	t4, sstatus		// get status register (for intr state)
	REG_S	t4, TF_SR(sp)		// save it

	mv	s0, tp			// remember curlwp
	mv	s1, sp			// remember kernel stack

	csrrci	t0, sstatus, SR_SIE	// disable interrupts
	PTR_L	t1, L_CPU(tp)		// get curcpu()

	PTR_S	sp, L_MD_KTF(tp)	// save trapframe ptr in oldlwp
	mv	tp, a0			// set thread pointer to newlwp
	fence	w,w			// for mutex_enter; see cpu_switchto
	PTR_S	tp, CI_CURLWP(t1)	// update curlwp
	/*
	 * No need for barrier after ci->ci_curlwp = softlwp -- when we
	 * enter a softint lwp, it can't be holding any mutexes, so it
	 * can't release any until after it has acquired them, so we
	 * need not participate in the protocol with mutex_vector_enter
	 * barriers here.
	 */
	PTR_L	sp, L_MD_KTF(tp)	// switch to its stack
#ifdef __HAVE_FAST_SOFTINTS
	csrw	sstatus, t0		// reenable interrupts
	call	_C_LABEL(softint_dispatch)
	csrrci	t0, sstatus, SR_SIE	// disable interrupts
#endif	/* __HAVE_FAST_SOFTINTS */
	PTR_L	t1, L_CPU(tp)		// get curcpu() again
	mv	tp, s0			// return to pinned lwp
	fence	w,w			// for mutex_enter; see cpu_switchto
	PTR_S	tp, CI_CURLWP(t1)	// restore curlwp
	fence	w,r			// for mutex_enter; see cpu_switchto
	csrw	sstatus, t0		// reenable interrupts
	mv	sp, s1			// restore stack pointer

	REG_L	ra, TF_RA(sp)		// get return address
	REG_L	s0, TF_S0(sp)		// restore register we used
	REG_L	s1, TF_S1(sp)		// restore register we used

	REG_L	a0, (TF_LEN + CALLFRAME_S0)(sp)	// Pass the softlwp
	REG_L	a1, (TF_LEN + CALLFRAME_RA)(sp)	// Pass the real RA

	addi	sp, sp, TF_LEN+CALLFRAME_SIZ	// drop trapframe/callframe
	ret				// return
END(cpu_fast_switchto)


/*
 * RISC-V only has a simple exception handler handles both synchronous traps
 * and interrupts.
 */

ENTRY_NP(cpu_exception_handler)
	csrrw	tp, sscratch, tp	// swap scratch and thread pointer
	beqz	tp, .Lexception_kernel	//   tp == 0, already on kernel stack
	//
	// The exception happened while user code was executing.  We need to
	// get the pointer to the user trapframe from the LWP md area.  Then we
	// save t1 and tp so we have a register to work with and to get curlwp
	// into tp.  We also save the saved SP into the trapframe.
	// Upon entry on an exception from user, sscratch will contain curlwp.
	//
	REG_S	sp, L_MD_USP(tp)	// save user stack pointer temporarily
	PTR_L	sp, L_MD_UTF(tp)	// trapframe pointer loaded
	REG_S	t1, TF_T1(sp)		// save t1
	REG_L	t1, L_MD_USP(tp)	// get user stack pointer
	REG_S	t1, TF_SP(sp)		// save user stack pointer in trapframe

	csrrw	t1, sscratch, zero	// swap saved thread pointer with 0
	REG_S	t1, TF_TP(sp)		// save thread pointer in trapframe
	li	t1, 0			// indicate user exception
	j	.Lexception_common

	//
	// The exception happened while we were already in the kernel.  That
	// means tp already has curlwp and sp has the kernel stack pointer so
	// just need to restore it and then adjust it down for space for the
	// trap frame.  We save t1 so we can use it to save the original sp
	// into the trapframe for use by the exception exiting code.
	//
.Lexception_kernel:
	csrrw	tp, sscratch, zero	// get back our thread pointer
	addi	sp, sp, -TF_LEN		// allocate stack frame
	REG_S	t1, TF_T1(sp)		// save t1
	addi	t1, sp, TF_LEN
	REG_S	t1, TF_SP(sp)		// save SP
	li	t1, 1			// indicate kernel exception

.Lexception_common:
	// Now we save all the temporary registers into the trapframe since
	// they will most certainly be changed.
	REG_S	ra, TF_RA(sp)		// save return address
	REG_S	gp, TF_GP(sp)		// save gp
	REG_S	a0, TF_A0(sp)		// save a0
	REG_S	a1, TF_A1(sp)		// save a1
	REG_S	a2, TF_A2(sp)		// save a2
	REG_S	a3, TF_A3(sp)		// save a3
	REG_S	a4, TF_A4(sp)		// save a4
	REG_S	a5, TF_A5(sp)		// save a5
	REG_S	a6, TF_A6(sp)		// save a6
	REG_S	a7, TF_A7(sp)		// save a7
	REG_S	t0, TF_T0(sp)		// save t0
					// t1 is already saved
	REG_S	t2, TF_T2(sp)		// save t2
	REG_S	t3, TF_T3(sp)		// save t3
	REG_S	t4, TF_T4(sp)		// save t4
	REG_S	t5, TF_T5(sp)		// save t5
	REG_S	t6, TF_T6(sp)		// save t6

#if defined(DDB)
	REG_S	s0, TF_S0(sp)		// save s0 - frame pointer - useful
	REG_S	s1, TF_S1(sp)		// save s1
	REG_S	s2, TF_S2(sp)		// save s2
	REG_S	s3, TF_S3(sp)		// save s3
	REG_S	s4, TF_S4(sp)		// save s4
	REG_S	s5, TF_S5(sp)		// save s5
	REG_S	s6, TF_S6(sp)		// save s6
	REG_S	s7, TF_S7(sp)		// save s7
	REG_S	s8, TF_S8(sp)		// save s8
	REG_S	s9, TF_S9(sp)		// save s9
	REG_S	s10, TF_S10(sp)		// save s10
	REG_S	s11, TF_S11(sp)		// save s11
#endif

	/* Set the global pointer */
	.option push
	.option norelax
	lla	gp, __global_pointer$
	.option pop

	// Now we get the trap CRSs
	mv	a0, sp			// trapframe pointer
	csrr	a1, sepc		// get exception pc
	csrr	a2, sstatus		// get status
	csrr	a3, scause		// get cause

	REG_S	a1, TF_PC(sp)
	REG_S	a2, TF_SR(sp)
	REG_S	a3, TF_CAUSE(sp)	// save cause

	// Now we've saved the trapfame, the cause is still in a3.

	bltz	a3, intr_handler	// MSB is set if interrupt

	// stval is only relevant for non-interrupts
	csrr	a4, stval		// get stval
	REG_S	a4, TF_TVAL(sp)

	beqz	t1, trap_user		// this was a user trap

	// This was a kernel exception
	call	_C_LABEL(cpu_trap)	// just call trap to handle it

ALTENTRY(exception_kernexit)
	// If we got here, we are returning from a kernel exception (either a
	// trap or interrupt).  Simply return the volatile registers and the
	// exception PC and status, load the saved SP from the trapframe, and
	// return from the exception

	REG_L	ra, TF_RA(sp)		// restore return address
	REG_L	gp, TF_GP(sp)		// restore gp
	REG_L	s0, TF_S0(sp)		// only restore from userland
	REG_L	s1, TF_S1(sp)		// only restore from userland
	REG_L	s2, TF_S2(sp)		// only restore from userland
	REG_L	s3, TF_S3(sp)		// only restore from userland
	REG_L	s4, TF_S4(sp)		// only restore from userland
	REG_L	s5, TF_S5(sp)		// only restore from userland
	REG_L	s6, TF_S6(sp)		// only restore from userland
	REG_L	s7, TF_S7(sp)		// only restore from userland
	REG_L	s8, TF_S8(sp)		// only restore from userland
	REG_L	s9, TF_S9(sp)		// only restore from userland
	REG_L	s10, TF_S10(sp)		// only restore from userland
	REG_L	s11, TF_S11(sp)		// only restore from userland
	REG_L	a0, TF_A0(sp)		// restore a0
	REG_L	a1, TF_A1(sp)		// restore a1
	REG_L	a2, TF_A2(sp)		// restore a2
	REG_L	a3, TF_A3(sp)		// restore a3
	REG_L	a4, TF_A4(sp)		// restore a4
	REG_L	a5, TF_A5(sp)		// restore a5
	REG_L	a6, TF_A6(sp)		// restore a6
	REG_L	a7, TF_A7(sp)		// restore a7
	REG_L	t2, TF_T2(sp)		// restore t2
	REG_L	t3, TF_T3(sp)		// restore t3
	REG_L	t4, TF_T4(sp)		// restore t4
	REG_L	t5, TF_T5(sp)		// restore t5
	REG_L	t6, TF_T6(sp)		// restore t6

	REG_L	t0, TF_PC(sp)		// fetch exception PC
	REG_L	t1, TF_SR(sp)		// fetch status

	csrw	sstatus, t1		// restore sstatus (needs to have SIE=0)
	csrw	sepc, t0		// restore exception PC

	REG_L	t0, TF_T0(sp)		// restore t0
	REG_L	t1, TF_T1(sp)		// restore t1
	REG_L	sp, TF_SP(sp)		// restore SP
	sret				// and we're done

trap_user:
#if 0
	/* Already saved */
	REG_S	s0, TF_S0(sp)		// only save from userland
	REG_S	s1, TF_S1(sp)		// only save from userland
	REG_S	s2, TF_S2(sp)		// only save from userland
	REG_S	s3, TF_S3(sp)		// only save from userland
	REG_S	s4, TF_S4(sp)		// only save from userland
	REG_S	s5, TF_S5(sp)		// only save from userland
	REG_S	s6, TF_S6(sp)		// only save from userland
	REG_S	s7, TF_S7(sp)		// only save from userland
	REG_S	s8, TF_S8(sp)		// only save from userland
	REG_S	s9, TF_S9(sp)		// only save from userland
	REG_S	s10, TF_S10(sp)		// only save from userland
	REG_S	s11, TF_S11(sp)		// only save from userland
#endif

	csrsi	sstatus, SR_SIE		// reenable interrupts
	li	t0, SR_SUM | SR_FS
	csrc	sstatus, t0		// disable user memory access and FP

	li	t0, CAUSE_SYSCALL	// let's see if this was a syscall
	beq	a3, t0, trap_syscall	//   yes it was

	call	_C_LABEL(cpu_trap)	// nope, just a regular trap

_C_LABEL(exception_userexit):
	csrci	sstatus, SR_SIE		// disable interrupts
	INT_L	t0, L_MD_ASTPENDING(tp)	// ast pending?
	bnez	t0, trap_doast		//   yes, handle it.
	csrw	sscratch, tp		// show we are coming from userland
	REG_L	tp, TF_TP(sp)		// only restore from userland
#if 0
	REG_L	s0, TF_S0(sp)		// only restore from userland
	REG_L	s1, TF_S1(sp)		// only restore from userland
	REG_L	s2, TF_S2(sp)		// only restore from userland
	REG_L	s3, TF_S3(sp)		// only restore from userland
	REG_L	s4, TF_S4(sp)		// only restore from userland
	REG_L	s5, TF_S5(sp)		// only restore from userland
	REG_L	s6, TF_S6(sp)		// only restore from userland
	REG_L	s7, TF_S7(sp)		// only restore from userland
	REG_L	s8, TF_S8(sp)		// only restore from userland
	REG_L	s9, TF_S9(sp)		// only restore from userland
	REG_L	s10, TF_S10(sp)		// only restore from userland
	REG_L	s11, TF_S11(sp)		// only restore from userland
#endif
	j	exception_kernexit

trap_syscall:
	PTR_LA	ra, exception_userexit
	PTR_L	t0, L_PROC(tp)		// get proc struct
	PTR_L	t0, P_MD_SYSCALL(t0)	// get syscall address from proc
	jr	t0			// and jump to it

intr_user:
	REG_S	s0, TF_S0(sp)		// only save from userland
	REG_S	s1, TF_S1(sp)		// only save from userland
	REG_S	s2, TF_S2(sp)		// only save from userland
	REG_S	s3, TF_S3(sp)		// only save from userland
	REG_S	s4, TF_S4(sp)		// only save from userland
	REG_S	s5, TF_S5(sp)		// only save from userland
	REG_S	s6, TF_S6(sp)		// only save from userland
	REG_S	s7, TF_S7(sp)		// only save from userland
	REG_S	s8, TF_S8(sp)		// only save from userland
	REG_S	s9, TF_S9(sp)		// only save from userland
	REG_S	s10, TF_S10(sp)		// only save from userland
	REG_S	s11, TF_S11(sp)		// only save from userland
	PTR_LA	ra, exception_userexit

	tail	_C_LABEL(cpu_intr)	// handle interrupt

/*
 */

trap_doast:
	INT_S	zero, L_MD_ASTPENDING(tp)
	csrsi	sstatus, SR_SIE		// reenable interrupts
	mv	a0, sp			// only argument is trapframe
	// ra is still exception_userexit ?
	tail	_C_LABEL(cpu_ast)


/*
 */

intr_handler:
	beqz	t1, intr_user
	PTR_LA	ra, exception_kernexit
	tail	_C_LABEL(cpu_intr)
END(cpu_exception_handler)


/*
 * int
 * cpu_set_onfault(struct faultbuf *fb, register_t retval)
 */
ENTRY_NP(cpu_set_onfault)
	REG_S	ra, FB_RA(a0)
	REG_S	s0, FB_S0(a0)
	REG_S	s1, FB_S1(a0)
	REG_S	s2, FB_S2(a0)
	REG_S	s3, FB_S3(a0)
	REG_S	s4, FB_S4(a0)
	REG_S	s5, FB_S5(a0)
	REG_S	s6, FB_S6(a0)
	REG_S	s7, FB_S7(a0)
	REG_S	s8, FB_S8(a0)
	REG_S	s9, FB_S9(a0)
	REG_S	s10, FB_S10(a0)
	REG_S	s11, FB_S11(a0)
	REG_S	sp, FB_SP(a0)
	REG_S	a1, FB_A0(a0)
	PTR_S	a0, L_MD_ONFAULT(tp)
	li	a0, 0
	ret
END(cpu_set_onfault)


ENTRY_NP(setjmp)
	REG_S	ra, FB_RA(a0)
	REG_S	s0, FB_S0(a0)
	REG_S	s1, FB_S1(a0)
	REG_S	s2, FB_S2(a0)
	REG_S	s3, FB_S3(a0)
	REG_S	s4, FB_S4(a0)
	REG_S	s5, FB_S5(a0)
	REG_S	s6, FB_S6(a0)
	REG_S	s7, FB_S7(a0)
	REG_S	s8, FB_S8(a0)
	REG_S	s9, FB_S9(a0)
	REG_S	s10, FB_S10(a0)
	REG_S	s11, FB_S11(a0)
	REG_S	sp, FB_SP(a0)
	li	a0, 0
	ret
END(setjmp)


ENTRY_NP(longjmp)
	REG_L	ra, FB_RA(a0)
	REG_L	s0, FB_S0(a0)
	REG_L	s1, FB_S1(a0)
	REG_L	s2, FB_S2(a0)
	REG_L	s3, FB_S3(a0)
	REG_L	s4, FB_S4(a0)
	REG_L	s5, FB_S5(a0)
	REG_L	s6, FB_S6(a0)
	REG_L	s7, FB_S7(a0)
	REG_L	s8, FB_S8(a0)
	REG_L	s9, FB_S9(a0)
	REG_L	s10, FB_S10(a0)
	REG_L	s11, FB_S11(a0)
	REG_L	sp, FB_SP(a0)
	mv	a0, a1
	ret
END(longjmp)
