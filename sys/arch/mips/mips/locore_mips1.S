/*	$NetBSD: locore_mips1.S,v 1.2 1999/01/15 01:23:13 castor Exp $	*/

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
#include <mips/asm.h>
#include <mips/cpuarch.h>
#include <machine/param.h>

#include "assym.h"

	.set	noreorder

	.globl	_C_LABEL(mips1_exceptionentry_start)
_C_LABEL(mips1_exceptionentry_start):
/*
 *----------------------------------------------------------------------------
 *
 * mips1_UTLBmiss --
 *
 *	Vector code for a MIPS-I user-space TLB miss from user-space.
 *
 *
 * This code is copied to the UTLB exception vector address to
 * handle user level TLB translation misses.
 * NOTE: This code must be relocatable!!!
 */
	.globl	_C_LABEL(mips1_UTLBMiss)
_C_LABEL(mips1_UTLBMiss):
	.set	noat
	mfc0	k0, MIPS_COP_0_BAD_VADDR	# get the virtual address
	lw	k1, _C_LABEL(curpcb)
	bltz	k0, 1f				# R3000 chip bug
	lw	k1, U_PCB_SEGTAB(k1)		# get the current segment table
	srl	k0, k0, SEGSHIFT		# compute segment table index
	sll	k0, k0, 2
	addu	k1, k1, k0
	mfc0	k0, MIPS_COP_0_BAD_VADDR	# get the virtual address
	lw	k1, 0(k1)			# get pointer to segment map
	srl	k0, k0, PGSHIFT - 2		# compute segment map index
	andi	k0, k0, (NPTEPG - 1) << 2
	beq	k1, zero, 2f			# invalid segment map
	addu	k1, k1, k0			# index into segment map
	lw	k0, 0(k1)			# get page PTE
	nop
	beq	k0, zero, 2f			# dont load invalid entries
	mtc0	k0, MIPS_COP_0_TLB_LOW
	mfc0	k1, MIPS_COP_0_EXC_PC		# get return address
	tlbwr					# update TLB
	j	k1
	rfe
1:
	mfc0	k1, MIPS_COP_0_EXC_PC		# get return address
	nop
	j	k1
	rfe
2:
	j	mips1_SlowFault			# handle the rest
	nop
	.set	at
	.globl	_C_LABEL(mips1_UTLBMissEnd)
_C_LABEL(mips1_UTLBMissEnd):


/*
 *----------------------------------------------------------------------------
 *
 * mips1_exception --
 *
 *	Vector code for the general exception vector 0x80000080
 *	on an r2000 or r3000.
 *
 * This code is copied to the general exception vector address to
 * handle all exceptions except RESET and UTLBMiss.
 * NOTE: This code must be relocatable!!!
 *
 *----------------------------------------------------------------------------
 */
	.globl	_C_LABEL(mips1_exception)
_C_LABEL(mips1_exception):
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
	la	k0, mips1_excausesw		# get base of the jump table
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
	.globl	_C_LABEL(mips1_exceptionEnd)
_C_LABEL(mips1_exceptionEnd):




/*----------------------------------------------------------------------------
 *
 * mips1_SlowFault --
 *
 * Alternate entry point into the mips1_UserGenExceptionor or
 * or mips1_user_Kern_exception, when the ULTB miss handler couldn't
 * find a TLB entry.
 *
 * Find out what mode we came from and call the appropriate handler.
 */
mips1_SlowFault:
	.set	noat
	mfc0	k0, MIPS_COP_0_STATUS
	nop
	and	k0, k0, MIPS1_SR_KU_PREV
	bne	k0, zero, _C_LABEL(mips1_UserGenException)
	nop
	.set	at
/*
 * Fall though ...
 */

/*
 * mips1_KernGenException
 *
 * Handle an exception from kernel mode.
 * Build trapframe on stack to hold interrupted kernel context, then
 * call trap() to process the condition.
 *
 * trapframe is synthesized on stack as 5th arg {
 *  	register_t cf_args[4];		+0 
 *  	register_t tf_regs[17];		+16	- trapframe begins here
 * 	register_t tf_sr;		+84	-
 * 	register_t tf_mullo;		+88	-
 * 	register_t tf_mulhi;		+92	-
 * 	register_t tf_epc;		+96	- may be changed by trap() call
 *  	register_t cf_sp;		+100
 *  	register_t cf_ra;		+104
 *  };
 */
NESTED_NOPROFILE(mips1_KernGenException, KERNFRAME_SIZ, ra)
	.set	noat
	.mask	0x80000000, -4
	subu	sp, sp, KERNFRAME_SIZ
/*
 * Save the relevant kernel registers onto the stack.
 * We don't need to save s0 - s8, sp and gp because
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
	mfc0	a2, MIPS_COP_0_BAD_VADDR	# 3rd arg is fault address
	sw	t8, TF_BASE+TF_REG_T8(sp)
	sw	t9, TF_BASE+TF_REG_T9(sp)
	sw	ra, TF_BASE+TF_REG_RA(sp)
	sw	a0, TF_BASE+TF_REG_SR(sp)
	mfc0	a3, MIPS_COP_0_EXC_PC		# 4th arg is exception PC
	sw	v0, TF_BASE+TF_REG_MULLO(sp)
	sw	v1, TF_BASE+TF_REG_MULHI(sp)
	sw	a3, TF_BASE+TF_REG_EPC(sp)
#ifdef DDB
	addu	v0, sp, KERNFRAME_SIZ 		# SP at trap for debugging
	sw	v0, KERNFRAME_SP(sp)
#endif
	jal	_C_LABEL(trap)
	sw	a3, KERNFRAME_RA(sp)		# for debugging

	lw	a0, TF_BASE+TF_REG_SR(sp)
	lw	t0, TF_BASE+TF_REG_MULLO(sp)
	lw	t1, TF_BASE+TF_REG_MULHI(sp)
	mtc0	a0, MIPS_COP_0_STATUS
	mtlo	t0
	mthi	t1
	lw	k0, TF_BASE+TF_REG_EPC(sp)
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
	j	k0				# return to interrupted point
	rfe	
	.set	at
	.global	_C_LABEL(mips1_KernGenExceptionEnd)
_C_LABEL(mips1_KernGenExceptionEnd):
END(mips1_KernGenException)

/*
 * mips1_UserGenException
 *
 * Handle an exception from user mode.
 * Save user context in u_pcb, then call trap() to process the condition.
 * The context can be manipulated alternatively via curproc->p_md.md_regs.
 */
NESTED_NOPROFILE(mips1_UserGenException, CALLFRAME_SIZ, ra)
	.set	noat
	.mask	0x80000000, -4
