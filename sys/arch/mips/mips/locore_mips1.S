/*	$NetBSD: locore_mips1.S,v 1.98 2022/03/13 22:18:56 andvar Exp $	*/

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
#include "opt_cputype.h"
#include "opt_ddb.h"
#include "opt_kgdb.h"

#include <sys/cdefs.h>

#include <mips/asm.h>
#include <mips/cpuregs.h>

RCSID("$NetBSD: locore_mips1.S,v 1.98 2022/03/13 22:18:56 andvar Exp $")

#include "assym.h"

#define	_SLLV		sllv

#define	_SLL		sll
#define	_SRL		srl
#define	WIRED_SHIFT	2

/*
 * Use correct-sized m?c0/dm?c0 opcodes.
 */
#define	_MFC0	mfc0
#define	_MTC0	mtc0

#if defined(__mips_n32) || defined(__mips_n64)
#error MIPS1 does not support N32/N64.
#endif

#define MIPSX(name)	__CONCAT(mips1_,name)

	.set	noreorder
	.text

EXPORT(MIPSX(exceptionentry_start))

/*
 * mipsN_utlb_miss
 *
 * A reference is made (in either kernel or user mode) to a page in
 * kuseg that has no matching TLB entry.  This routine is copied down
 * at 0x80000000 and total length must be less than 32 instructions.
 * No pc relative jump instruction is allowed.
 */
VECTOR(MIPSX(utlb_miss), unknown)
	.set	noat
	_MFC0	k0, MIPS_COP_0_BAD_VADDR	#00: k0=bad address
	lui	k1, %hi(CPUVAR(PMAP_SEG0TAB))	#01: k1=hi of seg0tab
	bltz	k0, 1f				# R3000 chip bug
	 PTR_SRL k0, SEGSHIFT-PTR_SCALESHIFT	#03: k0=seg offset (almost)
	PTR_L	k1, %lo(CPUVAR(PMAP_SEG0TAB))(k1) #04: k1=seg0tab
	andi	k0, (NSEGPG-1)<<PTR_SCALESHIFT	#07: k0=seg offset (mask 0x3)
	PTR_ADDU k1, k0				#08: k1=seg entry address
	PTR_L	k1, 0(k1)			#09: k1=seg entry
	_MFC0	k0, MIPS_COP_0_BAD_VADDR	#0a: k0=bad address (again)
	beqz	k1, MIPSX(nopagetable)		#0b: ==0 -- no page table
	 PTR_SRL k0, (PGSHIFT-PTPSHIFT)		#0c: k0=VPN (aka va>>10)
	andi	k0, (NPTEPG-1) << PTPSHIFT	#0d: k0=page table offset
	PTR_ADDU k1, k0				#0e: k1=pte address
	INT_L	k0, 0(k1)			#0f: k0=lo0 pte
	nop					#10: load delay
	beqz	k0, MIPSX(invalidpte)		#11: dont load invalid entries
	 nop					#12  branch delay
	mtc0	k0, MIPS_COP_0_TLB_LOW		#13: lo0 is loaded
	nop					#14: load delay
	tlbwr					#15: update TLB
1:
	_MFC0	k1, MIPS_COP_0_EXC_PC		#16: get return address
	nop					#17: load delay
	j	k1				#18: return from
	 rfe					#19:    exception
MIPSX(nopagetable):
MIPSX(invalidpte):
	j	MIPSX(slowfault)		#1a: handle the rest
	 nop					#1b: branch delay
	.set	at
VECTOR_END(MIPSX(utlb_miss))


/*
 * mipsN_exception
 *
 * Handles any exceptions other than reset and UTLB miss.  This routine
 * is copied down at 0x80000080 and total length must be less than 32
 * instructions.  No pc relative jump instruction is allowed.
 */
	.org	MIPSX(utlb_miss) + 0x80
VECTOR(MIPSX(exception), unknown)
/*
 * Find out what mode we came from and jump to the proper handler.
 */
	.set	noat
	mfc0	k0, MIPS_COP_0_STATUS		#00: get the status register
	mfc0	k1, MIPS_COP_0_CAUSE		#01: get the cause register
	and	k0, MIPS1_SR_KU_PREV		#02: test for user mode
	sll	k0, 4				#03: shift user bit for cause index
	and	k1, MIPS1_CR_EXC_CODE		#04: mask out the cause bits
	or	k1, k0				#05: change index to user table
	PTR_LA	k0, MIPSX(excpt_sw)		#06: get base of the jump table
	PTR_ADDU k0, k1				#08: get the address of the
						#  function entry.  Note that
						#  the cause is already
						#  shifted left by 2 bits so
						#  we dont have to shift.
	PTR_L	k0, 0(k0)			#09: get the function address
	nop					#0a: load delay
	j	k0				#0b: jump to the function
	 nop					#0c
	nop					#0d
	nop					#0e
	nop					#0f
	.set	at
VECTOR_END(MIPSX(exception))


/*----------------------------------------------------------------------------
 *
 * mipsN_slowfault
 *
 * Alternate entry point into the mipsN_user_gen_exception or
 * mipsN_kern_gen_exception, when the UTLB miss handler couldn't
 * find a TLB entry.
 *
 * Find out what mode we came from and call the appropriate handler.
 *
 *----------------------------------------------------------------------------
 */
MIPSX(slowfault):
	.set	noat
	mfc0	k0, MIPS_COP_0_STATUS
	nop
	and	k0, MIPS1_SR_KU_PREV
	bnez	k0, _C_LABEL(MIPSX(user_gen_exception))
	 nop
	.set	at
/*
 * Fall through ...
 */

/*
 * mipsN_kern_gen_exception
 *
 * Handle an exception during kernel mode.
 * Build trapframe on stack to hold interrupted kernel context, then
 * call trap() to process the condition.
 *
 * trapframe is pointed to by the 5th arg and a dummy sixth argument is used
 * to avoid alignment problems
 * {
 *	register_t cf_args[4 + 1];
 *	register_t cf_pad;		(for 8 word alignment)
 *	register_t cf_sp;
 *	register_t cf_ra;
 *	struct reg cf_tf;
 * };
 */
NESTED_NOPROFILE(MIPSX(kern_gen_exception), KERNFRAME_SIZ, ra)
	.set	noat
	.mask	0x80000000, -4
#ifdef PARANOIA
	PTR_L	k0, L_PCB(MIPS_CURLWP)
	nop
	slt	k0, k0, sp		# k0 = L_PCB(MIPS_CURLWP) < sp
1:	beqz	k0, 1b			# loop forever if false
	 nop
	PTR_L	k0, L_PCB(MIPS_CURLWP)
	nop
	PTR_ADDU k0, USPACE
	slt	k0, sp, k0		# k0 = sp < L_PCB(MIPS_CURLWP) + USPACE
2:	beqz	k0, 2b			# loop forever if false
	 nop
#endif /* PARANOIA
/*
 * Save the relevant kernel registers onto the stack.
 * We don't need to save s0 - s8, sp and gp because
 * the compiler does it for us.
 */
	PTR_SUBU sp, KERNFRAME_SIZ
	REG_S	AT, TF_BASE+TF_REG_AST(sp)
	REG_S	v0, TF_BASE+TF_REG_V0(sp)
	REG_S	v1, TF_BASE+TF_REG_V1(sp)
	mflo	v0
	mfhi	v1
	REG_S	a0, TF_BASE+TF_REG_A0(sp)
	REG_S	a1, TF_BASE+TF_REG_A1(sp)
	REG_S	a2, TF_BASE+TF_REG_A2(sp)
	REG_S	a3, TF_BASE+TF_REG_A3(sp)
	mfc0	a0, MIPS_COP_0_STATUS		# 1st arg is STATUS
	REG_S	t0, TF_BASE+TF_REG_T0(sp)
	REG_S	t1, TF_BASE+TF_REG_T1(sp)
	REG_S	t2, TF_BASE+TF_REG_T2(sp)
	REG_S	t3, TF_BASE+TF_REG_T3(sp)
	mfc0	a1, MIPS_COP_0_CAUSE		# 2nd arg is CAUSE
	REG_S	ta0, TF_BASE+TF_REG_TA0(sp)
	REG_S	ta1, TF_BASE+TF_REG_TA1(sp)
	REG_S	ta2, TF_BASE+TF_REG_TA2(sp)
	REG_S	ta3, TF_BASE+TF_REG_TA3(sp)
	_MFC0	a2, MIPS_COP_0_BAD_VADDR	# 3rd arg is fault address
	#REG_S	t8, TF_BASE+TF_REG_T8(sp)	# is MIPS_CURLWP
	REG_S	t9, TF_BASE+TF_REG_T9(sp)
	REG_S	ra, TF_BASE+TF_REG_RA(sp)
	REG_S	a0, TF_BASE+TF_REG_SR(sp)
	_MFC0	a3, MIPS_COP_0_EXC_PC		# 4th arg is exception PC
	REG_S	v0, TF_BASE+TF_REG_MULLO(sp)
	REG_S	v1, TF_BASE+TF_REG_MULHI(sp)
	REG_S	a3, TF_BASE+TF_REG_EPC(sp)
	REG_S	a1, TF_BASE+TF_REG_CAUSE(sp)
