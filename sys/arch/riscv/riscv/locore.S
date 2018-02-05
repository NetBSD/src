/* $NetBSD: locore.S,v 1.7 2018/02/05 10:41:12 maya Exp $ */
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

#include <machine/asm.h>
#include "assym.h"

	.globl	_C_LABEL(exception_userexit)
	.globl	_C_LABEL(cpu_Debugger_insn)

ENTRY_NP(start)
	// We get loaded and starting running at or near 0, not where we
	// should be. We need to construct an initial PDETAB
	
#ifdef _LP64
	li	t0, SR_U64|SR_S64
	li	t1, SR_IM|SR_VM|SR_EI
	csrs	sstatus, t0
#else
	li	t1, SR_IM|SR_VM|SR_U64|SR_S64|R_EI
#endif
	csrc	sstatus, t1

	li	s11, VM_MAX_KERNEL_ADDRESS
	li	s10, PAGE_SIZE
	li	s9, USPACE

#if 0
	// The kernel doesn't use gp/_gp since we'd have to reload it on
	// each exception.
	PTR_LA	gp, _C_LABEL(_gp)
#endif

	PTR_LA	a0, _C_LABEL(__bss_start)
	PTR_LA	s1, _C_LABEL(_end)
	li	a1, 0

	add	s1, s1, s10		// PAGE_SIZE
	addi	s1, s1, -1		//   -1 == PAGE_MASK
	neg	a1, a0			// -PAGE_SIZE
	and	s1, s1, a1		// s1 is page aligned end of kernel
					// s1 = uarea
	add	s2, s1, s9		// s2 = first PDE page
#ifdef _LP64
	add	s3, s2, s10		// s3 = second PDE page (RV64 only)
#else
	move	s3, 22
#endif
	add	s4, s3, s10		// s4 = first kernel PTE page
	add	s5, s1, s9		// s5 = kernel_end
	sub	a2, s5, a0
	call	memset			// zero through kernel_end

	// As a temporary hack, word 0 contains the amount of memory in MB
	INT_L	a7, (zero)		// load memory size
	slli	a7, a7, (20-PGSHIFT)	// convert MB to pages
.L01:	PTR_LA	t0, physmem
	INT_S	a7, (t0)		// store it in physmem

	li	t4, PTE_V | PTE_SX | PTE_SW | PTE_SR | PTE_G
#ifdef _LP64
	REG_S	t4, 0(s2)		// keep a mapping for the first 8GB.
	or	t0, s3, t4		// point to next page
	or	t0, t0, PTE_T		// tranverse it.
	REG_S	t0, -SZREG(s3)		// store in highest first level PDE
#endif

#if (VM_MIN_KERNEL_ADDRESS >> XSEGSHIFT) != (VM_MAX_KERNEL_ADDRESS >> XSEGSHIFT)
#error VM_MIN_KERNEL_ADDRESS not in same first level PDE as VM_MAX_KERNEL_ADDRESS
#endif
	// We allocated the kernel first PTE page so let's insert in the
	// page table.  For now, we assume it's in the same PDE page as the
	// direct-mapped memory.
	or	t0, s4, t4
	or	t0, t0, PTE_T
#if ((VM_MIN_KERNEL_ADDRESS >> SEGSHIFT) & (NPDEPG-1)) * SZREG
	li	t1, ((VM_MIN_KERNEL_ADDRESS >> SEGSHIFT) & (NPDEPG-1)) * SZREG
	add	t1, t1, s3
	REG_S	t0, 0(t1)
#else
	REG_S	t0, 0(s3)
#endif

	li	t0, ((VM_MAX_KERNEL_ADDRESS >> SEGSHIFT) & (NPDEPG-1)) * SZREG
	add	s3, s3, t0
	srli	a7, a7, (SEGSHIFT-PGSHIFT) // pages to segments
	li	t3, NBSEG		// load for ease

	//
	// Fill in the PDEs to direct map memory.
	//
.Lfill:	REG_S	t4, 0(s3)		// store PDE
	add	t4, t4, t3		// advance PA in PDE to next segment
	add	s3, s3, SZREG		// advance to next PDE slot
	addi	a7, a7, -1		// count down segment
	bgtz	a6, .Lfill		// loop if more

	csrw	sptbr, s1		// set the page table base
	li	t0, SR_VM
	csrs	sstatus, t0		// Enable VM

	// We should have a VM so let's start using our real addresses
	lui	t0, %hi(.Lmmu_on)	// load hi part of absolute address
	jr	t0, %lo(.Lmmu_on)	// jump to absolute address