/*
 * Save all of the registers except for the kernel temporaries in u_pcb.
 */
	lw	k1, _C_LABEL(curpcb)
	nop
	sw	AT, U_PCB_REGS+FRAME_AST(k1)
	sw	v0, U_PCB_REGS+FRAME_V0(k1)
	sw	v1, U_PCB_REGS+FRAME_V1(k1)
	mflo	v0
	sw	a0, U_PCB_REGS+FRAME_A0(k1)
	sw	a1, U_PCB_REGS+FRAME_A1(k1)
	sw	a2, U_PCB_REGS+FRAME_A2(k1)
	sw	a3, U_PCB_REGS+FRAME_A3(k1)
	mfhi	v1
	sw	t0, U_PCB_REGS+FRAME_T0(k1)
	sw	t1, U_PCB_REGS+FRAME_T1(k1)
	sw	t2, U_PCB_REGS+FRAME_T2(k1)
	sw	t3, U_PCB_REGS+FRAME_T3(k1)
	mfc0	a0, MIPS_COP_0_STATUS		# 1st arg is STATUS
	sw	t4, U_PCB_REGS+FRAME_T4(k1)
	sw	t5, U_PCB_REGS+FRAME_T5(k1)
	sw	t6, U_PCB_REGS+FRAME_T6(k1)
	sw	t7, U_PCB_REGS+FRAME_T7(k1)
	mfc0	a1, MIPS_COP_0_CAUSE		# 2nd arg is CAUSE
	sw	s0, U_PCB_REGS+FRAME_S0(k1)
	sw	s1, U_PCB_REGS+FRAME_S1(k1)
	sw	s2, U_PCB_REGS+FRAME_S2(k1)
	sw	s3, U_PCB_REGS+FRAME_S3(k1)
	mfc0	a2, MIPS_COP_0_BAD_VADDR	# 3rd arg is fault address
	sw	s4, U_PCB_REGS+FRAME_S4(k1)
	sw	s5, U_PCB_REGS+FRAME_S5(k1)
	sw	s6, U_PCB_REGS+FRAME_S6(k1)
	sw	s7, U_PCB_REGS+FRAME_S7(k1)
	mfc0	a3, MIPS_COP_0_EXC_PC		# 4th arg is exception PC
	sw	t8, U_PCB_REGS+FRAME_T8(k1)
	sw	t9, U_PCB_REGS+FRAME_T9(k1)
	sw	gp, U_PCB_REGS+FRAME_GP(k1)
	sw	sp, U_PCB_REGS+FRAME_SP(k1)
	sw	s8, U_PCB_REGS+FRAME_S8(k1)
	sw	ra, U_PCB_REGS+FRAME_RA(k1)
	sw	a0, U_PCB_REGS+FRAME_SR(k1)
	sw	v0, U_PCB_REGS+FRAME_MULLO(k1)
	sw	v1, U_PCB_REGS+FRAME_MULHI(k1)
	sw	a3, U_PCB_REGS+FRAME_EPC(k1)
	#addu	sp, k1, USPACE - CALLFRAME_SIZ	# switch to kernel SP
	li	sp, KERNELSTACK - CALLFRAME_SIZ	# XXX switch to kernel SP XXX
#ifdef __GP_SUPPORT__
	la	gp, _C_LABEL(_gp)		# switch to kernel GP
#endif
	.set	at
	and	t0, a0, ~MIPS_SR_COP_1_BIT	# turn off the FPU
	.set	noat
/*
 * Call the exception handler.
 */
	sw	a3, CALLFRAME_RA(sp)		# for debugging
	jal	_C_LABEL(trap)
	mtc0	t0, MIPS_COP_0_STATUS
/*
 * Restore user registers and return. NOTE: interrupts are enabled.
 */
	lw	a1, _C_LABEL(curpcb)
	nop
	lw	a0, U_PCB_REGS+FRAME_SR(a1)
	lw	t0, U_PCB_REGS+FRAME_MULLO(a1)
	lw	t1, U_PCB_REGS+FRAME_MULHI(a1)
	mtc0	a0, MIPS_COP_0_STATUS		# this should disable interrupts
	mtlo	t0
	mthi	t1
	move	k1, a1
	lw	k0, U_PCB_REGS+FRAME_EPC(k1)	# might be changed in trap()
	lw	AT, U_PCB_REGS+FRAME_AST(k1)
	lw	v0, U_PCB_REGS+FRAME_V0(k1)
	lw	v1, U_PCB_REGS+FRAME_V1(k1)
	lw	a0, U_PCB_REGS+FRAME_A0(k1)
	lw	a1, U_PCB_REGS+FRAME_A1(k1)
	lw	a2, U_PCB_REGS+FRAME_A2(k1)
	lw	a3, U_PCB_REGS+FRAME_A3(k1)
	lw	t0, U_PCB_REGS+FRAME_T0(k1)
	lw	t1, U_PCB_REGS+FRAME_T1(k1)
	lw	t2, U_PCB_REGS+FRAME_T2(k1)
	lw	t3, U_PCB_REGS+FRAME_T3(k1)
	lw	t4, U_PCB_REGS+FRAME_T4(k1)
	lw	t5, U_PCB_REGS+FRAME_T5(k1)
	lw	t6, U_PCB_REGS+FRAME_T6(k1)
	lw	t7, U_PCB_REGS+FRAME_T7(k1)
	lw	s0, U_PCB_REGS+FRAME_S0(k1)
	lw	s1, U_PCB_REGS+FRAME_S1(k1)
	lw	s2, U_PCB_REGS+FRAME_S2(k1)
	lw	s3, U_PCB_REGS+FRAME_S3(k1)
	lw	s4, U_PCB_REGS+FRAME_S4(k1)
	lw	s5, U_PCB_REGS+FRAME_S5(k1)
	lw	s6, U_PCB_REGS+FRAME_S6(k1)
	lw	s7, U_PCB_REGS+FRAME_S7(k1)
	lw	t8, U_PCB_REGS+FRAME_T8(k1)
	lw	t9, U_PCB_REGS+FRAME_T9(k1)
	lw	gp, U_PCB_REGS+FRAME_GP(k1)
	lw	sp, U_PCB_REGS+FRAME_SP(k1)
	lw	s8, U_PCB_REGS+FRAME_S8(k1)
	lw	ra, U_PCB_REGS+FRAME_RA(k1)
	j	k0				# return to interrupted point
	rfe
	.set	at
	.globl	_C_LABEL(mips1_UserGenExceptionEnd)
_C_LABEL(mips1_UserGenExceptionEnd):
END(mips1_UserGenException)

/*
 * mips1_SystemCall
 *
 * Save user context in u_pcb, then call syscall() to process a system call.
 * The context can be manipulated alternatively via curproc->p_md.md_regs.
 */
NESTED_NOPROFILE(mips1_SystemCall, CALLFRAME_SIZ, ra)
	.set	noat
	.mask	0x80000000, -4
	lw	k1, _C_LABEL(curpcb)
	nop
	sw	AT, U_PCB_REGS+FRAME_AST(k1)
	sw	v0, U_PCB_REGS+FRAME_V0(k1)
	sw	v1, U_PCB_REGS+FRAME_V1(k1)
	mflo	v0
	sw	a0, U_PCB_REGS+FRAME_A0(k1)
	sw	a1, U_PCB_REGS+FRAME_A1(k1)
	sw	a2, U_PCB_REGS+FRAME_A2(k1)
	sw	a3, U_PCB_REGS+FRAME_A3(k1)
	mfhi	v1
	sw	t0, U_PCB_REGS+FRAME_T0(k1)
	sw	t1, U_PCB_REGS+FRAME_T1(k1)
	sw	t2, U_PCB_REGS+FRAME_T2(k1)
	sw	t3, U_PCB_REGS+FRAME_T3(k1)
	mfc0	a0, MIPS_COP_0_STATUS		# 1st arg is STATUS
	sw	t4, U_PCB_REGS+FRAME_T4(k1)
	sw	t5, U_PCB_REGS+FRAME_T5(k1)
	sw	t6, U_PCB_REGS+FRAME_T6(k1)
	sw	t7, U_PCB_REGS+FRAME_T7(k1)
	mfc0	a1, MIPS_COP_0_CAUSE		# 2nd arg is CAUSE
	sw	s0, U_PCB_REGS+FRAME_S0(k1)
	sw	s1, U_PCB_REGS+FRAME_S1(k1)
	sw	s2, U_PCB_REGS+FRAME_S2(k1)
	sw	s3, U_PCB_REGS+FRAME_S3(k1)
	mfc0	a2, MIPS_COP_0_EXC_PC		# 3rd arg is PC
	sw	s4, U_PCB_REGS+FRAME_S4(k1)
	sw	s5, U_PCB_REGS+FRAME_S5(k1)
	sw	s6, U_PCB_REGS+FRAME_S6(k1)
	sw	s7, U_PCB_REGS+FRAME_S7(k1)
	sw	t8, U_PCB_REGS+FRAME_T8(k1)
	sw	t9, U_PCB_REGS+FRAME_T9(k1)
	sw	gp, U_PCB_REGS+FRAME_GP(k1)
	sw	sp, U_PCB_REGS+FRAME_SP(k1)
	sw	s8, U_PCB_REGS+FRAME_S8(k1)
	sw	ra, U_PCB_REGS+FRAME_RA(k1)
	sw	a0, U_PCB_REGS+FRAME_SR(k1)
	sw	v0, U_PCB_REGS+FRAME_MULLO(k1)
	sw	v1, U_PCB_REGS+FRAME_MULHI(k1)
	sw	a2, U_PCB_REGS+FRAME_EPC(k1)
	#addu	sp, k1, USPACE - CALLFRAME_SIZ	# switch to kernel SP
	li	sp, KERNELSTACK - CALLFRAME_SIZ	# XXX switch to kernel SP XXX