#if defined(DDB) || defined(KGDB)
	REG_S	s0, TF_BASE+TF_REG_S0(sp)
	REG_S	s1, TF_BASE+TF_REG_S1(sp)
	REG_S	s2, TF_BASE+TF_REG_S2(sp)
	REG_S	s3, TF_BASE+TF_REG_S3(sp)
	REG_S	s4, TF_BASE+TF_REG_S4(sp)
	REG_S	s5, TF_BASE+TF_REG_S5(sp)
	REG_S	s6, TF_BASE+TF_REG_S6(sp)
	REG_S	s7, TF_BASE+TF_REG_S7(sp)
	PTR_ADDU v0, sp, KERNFRAME_SIZ
	REG_S	v0, TF_BASE+TF_REG_SP(sp)
	REG_S	s8, TF_BASE+TF_REG_S8(sp)
	REG_S	gp, TF_BASE+TF_REG_GP(sp)
#endif
	PTR_ADDU v0, sp, TF_BASE
	REG_S	v0, KERNFRAME_ARG5(sp)		# 5th arg is p. to trapframe
#ifdef PARANOIA
	/*
	 * save PPL in trapframe
	 */
	PTR_L	t0, L_CPU(MIPS_CURLWP)
	nop
	INT_L	t1, CPU_INFO_CPL(t0)		# get current priority level
	nop
	INT_S	t1, TF_BASE+TF_PPL(sp)		# save priority level
#endif /* PARANOIA */

#if defined(DDB) || defined(DEBUG) || defined(KGDB)
	PTR_ADDU v0, sp, KERNFRAME_SIZ
	REG_S	v0, KERNFRAME_SP(sp)
#endif

#ifdef PARANOIA
	/*
	 * Verify our existing interrupt level.
	 */
	jal	_C_LABEL(splcheck)
	 nop
#endif /* PARANOIA */

	/*
	 * Call the trap handler.
	 */
	jal	_C_LABEL(trap)
	 REG_S	a3, KERNFRAME_RA(sp)		# for debugging

	/*
	 * Restore registers and return from the exception.
	 */
	REG_L	a0, TF_BASE+TF_REG_SR(sp)
	nop
	mtc0	a0, MIPS_COP_0_STATUS		# restore the SR, disable intrs

	/*
	 * Start of common kernel exception return code for both
	 * mipxN_kern_gen_exception and mipsN_kern_intr.
	 */
MIPSX(kern_return):
	REG_L	t0, TF_BASE+TF_REG_MULLO(sp)
	REG_L	t1, TF_BASE+TF_REG_MULHI(sp)
	REG_L	k1, TF_BASE+TF_REG_EPC(sp)	# might be changed inside trap
	mtlo	t0
	mthi	t1

#ifdef PARANOIA
	INT_L	t2, TF_BASE+TF_PPL(sp)		# get saved priority level
	PTR_L	t0, L_CPU(MIPS_CURLWP)
	nop
	INT_L	t1, CPU_INFO_CPL(t0)		# get current priority level
	nop
11:	bne	t2, t1, 11b			# loop forever if unequal
	 nop

	/*
	 * Verify our existing interrupt level.
	 */
	jal	_C_LABEL(splcheck)
	 nop
#endif /* PARANOIA */

	/*
	 * Check for kernel restartable atomic sequences.
	 */
	PTR_LA	t0, _C_LABEL(_lock_ras_start)
	li	t1, -MIPS_LOCK_RAS_SIZE
	and	t1, k1
	bne	t1, t0, 1f			# exception PC in RAS area?
	 nop
	jal	_C_LABEL(_restart_lock_ras)	# fix the pc (k1)
	 nop
1:

	REG_L	AT, TF_BASE+TF_REG_AST(sp)
	REG_L	v0, TF_BASE+TF_REG_V0(sp)
	REG_L	v1, TF_BASE+TF_REG_V1(sp)
	REG_L	a0, TF_BASE+TF_REG_A0(sp)
	REG_L	a1, TF_BASE+TF_REG_A1(sp)
	REG_L	a2, TF_BASE+TF_REG_A2(sp)
	REG_L	a3, TF_BASE+TF_REG_A3(sp)
	REG_L	t0, TF_BASE+TF_REG_T0(sp)
	REG_L	t1, TF_BASE+TF_REG_T1(sp)
	REG_L	t2, TF_BASE+TF_REG_T2(sp)
	REG_L	t3, TF_BASE+TF_REG_T3(sp)
	REG_L	ta0, TF_BASE+TF_REG_TA0(sp)
	REG_L	ta1, TF_BASE+TF_REG_TA1(sp)
	REG_L	ta2, TF_BASE+TF_REG_TA2(sp)
	REG_L	ta3, TF_BASE+TF_REG_TA3(sp)
	#REG_L	t8, TF_BASE+TF_REG_T8(sp)	# is MIPS_CURLWP
	REG_L	t9, TF_BASE+TF_REG_T9(sp)
	REG_L	ra, TF_BASE+TF_REG_RA(sp)
#ifdef DDBnotyet
	REG_L	s0, TF_BASE+TF_REG_S0(sp)
	REG_L	s1, TF_BASE+TF_REG_S1(sp)
	REG_L	s2, TF_BASE+TF_REG_S2(sp)
	REG_L	s3, TF_BASE+TF_REG_S3(sp)
	REG_L	s4, TF_BASE+TF_REG_S4(sp)
	REG_L	s5, TF_BASE+TF_REG_S5(sp)
	REG_L	s6, TF_BASE+TF_REG_S6(sp)
	REG_L	s7, TF_BASE+TF_REG_S7(sp)
	REG_L	s8, TF_BASE+TF_REG_S8(sp)
#endif
	PTR_ADDU sp, KERNFRAME_SIZ
	j	k1				# return to interrupted point
	rfe
	.set	at
END(MIPSX(kern_gen_exception))

/*
 * mipsN_kern_intr
 *
 * Handle an interrupt from kernel mode.
 * Build kernframe on stack to hold interrupted kernel context, then
 * call cpu_intr() to process it.
 *
 */
NESTED_NOPROFILE(MIPSX(kern_intr), KERNFRAME_SIZ, ra)
	.set	noat
	.mask	0x80000000, -4
#ifdef PARANOIA
	PTR_L	k0, L_PCB(MIPS_CURLWP)
	nop
	slt	k0, k0, sp			# k0 = L_PCB(MIPS_CURLWP) < sp
1:	beqz	k0, 1b				# loop forever if false
	 nop
	PTR_L	k0, L_PCB(MIPS_CURLWP)
	nop
	PTR_ADDU k0, USPACE
	slt	k0, sp, k0			# k0 = sp < L_PCB(MIPS_CURLWP) + USPACE
2:	beqz	k0, 2b				# loop forever if false
	 nop
	PTR_L	k0, L_CPU(MIPS_CURLWP)
	nop
	INT_L	k0, CPU_INFO_IDEPTH(k0)		# grab interrupt depth
	nop
	sltu	k0, k0, 3			# must be < 3
3:	beqz	k0, 3b				# loop forever if false
	 nop
#endif
	/*
	 * Save the relevant kernel registers onto the stack.  We don't need
	 * to save s0 - s8, sp, and gp because the compiler does it for us.
	 * But we use s0-s2 so need to save them.
	 */
	PTR_SUBU sp, KERNFRAME_SIZ
	REG_S	AT, TF_BASE+TF_REG_AST(sp)
	REG_S	v0, TF_BASE+TF_REG_V0(sp)
	REG_S	v1, TF_BASE+TF_REG_V1(sp)
	mflo	v0
	mfhi	v1
	REG_S	a0, TF_BASE+TF_REG_A0(sp)
	REG_S	a1, TF_BASE+TF_REG_A1(sp)
	REG_S	a2, TF_BASE+TF_REG_A2(sp)
	REG_S	a3, TF_BASE+TF_REG_A3(sp)
	REG_S	t0, TF_BASE+TF_REG_T0(sp)
	REG_S	t1, TF_BASE+TF_REG_T1(sp)
	REG_S	t2, TF_BASE+TF_REG_T2(sp)
	REG_S	t3, TF_BASE+TF_REG_T3(sp)
	REG_S	ta0, TF_BASE+TF_REG_TA0(sp)
	REG_S	ta1, TF_BASE+TF_REG_TA1(sp)
	REG_S	ta2, TF_BASE+TF_REG_TA2(sp)
	REG_S	ta3, TF_BASE+TF_REG_TA3(sp)
	REG_S	s0, TF_BASE+TF_REG_S0(sp)	# used for saved ipl/idepth
	REG_S	s1, TF_BASE+TF_REG_S1(sp)	# used for initial status
	mfc0	s1, MIPS_COP_0_STATUS
	REG_S	s2, TF_BASE+TF_REG_S2(sp)	# used for cpu_info
	#REG_S	t8, TF_BASE+TF_REG_T8(sp)	# already contains MIPS_CURLWP
	REG_S	t9, TF_BASE+TF_REG_T9(sp)
	REG_S	ra, TF_BASE+TF_REG_RA(sp)
	REG_S	s1, TF_BASE+TF_REG_SR(sp)
	REG_S	v0, TF_BASE+TF_REG_MULLO(sp)
	REG_S	v1, TF_BASE+TF_REG_MULHI(sp)
/*
 * Call the interrupt handler.
 */
	_MFC0	ta0, MIPS_COP_0_EXC_PC		# grab exception PC
	PTR_L	s2, L_CPU(MIPS_CURLWP)		# delay slot
	REG_S	ta0, TF_BASE+TF_REG_EPC(sp)	# and save it

#if defined(DDB) || defined(DEBUG) || defined(KGDB)
	REG_S	ta0, KERNFRAME_RA(sp)		# for debugging
#endif

#ifdef PARANOIA
	INT_L	s0, CPU_INFO_CPL(s2)
	nop					# load delay
	INT_S	s0, TF_BASE+TF_PPL(sp)		# save priority level

	/*
	 * Verify the current interrupt level
	 */
	jal	_C_LABEL(splcheck)
	 nop
