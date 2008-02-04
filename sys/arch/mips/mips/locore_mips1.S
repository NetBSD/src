/*	$NetBSD: locore_mips1.S,v 1.58.16.4 2008/02/04 09:22:13 yamt Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Digital Equipment Corporation and Ralph Campbell.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permited provided that the following conditions
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
#include "opt_cputype.h"
#include "opt_ddb.h"
#include "opt_kgdb.h"

#include <sys/cdefs.h>

#include <mips/asm.h>
#include <mips/cpuregs.h>
#include <machine/param.h>

#include "assym.h"

	.set	noreorder
	.text

EXPORT(mips1_exceptionentry_start)

/*
 * mips1_UTLBMiss
 *
 * A reference is made (in either kernel or user mode) to a page in
 * kuseg that has no matching TLB entry.  This routine is copied down
 * at 0x80000000 and total length must be less than 32 instructions.
 * No pc relative jump instruction is allowed.
 */
VECTOR(mips1_UTLBMiss, unknown)
	.set	noat
	mfc0	k0, MIPS_COP_0_BAD_VADDR	# get the virtual address
	lw	k1, _C_LABEL(segbase)		# get the current segment table
	bltz	k0, 1f				# R3000 chip bug
	srl	k0, k0, SEGSHIFT		# compute segment table index
	sll	k0, k0, 2
	addu	k1, k1, k0
	mfc0	k0, MIPS_COP_0_BAD_VADDR	# get the virtual address
	lw	k1, 0(k1)			# get pointer to segment map
	srl	k0, k0, PGSHIFT - 2		# compute segment map index
	and	k0, k0, (NPTEPG - 1) << 2
	beq	k1, zero, 2f			# invalid segment map
	addu	k1, k1, k0			# index into segment map
	lw	k0, 0(k1)			# get page PTE
	nop
	beq	k0, zero, 2f			# dont load invalid entries
	mtc0	k0, MIPS_COP_0_TLB_LOW
	nop
	tlbwr					# update TLB
1:
	mfc0	k1, MIPS_COP_0_EXC_PC		# get return address
	nop
	j	k1
	rfe
2:
	j	mips1_SlowFault			# handle the rest
	nop
	.set	at
VECTOR_END(mips1_UTLBMiss)


/*
 * mips1_exception
 *
 * Handles any exceptions other than reset and UTLB miss.  This routine
 * is copied down at 0x80000080 and total length must be less than 32
 * instructions.  No pc relative jump instruction is allowed.
 */
VECTOR(mips1_exception, unknown)
/*
 * Find out what mode we came from and jump to the proper handler.
 */
	.set	noat
	mfc0	k0, MIPS_COP_0_STATUS		# get the status register
	mfc0	k1, MIPS_COP_0_CAUSE		# get the cause register
	and	k0, k0, MIPS1_SR_KU_PREV	# test for user mode
	sll	k0, k0, 4			# shift user bit for cause index
	and	k1, k1, MIPS1_CR_EXC_CODE	# mask out the cause bits
	or	k1, k1, k0			# change index to user table
1:
	la	k0, mips1_excpt_sw		# get base of the jump table
	addu	k0, k0, k1			# get the address of the
						#  function entry.  Note that
						#  the cause is already
						#  shifted left by 2 bits so
						#  we dont have to shift
	lw	k0, 0(k0)			# get the function address
	nop
	j	k0				# jump to the function
	nop
	.set	at
VECTOR_END(mips1_exception)


/*
 * mips1_SlowFault
 *
 * UTLBMiss handler could not find out a TLB entry for the user process.
 * Dispatch a general case exception handler.
 */
mips1_SlowFault:
	.set	noat
	mfc0	k1, MIPS_COP_0_STATUS
	la	k0, _C_LABEL(mips1_KernGenException)
	and	k1, k1, MIPS1_SR_KU_PREV
	beq	k1, zero, 1f
	nop
	la	k0, _C_LABEL(mips1_UserGenException)
1:	j	k0
	nop
	.set	at

/*
 * mips1_KernGenException
 *
 * Handle an exception during kernel mode.
 * Build trapframe on stack to hold interrupted kernel context, then
 * call trap() to process the condition.
 *
 * trapframe is now pointed by 5th arg {
 *	register_t cf_args[4 + 1];
 *	register_t cf_pad;
 *	register_t cf_sp;
 *	register_t cf_ra;
 *	mips_reg_t tf_regs[17];		- trapframe begins here
 * 	mips_reg_t tf_sr;		-
 * 	mips_reg_t tf_mullo;		-
 * 	mips_reg_t tf_mulhi;		-
 * 	register_t tf_epc;		- may be changed by trap() call
 * };
 */
NESTED_NOPROFILE(mips1_KernGenException, KERNFRAME_SIZ, ra)
	.set	noat
	.mask	0x80000000, -4
#if defined(DDB) || defined(KGDB)
	la	k0, _C_LABEL(kdbaux)
	sw	s0, SF_REG_S0(k0)
	sw	s1, SF_REG_S1(k0)
	sw	s2, SF_REG_S2(k0)
	sw	s3, SF_REG_S3(k0)
	sw	s4, SF_REG_S4(k0)
	sw	s5, SF_REG_S5(k0)
	sw	s6, SF_REG_S6(k0)
	sw	s7, SF_REG_S7(k0)
	sw	sp, SF_REG_SP(k0)
	sw	s8, SF_REG_S8(k0)
	sw	gp, SF_REG_RA(k0)
#endif
/*
 * Save the relevant kernel registers onto the stack.
 * We don't need to save s0 - s8, sp and gp because
 * the compiler does it for us.
 */
	subu	sp, sp, KERNFRAME_SIZ
	sw	AT, TF_BASE+TF_REG_AST(sp)
	sw	v0, TF_BASE+TF_REG_V0(sp)
	sw	v1, TF_BASE+TF_REG_V1(sp)
	mflo	v0
	mfhi	v1
	sw	a0, TF_BASE+TF_REG_A0(sp)
	sw	a1, TF_BASE+TF_REG_A1(sp)
	sw	a2, TF_BASE+TF_REG_A2(sp)
	sw	a3, TF_BASE+TF_REG_A3(sp)
	mfc0	a0, MIPS_COP_0_STATUS		# 1st arg is STATUS
	sw	t0, TF_BASE+TF_REG_T0(sp)
	sw	t1, TF_BASE+TF_REG_T1(sp)
	sw	t2, TF_BASE+TF_REG_T2(sp)
	sw	t3, TF_BASE+TF_REG_T3(sp)
	mfc0	a1, MIPS_COP_0_CAUSE		# 2nd arg is CAUSE
	sw	t4, TF_BASE+TF_REG_T4(sp)
	sw	t5, TF_BASE+TF_REG_T5(sp)
	sw	t6, TF_BASE+TF_REG_T6(sp)
	sw	t7, TF_BASE+TF_REG_T7(sp)
	mfc0	a2, MIPS_COP_0_BAD_VADDR	# 3rd arg is fault address
	sw	t8, TF_BASE+TF_REG_T8(sp)
	sw	t9, TF_BASE+TF_REG_T9(sp)
	sw	ra, TF_BASE+TF_REG_RA(sp)
	sw	a0, TF_BASE+TF_REG_SR(sp)
	mfc0	a3, MIPS_COP_0_EXC_PC		# 4th arg is exception PC
	sw	v0, TF_BASE+TF_REG_MULLO(sp)
	sw	v1, TF_BASE+TF_REG_MULHI(sp)
	sw	a3, TF_BASE+TF_REG_EPC(sp)
	addu	v0, sp, TF_BASE
	sw	v0, KERNFRAME_ARG5(sp)		# 5th arg is p. to trapframe