.Lmmu_on:
	// MMU is on!
	csrw	sscratch, zero		// zero in sscratch to mark kernel

	PTR_LA	tp, _C_LABEL(lwp0)	// put curlwp in tp

	PTR_LA	a0, _C_LABEL(cpu_exception_handler)
	csrw	stvec, a0

	PTR_S	s1, L_PCB(tp)		// set uarea of lwp (already zeroed)
	addi	sp, s2, -TF_LEN		// switch to new stack
	PTR_S	sp, L_MD_UTF(tp)	// store pointer to empty trapframe

	PTR_LA	t1, _C_LABEL(kernel_pmap_store)
	add	t2, s2, s11 		// PA -> VA
	PTR_S	t2, PM_PDETAB(t1)	// VA of kernel PDETAB
	PTR_S	s2, PM_MD_PTBR(t1)	// PA of kernel PDETAB

	// Now we should ready to start initializing the kernel.
	PTR_LA	a0, _C_LABEL(start)	// kernel_start
	add	a1, s5, s11		// kernel_end
	call	_C_LABEL(init_riscv)	// do MD startup
	tail	_C_LABEL(main)		// and transfer to main
	// not reached
END(start)

//
// struct lwp *cpu_switchto(struct lwp *oldl, struct lwp *newl, bool returning);
//
ENTRY_NP(cpu_switchto)
	addi	sp, sp, -TF_LEN		// allocate trapframe

	beqz	a0, .Lswitchto_newlwp	// can skip saving oldl state?

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
	csrr	t4, sstatus		// get status for intr state
	REG_S	t4, TF_SR(sp)		// save it
	
	REG_S	sp, L_MD_KTF(a0)	// record trapframe pointer
.Lswitchto_newlwp:
	csrrci	t0, sstatus, SR_EI	// # disable interrupts

	move	tp, a1			// # put the new lwp in thread pointer

	PTR_L	t1, L_CPU(tp)		// # get curcpu
	PTR_S	tp, CI_CURLWP(t1)	// # update curcpu with the new curlwp

	REG_L	sp, L_MD_KTF(tp)	// # load its kernel stack pointer
	REG_L	t4, TF_SR(sp)		// # fetch status register
	csrw	sstatus, t4		// # restore it (and interrupts?)

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
	//	a1 = curcpu()
	//	tp = newl

	ret
END(cpu_switchto)

ENTRY_NP(cpu_lwp_trampoline)
	move	a1, tp			// get new lwp
	call	_C_LABEL(lwp_startup)	// call lwp startup

	move	a0, s1			// get saved arg
	jalr	s0			// call saved func

	// If the saved func returns, we are returning to user land.
	j	_C_LABEL(exception_userexit)
END(cpu_lwp_trampoline)

ENTRY_NP(cpu_fast_switchto_cleanup)
	INT_L	t0, CI_MTX_COUNT(a1)	// get mutex count
	REG_L	ra, CALLFRAME_RA(sp)	// get return address
	REG_L	a0, CALLFRAME_S0(sp)	// get pinned LWP
	addi	t0, t0, 1		// increment mutex count
	INT_S	t0, CI_MTX_COUNT(a1)	// save it
	PTR_S	zero, L_CTXSWTCH(a0)	// clear l_ctxswitch
	addi	sp, sp, CALLFRAME_SIZ	// remove callframe
#if IPL_SCHED != IPL_HIGH
	tail	_C_LABEL(splhigh)	// go back to IPL HIGH
#else
	ret				// just return
#endif
END(cpu_fast_switchto_cleanup)

//
// void cpu_fast_switchto(struct lwp *, int s);
//
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

	move	s0, tp			// remember curlwp
	move	s1, sp			// remember kernel stack

#if 0
	csrrci	t0, sstatus, SR_EI	// disable interrupts
#endif
	PTR_L	t1, L_CPU(tp)		// get curcpu()

	PTR_S	sp, L_MD_KTF(tp)	// save trapframe ptr in oldlwp
	move	tp, a0			// set thread pointer to newlwp
	PTR_S	tp, CI_CURLWP(t1)	// update curlwp
	PTR_L	sp, L_MD_KTF(tp)	// switch to its stack
#if 0
	csrw	sstatus, t0		// reenable interrupts
#endif
	call	_C_LABEL(softint_dispatch)
#if 0
	csrrci	t0, sstatus, SR_EI	// disable interrupts
#endif
	PTR_L	t1, L_CPU(tp)		// get curcpu() again
	move	tp, s0			// return to pinned lwp
	PTR_S	tp, CI_CURLWP(t1)	// restore curlwp
#if 0
	csrw	sstatus, t0		// reeanble interrupts
#endif
	move	sp, s1			// restore stack pointer

	REG_L	ra, (TF_RA + CALLFRAME_RA)(sp)	// get return address
	REG_L	s0, TF_S0(sp)		// restore register we used
	REG_L	s1, TF_S1(sp)		// restore register we used

	addi	sp, sp, TF_LEN+CALLFRAME_SIZ	// drop trapframe/callframe
	ret				// return
END(cpu_fast_switchto)