#endif /* PARANOIA */

	/*
	 * We first need to get to IPL_HIGH so that interrupts are masked.
	 */
	jal	_C_LABEL(splhigh_noprof)
	 nop

#ifdef PARANOIA
1:	bne	s0, v0, 1b
	 nop
#endif /* PARANOIA */

	sll	s0, v0, 8			# remember previous priority
						# low 8 bits used for idepth

#ifdef PARANOIA
	/*
	 * Interrupts at IPL_HIGH are not allowed.
	 */
	li	v1, IPL_HIGH
	sltu	t0, v0, v1
2:	beqz	t0, 2b
	 nop
#endif /* PARANOIA */

	INT_L	t1, CPU_INFO_IDEPTH(s2)		# we need to inc. intr depth
	nop					# load delay
	or	s0, t1				#   save old interrupt depth
	INT_ADDU t1, 1
	INT_S	t1, CPU_INFO_IDEPTH(s2)		#   store new interrupt depth

	/*
	 * Now that we're at splhigh so all interrupts are masked
	 * individually and we won't get interrupted here, turn the
	 * global interrupt enable bit on again. This will allow
	 * high-priority interrupts to be delivered once a
	 * low-priority interrupt handler lowers spl to execute.
	 */
	mfc0	v1, MIPS_COP_0_STATUS
	nop
	or	v0, v1, MIPS_SR_INT_IE
	mtc0	v0, MIPS_COP_0_STATUS		# write new status

	/*
	 * Now hard interrupts can be processed.
	 */
	move	a1, ta0				# 2nd arg is exception PC
	move	a2, s1				# 3rd arg is status
	jal	_C_LABEL(cpu_intr)		# cpu_intr(ppl, pc, status)
	 srl	a0, s0, 8			# 1st arg is previous pri level

	and	t1, s0, 0xff			# get previous interrupt depth
	INT_S	t1, CPU_INFO_IDEPTH(s2)		# to it previous value

#ifdef PARANOIA
	mfc0	t0, MIPS_COP_0_STATUS		# verify INT_IE is still set
	nop
	and	t0, MIPS_SR_INT_IE
3:	beqz	t0, 3b
	 nop
#endif /* PARANOIA */

#ifdef __HAVE_FAST_SOFTINTS
	and	a0, s1, MIPS_SOFT_INT_MASK	# were softints enabled?
	beqz	a0, 4f				#   nope
	 nop
	mfc0	v0, MIPS_COP_0_CAUSE		# grab the pending softints
	nop
	and	a0, v0				# are softints pending
	beqz	a0, 4f				#   nope
	 nop

	jal	_C_LABEL(softint_process)	# softint_process(pending)
	 nop

#ifdef __HAVE_PREEMPTION
	srl	v1, s0, 8			# get saved priority level
	bnez	v1, 4f				# branch if not at IPL_NONE
	 nop
	INT_L	t0, CPU_INFO_SOFTINTS(s2)	# get pending softints
	nop
	and	v0, t0, 1 << SOFTINT_KPREEMPT	# do we need a kernel preempt?
	beqz	v0, 4f				#   nope
	 nop
	xor	t0, v0				# clear preempt bit
	INT_S	t0, CPU_INFO_SOFTINTS(s2)	# and save it.
	jal	_C_LABEL(kpreempt)		# kpreempt(pc)
	 PTR_L	a0, TF_BASE+TF_REG_EPC(sp)
#endif /* __HAVE_PREEMPTION */
4:
#endif /* __HAVE_FAST_SOFTINTS */
	/*
	 * Interrupts handled, restore registers and return from the interrupt.
	 * First, clear interrupt enable
	 */
	mtc0	s1, MIPS_COP_0_STATUS		# disable interrupts

	srl	a0, s0, 8			# get previous priority level
#ifdef PARANOIA
	INT_L	t0, TF_BASE+TF_PPL(sp)		# get saved priority level
	nop
9:	bne	t0, a0, 9b			# should still match
	 nop

	li	t0, IPL_HIGH
	sltu	v0, a0, t0
8:	beqz	v0, 8b
	 nop
#endif /* PARANOIA */

	/*
	 * Restore IPL knowing interrupts are disabled
	 */
	jal	_C_LABEL(splx_noprof)		# splx(ppl)
	 nop

#ifdef PARANOIA
	mfc0	v0, MIPS_COP_0_STATUS
	nop
	or	v0, MIPS_SR_INT_IE
5:	bne	v0, s1, 5b
	 nop
#endif /* PARANOIA */

	/*
	 * Restore SR
	 */
	mtc0	s1, MIPS_COP_0_STATUS

	/*
	 * Restore s0-s2 and goto common kernel return code.
	 */
	REG_L	s0, TF_BASE+TF_REG_S0(sp)
	REG_L	s1, TF_BASE+TF_REG_S1(sp)
	b	MIPSX(kern_return)
	 REG_L	s2, TF_BASE+TF_REG_S2(sp)
	.set	at
END(MIPSX(kern_intr))

/*
 * mipsN_user_gen_exception
 *
 * Handle an exception during user mode.
 * Save user context atop the kernel stack, then call trap() to process
 * the condition.  The context can be manipulated alternatively via
 * curlwp->l_md.md_regs.
 */
NESTED_NOPROFILE(MIPSX(user_gen_exception), CALLFRAME_SIZ, ra)
	.set	noat
	.mask	0x80000000, -4
	/*
	 * Save all the registers except the kernel temporaries onto the stack.
	 */
	PTR_L	k1, CPUVAR(CURLWP)
	nop
	PTR_L	k0, L_PCB(k1)
	nop
	PTR_ADDU k0, USPACE - TF_SIZ - CALLFRAME_SIZ
	REG_S	AT, CALLFRAME_SIZ+TF_REG_AST(k0)
	REG_S	v0, CALLFRAME_SIZ+TF_REG_V0(k0)
	REG_S	v1, CALLFRAME_SIZ+TF_REG_V1(k0)
	mflo	v0
	REG_S	a0, CALLFRAME_SIZ+TF_REG_A0(k0)
	REG_S	a1, CALLFRAME_SIZ+TF_REG_A1(k0)
	REG_S	a2, CALLFRAME_SIZ+TF_REG_A2(k0)
	REG_S	a3, CALLFRAME_SIZ+TF_REG_A3(k0)
	mfhi	v1
	REG_S	t0, CALLFRAME_SIZ+TF_REG_T0(k0)
	REG_S	t1, CALLFRAME_SIZ+TF_REG_T1(k0)
	REG_S	t2, CALLFRAME_SIZ+TF_REG_T2(k0)
	REG_S	t3, CALLFRAME_SIZ+TF_REG_T3(k0)
	mfc0	a0, MIPS_COP_0_STATUS		# 1st arg is STATUS
	REG_S	ta0, CALLFRAME_SIZ+TF_REG_TA0(k0)
	REG_S	ta1, CALLFRAME_SIZ+TF_REG_TA1(k0)
	REG_S	ta2, CALLFRAME_SIZ+TF_REG_TA2(k0)
	REG_S	ta3, CALLFRAME_SIZ+TF_REG_TA3(k0)
	mfc0	a1, MIPS_COP_0_CAUSE		# 2nd arg is CAUSE
	REG_S	s0, CALLFRAME_SIZ+TF_REG_S0(k0)
	REG_S	s1, CALLFRAME_SIZ+TF_REG_S1(k0)
	REG_S	s2, CALLFRAME_SIZ+TF_REG_S2(k0)
	REG_S	s3, CALLFRAME_SIZ+TF_REG_S3(k0)
	_MFC0	a2, MIPS_COP_0_BAD_VADDR	# 3rd arg is fault address
	REG_S	s4, CALLFRAME_SIZ+TF_REG_S4(k0)
	REG_S	s5, CALLFRAME_SIZ+TF_REG_S5(k0)
	REG_S	s6, CALLFRAME_SIZ+TF_REG_S6(k0)
	REG_S	s7, CALLFRAME_SIZ+TF_REG_S7(k0)
	_MFC0	a3, MIPS_COP_0_EXC_PC		# 4th arg is exception PC
	REG_S	t8, CALLFRAME_SIZ+TF_REG_T8(k0)	# will be MIPS_CURLWP
	REG_S	t9, CALLFRAME_SIZ+TF_REG_T9(k0)
	REG_S	gp, CALLFRAME_SIZ+TF_REG_GP(k0)
	REG_S	sp, CALLFRAME_SIZ+TF_REG_SP(k0)
	REG_S	s8, CALLFRAME_SIZ+TF_REG_S8(k0)
	REG_S	ra, CALLFRAME_SIZ+TF_REG_RA(k0)
	REG_S	a0, CALLFRAME_SIZ+TF_REG_SR(k0)
	REG_S	v0, CALLFRAME_SIZ+TF_REG_MULLO(k0)
	REG_S	v1, CALLFRAME_SIZ+TF_REG_MULHI(k0)
	REG_S	a3, CALLFRAME_SIZ+TF_REG_EPC(k0)
#ifdef __GP_SUPPORT__
	PTR_LA	gp, _C_LABEL(_gp)		# switch to kernel GP
#endif
	move	sp, k0				# switch to kernel SP
	move	MIPS_CURLWP, k1
#ifndef NOFPU
	lui	t0, %hi(MIPS_SR_COP_1_BIT)
	and	t0, a0
	beqz	t0, 1f
	 xor	t0, a0				# turn off the FPU
	mtc0	t0, MIPS_COP_0_STATUS
	 nop