#ifdef __GP_SUPPORT__
	la	gp, _C_LABEL(_gp)		# switch to kernel GP
#endif
	.set	at
	and	t0, a0, ~MIPS_SR_COP_1_BIT	# turn off the FPU
	.set	noat
/*
 * Call the system call handler.
 */
	sw	a2, CALLFRAME_RA(sp)		# for debugging
	jal	_C_LABEL(syscall)
	mtc0	t0, MIPS_COP_0_STATUS
/*
 * Restore user registers and return. NOTE: interrupts are enabled.
 */
	lw	a1, _C_LABEL(curpcb)
	nop
	lw	a0, U_PCB_REGS+FRAME_SR(a1)
	lw	t0, U_PCB_REGS+FRAME_MULLO(a1)
	lw	t1, U_PCB_REGS+FRAME_MULHI(a1)
	mtc0	a0, MIPS_COP_0_STATUS		# this should disable interrupts
	mtlo	t0
	mthi	t1
	move	k1, a1
	lw	k0, U_PCB_REGS+FRAME_EPC(k1)	# might be changed in syscall()
	lw	AT, U_PCB_REGS+FRAME_AST(k1)
	lw	v0, U_PCB_REGS+FRAME_V0(k1)
	lw	v1, U_PCB_REGS+FRAME_V1(k1)
	lw	a0, U_PCB_REGS+FRAME_A0(k1)
	lw	a1, U_PCB_REGS+FRAME_A1(k1)
	lw	a2, U_PCB_REGS+FRAME_A2(k1)
	lw	a3, U_PCB_REGS+FRAME_A3(k1)
	lw	t0, U_PCB_REGS+FRAME_T0(k1)
	lw	t1, U_PCB_REGS+FRAME_T1(k1)
	lw	t2, U_PCB_REGS+FRAME_T2(k1)
	lw	t3, U_PCB_REGS+FRAME_T3(k1)
	lw	t4, U_PCB_REGS+FRAME_T4(k1)
	lw	t5, U_PCB_REGS+FRAME_T5(k1)
	lw	t6, U_PCB_REGS+FRAME_T6(k1)
	lw	t7, U_PCB_REGS+FRAME_T7(k1)
	lw	s0, U_PCB_REGS+FRAME_S0(k1)
	lw	s1, U_PCB_REGS+FRAME_S1(k1)
	lw	s2, U_PCB_REGS+FRAME_S2(k1)
	lw	s3, U_PCB_REGS+FRAME_S3(k1)
	lw	s4, U_PCB_REGS+FRAME_S4(k1)
	lw	s5, U_PCB_REGS+FRAME_S5(k1)
	lw	s6, U_PCB_REGS+FRAME_S6(k1)
	lw	s7, U_PCB_REGS+FRAME_S7(k1)
	lw	t8, U_PCB_REGS+FRAME_T8(k1)
	lw	t9, U_PCB_REGS+FRAME_T9(k1)
	lw	gp, U_PCB_REGS+FRAME_GP(k1)
	lw	sp, U_PCB_REGS+FRAME_SP(k1)
	lw	s8, U_PCB_REGS+FRAME_S8(k1)
	lw	ra, U_PCB_REGS+FRAME_RA(k1)
	j	k0				# return to syscall point
	rfe
	.set	at
	.globl	_C_LABEL(mips1_SystemCallEnd)
_C_LABEL(mips1_SystemCallEnd):
END(mips1_SystemCall)

/*
 * mips1_KernIntr
 *
 * Handle an interrupt from kernel mode.
 * Build intrframe on stack to hold interrupted kernel context, then
 * call interrupt() to process it.
 *
 */
NESTED_NOPROFILE(mips1_KernIntr, KERNFRAME_SIZ, ra)
	.set	noat
	.mask	0x80000000, -4
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
	sw	v0, TF_BASE+TF_REG_MULLO(sp)
	sw	v1, TF_BASE+TF_REG_MULHI(sp)
	sw	a2, TF_BASE+TF_REG_EPC(sp)
#ifdef DDB
	addu	v0, sp, KERNFRAME_SIZ 		# SP at interupt for debugging
	sw	v0, KERNFRAME_SP(sp)
#endif
/*
 * Call the interrupt handler.
 */
	jal	_C_LABEL(interrupt)
	sw	a2, KERNFRAME_RA(sp)		# for debugging

/*
 * Restore registers and return from the interrupt.
 */
	lw	a0, TF_BASE+TF_REG_SR(sp)
	lw	t0, TF_BASE+TF_REG_MULLO(sp)
	lw	t1, TF_BASE+TF_REG_MULHI(sp)
	mtc0	a0, MIPS_COP_0_STATUS		# this should disable interrupts
	mtlo	t0
	mthi	t1
	lw	k0, TF_BASE+TF_REG_EPC(sp)	# restore exception PC
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
	j	k0				# return to interrupted point
	rfe
	.set	at
	.globl	_C_LABEL(mips1_KernIntrEnd)
_C_LABEL(mips1_KernIntrEnd):
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
 *	interrupt(), not all the user registers have been saved in u.u_pcb.
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
 * Save the relevant user registers into the u_pcb.
 * We don't need to save s0 - s8 because the compiler does it for us.
 */
	lw	k1, _C_LABEL(curpcb)
	nop
	sw	AT, U_PCB_REGS+FRAME_AST(k1)
	sw	v0, U_PCB_REGS+FRAME_V0(k1)
	sw	v1, U_PCB_REGS+FRAME_V1(k1)
	mflo	v0
	sw	a0, U_PCB_REGS+FRAME_A0(k1)
	sw	a1, U_PCB_REGS+FRAME_A1(k1)
	sw	a2, U_PCB_REGS+FRAME_A2(k1)
	sw	a3, U_PCB_REGS+FRAME_A3(k1)
	mfhi	v1
	sw	t0, U_PCB_REGS+FRAME_T0(k1)
	sw	t1, U_PCB_REGS+FRAME_T1(k1)
	sw	t2, U_PCB_REGS+FRAME_T2(k1)
	sw	t3, U_PCB_REGS+FRAME_T3(k1)
	mfc0	a0, MIPS_COP_0_STATUS		# 1st arg is STATUS
	sw	t4, U_PCB_REGS+FRAME_T4(k1)
	sw	t5, U_PCB_REGS+FRAME_T5(k1)
	sw	t6, U_PCB_REGS+FRAME_T6(k1)
	sw	t7, U_PCB_REGS+FRAME_T7(k1)
	mfc0	a1, MIPS_COP_0_CAUSE		# 2nd arg is CAUSE
	sw	t8, U_PCB_REGS+FRAME_T8(k1)
	sw	t9, U_PCB_REGS+FRAME_T9(k1)
	sw	gp, U_PCB_REGS+FRAME_GP(k1)
	sw	sp, U_PCB_REGS+FRAME_SP(k1)
	mfc0	a2, MIPS_COP_0_EXC_PC		# 3rd arg is exception PC
	sw	ra, U_PCB_REGS+FRAME_RA(k1)
	sw	a0, U_PCB_REGS+FRAME_SR(k1)
	sw	v0, U_PCB_REGS+FRAME_MULLO(k1)
	sw	v1, U_PCB_REGS+FRAME_MULHI(k1)
	sw	a2, U_PCB_REGS+FRAME_EPC(k1)
	#addiu	sp, k1, USPACE - CALLFRAME_SIZ	# switch to kernel SP
	li	sp, KERNELSTACK-CALLFRAME_SIZ	# XXX switch to kernel SP XXX
#ifdef __GP_SUPPORT__
	la	gp, _C_LABEL(_gp)		# switch to kernel GP
#endif
	.set	at
	and	t0, a0, ~MIPS_SR_COP_1_BIT	# turn off the FPU
	.set	noat