/*
 * Call the trap handler.
 */
#if defined(DDB) || defined(DEBUG) || defined(KGDB)
	addu	v0, sp, KERNFRAME_SIZ
	sw	v0, KERNFRAME_SP(sp)
#endif
	jal	_C_LABEL(trap)
	sw	a3,	KERNFRAME_RA(sp)	# for debugging
/*
 * Restore registers and return from the exception.
 */
	lw	a0, TF_BASE+TF_REG_SR(sp)
	lw	t0, TF_BASE+TF_REG_MULLO(sp)
	lw	t1, TF_BASE+TF_REG_MULHI(sp)
	mtc0	a0, MIPS_COP_0_STATUS
	mtlo	t0
	mthi	t1
/*
 * Check for kernel restartable atomic sequences.
 */
	lw	k1, TF_BASE+TF_REG_EPC(sp)	# restore exception PC
	la	t0, _C_LABEL(_lock_ras_start)
	li	t1, -MIPS_LOCK_RAS_SIZE
	and	t1, t1, k1
	bne	t1, t0, 1f
	nop
	jal	_C_LABEL(_lock_ras)
1:
	lw	AT, TF_BASE+TF_REG_AST(sp)
	lw	v0, TF_BASE+TF_REG_V0(sp)
	lw	v1, TF_BASE+TF_REG_V1(sp)
	lw	a0, TF_BASE+TF_REG_A0(sp)
	lw	a1, TF_BASE+TF_REG_A1(sp)
	lw	a2, TF_BASE+TF_REG_A2(sp)
	lw	a3, TF_BASE+TF_REG_A3(sp)
	lw	t0, TF_BASE+TF_REG_T0(sp)
	lw	t1, TF_BASE+TF_REG_T1(sp)
	lw	t2, TF_BASE+TF_REG_T2(sp)
	lw	t3, TF_BASE+TF_REG_T3(sp)
	lw	t4, TF_BASE+TF_REG_T4(sp)
	lw	t5, TF_BASE+TF_REG_T5(sp)
	lw	t6, TF_BASE+TF_REG_T6(sp)
	lw	t7, TF_BASE+TF_REG_T7(sp)
	lw	t8, TF_BASE+TF_REG_T8(sp)
	lw	t9, TF_BASE+TF_REG_T9(sp)
	lw	ra, TF_BASE+TF_REG_RA(sp)
	addu	sp, sp, KERNFRAME_SIZ
#ifdef DDBnotyet
	la	k0, _C_LABEL(kdbaux)
	lw	s0, SF_REG_S0(k0)
	lw	s1, SF_REG_S1(k0)
	lw	s2, SF_REG_S2(k0)
	lw	s3, SF_REG_S3(k0)
	lw	s4, SF_REG_S4(k0)
	lw	s5, SF_REG_S5(k0)
	lw	s6, SF_REG_S6(k0)
	lw	s7, SF_REG_S7(k0)
	lw	sp, SF_REG_SP(k0)
	lw	s8, SF_REG_S8(k0)
	lw	gp, SF_REG_RA(k0)
#endif
	j	k1				# return to interrupted point
	rfe
	.set	at
END(mips1_KernGenException)

/*
 * mips1_UserGenException
 *
 * Handle an exception during user mode.
 * Save user context atop of kernel stack, then call trap() to process
 * the condition.  The context can be manipulated alternatively via
 * curlwp->l_md.md_regs.
 */
NESTED_NOPROFILE(mips1_UserGenException, CALLFRAME_SIZ, ra)
	.set	noat
	.mask	0x80000000, -4
/*
 * Save all the registers but the kernel temporaries onto stack.
 */
	lw	k1, CPUVAR(CURLWP)
	nop
	lw	k1, L_ADDR(k1)
	nop
	addu	k1, k1, USPACE - FRAME_SIZ
	sw	AT, FRAME_AST(k1)
	sw	v0, FRAME_V0(k1)
	sw	v1, FRAME_V1(k1)
	mflo	v0
	sw	a0, FRAME_A0(k1)
	sw	a1, FRAME_A1(k1)
	sw	a2, FRAME_A2(k1)
	sw	a3, FRAME_A3(k1)
	mfhi	v1
	sw	t0, FRAME_T0(k1)
	sw	t1, FRAME_T1(k1)
	sw	t2, FRAME_T2(k1)
	sw	t3, FRAME_T3(k1)
	mfc0	a0, MIPS_COP_0_STATUS		# 1st arg is STATUS
	sw	t4, FRAME_T4(k1)
	sw	t5, FRAME_T5(k1)
	sw	t6, FRAME_T6(k1)
	sw	t7, FRAME_T7(k1)
	mfc0	a1, MIPS_COP_0_CAUSE		# 2nd arg is CAUSE
	sw	s0, FRAME_S0(k1)
	sw	s1, FRAME_S1(k1)
	sw	s2, FRAME_S2(k1)
	sw	s3, FRAME_S3(k1)
	mfc0	a2, MIPS_COP_0_BAD_VADDR	# 3rd arg is fault address
	sw	s4, FRAME_S4(k1)
	sw	s5, FRAME_S5(k1)
	sw	s6, FRAME_S6(k1)
	sw	s7, FRAME_S7(k1)
	mfc0	a3, MIPS_COP_0_EXC_PC		# 4th arg is exception PC
	sw	t8, FRAME_T8(k1)
	sw	t9, FRAME_T9(k1)
	sw	gp, FRAME_GP(k1)
	sw	sp, FRAME_SP(k1)
	sw	s8, FRAME_S8(k1)
	sw	ra, FRAME_RA(k1)
	sw	a0, FRAME_SR(k1)
	sw	v0, FRAME_MULLO(k1)
	sw	v1, FRAME_MULHI(k1)
	sw	a3, FRAME_EPC(k1)
	addu	sp, k1, -CALLFRAME_SIZ		# switch to kernel SP