1:
#endif
/*
 * Call the trap handler.
 */
	jal	_C_LABEL(trap)
	 REG_S	a3, CALLFRAME_RA(sp)		# for debugging
/*
 * Check pending asynchronous traps.
 */
	INT_L	v0, L_MD_ASTPENDING(MIPS_CURLWP)# any pending ast?
	nop
	beqz	v0, MIPSX(user_return)		# if no, skip ast processing
	 nop
/*
 * We have pending asynchronous traps; all the state is already saved.
 */
	lui	ra, %hi(MIPSX(user_return))	# return directly to user return
	j	_C_LABEL(ast)
	 PTR_ADDIU ra, %lo(MIPSX(user_return))	# return directly to user return
	.set	at
END(MIPSX(user_gen_exception))

/*----------------------------------------------------------------------------
 *
 * mipsN_user_intr
 *
 *	Handle an interrupt from user mode.
 *	We save partial state onto the kernel stack since we know there will
 *	always a kernel stack and chances are we won't need the registers we
 *	don't save.  If there is a pending asynchronous system trap, then save
 *	the remaining state and call ast().
 *
 * Results:
 * 	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------------
 */
NESTED_NOPROFILE(MIPSX(user_intr), CALLFRAME_SIZ, ra)
	.set	noat
	.mask	0x80000000, -4
/*
 * Save the relevant user registers onto the kernel stack.
 * We don't need to save s0 - s8 because the compiler does it for us.
 */
	PTR_L	k1, CPUVAR(CURLWP)
	nop
	PTR_L	k0, L_PCB(k1)				# XXXuvm_lwp_getuarea
	nop
	PTR_ADDU k0, USPACE - TF_SIZ - CALLFRAME_SIZ
	REG_S	AT, CALLFRAME_SIZ+TF_REG_AST(k0)	# $1
	REG_S	v0, CALLFRAME_SIZ+TF_REG_V0(k0)		# $2
	REG_S	v1, CALLFRAME_SIZ+TF_REG_V1(k0)		# $3
	mflo	v0
	REG_S	a0, CALLFRAME_SIZ+TF_REG_A0(k0)		# $4
	REG_S	a1, CALLFRAME_SIZ+TF_REG_A1(k0)		# $5
	REG_S	a2, CALLFRAME_SIZ+TF_REG_A2(k0)		# $6
	REG_S	a3, CALLFRAME_SIZ+TF_REG_A3(k0)		# $7
	mfhi	v1
	REG_S	t0, CALLFRAME_SIZ+TF_REG_T0(k0)		# $8
	REG_S	t1, CALLFRAME_SIZ+TF_REG_T1(k0)		# $9
	REG_S	t2, CALLFRAME_SIZ+TF_REG_T2(k0)		# $10
	REG_S	t3, CALLFRAME_SIZ+TF_REG_T3(k0)		# $11
	mfc0	t0, MIPS_COP_0_CAUSE
	REG_S	ta0, CALLFRAME_SIZ+TF_REG_TA0(k0)	# $12
	REG_S	ta1, CALLFRAME_SIZ+TF_REG_TA1(k0)	# $13
	REG_S	ta2, CALLFRAME_SIZ+TF_REG_TA2(k0)	# $14
	REG_S	ta3, CALLFRAME_SIZ+TF_REG_TA3(k0)	# $15
	REG_S	s0, CALLFRAME_SIZ+TF_REG_S0(k0)		# $16
	REG_S	s1, CALLFRAME_SIZ+TF_REG_S1(k0)		# $17
	mfc0	s1, MIPS_COP_0_STATUS
	REG_S	t8, CALLFRAME_SIZ+TF_REG_T8(k0)		# $24 MIPS_CURLWP
	REG_S	t9, CALLFRAME_SIZ+TF_REG_T9(k0)		# $25
	REG_S	gp, CALLFRAME_SIZ+TF_REG_GP(k0)		# $28
	REG_S	sp, CALLFRAME_SIZ+TF_REG_SP(k0)		# $29
	REG_S	ra, CALLFRAME_SIZ+TF_REG_RA(k0)		# $31
	REG_S	s1, CALLFRAME_SIZ+TF_REG_SR(k0)
	_MFC0	ta0, MIPS_COP_0_EXC_PC
	REG_S	v0, CALLFRAME_SIZ+TF_REG_MULLO(k0)
	REG_S	v1, CALLFRAME_SIZ+TF_REG_MULHI(k0)
	REG_S	ta0, CALLFRAME_SIZ+TF_REG_EPC(k0)
	REG_S	t0, CALLFRAME_SIZ+TF_REG_CAUSE(k0)
	move	sp, k0				# switch to kernel SP
	move	MIPS_CURLWP, k1			# set curlwp reg (t8)
#if defined(DDB) || defined(DEBUG) || defined(KGDB)
	REG_S	ta0, CALLFRAME_RA(sp)		# for debugging
#endif
#ifdef __GP_SUPPORT__
	PTR_LA	gp, _C_LABEL(_gp)		# switch to kernel GP
#endif

	/*
	 * We first need to get to IPL_HIGH so that interrupts are masked.
	 */
	jal	_C_LABEL(splhigh_noprof)	# splhigh()
	 nop
	move	s0, v0				# remember previous priority

	/*
	 * Now that we're at splhigh so all interrupts are masked
	 * individually and we won't get interrupted here, turn the
	 * global interrupt enable bit on again. This will allow
	 * high-priority interrupts to be delivered once a
	 * low-priority interrupt handler lowers spl to execute.
	 *
	 * Also switch off the FPU.
	 */
	mfc0	v1, MIPS_COP_0_STATUS
#ifndef NOFPU
	lui	v0, %hi(MIPS_SR_COP_1_BIT)
	and	v0, v1
	or	v0, MIPS_SR_INT_IE		# make sure intrs are still on
#else
	li	v0, MIPS_SR_INT_IE		# reenable intrs
#endif
	xor	v0, v1
	mtc0	v0, MIPS_COP_0_STATUS
	nop

	/*
	 * Since we interrupted user mode, the new interrupt depth must be 1.
	 */
	PTR_L	t0, L_CPU(MIPS_CURLWP)
	li	t1, 1
	INT_S	t1, CPU_INFO_IDEPTH(t0)		# store new interrupt depth (1)

	/*
	 * Now hard interrupts can be processed.
	 */
	move	a1, ta0				# 2nd arg is exception pc
	move	a2, s1				# 3rd arg is status
	jal	_C_LABEL(cpu_intr)		# cpu_intr(ppl, pc, status)
	 move	a0, s0				# 1st arg is previous pri level

	/*
	 * Interrupt depth is now back to 0.
	 */
	PTR_L	t0, L_CPU(MIPS_CURLWP)
	nop
	INT_S	zero, CPU_INFO_IDEPTH(t0)

#ifdef __HAVE_FAST_SOFTINTS
	/*
	 * This an interrupt from user mode so both softints must be enabled.
	 * No need to check (unless we're being paranoid).
	 */
#ifdef PARANOIA
	and	a0, s1, MIPS_SOFT_INT_MASK	# get softints enabled bits
	xor	a0, MIPS_SOFT_INT_MASK		# invert them.
1:	bnez	a0, 1b				# loop forever if disabled
	 nop
#endif
	mfc0	a0, MIPS_COP_0_CAUSE		# grab the pending softints
	nop					# load delay
	and	a0, MIPS_SOFT_INT_MASK		# are there softints pending
	beqz	a0, 4f				#   nope
	 nop
	jal	_C_LABEL(softint_process)	# softint_process(pending)
	 nop
4:
#endif
	/*
	 * Disable interrupts
	 */
	mfc0	v1, MIPS_COP_0_STATUS
	nop					# delay slot
	and	v0, v1, MIPS_SR_INT_IE		# clear interrupt enable
	xor	v0, v1
	mtc0	v0, MIPS_COP_0_STATUS		# interrupts are disabled

	/*
	 * Restore IPL knowing interrupts are off
	 */
	jal	_C_LABEL(splx_noprof)
	 move	a0, s0				# fetch previous priority level

	/*
	 * Check pending asynchronous traps.
	 */
	REG_L	s0, CALLFRAME_SIZ+TF_REG_S0(sp)	# restore
	REG_L	s1, CALLFRAME_SIZ+TF_REG_S1(sp)	# restore
	INT_L	v0, L_MD_ASTPENDING(MIPS_CURLWP)# any pending ast?
	nop
	beqz	v0, MIPSX(user_intr_return)	# if no, skip ast processing
	 nop

	/*
	 * We have a pending asynchronous trap; save remaining user state into
	 * trapframe.
	 */
	#REG_S	s0, CALLFRAME_SIZ+TF_REG_S0(sp)	# $16 (saved above)
	#REG_S	s1, CALLFRAME_SIZ+TF_REG_S1(sp)	# $17 (saved above)
	REG_S	s2, CALLFRAME_SIZ+TF_REG_S2(sp)	# $18
	REG_S	s3, CALLFRAME_SIZ+TF_REG_S3(sp)	# $19
	REG_S	s4, CALLFRAME_SIZ+TF_REG_S4(sp)	# $20
	REG_S	s5, CALLFRAME_SIZ+TF_REG_S5(sp)	# $21
	REG_S	s6, CALLFRAME_SIZ+TF_REG_S6(sp)	# $22
	REG_S	s7, CALLFRAME_SIZ+TF_REG_S7(sp)	# $23
	REG_S	s8, CALLFRAME_SIZ+TF_REG_S8(sp)	# $30

	mfc0	t0, MIPS_COP_0_STATUS
	PTR_LA	ra, MIPSX(user_return)		# load delay
	or	t0, MIPS_SR_INT_IE		# enable interrupts
	j	_C_LABEL(ast)			# ast()
	 mtc0	t0, MIPS_COP_0_STATUS		# enable interrupts (spl0)
	.set	at