/*
 * Call the interrupt handler.
 */
	sw	a2, CALLFRAME_RA(sp)		# for debugging
	jal	_C_LABEL(interrupt)
	mtc0	t0, MIPS_COP_0_STATUS
/*
 * Check pending asynchoronous traps.
 */
	lw	a1, _C_LABEL(curpcb)
	nop
	lw	a0, U_PCB_REGS+FRAME_SR(a1)
	lw	v0, _C_LABEL(astpending)	# any pending ast?
	mtc0	a0, MIPS_COP_0_STATUS		# restore SR, disable intrs
	beq	v0, zero, 1f			# if no, skip ast processing
	nop					# -delay slot-
/*
 * We have pending asynchronous traps; save remaining user state in u_pcb.
 */
	sw	s0, U_PCB_REGS+FRAME_S0(a1)
	sw	s1, U_PCB_REGS+FRAME_S1(a1)
	sw	s2, U_PCB_REGS+FRAME_S2(a1)
	sw	s3, U_PCB_REGS+FRAME_S3(a1)
	sw	s4, U_PCB_REGS+FRAME_S4(a1)
	sw	s5, U_PCB_REGS+FRAME_S5(a1)
	sw	s6, U_PCB_REGS+FRAME_S6(a1)
	sw	s7, U_PCB_REGS+FRAME_S7(a1)
	sw	s8, U_PCB_REGS+FRAME_S8(a1)

	lw	a0, U_PCB_REGS+FRAME_EPC(a1)	# argument is interrupted PC
	li	t0, MIPS_HARD_INT_MASK | MIPS_SR_INT_IE
	jal	_C_LABEL(ast)
	mtc0	t0, MIPS_COP_0_STATUS		# enable interrupts (spl0)

	lw	a1, _C_LABEL(curpcb)
	nop
	lw	a0, U_PCB_REGS+FRAME_SR(a1)
	lw	s0, U_PCB_REGS+FRAME_S0(a1)
	mtc0	a0, MIPS_COP_0_STATUS		# this should disable interrupts
	lw	s1, U_PCB_REGS+FRAME_S1(a1)
	lw	s2, U_PCB_REGS+FRAME_S2(a1)
	lw	s3, U_PCB_REGS+FRAME_S3(a1)
	lw	s4, U_PCB_REGS+FRAME_S4(a1)
	lw	s5, U_PCB_REGS+FRAME_S5(a1)
	lw	s6, U_PCB_REGS+FRAME_S6(a1)
	lw	s7, U_PCB_REGS+FRAME_S7(a1)
	lw	s8, U_PCB_REGS+FRAME_S8(a1)
1:
	lw	v0, U_PCB_REGS+FRAME_MULLO(a1)
	lw	v1, U_PCB_REGS+FRAME_MULHI(a1)
	mtlo	v0
	mthi	v1
	move	k1, a1
	lw	k0, U_PCB_REGS+FRAME_EPC(k1)	# exception PC
	lw	AT, U_PCB_REGS+FRAME_AST(k1)
	lw	v0, U_PCB_REGS+FRAME_V0(k1)
	lw	v1, U_PCB_REGS+FRAME_V1(k1)
	lw	a0, U_PCB_REGS+FRAME_A0(k1)
	lw	a1, U_PCB_REGS+FRAME_A1(k1)
	lw	a2, U_PCB_REGS+FRAME_A2(k1)
	lw	a3, U_PCB_REGS+FRAME_A3(k1)
	lw	t0, U_PCB_REGS+FRAME_T0(k1)
	lw	t1, U_PCB_REGS+FRAME_T1(k1)
	lw	t2, U_PCB_REGS+FRAME_T2(k1)
	lw	t3, U_PCB_REGS+FRAME_T3(k1)
	lw	t4, U_PCB_REGS+FRAME_T4(k1)
	lw	t5, U_PCB_REGS+FRAME_T5(k1)
	lw	t6, U_PCB_REGS+FRAME_T6(k1)
	lw	t7, U_PCB_REGS+FRAME_T7(k1)
	lw	t8, U_PCB_REGS+FRAME_T8(k1)
	lw	t9, U_PCB_REGS+FRAME_T9(k1)
	lw	gp, U_PCB_REGS+FRAME_GP(k1)
	lw	sp, U_PCB_REGS+FRAME_SP(k1)
	lw	ra, U_PCB_REGS+FRAME_RA(k1)
	j	k0				# return to interrupted point
	rfe
	.set	at
	.globl	_C_LABEL(mips1_UserIntrEnd)
_C_LABEL(mips1_UserIntrEnd):
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
LEAF(mips1_TLBModException)
	.set	noat
	tlbp					# find the TLB entry
	mfc0	k0, MIPS_COP_0_TLB_LOW		# get the physical address
	mfc0	k1, MIPS_COP_0_TLB_INDEX	# check to be sure its valid
	or	k0, k0, MIPS1_TLB_MOD_BIT	# update TLB
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
LEAF(mips1_TLBMissException)
	.set	noat
	mfc0	k0, MIPS_COP_0_BAD_VADDR	# get the fault address
	li	k1, VM_MIN_KERNEL_ADDRESS	# compute index
	subu	k0, k0, k1
	lw	k1, _C_LABEL(Sysmapsize)	# index within range?
	srl	k0, k0, PGSHIFT
	sltu	k1, k0, k1
	beq	k1, zero, 1f			# No. check for valid stack
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

1:
	subu	k0, sp, UADDR + 0x200		# check to see if we have a
	sltiu	k0, UPAGES*NBPG - 0x200		#  valid kernel stack
	bne	k0, zero, _C_LABEL(mips1_KernGenException) # Go panic
	nop
#ifdef DDB
	la	sp, start - (22 * 4)		# build trapframe on stack
	sw	AT, TF_REG_AST(sp)
	sw	v0, TF_REG_V0(sp)
	sw	v1, TF_REG_V1(sp)
	sw	a0, TF_REG_A0(sp)
	sw	a1, TF_REG_A1(sp)
	sw	a2, TF_REG_A2(sp)
	sw	a3, TF_REG_A3(sp)
	sw	t0, TF_REG_T0(sp)
	sw	t1, TF_REG_T1(sp)
	sw	t2, TF_REG_T2(sp)
	sw	t3, TF_REG_T3(sp)
	sw	t4, TF_REG_T4(sp)
	sw	t5, TF_REG_T5(sp)
	sw	t6, TF_REG_T6(sp)
	sw	t7, TF_REG_T7(sp)
	sw	t8, TF_REG_T8(sp)
	sw	t9, TF_REG_T9(sp)
	mfc0	a0, MIPS_COP_0_STATUS
	mflo	v0
	mfhi	v1
	mfc0	a1, epc
	sw	ra, TF_REG_RA(sp)
	sw	a0, TF_REG_SR(sp)
	sw	v0, TF_REG_MULLO(sp)
	sw	v1, TF_REG_MULHI(sp)
	sw	a1, TF_REG_EPC(sp)
	move	a1, sp
	subiu	sp, sp, CALLFRAME_SIZ
	sw	zero, CALLFRAME_RA(sp)
	sw	zero, CALLFRAME_SIZ-8(sp)
	jal	_C_LABEL(ddb_trap)
	move	a0, zero
#endif
	la	sp, start - CALLFRAME_SIZ	# set sp to a valid place
	PANIC("kernel stack overflow")
	.set	at
END(mips1_TLBMissException)


/*--------------------------------------------------------------------------
 *
 * mips1_TLBWriteIndexed --
 *
 *	Write the given entry into the TLB at the given index.
 *
 *	mips1_TLBWriteIndexed(index, highEntry, lowEntry)
 *		int index;
 *		int highEntry;
 *		int lowEntry;
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	TLB entry set.
 *
 *--------------------------------------------------------------------------
 */