#ifdef __GP_SUPPORT__
	la	gp, _C_LABEL(_gp)		# switch to kernel GP
#endif
	.set	at
	and	t0, a0, ~MIPS_SR_COP_1_BIT	# turn off the FPU
	.set	noat
#if defined(DDB) || defined(DEBUG) || defined(KGDB)
	move	ra, a3
	sw	ra, CALLFRAME_RA(sp)
#endif
/*
 * Call the exception handler.
 */
	lw	MIPS_CURLWP, CPUVAR(CURLWP)
	jal	_C_LABEL(trap)
	mtc0	t0, MIPS_COP_0_STATUS
/*
 * Check pending asynchronous traps.
 */
	lw	t0, L_MD_ASTPENDING(MIPS_CURLWP)# any pending ast?
	nop
	beq	t0, zero, 1f			# if no, skip ast processing
	nop
/*
 * We have pending asynchronous traps; all the state is already saved.
 */
	jal	_C_LABEL(ast)
	lw	a0, CALLFRAME_SIZ + FRAME_EPC(sp)
1:
	la	ra, _C_LABEL(mips1_xcpt_return)
	j	ra				# pretend function return
	nop
	.set	at
END(mips1_UserGenException)

/*
 * mips1_SystemCall
 *
 * Save user context atop of kernel stack, then call syscall() to process
 * the condition.  The context can be manipulated alternatively via
 * curlwp->l_md.md_regs.
 */
NESTED_NOPROFILE(mips1_SystemCall, CALLFRAME_SIZ, ra)
	.set	noat
	.mask	0x80000000, -4
/*
 * Save all the registers but kernel temporaries onto stack.
 */
	lw	k1, CPUVAR(CURLWP)
	nop
	lw	k1, L_ADDR(k1)
	nop
	addu	k1, k1, USPACE - FRAME_SIZ
	#sw	AT, FRAME_AST(k1)
	.set	at
	sw	v0, FRAME_V0(k1)		# syscall #
	sw	v1, FRAME_V1(k1)		# used by syscall()
	mflo	v0
	sw	a0, FRAME_A0(k1)
	sw	a1, FRAME_A1(k1)
	sw	a2, FRAME_A2(k1)
	sw	a3, FRAME_A3(k1)
	lw	a0, CPUVAR(CURLWP)		# 1st arg is curlwp
	mfhi	v1
	#sw	t0, FRAME_T0(k1)		# no need to save temp regs
	#sw	t1, FRAME_T1(k1)
	#sw	t2, FRAME_T2(k1)
	#sw	t3, FRAME_T3(k1)
	mfc0	a1, MIPS_COP_0_STATUS		# 2nd arg is STATUS
	#sw	t4, FRAME_T4(k1)
	#sw	t5, FRAME_T5(k1)
	#sw	t6, FRAME_T6(k1)
	sw	t7, FRAME_T7(k1)
	mfc0	a2, MIPS_COP_0_CAUSE		# 3rd arg is CAUSE
	sw	s0, FRAME_S0(k1)
	sw	s1, FRAME_S1(k1)
	sw	s2, FRAME_S2(k1)
	sw	s3, FRAME_S3(k1)
	mfc0	a3, MIPS_COP_0_EXC_PC		# 4th arg is PC
	sw	s4, FRAME_S4(k1)
	sw	s5, FRAME_S5(k1)
	sw	s6, FRAME_S6(k1)
	sw	s7, FRAME_S7(k1)
	#sw	t8, FRAME_T8(k1)
	#sw	t9, FRAME_T9(k1)
	sw	gp, FRAME_GP(k1)
	sw	sp, FRAME_SP(k1)
	sw	s8, FRAME_S8(k1)
	sw	ra, FRAME_RA(k1)
	sw	a1, FRAME_SR(k1)
	sw	v0, FRAME_MULLO(k1)
	sw	v1, FRAME_MULHI(k1)
	sw	a3, FRAME_EPC(k1)
	addu	sp, k1, -CALLFRAME_SIZ		# switch to kernel SP
#ifdef __GP_SUPPORT__
	la	gp, _C_LABEL(_gp)		# switch to kernel GP
#endif
#if defined(DDB) || defined(DEBUG) || defined(KGDB)
	move	ra, a3
	sw	ra, CALLFRAME_RA(sp)
#endif
	lw	t0, L_PROC(a0)			# t0 = curlwp->l_proc
	move	MIPS_CURLWP, a0			# set curlwp reg
	lw	t1, P_MD_SYSCALL(t0)		# t1 = syscall
	ori	t0, a1, MIPS_SR_INT_IE		# turn on IEc, enable intr.
	and	t0, t0, ~MIPS_SR_COP_1_BIT	# turn off FPU
/*
 * Call the system call handler.
 */
	jal	t1
	mtc0	t0, MIPS_COP_0_STATUS
/*
 * Check pending asynchronous traps.
 */
	lw	t0, L_MD_ASTPENDING(MIPS_CURLWP)# any pending ast?
	nop
	beq	t0, zero, 1f
	nop
/*
 * We have pending asynchronous traps; all the state is already saved.
 */
	jal	_C_LABEL(ast)
	lw	a0, CALLFRAME_SIZ + FRAME_EPC(sp)
1:
	la	ra, _C_LABEL(mips1_xcpt_return)
	j	ra				# pretend function return
	nop
END(mips1_SystemCall)

/*
 * mips1_KernIntr
 *
 * Handle an interrupt from kernel mode.
 * Build kernframe on stack to hold interrupted kernel context, then
 * call cpu_intr() to process it.
 *
 */
NESTED_NOPROFILE(mips1_KernIntr, KERNFRAME_SIZ, ra)
	.set	noat
	.mask	0x80000000, (CALLFRAME_RA - KERNFRAME_SIZ)
	subu	sp, sp, KERNFRAME_SIZ
/*
 * Save the relevant kernel registers onto the stack.
 * We don't need to save s0 - s8 and sp because
 * the compiler does it for us.
 */
	sw	AT, TF_BASE+TF_REG_AST(sp)
	sw	v0, TF_BASE+TF_REG_V0(sp)
	sw	v1, TF_BASE+TF_REG_V1(sp)
	mflo	v0
	mfhi	v1
	sw	a0, TF_BASE+TF_REG_A0(sp)
	sw	a1, TF_BASE+TF_REG_A1(sp)
	sw	a2, TF_BASE+TF_REG_A2(sp)
	sw	a3, TF_BASE+TF_REG_A3(sp)
	mfc0	a0, MIPS_COP_0_STATUS		# 1st arg is STATUS
	sw	t0, TF_BASE+TF_REG_T0(sp)
	sw	t1, TF_BASE+TF_REG_T1(sp)
	sw	t2, TF_BASE+TF_REG_T2(sp)
	sw	t3, TF_BASE+TF_REG_T3(sp)
	mfc0	a1, MIPS_COP_0_CAUSE		# 2nd arg is CAUSE
	sw	t4, TF_BASE+TF_REG_T4(sp)
	sw	t5, TF_BASE+TF_REG_T5(sp)
	sw	t6, TF_BASE+TF_REG_T6(sp)
	sw	t7, TF_BASE+TF_REG_T7(sp)
	mfc0	a2, MIPS_COP_0_EXC_PC		# 3rd arg is exception PC
	sw	t8, TF_BASE+TF_REG_T8(sp)
	sw	t9, TF_BASE+TF_REG_T9(sp)
	sw	ra, TF_BASE+TF_REG_RA(sp)
	sw	a0, TF_BASE+TF_REG_SR(sp)
	and	a3, a0, a1			# 4th is STATUS & CAUSE
	sw	v0, TF_BASE+TF_REG_MULLO(sp)
	sw	v1, TF_BASE+TF_REG_MULHI(sp)
	sw	a2, TF_BASE+TF_REG_EPC(sp)