END(MIPSX(user_intr))

/*
 * mipsN_systemcall
 *
 * Save user context atop of kernel stack, then call syscall() to process
 * a system call.  The context can be manipulated alternatively via
 * curlwp->l_md.md_utf->tf_regs.
 */
NESTED_NOPROFILE(MIPSX(systemcall), CALLFRAME_SIZ, ra)
	.set	noat
	.mask	0x80000000, -4
	/*
	 * Save all the registers but kernel temporaries onto the stack.
	 */
	PTR_L	k1, CPUVAR(CURLWP)
	nop
	PTR_L	k0, L_PCB(k1)
	nop
	PTR_ADDU k0, USPACE - TF_SIZ - CALLFRAME_SIZ
	#REG_S	AT, CALLFRAME_SIZ+TF_REG_AST(k0)
	#.set	at
	REG_S	v0, CALLFRAME_SIZ+TF_REG_V0(k0)		# syscall #
	REG_S	v1, CALLFRAME_SIZ+TF_REG_V1(k0)		# used by syscall()
	mflo	v0
	REG_S	a0, CALLFRAME_SIZ+TF_REG_A0(k0)
	REG_S	a1, CALLFRAME_SIZ+TF_REG_A1(k0)
	REG_S	a2, CALLFRAME_SIZ+TF_REG_A2(k0)
	REG_S	a3, CALLFRAME_SIZ+TF_REG_A3(k0)
	move	a0, k1					# 1st arg is curlwp
	mfhi	v1
	mfc0	a1, MIPS_COP_0_STATUS			# 2nd arg is STATUS
	REG_S	s0, CALLFRAME_SIZ+TF_REG_S0(k0)
	REG_S	s1, CALLFRAME_SIZ+TF_REG_S1(k0)
	REG_S	s2, CALLFRAME_SIZ+TF_REG_S2(k0)
	REG_S	s3, CALLFRAME_SIZ+TF_REG_S3(k0)
	mfc0	a2, MIPS_COP_0_CAUSE			# 3rd arg is CAUSE
	REG_S	s4, CALLFRAME_SIZ+TF_REG_S4(k0)
	REG_S	s5, CALLFRAME_SIZ+TF_REG_S5(k0)
	REG_S	s6, CALLFRAME_SIZ+TF_REG_S6(k0)
	REG_S	s7, CALLFRAME_SIZ+TF_REG_S7(k0)
	_MFC0	a3, MIPS_COP_0_EXC_PC			# 4th arg is PC
	REG_S	t0, CALLFRAME_SIZ+TF_REG_T0(k0)
	REG_S	t1, CALLFRAME_SIZ+TF_REG_T1(k0)
	REG_S	t2, CALLFRAME_SIZ+TF_REG_T2(k0)
	REG_S	t3, CALLFRAME_SIZ+TF_REG_T3(k0)		# syscall saved gp for fork
	REG_S	ta0, CALLFRAME_SIZ+TF_REG_TA0(k0)
	REG_S	ta1, CALLFRAME_SIZ+TF_REG_TA1(k0)
	REG_S	ta2, CALLFRAME_SIZ+TF_REG_TA2(k0)
	REG_S	ta3, CALLFRAME_SIZ+TF_REG_TA3(k0)
	REG_S	t8, CALLFRAME_SIZ+TF_REG_T8(k0)		# will be MIPS_CURLWP
	REG_S	t9, CALLFRAME_SIZ+TF_REG_T9(k0)
	REG_S	gp, CALLFRAME_SIZ+TF_REG_GP(k0)
	REG_S	sp, CALLFRAME_SIZ+TF_REG_SP(k0)
	REG_S	s8, CALLFRAME_SIZ+TF_REG_S8(k0)
	REG_S	ra, CALLFRAME_SIZ+TF_REG_RA(k0)
	REG_S	a1, CALLFRAME_SIZ+TF_REG_SR(k0)
	REG_S	v0, CALLFRAME_SIZ+TF_REG_MULLO(k0)
	REG_S	v1, CALLFRAME_SIZ+TF_REG_MULHI(k0)
	REG_S	a3, CALLFRAME_SIZ+TF_REG_EPC(k0)
	PTR_L	t0, L_PROC(a0)			# curlwp->l_proc (used below)
	move	sp, k0				# switch to kernel SP
	move	MIPS_CURLWP, a0			# set curlwp reg
#ifdef __GP_SUPPORT__
	PTR_LA	gp, _C_LABEL(_gp)		# switch to kernel GP
#endif
#if defined(DDB) || defined(DEBUG) || defined(KGDB)
	move	ra, a3
	REG_S	ra, CALLFRAME_RA(sp)
#endif
	PTR_L	t9, P_MD_SYSCALL(t0)		# t9 = syscall
	/*
	 * Turn off FPU
	 */
#ifdef NOFPU
	li	t0, MIPS_SR_INT_IE
#else
	lui	t0, %hi(MIPS_SR_COP_1_BIT)
	and	t0, a1
	ori	t0, MIPS_SR_INT_IE		# turn on IEc, enable intr.
#endif
	xor	t0, a1				# turns off the FPU & ints on
	mtc0	t0, MIPS_COP_0_STATUS		# re-enable interrupts
/*
 * Call the system call handler.
 */
	jalr	t9
	 nop
/*
 * Check pending asynchronous traps.
 */
	INT_L	v0, L_MD_ASTPENDING(MIPS_CURLWP)# any pending ast?
	nop
	beqz	v0, MIPSX(user_return)		# no, skip ast processing
	 nop
/*
 * We have pending asynchronous traps; all the state is already saved.
 */
	lui	ra, %hi(MIPSX(user_return))	# return directly to user return
	j	_C_LABEL(ast)
	 PTR_ADDIU ra, %lo(MIPSX(user_return))	# return directly to user return
	.set	at
END(MIPSX(systemcall))

/*----------------------------------------------------------------------------
 *
 *	R3000 TLB exception handlers
 *
 *----------------------------------------------------------------------------
 */

/*----------------------------------------------------------------------------
 *
 * mipsN_kern_tlb_miss --
 *
 *	Handle a TLB miss exception from kernel mode in kernel space.
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
LEAF_NOPROFILE(MIPSX(kern_tlb_miss))
	.set	noat
	_MFC0	k0, MIPS_COP_0_BAD_VADDR	# get the fault address
	PTR_LA	k1, _C_LABEL(pmap_kern_segtab)	# get address of kernel segtab
	PTR_SRL	k0, SEGSHIFT - PTR_SCALESHIFT	# get segtab index (part1)
	and	k0, (NSEGPG-1) << PTR_SCALESHIFT # get segtab index (part2)
	PTR_ADDU k1, k0				# add index to segtab addr
	PTR_L	k1, 0(k1)			# load address of PTP
	_MFC0	k0, MIPS_COP_0_BAD_VADDR	# get the fault address
	/*
	 * If there isn't a PTP for this, let trap panic for us.
	 */
	beqz	k1, _C_LABEL(MIPSX(kern_gen_exception))	# full trap processing
	 PTR_SRL k0, PGSHIFT - PTPSHIFT		# - delay slot -
	and	k0, (NPTEPG-1) << PTPSHIFT	# get ptp index (part2)
	PTR_ADDU k1, k0				# add to PTP address
	INT_L	k0, 0(k1)			# get PTE entry
	_MFC0	k1, MIPS_COP_0_EXC_PC		# get return address
	mtc0	k0, MIPS_COP_0_TLB_LOW		# save PTE entry
	and	k0, MIPS1_PG_V			# check for valid PTE entry
	beqz	k0, _C_LABEL(MIPSX(kern_gen_exception)) # PTE invalid
	 nop
	tlbwr					# write random TLB
	j	k1
	 rfe
	.set	at
END(MIPSX(kern_tlb_miss))

#if 0
/*----------------------------------------------------------------------------
 *
 * mipsN_tlb_invalid_exception --
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
LEAF_NOPROFILE(MIPSX(tlb_mod_exception))
	.set	noat
	tlbp					# find the TLB entry
	mfc0	k0, MIPS_COP_0_TLB_LOW		# get the physical address
	mfc0	k1, MIPS_COP_0_TLB_INDEX	# check to be sure its valid
	or	k0, k0, MIPS1_TLB_DIRTY_BIT	# update TLB
	blt	k1, zero, 4f			# not found!!!
	mtc0	k0, MIPS_COP_0_TLB_LOW
	li	k1, MIPS_KSEG0_START
	PTR_SUBU k0, k1
	srl	k0, k0, MIPS1_TLB_PHYS_PAGE_SHIFT
	PTR_L	k1, pmap_attributes		# DANGER!  DANGER!
	PTR_ADDU k0, k1
	lbu	k1, 0(k0)			# fetch old value
	nop
	or	k1, k1, 1			# set modified bit
	sb	k1, 0(k0)			# save new value
	_MFC0	k0, MIPS_COP_0_EXC_PC		# get return address
	nop
	j	k0
	rfe
4:
	break	0				# panic
	.set	at
END(MIPSX(tlb_mod_exception))
#endif

/*
 * Mark where code entered from exception handler jumptable
 * ends, for stack traceback code.
 */

	.globl	_C_LABEL(MIPSX(exceptionentry_end))
_C_LABEL(MIPSX(exceptionentry_end)):