LEAF(mips1_TLBWriteIndexed)
	mfc0	v1, MIPS_COP_0_STATUS		# Save the status register.
	mtc0	zero, MIPS_COP_0_STATUS		# Disable interrupts
	mfc0	t0, MIPS_COP_0_TLB_HI		# Save the current PID.

	sll	a0, a0, MIPS1_TLB_INDEX_SHIFT
	mtc0	a0, MIPS_COP_0_TLB_INDEX	# Set the index.
	mtc0	a1, MIPS_COP_0_TLB_HI		# Set up entry high.
	mtc0	a2, MIPS_COP_0_TLB_LOW		# Set up entry low.
	nop
	tlbwi					# Write the TLB

	mtc0	t0, MIPS_COP_0_TLB_HI		# Restore the PID.
	j	ra
	mtc0	v1, MIPS_COP_0_STATUS		# Restore the status register
END(mips1_TLBWriteIndexed)

#if 0
/*--------------------------------------------------------------------------
 *
 * mips1_TLBWriteRandom --
 *
 *	Write the given entry into the TLB at a random location.
 *
 *	mips1_TLBWriteRandom(highEntry, lowEntry)
 *		unsigned highEntry;
 *		unsigned lowEntry;
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	TLB entry set.
 *
 *--------------------------------------------------------------------------
 */
LEAF(mips1_TLBWriteRandom)
	mfc0	v1, MIPS_COP_0_STATUS		# Save the status register.
	mtc0	zero, MIPS_COP_0_STATUS		# Disable interrupts
	mfc0	v0, MIPS_COP_0_TLB_HI		# Save the current PID.
	nop

	mtc0	a0, MIPS_COP_0_TLB_HI		# Set up entry high.
	mtc0	a1, MIPS_COP_0_TLB_LOW		# Set up entry low.
	nop
	tlbwr					# Write the TLB

	mtc0	v0, MIPS_COP_0_TLB_HI		# Restore the PID.
	j	ra
	mtc0	v1, MIPS_COP_0_STATUS		# Restore the status register
END(mips1_TLBWriteRandom)
#endif

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
 * mips1_TLBFlush --
 *
 *	Flush the "random" entries from the TLB.
 *
 *	mips1_TLBFlush()
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The TLB is flushed.
 *
 *--------------------------------------------------------------------------
 */
LEAF(mips1_TLBFlush)
	mfc0	v1, MIPS_COP_0_STATUS		# Save the status register.
	mtc0	zero, MIPS_COP_0_STATUS		# Disable interrupts
	mfc0	t0, MIPS_COP_0_TLB_HI		# Save the PID
	li	t1, MIPS_KSEG0_START		# invalid address
	mtc0	t1, MIPS_COP_0_TLB_HI		# Mark entry high as invalid
	mtc0	zero, MIPS_COP_0_TLB_LOW	# Zero out low entry.
/*
 * Align the starting value (t1) and the upper bound (t2).
 */
	li	t1, MIPS1_TLB_FIRST_RAND_ENTRY << MIPS1_TLB_INDEX_SHIFT
	li	t2, MIPS1_TLB_NUM_TLB_ENTRIES << MIPS1_TLB_INDEX_SHIFT
1:
	mtc0	t1, MIPS_COP_0_TLB_INDEX	# Set the index register.
	addu	t1, t1, 1 << MIPS1_TLB_INDEX_SHIFT	# Increment index.
	bne	t1, t2, 1b
	tlbwi					# Write the TLB entry.

	mtc0	t0, MIPS_COP_0_TLB_HI		# Restore the PID
	j	ra
	mtc0	v1, MIPS_COP_0_STATUS		# Restore the status register
END(mips1_TLBFlush)

#if 0
/*--------------------------------------------------------------------------
 *
 * mips1_TLBFlushPID --
 *
 *	Flush all entries with the given PID from the TLB.
 *
 *	mips1_TLBFlushPID(pid)
 *		int pid;
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	All entries corresponding to this PID are flushed.
 *
 *--------------------------------------------------------------------------
 */
LEAF(mips1_TLBFlushPID)
	mfc0	v1, MIPS_COP_0_STATUS		# Save the status register.
	mtc0	zero, MIPS_COP_0_STATUS		# Disable interrupts
	mfc0	t0, MIPS_COP_0_TLB_HI		# Save the current PID
	sll	a0, a0, MIPS1_TLB_PID_SHIFT	# Align the pid to flush.
/*
 * Align the starting value (t1) and the upper bound (t2).
 */
	li	t1, MIPS1_TLB_FIRST_RAND_ENTRY << MIPS1_TLB_INDEX_SHIFT
	li	t2, MIPS1_TLB_NUM_TLB_ENTRIES << MIPS1_TLB_INDEX_SHIFT
	mtc0	t1, MIPS_COP_0_TLB_INDEX	# Set the index register
1:
	addu	t1, t1, 1 << MIPS1_TLB_INDEX_SHIFT	# Increment index.
	tlbr					# Read from the TLB
	mfc0	t4, MIPS_COP_0_TLB_HI		# Fetch the hi register.
	nop
	and	t4, t4, MIPS1_TLB_PID		# compare PIDs
	bne	t4, a0, 2f
	li	v0, MIPS_KSEG0_START		# invalid address
	mtc0	v0, MIPS_COP_0_TLB_HI		# Mark entry high as invalid
	mtc0	zero, MIPS_COP_0_TLB_LOW	# Zero out low entry.
	nop
	tlbwi					# Write the entry.
2:
	bne	t1, t2, 1b
	mtc0	t1, MIPS_COP_0_TLB_INDEX	# Set the index register

	mtc0	t0, MIPS_COP_0_TLB_HI		# restore PID
	j	ra
	mtc0	v1, MIPS_COP_0_STATUS		# Restore the status register
END(mips1_TLBFlushPID)
#endif

/*--------------------------------------------------------------------------
 *
 * mips1_TLBFlushAddr --
 *
 *	Flush any TLB entries for the given address and TLB PID.
 *
 *	mips1_TLBFlushAddr(highreg)
 *		unsigned highreg;
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The process's page is flushed from the TLB.
 *
 *--------------------------------------------------------------------------
 */
LEAF(mips1_TLBFlushAddr)
	mfc0	v1, MIPS_COP_0_STATUS		# Save the status register.
	mtc0	zero, MIPS_COP_0_STATUS		# Disable interrupts
	mfc0	t0, MIPS_COP_0_TLB_HI		# Get current PID
	nop

	mtc0	a0, MIPS_COP_0_TLB_HI		# look for addr & PID
	nop
	tlbp					# Probe for the entry.
	mfc0	v0, MIPS_COP_0_TLB_INDEX	# See what we got
	li	t1, MIPS_KSEG0_START		# Load invalid entry.
	bltz	v0, 1f				# index < 0 => !found
	mtc0	t1, MIPS_COP_0_TLB_HI		# Mark entry high as invalid
	mtc0	zero, MIPS_COP_0_TLB_LOW	# Zero out low entry.
	nop
	tlbwi
1:
	mtc0	t0, MIPS_COP_0_TLB_HI		# restore PID
	j	ra
	mtc0	v1, MIPS_COP_0_STATUS		# Restore the status register
END(mips1_TLBFlushAddr)

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
	mfc0	v1, MIPS_COP_0_STATUS	# Save the status register.
	mtc0	zero, MIPS_COP_0_STATUS		# Disable interrupts
	mfc0	t0, MIPS_COP_0_TLB_HI		# Save current PID
	nop				# 2 cycles before intr disabled
	mtc0	a0, MIPS_COP_0_TLB_HI		# init high reg.
	nop
	tlbp				# Probe for the entry.
	mfc0	v0, MIPS_COP_0_TLB_INDEX	# See what we got
	mtc0	a1, MIPS_COP_0_TLB_LOW		# init low reg.
	bltz	v0, 1f			# index < 0 => !found
	sra	v0, v0, MIPS1_TLB_INDEX_SHIFT	# convert index to regular num
	b	2f
	tlbwi				# update slot found
1:
	mtc0	a0, MIPS_COP_0_TLB_HI		# init high reg.
	nop
	tlbwr				# enter into a random slot
2:
	mtc0	t0, MIPS_COP_0_TLB_HI		# restore PID
	j	ra
	mtc0	v1, MIPS_COP_0_STATUS	# Restore the status register
END(mips1_TLBUpdate)