#if defined(DDB) || defined(DEBUG) || defined(KGDB)
	move	ra, a2
	sw	ra, KERNFRAME_RA(sp)		# for debugging
#endif
/*
 * Call the interrupt handler.
 */
	jal	_C_LABEL(cpu_intr)
	nop
/*
 * Restore registers and return from the interrupt.
 */
	lw	a0, TF_BASE+TF_REG_SR(sp)
	lw	t0, TF_BASE+TF_REG_MULLO(sp)
	lw	t1, TF_BASE+TF_REG_MULHI(sp)
	mtc0	a0, MIPS_COP_0_STATUS		# this should disable interrupts
	mtlo	t0
	mthi	t1
/*
 * Check for kernel restartable atomic sequences.
 */
	lw	k1, TF_BASE+TF_REG_EPC(sp)	# restore exception PC
	la	t0, _C_LABEL(_lock_ras_start)
	li	t1, -MIPS_LOCK_RAS_SIZE
	and	t1, t1, k1
	bne	t1, t0, 1f
	nop
	jal	_C_LABEL(_lock_ras)
1:
	lw	AT, TF_BASE+TF_REG_AST(sp)
	lw	v0, TF_BASE+TF_REG_V0(sp)
	lw	v1, TF_BASE+TF_REG_V1(sp)
	lw	a0, TF_BASE+TF_REG_A0(sp)
	lw	a1, TF_BASE+TF_REG_A1(sp)
	lw	a2, TF_BASE+TF_REG_A2(sp)
	lw	a3, TF_BASE+TF_REG_A3(sp)
	lw	t0, TF_BASE+TF_REG_T0(sp)
	lw	t1, TF_BASE+TF_REG_T1(sp)
	lw	t2, TF_BASE+TF_REG_T2(sp)
	lw	t3, TF_BASE+TF_REG_T3(sp)
	lw	t4, TF_BASE+TF_REG_T4(sp)
	lw	t5, TF_BASE+TF_REG_T5(sp)
	lw	t6, TF_BASE+TF_REG_T6(sp)
	lw	t7, TF_BASE+TF_REG_T7(sp)
	lw	t8, TF_BASE+TF_REG_T8(sp)
	lw	t9, TF_BASE+TF_REG_T9(sp)
	lw	ra, TF_BASE+TF_REG_RA(sp)
	addu	sp, sp, KERNFRAME_SIZ		# restore kernel SP
	j	k1				# return to interrupted point
	rfe
	.set	at
END(mips1_KernIntr)

/*----------------------------------------------------------------------------
 * XXX this comment block should be updated XXX
 * mips1_UserIntr --
 *
 *	Handle an interrupt from user mode.
 *	Note: we save minimal state in the u.u_pcb struct and use the standard
 *	kernel stack since there has to be a u page if we came from user mode.
 *	If there is a pending software interrupt, then save the remaining state
 *	and call softintr(). This is all because if we call switch() inside
 *	cpu_intr(), not all the user registers have been saved in u.u_pcb.
 *
 * Results:
 * 	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------------
 */
NESTED_NOPROFILE(mips1_UserIntr, CALLFRAME_SIZ, ra)
	.set	noat
	.mask	0x80000000, -4
/*
 * Save the relevant user registers onto the stack.
 * We don't need to save s0 - s8 because the compiler does it for us.
 */
	lw	k1, CPUVAR(CURLWP)
	nop
	lw	k1, L_ADDR(k1)
	nop
	addu	k1, k1, USPACE - FRAME_SIZ
	sw	AT, FRAME_AST(k1)
	sw	v0, FRAME_V0(k1)
	sw	v1, FRAME_V1(k1)
	mflo	v0
	sw	a0, FRAME_A0(k1)
	sw	a1, FRAME_A1(k1)
	sw	a2, FRAME_A2(k1)
	sw	a3, FRAME_A3(k1)
	mfhi	v1
	sw	t0, FRAME_T0(k1)
	sw	t1, FRAME_T1(k1)
	sw	t2, FRAME_T2(k1)
	sw	t3, FRAME_T3(k1)
	mfc0	a0, MIPS_COP_0_STATUS		# 1st arg is STATUS
	sw	t4, FRAME_T4(k1)
	sw	t5, FRAME_T5(k1)
	sw	t6, FRAME_T6(k1)
	sw	t7, FRAME_T7(k1)
	mfc0	a1, MIPS_COP_0_CAUSE		# 2nd arg is CAUSE
	sw	t8, FRAME_T8(k1)
	sw	t9, FRAME_T9(k1)
	sw	gp, FRAME_GP(k1)
	sw	sp, FRAME_SP(k1)
	mfc0	a2, MIPS_COP_0_EXC_PC		# 3rd arg is exception PC
	sw	ra, FRAME_RA(k1)
	sw	a0, FRAME_SR(k1)
	sw	v0, FRAME_MULLO(k1)
	sw	v1, FRAME_MULHI(k1)
	and	a3, a0, a1			# 4th is STATUS & CAUSE
	sw	a2, FRAME_EPC(k1)
	addu	sp, k1, -CALLFRAME_SIZ		# switch to kernel SP
#ifdef __GP_SUPPORT__
	la	gp, _C_LABEL(_gp)		# switch to kernel GP
#endif
	sw	MIPS_CURLWP, MIPS_CURLWP_FRAME(k1)# save curlwp reg
	lw	MIPS_CURLWP, CPUVAR(CURLWP)	# set curlwp reg
	.set	at
	and	t0, a0, ~MIPS_SR_COP_1_BIT	# turn off the FPU
	.set	noat
#if defined(DDB) || defined(DEBUG) || defined(KGDB)
	move	ra, a2
	sw	ra, CALLFRAME_RA(sp)		# for debugging
#endif
/*
 * Call the interrupt handler.
 */
	jal	_C_LABEL(cpu_intr)
	mtc0	t0, MIPS_COP_0_STATUS