/*--------------------------------------------------------------------------
 *
 * mipsN_tlb_get_asid --
 *
 *	Return the pid from the TLB pid reg.
 *
 *	tlb_asid_t mipsN_tlb_get_asid(void)
 *
 * Results:
 *	The current ASID.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------------------
 */
LEAF(MIPSX(tlb_get_asid))
	mfc0	v0, MIPS_COP_0_TLB_HI		# Read the hi reg value
	nop
	and	v0, MIPS1_TLB_PID		# mask out only the PID
	j	ra
	 srl	v0, MIPS1_TLB_PID_SHIFT		# put PID in right spot
END(MIPSX(tlb_get_asid))

/*--------------------------------------------------------------------------
 *
 * mipsN_tlb_set_asid --
 *
 *	Write the given pid into the TLB pid reg.
 *
 *	void mipsN_tlb_set_asid(tlb_asid_t pid)
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	PID set in the entry hi register.
 *
 *--------------------------------------------------------------------------
 */
LEAF(MIPSX(tlb_set_asid))
	sll	a0, MIPS1_TLB_PID_SHIFT		# put PID in right spot
	and	a0, MIPS1_TLB_PID
	mtc0	a0, MIPS_COP_0_TLB_HI		# Write the hi reg value
	j	ra
	nop
END(MIPSX(tlb_set_asid))

/*--------------------------------------------------------------------------
 *
 * mipsN_tlb_update_addr --
 *
 *	Update the TLB if highreg is found; otherwise, do_nothing
 *
 *	bool mipsN_tlb_update_addr(vaddr_t va, tlb_asid_t asid,
 *	    pt_entry_t pte, bool insert);
 *
 * Results:
 *	0 if skipped, 1 if updated
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------------------
 */
LEAF(MIPSX(tlb_update_addr))
	mfc0	ta0, MIPS_COP_0_STATUS		# save the status register
	mtc0	zero, MIPS_COP_0_STATUS		# disable interrupts
	nop
	mfc0	ta1, MIPS_COP_0_TLB_HI		# save current PID
	nop
	sll	a1, MIPS1_TLB_PID_SHIFT
	or	a0, a1
	mtc0	a0, MIPS_COP_0_TLB_HI		# set entryhi
	nop
	tlbp					# probe the existence
	mfc0	v0, MIPS_COP_0_TLB_INDEX	# see what we got
	mtc0	a2, MIPS_COP_0_TLB_LOW		# set new entrylo
	bltz	v0, 2f				# index < 0 => !found
	 nop
	tlbwi					# update slot found
	b	3f				# return
	 li	v0, 1				#   and show success
2:
	beqz	a2, 3f				# return
	 li	v0, 0				#   and show failure
	tlbwr					# put it in a new slot
	li	v0, 1				# show success
3:
	mtc0	ta1, MIPS_COP_0_TLB_HI		# restore current PID
	j	ra
	 mtc0	ta0, MIPS_COP_0_STATUS		# restore interrupts
END(MIPSX(tlb_update_addr))

/*--------------------------------------------------------------------------
 *
 * mipsN_tlb_read_entry --
 *
 *	Read the TLB entry.
 *
 *	void mipsN_tlb_read_entry(register_t entry, struct tlbmask *tlb)
 *
 * Results:
 *	tlb will contain the TLB entry found (tlb_lo1/tlb_mask will be 0).
 *
 *--------------------------------------------------------------------------
 */
LEAF(MIPSX(tlb_read_entry))
	mfc0	ta0, MIPS_COP_0_STATUS		# Save the status register.
	mtc0	zero, MIPS_COP_0_STATUS		# Disable interrupts
	mfc0	ta1, MIPS_COP_0_TLB_HI		# Get current PID

	sll	a0, MIPS1_TLB_INDEX_SHIFT
	mtc0	a0, MIPS_COP_0_TLB_INDEX	# Set the index register
	nop
	tlbr					# Read from the TLB
	mfc0	t2, MIPS_COP_0_TLB_HI		# fetch the hi entry
	mfc0	t3, MIPS_COP_0_TLB_LOW		# fetch the low entry

	mtc0	ta1, MIPS_COP_0_TLB_HI		# Restore proper PID
						# (before touching memory)
	mtc0	ta0, MIPS_COP_0_STATUS		# Restore the status register

	PTR_S	t2, TLBMASK_HI(a1)
	REG_S	t3, TLBMASK_LO0(a1)
	REG_S	zero, TLBMASK_LO1(a1)
	j	ra
	 INT_S	zero, TLBMASK_MASK(a1)
END(MIPSX(tlb_read_entry))

/*--------------------------------------------------------------------------
 *
 * mipsX_tlb_write_entry --
 *
 *	Write the TLB entry.
 *
 *	void mipsX_tlb_write_entry(size_t entry, struct tlbmask *tlb)
 *
 * Results:
 *	None.
 *
 *--------------------------------------------------------------------------
 */
LEAF(MIPSX(tlb_write_entry))
	PTR_L	t2, TLBMASK_HI(a1)		# fetch the hi entry
	INT_L	t3, TLBMASK_LO0(a1)		# fetch the low entry
	mfc0	ta0, MIPS_COP_0_STATUS		# Save the status register.
	mtc0	zero, MIPS_COP_0_STATUS		# Disable interrupts
	mfc0	ta1, MIPS_COP_0_TLB_HI		# Get current PID

	sll	a0, MIPS1_TLB_INDEX_SHIFT
	mtc0	a0, MIPS_COP_0_TLB_INDEX	# Set the index register
	nop
	mtc0	t2, MIPS_COP_0_TLB_HI
	mtc0	t3, MIPS_COP_0_TLB_LOW

	tlbwi					# Write to the TLB entry

	mtc0	ta1, MIPS_COP_0_TLB_HI		# restore PID
	j	ra
	 mtc0	ta0, MIPS_COP_0_STATUS		# Restore the status register
END(MIPSX(tlb_write_entry))

/*
 * void mipsN_tlb_invalidate_addr(vaddr_t va, tlb_asid_t asid)
 *
 * Invalidate a TLB entry for given virtual address if found in TLB.
 */
LEAF(MIPSX(tlb_invalidate_addr))
	mfc0	ta0, MIPS_COP_0_STATUS		# save status register
	mtc0	zero, MIPS_COP_0_STATUS		# disable interrupts
	mfc0	ta1, MIPS_COP_0_TLB_HI		# save current PID
	nop

	sll	a1, MIPS1_TLB_PID_SHIFT		# move ASID into position
	and	a1, MIPS1_TLB_PID		# make it off
	or	a0, a1				# merge with addr
	mtc0	a0, MIPS_COP_0_TLB_HI		# look for addr & PID
	nop
	tlbp					# probe the entry in question
	mfc0	a0, MIPS_COP_0_TLB_INDEX	# see what we got
	li	t1, MIPS_KSEG0_START		# load invalid address
	bltz	a0, 1f				# index < 0 then skip
	 nop
	mtc0	t1, MIPS_COP_0_TLB_HI		# make entryHi invalid
	mtc0	zero, MIPS_COP_0_TLB_LOW	# zero out entryLo
	nop
	tlbwi
1:
	mtc0	ta1, MIPS_COP_0_TLB_HI		# restore PID
	j	ra
	 mtc0	ta0, MIPS_COP_0_STATUS		# restore the status register
END(MIPSX(tlb_invalidate_addr))

/*
 * void mipsN_tlb_invalidate_asids(uint32_t asid_lo, uint32_t asid_hi)
 *
 * Invalidate TLB entries belonging to asids (asid_lo,asid_hi]
 * leaving entries for kernel space marked global intact.
 */
LEAF(MIPSX(tlb_invalidate_asids))
	mfc0	ta1, MIPS_COP_0_TLB_HI		# save EntryHi
	mfc0	ta0, MIPS_COP_0_STATUS		# save status register
	mtc0	zero, MIPS_COP_0_STATUS		# disable interrupts

	INT_L	t2, _C_LABEL(mips_options) + MO_NUM_TLB_ENTRIES
	li	t1, MIPS1_TLB_FIRST_RAND_ENTRY << MIPS1_TLB_INDEX_SHIFT
	li	v0, MIPS_KSEG0_START		# invalid address
	sll	t2, MIPS1_TLB_INDEX_SHIFT

	# do {} while (t1 < t2)
1:
	mtc0	t1, MIPS_COP_0_TLB_INDEX	# set index
	nop
	tlbr					# obtain an entry
	mfc0	t0, MIPS_COP_0_TLB_LOW
	nop
	and	t0, t0, MIPS1_PG_G		# check to see it has G bit
	bnez	t0, 2f
	 nop

	mfc0	t0, MIPS_COP_0_TLB_HI		# get va and ASID
	nop
	and	t0, MIPS1_TLB_PID 		# mask off ASID
	srl	t0, MIPS1_TLB_PID_SHIFT
	sltu	v1, t0, a0			# < asid_lo
	bnez	v1, 2f				# yes, next tlb entry
	 nop
	sltu	v1, t0, a1			# < asid_hi
	beqz	v1, 2f				# no, next tlb entry
	 nop

	mtc0	v0, MIPS_COP_0_TLB_HI		# make entryHi invalid
	mtc0	zero, MIPS_COP_0_TLB_LOW	# zero out entryLo
	nop
	tlbwi					# invalidate the TLB entry
2:
	addu	t1, t1, 1 << MIPS1_TLB_INDEX_SHIFT	# increment index
	bne	t1, t2, 1b
	nop

	mtc0	ta1, MIPS_COP_0_TLB_HI		# restore entryHi

	j	ra				# new TLBpid will be set soon
	 mtc0	ta0, MIPS_COP_0_STATUS		# restore status register