/*--------------------------------------------------------------------------
 *
 * mips1_TLBFind --
 *
 *	Search the TLB for the given entry.
 *
 *	mips1_TLBFind(hi, tlb)
 *		unsigned hi;
 *		struct mips1_tlb *tlb;
 *
 * Results:
 *	Returns a value >= 0 if the entry was found (the index).
 *	Returns a value < 0 if the entry was not found.
 *
 *--------------------------------------------------------------------------
 */
LEAF(mips1_TLBFind)
	mfc0	v1, MIPS_COP_0_STATUS	# Save the status register.
	mtc0	zero, MIPS_COP_0_STATUS		# Disable interrupts
	mfc0	t0, MIPS_COP_0_TLB_HI		# Get current PID
	nop
	mtc0	a0, MIPS_COP_0_TLB_HI		# Set up entry high.
	nop
	tlbp				# Probe for the entry.
	mfc0	v0, MIPS_COP_0_TLB_INDEX	# See what we got
	nop
	bltz	v0, 1f			# not found
	nop
	tlbr				# read TLB
	mfc0	t1, MIPS_COP_0_TLB_HI		# See what we got
	mfc0	t2, MIPS_COP_0_TLB_LOW		# See what we got
	sw	t1, 0(a1)
	sw	t2, 4(a1)
	srl	v0, v0, MIPS1_TLB_INDEX_SHIFT	# convert index to regular num
1:
	mtc0	t0, MIPS_COP_0_TLB_HI		# Restore current PID
	j	ra
	mtc0	v1, MIPS_COP_0_STATUS	# Restore the status register
END(mips1_TLBFind)

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
	mfc0	v1, MIPS_COP_0_STATUS	# Save the status register.
	mtc0	zero, MIPS_COP_0_STATUS		# Disable interrupts
	mfc0	t0, MIPS_COP_0_TLB_HI		# Get current PID

	sll	a0, a0, MIPS1_TLB_INDEX_SHIFT
	mtc0	a0, MIPS_COP_0_TLB_INDEX	# Set the index register
	nop
	tlbr				# Read from the TLB
	mfc0	t2, MIPS_COP_0_TLB_HI		# fetch the hi entry
	mfc0	t3, MIPS_COP_0_TLB_LOW		# fetch the low entry
	sw	t2, 0(a1)
	sw	t3, 4(a1)

	mtc0	t0, MIPS_COP_0_TLB_HI		# restore PID
	j	ra
	mtc0	v1, MIPS_COP_0_STATUS	# Restore the status register
END(mips1_TLBRead)

/*--------------------------------------------------------------------------
 *
 * mips1_TLBGetPID --
 *
 *	mips1_TLBGetPID()
 *
 * Results:
 *	Returns the current TLB pid reg.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------------------
 */
LEAF(mips1_TLBGetPID)
	mfc0	v0, MIPS_COP_0_TLB_HI		# get PID
	nop
	and	v0, v0, MIPS1_TLB_PID		# mask off PID
	j	ra
	srl	v0, v0, MIPS1_TLB_PID_SHIFT	# put PID in right spot
END(mips1_TLBGetPID)


/*----------------------------------------------------------------------------
 *
 *	R3000 cache sizing and flushing code.
 *
 *----------------------------------------------------------------------------
 */


/*----------------------------------------------------------------------------
 *
 * mips1_ConfigCache --
 *
 *	Size the caches.
 *	NOTE: should only be called from mach_init().
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The size of the data cache is stored into mips_L1DCacheSize and the
 *	size of instruction cache is stored into mips_L1ICacheSize.
 *
 *----------------------------------------------------------------------------
 */
NESTED(mips1_ConfigCache, CALLFRAME_SIZ, ra)
	subu	sp, sp, CALLFRAME_SIZ
	sw	ra, CALLFRAME_RA(sp)		# Save return address.
	.mask	0x80000000, -4
	mtc0	zero, MIPS_COP_0_STATUS		# Disable interrupts.
	la	v0, 1f
	or	v0, MIPS_KSEG1_START		# Run uncached.
	j	v0
	nop
1:
/*
 * This works because jal doesn't change pc[31..28] and the
 * linker still thinks SizeCache is in the cached region so it computes
 * the correct address without complaining.
 */
	jal	_C_LABEL(mips1_SizeCache)	# Get the size of the d-cache.
	nop
	sw	v0, _C_LABEL(mips_L1DCacheSize)
	nop					# Make sure sw out of pipe
	nop
	nop
	nop
	li	v0, MIPS_SR_SWAP_CACHES		# Swap caches
	mtc0	v0, MIPS_COP_0_STATUS
	nop					# Insure caches stable
	nop
	nop
	nop
	jal	_C_LABEL(mips1_SizeCache)	# Get the size of the i-cache.
	nop
	mtc0	zero, MIPS_COP_0_STATUS		# Swap back caches and enable.
	nop
	nop
	nop
	nop
	sw	v0, _C_LABEL(mips_L1ICacheSize)
	la	t0, 1f
	j	t0				# Back to cached mode
	nop
1:
	lw	ra, CALLFRAME_RA(sp)		# Restore return addr
	addu	sp, sp, CALLFRAME_SIZ		# Restore sp.
	j	ra
	nop
END(mips1_ConfigCache)

/*----------------------------------------------------------------------------
 *
 * mips1_SizeCache --
 *
 *	Get the size of the cache.
 *
 * Results:
 *	The size of the cache.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------------
 */
LEAF(mips1_SizeCache)
	mfc0	t0, MIPS_COP_0_STATUS		# Save the current status reg.
	nop
	or	v0, t0, MIPS_SR_ISOL_CACHES	# Isolate the caches.
	nop					# Make sure no stores in pipe
	mtc0	v0, MIPS_COP_0_STATUS
	nop					# Make sure isolated
	nop
	nop
/*
 * Clear cache size boundaries.
 */
	li	v0, MIPS_MIN_CACHE_SIZE
	li	v1, MIPS_KSEG0_START
	li	t2, MIPS_MAX_CACHE_SIZE
1:
	addu	t1, v0, v1			# Compute address to clear
	sw	zero, 0(t1)			# Clear cache memory
	bne	v0, t2, 1b
	sll	v0, v0, 1

	li	v0, -1
	sw	v0, 0(v1)			# Store marker in cache
	li	v0, MIPS_MIN_CACHE_SIZE
2:
	addu	t1, v0, v1			# Compute address
	lw	t3, 0(t1)			# Look for marker
	nop
	bne	t3, zero, 3f			# Found marker.
	nop
	bne	v0, t2, 2b			# keep looking
	sll	v0, v0, 1			# cache size * 2

	move	v0, zero			# must be no cache
3:
	mtc0	t0, MIPS_COP_0_STATUS
	nop					# Make sure unisolated
	nop
	nop
	nop
	j	ra
	nop
END(mips1_SizeCache)

/*----------------------------------------------------------------------------
 *
 * mips1_FlushCache --
 *
 *	Flush the caches.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The contents of the caches is flushed.
 *
 *----------------------------------------------------------------------------
 */
LEAF(mips1_FlushCache)
	lw	t1, _C_LABEL(mips_L1ICacheSize)	# Must load before isolating
	lw	t2, _C_LABEL(mips_L1DCacheSize)	# Must load before isolating
	mfc0	t3, MIPS_COP_0_STATUS 		# Save the status register.
	mtc0	zero, MIPS_COP_0_STATUS		# Disable interrupts.
	la	v0, 1f
	or	v0, MIPS_KSEG1_START		# Run uncached.
	j	v0
	nop
/*
 * Flush the instruction cache.
 */
1:
	li	v0, MIPS_SR_ISOL_CACHES | MIPS_SR_SWAP_CACHES
	mtc0	v0, MIPS_COP_0_STATUS		# Isolate and swap caches.
	li	t0, MIPS_KSEG1_START
	subu	t0, t0, t1
	li	t1, MIPS_KSEG1_START
	la	v0, 1f				# Run cached
	j	v0
	nop
1:
	addu	t0, t0, 4
	bne	t0, t1, 1b
	sb	zero, -4(t0)

	la	v0, 1f
	or	v0, MIPS_KSEG1_START
	j	v0				# Run uncached
	nop