/*
 * Check pending asynchoronous traps.
 */
	addu	a1, sp, CALLFRAME_SIZ
	nop
	lw	a0, FRAME_SR(a1)
	lw	v0, L_MD_ASTPENDING(MIPS_CURLWP)# any pending ast?
	mtc0	a0, MIPS_COP_0_STATUS		# restore SR, disable intrs
	beq	v0, zero, 1f			# if no, skip ast processing
	lw	MIPS_CURLWP, MIPS_CURLWP_FRAME(a1) # restore curlwp reg
/*
 * We have pending asynchronous traps; save remaining user state in stack. 
 */
	sw	s0, FRAME_S0(a1)
	sw	s1, FRAME_S1(a1)
	sw	s2, FRAME_S2(a1)
	sw	s3, FRAME_S3(a1)
	sw	s4, FRAME_S4(a1)
	sw	s5, FRAME_S5(a1)
	sw	s6, FRAME_S6(a1)
	sw	s7, FRAME_S7(a1)
	sw	s8, FRAME_S8(a1)

	lw	a0, FRAME_EPC(a1)		# argument is interrupted PC
	li	t0, MIPS_HARD_INT_MASK | MIPS_SR_INT_IE
	lw	MIPS_CURLWP, CPUVAR(CURLWP)	# set curlwp reg
	jal	_C_LABEL(ast)
	mtc0	t0, MIPS_COP_0_STATUS		# enable interrupts (spl0)

	addu	a1, sp, CALLFRAME_SIZ
	lw	a0, FRAME_SR(a1)
	lw	s0, FRAME_S0(a1)
	mtc0	a0, MIPS_COP_0_STATUS		# this should disable interrupts
	lw	s1, FRAME_S1(a1)
	lw	s2, FRAME_S2(a1)
	lw	s3, FRAME_S3(a1)
	lw	s4, FRAME_S4(a1)
	lw	s5, FRAME_S5(a1)
	lw	s6, FRAME_S6(a1)
	lw	s7, FRAME_S7(a1)
	lw	s8, FRAME_S8(a1)
1:
	lw	v0, FRAME_MULLO(a1)
	lw	v1, FRAME_MULHI(a1)
	mtlo	v0
	mthi	v1
	move	k1, a1
	lw	k0, FRAME_EPC(k1)		# exception PC
	lw	AT, FRAME_AST(k1)
	lw	v0, FRAME_V0(k1)
	lw	v1, FRAME_V1(k1)
	lw	a0, FRAME_A0(k1)
	lw	a1, FRAME_A1(k1)
	lw	a2, FRAME_A2(k1)
	lw	a3, FRAME_A3(k1)
	lw	t0, FRAME_T0(k1)
	lw	t1, FRAME_T1(k1)
	lw	t2, FRAME_T2(k1)
	lw	t3, FRAME_T3(k1)
	lw	t4, FRAME_T4(k1)
	lw	t5, FRAME_T5(k1)
	lw	t6, FRAME_T6(k1)
	lw	t7, FRAME_T7(k1)
	lw	t8, FRAME_T8(k1)
	lw	t9, FRAME_T9(k1)
	lw	gp, FRAME_GP(k1)
	lw	sp, FRAME_SP(k1)
	lw	ra, FRAME_RA(k1)
	j	k0				# return to interrupted point
	rfe
	.set	at
END(mips1_UserIntr)

/*
 * Mark where code entreed from exception hander jumptable
 * ends, for stack traceback code.
 */

	.globl	_C_LABEL(mips1_exceptionentry_end)
_C_LABEL(mips1_exceptionentry_end):


#if 0
/*----------------------------------------------------------------------------
 *
 * mips1_TLBModException --
 *
 *	Handle a TLB modified exception.
 *	The BaddVAddr, Context, and EntryHi registers contain the failed
 *	virtual address.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------------
 */
LEAF_NOPROFILE(mips1_TLBModException)
	.set	noat
	tlbp					# find the TLB entry
	mfc0	k0, MIPS_COP_0_TLB_LOW		# get the physical address
	mfc0	k1, MIPS_COP_0_TLB_INDEX	# check to be sure its valid
	or	k0, k0, MIPS1_TLB_DIRTY_BIT	# update TLB
	blt	k1, zero, 4f			# not found!!!
	mtc0	k0, MIPS_COP_0_TLB_LOW
	li	k1, MIPS_KSEG0_START
	subu	k0, k0, k1
	srl	k0, k0, MIPS1_TLB_PHYS_PAGE_SHIFT
	la	k1, pmap_attributes
	addu	k0, k0, k1
	lbu	k1, 0(k0)			# fetch old value
	nop
	or	k1, k1, 1			# set modified bit
	sb	k1, 0(k0)			# save new value
	mfc0	k0, MIPS_COP_0_EXC_PC		# get return address
	nop
	j	k0
	rfe
4:
	break	0				# panic
	.set	at
END(mips1_TLBModException)
#endif

/*----------------------------------------------------------------------------
 *
 * mips1_TLBMissException --
 *
 *	Handle a TLB miss exception from kernel mode.
 *	The BaddVAddr, Context, and EntryHi registers contain the failed
 *	virtual address.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------------
 */
LEAF_NOPROFILE(mips1_TLBMissException)
	.set	noat
	mfc0	k0, MIPS_COP_0_BAD_VADDR	# get the fault address
	li	k1, VM_MIN_KERNEL_ADDRESS	# compute index
	subu	k0, k0, k1
	lw	k1, _C_LABEL(Sysmapsize)	# index within range?
	srl	k0, k0, PGSHIFT
	sltu	k1, k0, k1
	beq	k1, zero, outofworld		# No. falling beyond...
	nop
	lw	k1, _C_LABEL(Sysmap)
	sll	k0, k0, 2			# compute offset from index
	addu	k1, k1, k0
	lw	k0, 0(k1)			# get PTE entry
	mfc0	k1, MIPS_COP_0_EXC_PC		# get return address
	mtc0	k0, MIPS_COP_0_TLB_LOW		# save PTE entry
	and	k0, k0, MIPS1_PG_V		# check for valid entry
	beq	k0, zero, _C_LABEL(mips1_KernGenException) # PTE invalid
	nop
	tlbwr					# update TLB
	j	k1
	rfe

outofworld:
	/* Ensure we have a valid sp so panic has a chance */
	move	a1, sp
	la	sp, start			# set sp to a valid place
	PANIC("TLB out of universe: ksp was %p")
	.set	at
END(mips1_TLBMissException)


/*--------------------------------------------------------------------------
 *
 * mips1_SetPID --
 *
 *	Write the given pid into the TLB pid reg.
 *
 *	mips1_SetPID(pid)
 *		int pid;
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	PID set in the entry hi register.
 *
 *--------------------------------------------------------------------------
 */