END(MIPSX(tlb_invalidate_asids))

/*
 * void mipsN_tlb_invalidate_all(void)
 *
 * Invalidate TLB entirely.
 */
LEAF(MIPSX(tlb_invalidate_all))
	INT_L	a0, _C_LABEL(mips_options) + MO_NUM_TLB_ENTRIES

	mfc0	ta0, MIPS_COP_0_STATUS		# save the status register.
	mtc0	zero, MIPS_COP_0_STATUS		# disable interrupts

	mfc0	ta1, MIPS_COP_0_TLB_HI		# save current PID
	li	t0, MIPS_KSEG0_START		# invalid address
	mtc0	t0, MIPS_COP_0_TLB_HI		# make entryHi invalid
	mtc0	zero, MIPS_COP_0_TLB_LOW	# zero out entryLo

	move	t0, zero
	sll	a0, MIPS1_TLB_INDEX_SHIFT

	# do {} while (t1 < a0)
1:
	mtc0	t0, MIPS_COP_0_TLB_INDEX	# set TLBindex
	addu	t0, t0, 1 << MIPS1_TLB_INDEX_SHIFT	# increment index
	bne	t0, a0, 1b
	 tlbwi					# invalidate the entry

	mtc0	ta1, MIPS_COP_0_TLB_HI		# restore PID
	j	ra
	 mtc0	ta0, MIPS_COP_0_STATUS		# restore status register
END(MIPSX(tlb_invalidate_all))

/*
 * u_int mipsN_tlb_record_asids(u_long *bitmap, uint32_t asid_max)
 *
 * Scan the random part of the TLB looking at non-global entries and
 * record each ASID in use into the bitmap.  Additionally, return the
 * number of new unique ASIDs encountered.
 */
LEAF(MIPSX(tlb_record_asids))
	mfc0	ta1, MIPS_COP_0_TLB_HI		# save EntryHi
	li	v1, MIPS1_TLB_FIRST_RAND_ENTRY << MIPS1_TLB_INDEX_SHIFT
	INT_L	a3, _C_LABEL(mips_options) + MO_NUM_TLB_ENTRIES
	move	ta2, zero
	li	ta3, 1
	sll	a3, MIPS1_TLB_INDEX_SHIFT

	mfc0	ta0, MIPS_COP_0_STATUS		# save status register
	mtc0	zero, MIPS_COP_0_STATUS		# disable interrupts

	move	v0, zero			# start at zero ASIDs

	# do {} while (v1 < ta1)
1:
	mtc0	v1, MIPS_COP_0_TLB_INDEX	# set index
	nop
	tlbr					# obtain an entry
	mfc0	t0, MIPS_COP_0_TLB_LOW
	nop
	and	t0, MIPS1_PG_G			# check to see it has G bit
	bnez	t0, 4f
	 nop

	mfc0	t0, MIPS_COP_0_TLB_HI		# get va and ASID
	nop
	and	t0, MIPS1_TLB_PID
	srl	t0, MIPS1_TLB_PID_SHIFT		# shift to low bits
	bgt	t0, a1, 4f			# > ASID max? skip
	 nop

	srl	a2, t0, 3 + LONG_SCALESHIFT	# drop low 5 bits
	sll	a2, LONG_SCALESHIFT		# make an index for the bitmap
	sllv	t0, ta3, t0			# t0 is mask (ta3 == 1)

	PTR_ADDU a2, a0				# index into the bitmap
	beq	a2, ta2, 3f			# is the desired cell loaded?
	 nop					#   yes, don't reload it
	beqz	ta2, 2f				# have we ever loaded it?
	 nop					#   nope, so don't save it

	LONG_S	t2, 0(ta2)			# save the updated value.
2:
	LONG_L	t2, 0(a2)			# and load it
	 move	ta2, a2				# remember the new cell's addr
3:
	and	t1, t2, t0			# see if this asid was recorded
	sltu	t1, t1, ta3			# t1 = t1 < 1 (aka t1 == 0)
	addu	v0, t1				# v0 += t1
	or	t2, t0				# or in the new ASID bits

4:
	addu	v1, 1 << MIPS1_TLB_INDEX_SHIFT	# increment TLB entry #
	bne	v1, a3, 1b			# keep lookup if not limit
	 nop

	beqz	ta2, 5f				# do we have a cell to write?
	 nop					#   nope, nothing.

	LONG_S	t2, 0(ta2)			# save the updated value.
5:
	mtc0	ta1, MIPS_COP_0_TLB_HI		# restore entryHi

	j	ra				# new TLBpid will be set soon
	 mtc0	ta0, MIPS_COP_0_STATUS		# restore status register
END(MIPSX(tlb_record_asids))

/*----------------------------------------------------------------------------
 *
 *	R3000 trampolines and context resume
 *
 *----------------------------------------------------------------------------
 */

/*----------------------------------------------------------------------------
 *
 * mipsN_lwp_trampoline
 *
 * Special arrangement for a process about to go user mode right after
 * fork() system call.  When the first CPU tick is scheduled to run the
 * forked child, it starts running from here.  Then, a service function
 * is called with one argument supplied to complete final preparations,
 * and the process returns to user mode as if the fork() system call is
 * handled in a normal way.  No need to save any registers although this
 * calls another.
 *----------------------------------------------------------------------------
 */
LEAF(MIPSX(lwp_trampoline))
	PTR_ADDU sp, -CALLFRAME_SIZ

	# Call lwp_startup(), with args from cpu_switchto()/cpu_lwp_fork()
	move	a0, v0
	jal	_C_LABEL(lwp_startup)
	 move	a1, MIPS_CURLWP

	# Call the routine specified by cpu_lwp_fork()
	jalr	s0
	 move	a0, s1

	# Return to user (won't happen if a kernel thread)
	.set	noat
MIPSX(user_return):
	REG_L	s0, CALLFRAME_SIZ+TF_REG_S0(sp)		# $16
	REG_L	s1, CALLFRAME_SIZ+TF_REG_S1(sp)		# $17
	REG_L	s2, CALLFRAME_SIZ+TF_REG_S2(sp)		# $18
	REG_L	s3, CALLFRAME_SIZ+TF_REG_S3(sp)		# $19
	REG_L	s4, CALLFRAME_SIZ+TF_REG_S4(sp)		# $20
	REG_L	s5, CALLFRAME_SIZ+TF_REG_S5(sp)		# $21
	REG_L	s6, CALLFRAME_SIZ+TF_REG_S6(sp)		# $22
	REG_L	s7, CALLFRAME_SIZ+TF_REG_S7(sp)		# $23
	REG_L	s8, CALLFRAME_SIZ+TF_REG_S8(sp)		# $30
MIPSX(user_intr_return):
	REG_L	a0, CALLFRAME_SIZ+TF_REG_SR(sp)
	REG_L	t0, CALLFRAME_SIZ+TF_REG_MULLO(sp)
	REG_L	t1, CALLFRAME_SIZ+TF_REG_MULHI(sp)
	mtc0	a0, MIPS_COP_0_STATUS	# this should disable interrupts
	mtlo	t0
	mthi	t1
	move	k1, sp
	REG_L	AT, TF_BASE+TF_REG_AST(sp)
	REG_L	k0, CALLFRAME_SIZ+TF_REG_EPC(k1)
	REG_L	AT, CALLFRAME_SIZ+TF_REG_AST(k1)
	REG_L	v0, CALLFRAME_SIZ+TF_REG_V0(k1)
	REG_L	v1, CALLFRAME_SIZ+TF_REG_V1(k1)
	REG_L	a0, CALLFRAME_SIZ+TF_REG_A0(k1)
	REG_L	a1, CALLFRAME_SIZ+TF_REG_A1(k1)
	REG_L	a2, CALLFRAME_SIZ+TF_REG_A2(k1)
	REG_L	a3, CALLFRAME_SIZ+TF_REG_A3(k1)
	REG_L	t0, CALLFRAME_SIZ+TF_REG_T0(k1)
	REG_L	t1, CALLFRAME_SIZ+TF_REG_T1(k1)
	REG_L	t2, CALLFRAME_SIZ+TF_REG_T2(k1)
	REG_L	t3, CALLFRAME_SIZ+TF_REG_T3(k1)
	REG_L	ta0, CALLFRAME_SIZ+TF_REG_TA0(k1)
	REG_L	ta1, CALLFRAME_SIZ+TF_REG_TA1(k1)
	REG_L	ta2, CALLFRAME_SIZ+TF_REG_TA2(k1)
	REG_L	ta3, CALLFRAME_SIZ+TF_REG_TA3(k1)
	REG_L	t8, CALLFRAME_SIZ+TF_REG_T8(k1)
	REG_L	t9, CALLFRAME_SIZ+TF_REG_T9(k1)
	REG_L	gp, CALLFRAME_SIZ+TF_REG_GP(k1)
	REG_L	ra, CALLFRAME_SIZ+TF_REG_RA(k1)
	REG_L	sp, CALLFRAME_SIZ+TF_REG_SP(k1)
	nop
	j	k0
	rfe
	.set	at
END(MIPSX(lwp_trampoline))

/*
 * void mipsN_cpu_switch_resume(struct lwp *newlwp)
 *
 * Wiredown the USPACE of newproc with TLB entry#0 and #1.  Check
 * if target USPACE is already referred by any TLB entry before
 * doing that, and make sure TBIS(them) in the case.
 */