/*
 * Flush the data cache.
 */
1:
	li	v0, MIPS_SR_ISOL_CACHES
	mtc0	v0, MIPS_COP_0_STATUS		# Isolate and swap back caches
	li	t0, MIPS_KSEG1_START
	subu	t0, t0, t2
	la	v0, 1f
	j	v0				# Back to cached mode
	nop
1:
	addu	t0, t0, 4
	bne	t0, t1, 1b
	sb	zero, -4(t0)

	nop					# Insure isolated stores
	nop					#   out of pipe.
	nop
	nop
	mtc0	t3, MIPS_COP_0_STATUS		# Restore status reg.
	nop					# Insure cache unisolated.
	nop
	nop
	nop
	j	ra
	nop
END(mips1_FlushCache)

/*----------------------------------------------------------------------------
 *
 * mips1_FlushICache --
 *
 *	void mips1_FlushICache(addr, len)
 *		vaddr_t addr; vsize_t len;
 *
 *	Flush instruction cache for range of addr to addr + len - 1.
 *	The address can be any valid address so long as no TLB misses occur.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The contents of the cache is flushed.
 *
 *----------------------------------------------------------------------------
 */
LEAF(mips1_FlushICache)
	mfc0	t0, MIPS_COP_0_STATUS		# Save SR
	mtc0	zero, MIPS_COP_0_STATUS		# Disable interrupts.

	la	v1, 1f
	or	v1, MIPS_KSEG1_START		# Run uncached.
	j	v1
	nop
1:
#ifdef pmax
	/*
	 * The insn below attempts to wait for the  writebuffer to drain. 
	 * It does not work on all MIPS hosts or maybe even all DEC machines.
	 * It assumes that the COP0 usability bit is wired to external
	 * writebuffer logic. This is not true on some DECstations or on
	 * the SONY news3400.
	 * Why we are even doing this for an I-cache flush is beyond me.
	 */
	bc0f	1b				# make sure stores are complete
#endif	/* pmax */
	li	v1, MIPS_SR_ISOL_CACHES | MIPS_SR_SWAP_CACHES	# DELAYSLOT

	mtc0	v1, MIPS_COP_0_STATUS
	nop
	addu	a1, a1, a0			# compute ending address
1:
	addu	a0, a0, 4
	bne	a0, a1, 1b
	sb	zero, -4(a0)

	mtc0	t0, MIPS_COP_0_STATUS		# enable interrupts
	j	ra				# return and run cached
	nop
END(mips1_FlushICache)

/*----------------------------------------------------------------------------
 *
 * mips1_FlushDCache --
 *
 *	void mips1_FlushDCache(addr, len)
 *		vaddr_t addr; vsize_t len;
 *
 *	Flush data cache for range of addr to addr + len - 1.
 *	The address can be any valid address so long as no TLB misses occur.
 *	(Be sure to use cached K0SEG kernel addresses)
 * Results:
 *	None.
 *
 * Side effects:
 *	The contents of the cache is flushed.
 *
 *----------------------------------------------------------------------------
 */
LEAF(mips1_FlushDCache)
	mfc0	t0, MIPS_COP_0_STATUS		# Save SR
	mtc0	zero, MIPS_COP_0_STATUS		# Disable interrupts.
	nop

#ifdef pmax
	/*
	 * The insn below attempts to wait for the writebuffer to drain. 
	 * It does not work on all MIPS hosts or maybe even all DEC machines.
	 * It assumes that the COP0 usability bit is wired to external
	 * writebuffer logic. This is not true on some DECstations or on
	 * the SONY news3400. (note the following ld is in our delayslot.)
	 */
1:
	bc0f	1b				# make sure stores are complete
#endif

	li	v1, MIPS_SR_ISOL_CACHES		# DELAYSLOT
	mtc0	v1, MIPS_COP_0_STATUS
	nop
	addu	t1, a1, a0			# compute ending address
1:
	sb	zero, 0(a0)
	sb	zero, 4(a0)
	sb	zero, 8(a0)
	sb	zero, 12(a0)
	sb	zero, 16(a0)
	sb	zero, 20(a0)
	sb	zero, 24(a0)
	addu	a0, 32
	bltu	a0, t1, 1b
	sb	zero, -4(a0)

	nop					# drain pipeline
	nop
	mtc0	t0, MIPS_COP_0_STATUS		# enable interrupts
	nop
	j	ra				# return and run cached
	nop
END(mips1_FlushDCache)

/*----------------------------------------------------------------------------
 *
 * mips1_wbflush --
 *
 *	Return when the write buffer is empty.
 *
 *	mips1_wbflush()
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------------
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

/*
 * mips1_proc_trampoline()
 *
 * Arrange for a function to be invoked neatly, after a cpu_switch().
 * Invoke the service function with one argument, specified by the s0
 * and s1 respectively.  There is no need register save operation.
 */
LEAF(mips1_proc_trampoline) 
	jal	ra, s0
	move	a0, s1
	.set	noat
	lw	a1, _C_LABEL(curpcb)
	nop
	lw	a0, U_PCB_REGS+FRAME_SR(a1)
	lw	t0, U_PCB_REGS+FRAME_MULLO(a1)
	lw	t1, U_PCB_REGS+FRAME_MULHI(a1)
	mtc0	a0, MIPS_COP_0_STATUS		# this should disable interrupts
	mtlo	t0
	mthi	t1
	nop
	move	k1, a1
	lw	k0, U_PCB_REGS+FRAME_EPC(k1)
	lw	AT, U_PCB_REGS+FRAME_AST(k1)
	lw	v0, U_PCB_REGS+FRAME_V0(k1)
	lw	v1, U_PCB_REGS+FRAME_V1(k1)
	lw	a0, U_PCB_REGS+FRAME_A0(k1)
	lw	a1, U_PCB_REGS+FRAME_A1(k1)
	lw	a2, U_PCB_REGS+FRAME_A2(k1)
	lw	a3, U_PCB_REGS+FRAME_A3(k1)
	lw	t0, U_PCB_REGS+FRAME_T0(k1)
	lw	t1, U_PCB_REGS+FRAME_T1(k1)
	lw	t2, U_PCB_REGS+FRAME_T2(k1)
	lw	t3, U_PCB_REGS+FRAME_T3(k1)
	lw	t4, U_PCB_REGS+FRAME_T4(k1)
	lw	t5, U_PCB_REGS+FRAME_T5(k1)
	lw	t6, U_PCB_REGS+FRAME_T6(k1)
	lw	t7, U_PCB_REGS+FRAME_T7(k1)
	lw	s0, U_PCB_REGS+FRAME_S0(k1)
	lw	s1, U_PCB_REGS+FRAME_S1(k1)
	lw	s2, U_PCB_REGS+FRAME_S2(k1)
	lw	s3, U_PCB_REGS+FRAME_S3(k1)
	lw	s4, U_PCB_REGS+FRAME_S4(k1)
	lw	s5, U_PCB_REGS+FRAME_S5(k1)
	lw	s6, U_PCB_REGS+FRAME_S6(k1)
	lw	s7, U_PCB_REGS+FRAME_S7(k1)
	lw	t8, U_PCB_REGS+FRAME_T8(k1)
	lw	t9, U_PCB_REGS+FRAME_T9(k1)
	lw	gp, U_PCB_REGS+FRAME_GP(k1)
	lw	s8, U_PCB_REGS+FRAME_S8(k1)
	lw	ra, U_PCB_REGS+FRAME_RA(k1)
	lw	sp, U_PCB_REGS+FRAME_SP(k1)
	nop
	j	k0
	rfe
	.set	at
END(mips1_proc_trampoline)

/*
 * switch_exit(struct proc *)
 *
 * Make the named process exit.  Switch SP to nullproc stack, schedule
 * the exiting proc's vmspace and stack to be freed, then jump into the
 * middle of cpu_switch().  MUST BE CALLED AT SPLHIGH.
 */
	.globl	sw1