LEAF(mips1_SetPID)
	sll	a0, a0, MIPS1_TLB_PID_SHIFT	# put PID in right spot
	mtc0	a0, MIPS_COP_0_TLB_HI		# Write the hi reg value
	j	ra
	nop
END(mips1_SetPID)


/*--------------------------------------------------------------------------
 *
 * mips1_TLBUpdate --
 *
 *	Update the TLB if highreg is found; otherwise, enter the data.
 *
 *	mips1_TLBUpdate(highreg, lowreg)
 *		unsigned highreg, lowreg;
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------------------
 */
LEAF(mips1_TLBUpdate)
	mfc0	v1, MIPS_COP_0_STATUS		# save the status register
	mtc0	zero, MIPS_COP_0_STATUS		# disable interrupts
	nop
	mfc0	t0, MIPS_COP_0_TLB_HI		# save current PID
	nop
	mtc0	a0, MIPS_COP_0_TLB_HI		# set entryhi
	nop
	tlbp					# probe the existence
	mfc0	v0, MIPS_COP_0_TLB_INDEX	# see what we got
	mtc0	a1, MIPS_COP_0_TLB_LOW		# set new entrylo
	bgez	v0, 2f				# index < 0 => !found
	nop
	b	3f
	tlbwr					# add vicitimizing another
2:
	tlbwi					# update the existing one
3:
	mtc0	t0, MIPS_COP_0_TLB_HI		# restore current PID
	j	ra
	mtc0	v1, MIPS_COP_0_STATUS
END(mips1_TLBUpdate)

/*--------------------------------------------------------------------------
 *
 * mips1_TLBRead --
 *
 *	Read the TLB entry.
 *
 *	mips1_TLBRead(entry, tlb)
 *		unsigned entry;
 *		struct mips1_tlb *tlb;
 *
 * Results:
 *	Returns MIPS1 TLB HI/LO in a pair.
 *
 *--------------------------------------------------------------------------
 */
LEAF(mips1_TLBRead)
	mfc0	v1, MIPS_COP_0_STATUS		# Save the status register.
	mtc0	zero, MIPS_COP_0_STATUS		# Disable interrupts
	mfc0	t0, MIPS_COP_0_TLB_HI		# Get current PID

	sll	a0, a0, MIPS1_TLB_INDEX_SHIFT
	mtc0	a0, MIPS_COP_0_TLB_INDEX	# Set the index register
	nop
	tlbr					# Read from the TLB
	mfc0	t2, MIPS_COP_0_TLB_HI		# fetch the hi entry
	mfc0	t3, MIPS_COP_0_TLB_LOW		# fetch the low entry
	sw	t2, 0(a1)
	sw	t3, 4(a1)

	mtc0	t0, MIPS_COP_0_TLB_HI		# restore PID
	j	ra
	mtc0	v1, MIPS_COP_0_STATUS		# Restore the status register
END(mips1_TLBRead)

/*----------------------------------------------------------------------------
 *
 *	R3000 cache sizing and flushing code.
 *
 *----------------------------------------------------------------------------
 */
#ifndef ENABLE_MIPS_TX3900
/*
 * void mips1_wbflush(void)
 *
 * Drain processor's write buffer, normally used to ensure any I/O
 * register write operations are done before subsequent manipulations.
 *
 * Some hardware implementations have a WB chip independent from CPU
 * core, and CU0 (Coprocessor Usability #0) bit of CP0 status register
 * is wired to indicate writebuffer condition.  This code does busy-loop
 * while CU0 bit indicates false condition.
 *
 * For other hardwares which have the writebuffer logic is implemented
 * in a system controller ASIC chip, wbflush operation would done
 * differently.
 */
LEAF(mips1_wbflush)
	nop
	nop
	nop
	nop
1:	bc0f	1b
	nop
	j	ra
	nop
END(mips1_wbflush)
#else /* !ENABLE_MIPS_TX3900 */
/*
 *	The differences between R3900 and R3000.
 *	1. Cache system 
 *		Physical-index physical-tag	
 *		fixed line-size
 *		refil-size 4/8/16/32 words (set in config register)
 *		TX3912 
 *		       Write-through
 *		       I-cache 4KB/16B direct mapped (256line)
 *		       D-cache 1KB/4B 2-way sa (128line)
 *		       Cache snoop	
 *		TX3922
 *		       Write-through/write-back (set in config register)
 *		       I-cache 16KB/16B 2-way sa 
 *		       D-cache 8KB/16B 2-way sa
 *		       Cache snoop	
 *
 *	2. Coprocessor1
 *	2.1	cache operation.
 *		R3900 uses MIPSIII cache op like method.
 *	2.2	R3900 specific CP0 register.
 *		(mips/include/r3900regs.h overrides cpuregs.h)
 *	2.3	# of TLB entries
 *		TX3912 32 entries
 *		TX3922 64 entries
 *
 *	3. System address map
 *		kseg2 0xff000000-0xfffeffff is reserved.
 *		(mips/include/vmparam.h)
 *
 *  + If defined both MIPS1 and ENABLE_MIPS_TX3900, it generates kernel for
 * R3900. If defined MIPS1 only, No R3900 feature include.
 *  + R3920 core has write-back mode. but it is always disabled in NetBSD.
 */

LEAF_NOPROFILE(tx3900_cp0_config_read)
	mfc0	v0, R3900_COP_0_CONFIG
	j	ra
	nop
END(tx3900_cp0_config_read)

LEAF(mips1_wbflush)
	.set push
	.set mips2
	sync	
	.set pop
	j	ra
	nop
END(mips1_wbflush)
#endif /* !ENABLE_MIPS_TX3900 */

/*
 * mips1_lwp_trampoline
 *
 * Special arrangement for a process about to go user mode right after
 * fork() system call.  When the first CPU tick is scheduled to the
 * forked child, it starts running from here.  Then, a service function
 * is called with one argument supplied to complete final preparations,
 * and the process returns to user mode as if the fork() system call is
 * handled in a normal way.  No need to save any registers although this
 * calls another.
 */
LEAF(mips1_lwp_trampoline)
	addu	sp, sp, -CALLFRAME_SIZ

	# Call lwp_startup(), with args from cpu_switchto()/cpu_setfunc()
	la	t0, _C_LABEL(lwp_startup)
	move	a0, v0
	jal	ra, t0
	move	a1, s7

	# Call the routine specified by cpu_setfunc()
	jal	ra, s0			
	move	a0, s1

	# Return to user (won't happen if a kernel thread)
	.set	noat