// RISCV only has a simple exception handler handles both synchronous traps
// and interrupts.
ENTRY_NP(cpu_exception_handler)
	csrrw	tp, sscratch, tp	// swap scratch and thread pointer
	beqz	tp, .Lexception_kernel	//   tp == 0, already on kernel stack
	//
	// The execption happened while user code was executing.  We need to
	// get the pointer to the user trapframe from the LWP md area.  Then we
	// save t1 and tp so we have a register to work with and to get curlwp
	// into tp.  We also save the saved SP into the trapframe.
	// Upon entry on an exception from user, sscratch will contain curlwp.
	//
	REG_S	sp, L_MD_USP(tp)	// save user stack pointer temporarily
	PTR_L	sp, L_MD_UTF(sp)	// trapframe pointer loaded
	REG_S	t1, TF_T1(sp)		// save t1
	REG_L	t1, L_MD_USP(tp)	// get user stack pointer
	REG_S	t1, TF_SP(sp)		// save thread pointer in trapframe
	csrrw	t1, sscratch, zero	// swap saved thread pointer with 0
	REG_L	t1, TF_TP(sp)		// save thread pointer in trapframe
	li	t1, 0			// indicate user exception
	j	.Lexception_common

	//
	// The exception happened while we were already in the kernel.  That
	// means tp already has curlwp and sp has the kernel stack pointer so
	// just need to restore it and then adjust it down for space for the
	// trap frame.  We save t1 so we can use it the original sp into the
	// trapframe for use by the exception exiting code.
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

	// Now we get the 
	move	a0, sp			// trapframe pointer
	csrr	a1, sepc		// get execption pc
	csrr	a2, sstatus		// get status
	csrr	a3, scause		// get cause

	REG_S	a1, TF_PC(sp)
	INT_S	a2, TF_SR(sp)
	INT_S	a3, TF_CAUSE(sp)	// save cause

	// Now we've saved the trapfame, the cause is still in a3.

	bltz	a3, intr_handler	// MSB is set if interrupt

	// badaddr is only relavent for non-interrupts
	csrr	a4, sbadaddr		// get badaddr
	REG_S	a4, TF_BADADDR(sp)

	beqz	t1, trap_user		// this was a user trap
	// This was a kernel exception
	call	_C_LABEL(cpu_trap)	// just call trap to handle it
exception_kernexit:
	// If we got here, we are returning from a kernel exception (either a
	// trap or interrupt).  Simply return the volatile registers and the
	// exception PC and status, load the saved SP from the trapframe, and
	// return from the exception
	csrrci	zero, sstatus, SR_EI	// disable interrupts

	REG_L	ra, TF_RA(sp)		// restore return address
	REG_L	gp, TF_GP(sp)		// restore gp
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
	REG_L	t5, TF_T3(sp)		// restore t5
	REG_L	t6, TF_T4(sp)		// restore t6

	REG_L	t0, TF_PC(sp)		// fetch execption PC
	REG_L	t1, TF_SR(sp)		// fetch status

	csrw	sepc, t0		// restore execption PC
	csrw	sstatus, t1		// restore status

	REG_L	t0, TF_T0(sp)		// restore t0
	REG_L	t1, TF_T1(sp)		// restore t1
	REG_L	sp, TF_SP(sp)		// restore SP
	sret				// and we're done

trap_user:
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

	csrsi	sstatus, SR_EI		// reenable interrupts

	li	t0, CAUSE_SYSCALL	// let's see if this was a syscall
	beq	a3, t0, trap_syscall	//   yes it was

	call	_C_LABEL(cpu_trap)	// nope, just a regular trap
_C_LABEL(exception_userexit):
	INT_L	t0, L_MD_ASTPENDING(tp)	// ast pending?
	bnez	t0, trap_doast		//   yes, handle it.
	csrrci	zero, sstatus, SR_EI	// disable interrupts
	csrw	sscratch, tp		// show we are coming from userland
	REG_L	tp, TF_TP(sp)		// only restore from userland
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
	j	exception_kernexit

trap_syscall:
.L0:	PTR_L	ra, exception_userexit
	PTR_L	t0, L_PROC(tp)		// get proc struct
	PTR_L	t0, P_MD_SYSCALL(t0)	// get syscall address from proc
	jr	t0			// and jump to it

intr_usersave:
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
trap_doast:
	move	a0, sp			// only argument is trapframe
	tail	_C_LABEL(cpu_ast)

intr_user:
	call	_C_LABEL(cpu_intr)	// handle interrupt
	INT_L	t0, L_MD_ASTPENDING(tp)	// get astpending
	bnez	t0, intr_usersave	//    if one is pending, deal with in

	csrw	sscratch, tp		// show we are coming from userland
	REG_L	tp, TF_TP(sp)		// restore thread pointer
	j	exception_kernexit	// do standard exception exit

intr_handler:
	beqz	t1, intr_user
	call	_C_LABEL(cpu_intr)
	j	exception_kernexit
END(cpu_exception_handler)

// int cpu_set_onfault(struct faultbuf *fb, register_t retval)
//
ENTRY(cpu_set_onfault)
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

ENTRY(setjmp)
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

ENTRY(longjmp)
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
	move	a0, a1
	ret
END(longjmp)

ENTRY_NP(cpu_Debugger)
cpu_Debugger_insn:
	sbreak
	ret
END(cpu_Debugger)