LEAF(mips1_switch_exit)
	la	v1, _C_LABEL(nullproc)		# !! SP runs on p->p_addr !!
	lw	t0, P_MD_UPTE+0(v1)		# t0 = first u. pte
	lw	t1, P_MD_UPTE+4(v1)		# t1 = 2nd u. pte
	li	v0, MIPS_KSEG0_START		# clear wired entries for pcb
	mtc0	v0,  MIPS_COP_0_TLB_HI
	mtc0	zero, MIPS_COP_0_TLB_LOW
	li	v0, 2 << MIPS1_TLB_INDEX_SHIFT
	mtc0	v0, MIPS_COP_0_TLB_INDEX	# clear wired entry 2
	nop
	tlbwi
	li	v0, 3 << MIPS1_TLB_INDEX_SHIFT
	mtc0	v0, MIPS_COP_0_TLB_INDEX	# clear wired entry 3
	nop
	tlbwi
	li	v0, UADDR			# still using UADDR for kstack
	nop
	mtc0	zero, MIPS_COP_0_TLB_INDEX	# set the index register
	mtc0	v0, MIPS_COP_0_TLB_HI		# init high entry
	mtc0	t0, MIPS_COP_0_TLB_LOW		# init low entry
	li	t0, 1 << MIPS1_TLB_INDEX_SHIFT
	tlbwi					# Write the TLB entry.
	addu	v0, v0, NBPG			# 2nd HI entry
	mtc0	t0, MIPS_COP_0_TLB_INDEX	# set the index register
	mtc0	v0, MIPS_COP_0_TLB_HI		# init high entry
	mtc0	t1, MIPS_COP_0_TLB_LOW		# init low entry
	subu	v0, v0, NBPG			# back to start of u-area
	tlbwi					# Write the TLB entry.

	addu	sp, v0, USPACE - CALLFRAME_SIZ
	la	ra, sw1				# rathole to cpu_switch()
	j	exit2				# proc already in a0
	nop
END(mips1_switch_exit)

/*
 * Resume process indicated by the pte's for its u struct
 * NOTE: This is hard coded to UPAGES == 2.
 * Also, there should be no TLB faults at this point.
 *
 * Entered from CPU-common cpu_switch()
 * a0 = p_addr [curpcb]
 * a1 = First u.pte
 * a2 = Second u.pte
 * a3 = TLB Process ID for this process
 */
/*
 * XXXX - almost done with the UADDR mapping; only the kernel stack should
 * now be using UADDR.  The first TLB entries are are used to map
 * UADDR to the u-area.  Then the following TLB entries are used to
 * map the current u-area.  Also, any current mapping for the u-area
 * address space needs to be flushed as well.
 *
 * Both the UADDR and u-area are now mapped global.
 *
 * NB: cannot be profiled due to weird argument-assing convention.
 *     mcount would trash v0, t0, t1.
 */
LEAF_NOPROFILE(mips1_cpu_switch_resume)
	sll	a3, a3, MIPS1_TLB_PID_SHIFT	# a3 = aligned PID
	li	v0, UADDR			# HI entry; PID = 0
	ori	a1, MIPS1_PG_G			# Global
	ori	a2, MIPS1_PG_G			# Global
	mtc0	zero, MIPS_COP_0_TLB_INDEX	# set the index register
	mtc0	v0, MIPS_COP_0_TLB_HI		# init high entry
	mtc0	a1, MIPS_COP_0_TLB_LOW		# init low entry
	li	s1, 1 << MIPS1_TLB_INDEX_SHIFT
	tlbwi					# Write the TLB entry.
	addu	v0, v0, NBPG			# 2nd HI entry
	mtc0	s1, MIPS_COP_0_TLB_INDEX	# set the index register
	mtc0	v0, MIPS_COP_0_TLB_HI		# init high entry
	mtc0	a2, MIPS_COP_0_TLB_LOW		# init low entry
	nop
	tlbwi					# Write the TLB entry.
 # now map p_addr; may need to flush random TLB entries
	mtc0	a0, MIPS_COP_0_TLB_HI		# PID = 0
	nop
	tlbp					# probe for entry
	mfc0	s1, MIPS_COP_0_TLB_INDEX	# see if we found it
	nop
	bltz	s1, 1f				# not found
	li	s1, MIPS_KSEG0_START
	mtc0	s1,  MIPS_COP_0_TLB_HI
	mtc0	zero, MIPS_COP_0_TLB_LOW
	nop
	tlbwi
	nop
	mtc0	a0, MIPS_COP_0_TLB_HI		# restore High
1:
	li	s1, 2 << MIPS1_TLB_INDEX_SHIFT
	mtc0	s1, MIPS_COP_0_TLB_INDEX
	mtc0	a1, MIPS_COP_0_TLB_LOW
	nop
	tlbwi
	addu	a0, a0, NBPG
	mtc0	a0, MIPS_COP_0_TLB_HI		# second page of p_addr
	nop
	tlbp					# probe for entry
	mfc0	s1, MIPS_COP_0_TLB_INDEX	# see if we found it
	nop
	bltz	s1, 1f				# not found
	li	s1, MIPS_KSEG0_START
	mtc0	s1,  MIPS_COP_0_TLB_HI
	mtc0	zero, MIPS_COP_0_TLB_LOW
	nop
	tlbwi
	nop
	mtc0	a0, MIPS_COP_0_TLB_HI		# restore High
1:
	li	s1, 3 << MIPS1_TLB_INDEX_SHIFT
	mtc0	s1, MIPS_COP_0_TLB_INDEX
	mtc0	a2, MIPS_COP_0_TLB_LOW
	nop
	tlbwi
	mtc0	a3, MIPS_COP_0_TLB_HI		# set PID [UADDR/pcb GLOBAL]
/*
 * Now running on new u struct.
 * Restore registers and return.
 */
	lw	t0, _C_LABEL(curpcb)
	nop
	lw	v0, U_PCB_CONTEXT+SF_REG_ST(t0)	# restore kernel context
	lw	ra, U_PCB_CONTEXT+SF_REG_RA(t0)
	lw	s0, U_PCB_CONTEXT+SF_REG_S0(t0)
	lw	s1, U_PCB_CONTEXT+SF_REG_S1(t0)
	lw	s2, U_PCB_CONTEXT+SF_REG_S2(t0)
	lw	s3, U_PCB_CONTEXT+SF_REG_S3(t0)
	lw	s4, U_PCB_CONTEXT+SF_REG_S4(t0)
	lw	s5, U_PCB_CONTEXT+SF_REG_S5(t0)
	lw	s6, U_PCB_CONTEXT+SF_REG_S6(t0)
	lw	s7, U_PCB_CONTEXT+SF_REG_S7(t0)
	lw	sp, U_PCB_CONTEXT+SF_REG_SP(t0)
	lw	s8, U_PCB_CONTEXT+SF_REG_S8(t0)
	nop
	mtc0	v0, MIPS_COP_0_STATUS
	j	ra
	li	v0, 1				# possible return to 'savectx()'
END(mips1_cpu_switch_resume)

	.data
mips1_excausesw:
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
	.word _C_LABEL(mips1_KernGenException)	# 16 reserved
	.word _C_LABEL(mips1_KernGenException)	# 17 reserved
	.word _C_LABEL(mips1_KernGenException)	# 18 reserved
	.word _C_LABEL(mips1_KernGenException)	# 19 reserved
	.word _C_LABEL(mips1_KernGenException)	# 20 reserved
	.word _C_LABEL(mips1_KernGenException)	# 21 reserved
	.word _C_LABEL(mips1_KernGenException)	# 22 reserved
	.word _C_LABEL(mips1_KernGenException)	# 23 watch exception
	.word _C_LABEL(mips1_KernGenException)	# 24 reserved
	.word _C_LABEL(mips1_KernGenException)	# 25 reserved
	.word _C_LABEL(mips1_KernGenException)	# 26 reserved
	.word _C_LABEL(mips1_KernGenException)	# 27 reserved
	.word _C_LABEL(mips1_KernGenException)	# 28 reserved
	.word _C_LABEL(mips1_KernGenException)	# 29 reserved
	.word _C_LABEL(mips1_KernGenException)	# 30 reserved
	.word _C_LABEL(mips1_KernGenException)	# 31 v. coherence exception data
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