XLEAF(mips1_xcpt_return)
	addu	a1, sp, CALLFRAME_SIZ	# a1 points exception frame
	lw	a0, FRAME_SR(a1)
	lw	t0, FRAME_MULLO(a1)
	lw	t1, FRAME_MULHI(a1)
	mtc0	a0, MIPS_COP_0_STATUS	# this should disable interrupts
	mtlo	t0
	mthi	t1
	nop
	move	k1, a1
	lw	k0, FRAME_EPC(k1)
	lw	AT, FRAME_AST(k1)
	lw	v0, FRAME_V0(k1)
	lw	v1, FRAME_V1(k1)
	lw	a0, FRAME_A0(k1)
	lw	a1, FRAME_A1(k1)
	lw	a2, FRAME_A2(k1)
	lw	a3, FRAME_A3(k1)
	lw	t0, FRAME_T0(k1)
	lw	t1, FRAME_T1(k1)
	lw	t2, FRAME_T2(k1)
	lw	t3, FRAME_T3(k1)
	lw	t4, FRAME_T4(k1)
	lw	t5, FRAME_T5(k1)
	lw	t6, FRAME_T6(k1)
	lw	t7, FRAME_T7(k1)
	lw	s0, FRAME_S0(k1)
	lw	s1, FRAME_S1(k1)
	lw	s2, FRAME_S2(k1)
	lw	s3, FRAME_S3(k1)
	lw	s4, FRAME_S4(k1)
	lw	s5, FRAME_S5(k1)
	lw	s6, FRAME_S6(k1)
	lw	s7, FRAME_S7(k1)
	lw	t8, FRAME_T8(k1)
	lw	t9, FRAME_T9(k1)
	lw	gp, FRAME_GP(k1)
	lw	s8, FRAME_S8(k1)
	lw	ra, FRAME_RA(k1)
	lw	sp, FRAME_SP(k1)
	nop
	j	k0
	rfe
	.set	at
END(mips1_lwp_trampoline)

/*
 * void mips1_cpu_switch_resume(struct lwp *newlwp)
 *
 * Wiredown the USPACE of newproc with TLB entry#0 and #1.  Check
 * if target USPACE is already refered by any TLB entry before
 * doing that, and make sure TBIS(them) in the case.
 */
LEAF_NOPROFILE(mips1_cpu_switch_resume)
	lw	a1, L_MD_UPTE_0(a0)		# a1 = upte[0]
	lw	a2, L_MD_UPTE_1(a0)		# a2 = upte[1]
	lw	s0, L_ADDR(a0)			# va = l->l_addr
	li	s2, MIPS_KSEG2_START
	blt	s0, s2, resume
	nop

	mtc0	s0, MIPS_COP_0_TLB_HI		# VPN = va
	nop
	tlbp					# probe 1st VPN
	mfc0	s1, MIPS_COP_0_TLB_INDEX
	nop
	bltz	s1, entry0set
	li	s1, MIPS_KSEG0_START		# found, then
	mtc0	s1, MIPS_COP_0_TLB_HI
	mtc0	zero, MIPS_COP_0_TLB_LOW
	nop
	tlbwi					# TBIS(va)
	nop
	mtc0	s0, MIPS_COP_0_TLB_HI		# set 1st VPN again
entry0set:
	mtc0	zero, MIPS_COP_0_TLB_INDEX	# TLB index #0
	ori	a1, a1, MIPS1_PG_G
	mtc0	a1, MIPS_COP_0_TLB_LOW		# 1st PFN w/ PG_G
	nop
	tlbwi					# set TLB entry #0

	addu	s0, s0, PAGE_SIZE
	mtc0	s0, MIPS_COP_0_TLB_HI		# VPN = va+PAGE_SIZE
	nop
	tlbp					# probe 2nd VPN
	mfc0	s1, MIPS_COP_0_TLB_INDEX
	nop
	bltz	s1, entry1set
	li	s1, MIPS_KSEG0_START		# found, then
	mtc0	s1, MIPS_COP_0_TLB_HI
	mtc0	zero, MIPS_COP_0_TLB_LOW
	nop
	tlbwi					# TBIS(va+PAGE_SIZE)
	nop
	mtc0	s0, MIPS_COP_0_TLB_HI		# set 2nd VPN again
entry1set:
	li	s1, 1 << MIPS1_TLB_INDEX_SHIFT
	mtc0	s1, MIPS_COP_0_TLB_INDEX	# TLB index #1
	ori	a2, a2, MIPS1_PG_G
	mtc0	a2, MIPS_COP_0_TLB_LOW		# 2nd PFN w/ PG_G
	nop
	tlbwi					# set TLB entry #1

resume:
	j	ra
	nop
	END(mips1_cpu_switch_resume)

/*
 * void mips1_TBIS(vaddr_t va)
 *
 * Invalidate a TLB entry for given virtual address if found in TLB.
 */
LEAF(mips1_TBIS)
	mfc0	v1, MIPS_COP_0_STATUS		# save status register
	mtc0	zero, MIPS_COP_0_STATUS		# disable interrupts
	mfc0	t0, MIPS_COP_0_TLB_HI		# save current PID
	nop

	mtc0	a0, MIPS_COP_0_TLB_HI		# look for addr & PID
	nop
	tlbp					# probe the entry in question
	mfc0	a0, MIPS_COP_0_TLB_INDEX	# see what we got
	li	t1, MIPS_KSEG0_START		# load invalid address
	bltz	a0, 1f				# index < 0 then skip
	mtc0	t1, MIPS_COP_0_TLB_HI		# make entryHi invalid
	mtc0	zero, MIPS_COP_0_TLB_LOW	# zero out entryLo
	nop
	tlbwi
1:
	mtc0	t0, MIPS_COP_0_TLB_HI		# restore PID
	j	ra
	mtc0	v1, MIPS_COP_0_STATUS		# restore the status register
	END(mips1_TBIS)

/*
 * void mips1_TBIAP(int sizeofTLB)
 *
 * Invalidate TLB entries belonging to per process user spaces while
 * leaving entries for kernel space marked global intact.
 */
LEAF(mips1_TBIAP)
	mfc0	v1, MIPS_COP_0_STATUS		# save status register
	mtc0	zero, MIPS_COP_0_STATUS		# disable interrupts

	li	t1, MIPS1_TLB_FIRST_RAND_ENTRY << MIPS1_TLB_INDEX_SHIFT
	sll	t2, a0, MIPS1_TLB_INDEX_SHIFT
	li	v0, MIPS_KSEG0_START		# invalid address

	# do {} while (t1 < t2)
1:
	mtc0	t1, MIPS_COP_0_TLB_INDEX	# set index
	nop
	tlbr					# obtain an entry
	mfc0	a0, MIPS_COP_0_TLB_LOW
	nop
	and	a0, a0, MIPS1_PG_G		# check to see it has G bit
	bnez	a0, 2f
	nop
	mtc0	v0, MIPS_COP_0_TLB_HI		# make entryHi invalid
	mtc0	zero, MIPS_COP_0_TLB_LOW	# zero out entryLo
	nop
	tlbwi					# invalidate the TLB entry