LEAF_NOPROFILE(MIPSX(cpu_switch_resume))
	INT_L	a1, L_MD_UPTE_0(a0)		# a1 = upte[0]
	INT_L	a2, L_MD_UPTE_1(a0)		# a2 = upte[1]
	PTR_L	s0, L_PCB(a0)			# va = l->l_addr
	li	s2, VM_MIN_KERNEL_ADDRESS
	blt	s0, s2, resume
	nop

	mfc0	t3, MIPS_COP_0_TLB_HI		# save PID
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
	nop
	mfc0	t3, MIPS_COP_0_TLB_HI		# restore PID

resume:
	j	ra
	nop
END(MIPSX(cpu_switch_resume))

/*----------------------------------------------------------------------------
 *
 *	R3000 cache sizing and flushing code.
 *
 *----------------------------------------------------------------------------
 */
#ifndef ENABLE_MIPS_TX3900
/*
 * void mipsN_wbflush(void)
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
LEAF(MIPSX(wbflush))
	nop
	nop
	nop
	nop
1:	bc0f	1b
	nop
	j	ra
	nop
END(MIPSX(wbflush))
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

LEAF(MIPSX(wbflush))
	.set push
	.set mips2
	sync
	.set pop
	j	ra
	nop
END(MIPSX(wbflush))
#endif /* !ENABLE_MIPS_TX3900 */

	.rdata

	.globl _C_LABEL(MIPSX(locore_vec))
_C_LABEL(MIPSX(locore_vec)):
	PTR_WORD _C_LABEL(MIPSX(cpu_switch_resume))
	PTR_WORD _C_LABEL(MIPSX(lwp_trampoline))
	PTR_WORD _C_LABEL(MIPSX(wbflush))		# wbflush
	PTR_WORD _C_LABEL(MIPSX(tlb_get_asid))
	PTR_WORD _C_LABEL(MIPSX(tlb_set_asid))
	PTR_WORD _C_LABEL(MIPSX(tlb_invalidate_asids))
	PTR_WORD _C_LABEL(MIPSX(tlb_invalidate_addr))
	PTR_WORD _C_LABEL(nullop)			# tlb_invalidate_globals
	PTR_WORD _C_LABEL(MIPSX(tlb_invalidate_all))
	PTR_WORD _C_LABEL(MIPSX(tlb_record_asids))
	PTR_WORD _C_LABEL(MIPSX(tlb_update_addr))
	PTR_WORD _C_LABEL(MIPSX(tlb_read_entry))
	PTR_WORD _C_LABEL(MIPSX(tlb_write_entry))

	.globl _C_LABEL(MIPSX(locoresw))
_C_LABEL(MIPSX(locoresw)):
	PTR_WORD _C_LABEL(MIPSX(wbflush))		# lsw_wbflush
	PTR_WORD _C_LABEL(nullop)			# lsw_cpu_idle
	PTR_WORD _C_LABEL(nullop)			# lsw_send_ipi
	PTR_WORD _C_LABEL(nullop)			# lsw_cpu_offline_md
	PTR_WORD _C_LABEL(nullop)			# lsw_cpu_init
	PTR_WORD _C_LABEL(nullop)			# lsw_cpu_run
	PTR_WORD _C_LABEL(nullop)			# lsw_bus_error

MIPSX(excpt_sw):
	####
	#### The kernel exception handlers.
	####
	PTR_WORD _C_LABEL(MIPSX(kern_intr))	# 0 external interrupt
	PTR_WORD _C_LABEL(MIPSX(kern_gen_exception))# 1 TLB modification
	PTR_WORD _C_LABEL(MIPSX(kern_tlb_miss))	# 2 TLB miss (LW/I-fetch)
	PTR_WORD _C_LABEL(MIPSX(kern_tlb_miss))	# 3 TLB miss (SW)
	PTR_WORD _C_LABEL(MIPSX(kern_gen_exception))# 4 address error (LW/I-fetch)
	PTR_WORD _C_LABEL(MIPSX(kern_gen_exception))# 5 address error (SW)
	PTR_WORD _C_LABEL(MIPSX(kern_gen_exception))# 6 bus error (I-fetch)
	PTR_WORD _C_LABEL(MIPSX(kern_gen_exception))# 7 bus error (load or store)
	PTR_WORD _C_LABEL(MIPSX(kern_gen_exception))# 8 system call
	PTR_WORD _C_LABEL(MIPSX(kern_gen_exception))# 9 breakpoint
	PTR_WORD _C_LABEL(MIPSX(kern_gen_exception))# 10 reserved instruction
	PTR_WORD _C_LABEL(MIPSX(kern_gen_exception))# 11 coprocessor unusable
	PTR_WORD _C_LABEL(MIPSX(kern_gen_exception))# 12 arithmetic overflow
	PTR_WORD _C_LABEL(MIPSX(kern_gen_exception))# 13 r3k reserved
	PTR_WORD _C_LABEL(MIPSX(kern_gen_exception))# 14 r3k reserved
	PTR_WORD _C_LABEL(MIPSX(kern_gen_exception))# 15 r3k reserved
	PTR_WORD _C_LABEL(MIPSX(kern_gen_exception))# 16 never happens w/ MIPS1
	PTR_WORD _C_LABEL(MIPSX(kern_gen_exception))# 17 never happens w/ MIPS1
	PTR_WORD _C_LABEL(MIPSX(kern_gen_exception))# 18 never happens w/ MIPS1
	PTR_WORD _C_LABEL(MIPSX(kern_gen_exception))# 19 never happens w/ MIPS1
	PTR_WORD _C_LABEL(MIPSX(kern_gen_exception))# 20 never happens w/ MIPS1
	PTR_WORD _C_LABEL(MIPSX(kern_gen_exception))# 21 never happens w/ MIPS1
	PTR_WORD _C_LABEL(MIPSX(kern_gen_exception))# 22 never happens w/ MIPS1
	PTR_WORD _C_LABEL(MIPSX(kern_gen_exception))# 23 never happens w/ MIPS1
	PTR_WORD _C_LABEL(MIPSX(kern_gen_exception))# 24 never happens w/ MIPS1
	PTR_WORD _C_LABEL(MIPSX(kern_gen_exception))# 25 never happens w/ MIPS1
	PTR_WORD _C_LABEL(MIPSX(kern_gen_exception))# 26 never happens w/ MIPS1
	PTR_WORD _C_LABEL(MIPSX(kern_gen_exception))# 27 never happens w/ MIPS1
	PTR_WORD _C_LABEL(MIPSX(kern_gen_exception))# 28 never happens w/ MIPS1
	PTR_WORD _C_LABEL(MIPSX(kern_gen_exception))# 29 never happens w/ MIPS1
	PTR_WORD _C_LABEL(MIPSX(kern_gen_exception))# 30 never happens w/ MIPS1
	PTR_WORD _C_LABEL(MIPSX(kern_gen_exception))# 31 never happens w/ MIPS1
	#####
	##### The user exception handlers.
	#####
	PTR_WORD _C_LABEL(MIPSX(user_intr))	#  0
	PTR_WORD _C_LABEL(MIPSX(user_gen_exception))#  1
	PTR_WORD _C_LABEL(MIPSX(user_gen_exception))#  2
	PTR_WORD _C_LABEL(MIPSX(user_gen_exception))#  3
	PTR_WORD _C_LABEL(MIPSX(user_gen_exception))#  4
	PTR_WORD _C_LABEL(MIPSX(user_gen_exception))#  5
	PTR_WORD _C_LABEL(MIPSX(user_gen_exception))#  6
	PTR_WORD _C_LABEL(MIPSX(user_gen_exception))#  7
	PTR_WORD _C_LABEL(MIPSX(systemcall))	#  8
	PTR_WORD _C_LABEL(MIPSX(user_gen_exception))#  9
	PTR_WORD _C_LABEL(MIPSX(user_gen_exception))# 10
	PTR_WORD _C_LABEL(MIPSX(user_gen_exception))# 11
	PTR_WORD _C_LABEL(MIPSX(user_gen_exception))# 12
	PTR_WORD _C_LABEL(MIPSX(user_gen_exception))# 13
	PTR_WORD _C_LABEL(MIPSX(user_gen_exception))# 14
	PTR_WORD _C_LABEL(MIPSX(user_gen_exception))# 15
	PTR_WORD _C_LABEL(MIPSX(user_gen_exception))# 16
	PTR_WORD _C_LABEL(MIPSX(user_gen_exception))# 17
	PTR_WORD _C_LABEL(MIPSX(user_gen_exception))# 18
	PTR_WORD _C_LABEL(MIPSX(user_gen_exception))# 19
	PTR_WORD _C_LABEL(MIPSX(user_gen_exception))# 20
	PTR_WORD _C_LABEL(MIPSX(user_gen_exception))# 21
	PTR_WORD _C_LABEL(MIPSX(user_gen_exception))# 22
	PTR_WORD _C_LABEL(MIPSX(user_gen_exception))# 23
	PTR_WORD _C_LABEL(MIPSX(user_gen_exception))# 24
	PTR_WORD _C_LABEL(MIPSX(user_gen_exception))# 25
	PTR_WORD _C_LABEL(MIPSX(user_gen_exception))# 26
	PTR_WORD _C_LABEL(MIPSX(user_gen_exception))# 27
	PTR_WORD _C_LABEL(MIPSX(user_gen_exception))# 28
	PTR_WORD _C_LABEL(MIPSX(user_gen_exception))# 29
	PTR_WORD _C_LABEL(MIPSX(user_gen_exception))# 20
	PTR_WORD _C_LABEL(MIPSX(user_gen_exception))# 31