2:
	addu	t1, t1, 1 << MIPS1_TLB_INDEX_SHIFT	# increment index
	bne	t1, t2, 1b
	nop

	j	ra				# new TLBpid will be set soon
	mtc0	v1, MIPS_COP_0_STATUS		# restore status register
	END(mips1_TBIAP)

/*
 * void mips1_TBIA(int sizeofTLB)
 *
 * Invalidate TLB entirely.
 */
LEAF(mips1_TBIA)
	mfc0	v1, MIPS_COP_0_STATUS		# save the status register.
	mtc0	zero, MIPS_COP_0_STATUS		# disable interrupts

	li	t1, MIPS_KSEG0_START
	mtc0	t1, MIPS_COP_0_TLB_HI		# make entryHi invalid
	mtc0	zero, MIPS_COP_0_TLB_LOW	# zero out entryLo

	move	t1, zero
	sll	a0, a0, MIPS1_TLB_INDEX_SHIFT

	# do {} while (t1 < a0)
1:
	mtc0	t1, MIPS_COP_0_TLB_INDEX	# set TLBindex
	addu	t1, t1, 1 << MIPS1_TLB_INDEX_SHIFT	# increment index
	bne	t1, a0, 1b
	tlbwi					# invalidate the entry

	j	ra
	mtc0	v1, MIPS_COP_0_STATUS		# restore status register
	END(mips1_TBIA)

	.data

	.globl _C_LABEL(mips1_locoresw)
_C_LABEL(mips1_locoresw):
	.word _C_LABEL(mips1_cpu_switch_resume)
	.word _C_LABEL(mips1_lwp_trampoline)
	.word _C_LABEL(nullop)			# idle

mips1_excpt_sw:
	####
	#### The kernel exception handlers.
	####
	.word _C_LABEL(mips1_KernIntr)		# 0 external interrupt
	.word _C_LABEL(mips1_KernGenException)	# 1 TLB modification
	.word _C_LABEL(mips1_TLBMissException)	# 2 TLB miss (LW/I-fetch)
	.word _C_LABEL(mips1_TLBMissException)	# 3 TLB miss (SW)
	.word _C_LABEL(mips1_KernGenException)	# 4 address error (LW/I-fetch)
	.word _C_LABEL(mips1_KernGenException)	# 5 address error (SW)
	.word _C_LABEL(mips1_KernGenException)	# 6 bus error (I-fetch)
	.word _C_LABEL(mips1_KernGenException)	# 7 bus error (load or store)
	.word _C_LABEL(mips1_KernGenException)	# 8 system call
	.word _C_LABEL(mips1_KernGenException)	# 9 breakpoint
	.word _C_LABEL(mips1_KernGenException)	# 10 reserved instruction
	.word _C_LABEL(mips1_KernGenException)	# 11 coprocessor unusable
	.word _C_LABEL(mips1_KernGenException)	# 12 arithmetic overflow
	.word _C_LABEL(mips1_KernGenException)	# 13 r3k reserved
	.word _C_LABEL(mips1_KernGenException)	# 14 r3k reserved
	.word _C_LABEL(mips1_KernGenException)	# 15 r3k reserved
	.word _C_LABEL(mips1_KernGenException)	# 16 never happens w/ MIPS1
	.word _C_LABEL(mips1_KernGenException)	# 17 never happens w/ MIPS1
	.word _C_LABEL(mips1_KernGenException)	# 18 never happens w/ MIPS1
	.word _C_LABEL(mips1_KernGenException)	# 19 never happens w/ MIPS1
	.word _C_LABEL(mips1_KernGenException)	# 20 never happens w/ MIPS1
	.word _C_LABEL(mips1_KernGenException)	# 21 never happens w/ MIPS1
	.word _C_LABEL(mips1_KernGenException)	# 22 never happens w/ MIPS1
	.word _C_LABEL(mips1_KernGenException)	# 23 never happens w/ MIPS1
	.word _C_LABEL(mips1_KernGenException)	# 24 never happens w/ MIPS1
	.word _C_LABEL(mips1_KernGenException)	# 25 never happens w/ MIPS1
	.word _C_LABEL(mips1_KernGenException)	# 26 never happens w/ MIPS1
	.word _C_LABEL(mips1_KernGenException)	# 27 never happens w/ MIPS1
	.word _C_LABEL(mips1_KernGenException)	# 28 never happens w/ MIPS1
	.word _C_LABEL(mips1_KernGenException)	# 29 never happens w/ MIPS1
	.word _C_LABEL(mips1_KernGenException)	# 30 never happens w/ MIPS1
	.word _C_LABEL(mips1_KernGenException)	# 31 never happens w/ MIPS1
	#####
	##### The user exception handlers.
	#####
	.word _C_LABEL(mips1_UserIntr)		#  0
	.word _C_LABEL(mips1_UserGenException)	#  1
	.word _C_LABEL(mips1_UserGenException)	#  2
	.word _C_LABEL(mips1_UserGenException)	#  3
	.word _C_LABEL(mips1_UserGenException)	#  4
	.word _C_LABEL(mips1_UserGenException)	#  5
	.word _C_LABEL(mips1_UserGenException)	#  6
	.word _C_LABEL(mips1_UserGenException)	#  7
	.word _C_LABEL(mips1_SystemCall)	#  8
	.word _C_LABEL(mips1_UserGenException)	#  9
	.word _C_LABEL(mips1_UserGenException)	# 10
	.word _C_LABEL(mips1_UserGenException)	# 11
	.word _C_LABEL(mips1_UserGenException)	# 12
	.word _C_LABEL(mips1_UserGenException)	# 13
	.word _C_LABEL(mips1_UserGenException)	# 14
	.word _C_LABEL(mips1_UserGenException)	# 15
	.word _C_LABEL(mips1_UserGenException)	# 16
	.word _C_LABEL(mips1_UserGenException)	# 17
	.word _C_LABEL(mips1_UserGenException)	# 18
	.word _C_LABEL(mips1_UserGenException)	# 19
	.word _C_LABEL(mips1_UserGenException)	# 20
	.word _C_LABEL(mips1_UserGenException)	# 21
	.word _C_LABEL(mips1_UserGenException)	# 22
	.word _C_LABEL(mips1_UserGenException)	# 23
	.word _C_LABEL(mips1_UserGenException)	# 24
	.word _C_LABEL(mips1_UserGenException)	# 25
	.word _C_LABEL(mips1_UserGenException)	# 26
	.word _C_LABEL(mips1_UserGenException)	# 27
	.word _C_LABEL(mips1_UserGenException)	# 28
	.word _C_LABEL(mips1_UserGenException)	# 29
	.word _C_LABEL(mips1_UserGenException)	# 20
	.word _C_LABEL(mips1_UserGenException)	# 31
