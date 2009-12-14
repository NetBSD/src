/*	$NetBSD: locore.S,v 1.171 2009/12/14 00:46:06 matt Exp $	*/

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
#include "opt_iso.h"
#include "opt_lockdebug.h"
#include "opt_multiprocessor.h"

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
#ifdef _LP64
	li	t0, MIPS_SR_KX			# turn on XKSEG and XKPHYS
#else
	li	t0, 0				# Disable interrupts and
#endif
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
#ifdef _LP64
	or	t0, MIPS_SR_KX			# turn on XKSEG and XKPHYS
#endif
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
	INT_S	t0, _C_LABEL(cpu_id)		# save PRID register
	INT_S	t1, _C_LABEL(fpu_id)		# save FPU ID register
	PTR_LA	MIPS_CURLWP, _C_LABEL(lwp0)	# set curlwp, curcpu
	PTR_LA	t0, _C_LABEL(cpu_info_store)
	PTR_S	MIPS_CURLWP, CPU_INFO_CURLWP(t0)
	PTR_S	t0, L_CPU(MIPS_CURLWP) 
	jal	_C_LABEL(mach_init)		# mach_init(a0, a1, a2, a3)
	nop

	# XXXuvm_lwp_getuarea
	PTR_L	sp, L_PCB(MIPS_CURLWP)		# switch to lwp0 stack
	nop
	PTR_ADDU sp, USPACE - FRAME_SIZ - CALLFRAME_SIZ
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
 */
NESTED(cpu_switchto, CALLFRAME_SIZ, ra)
/*
 * Save old context, unless the LWP is exiting.
 */
	beq	a0, zero, 1f
	nop
	PTR_L	a2, L_PCB(a0)			# a2 = l->l_addr
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
	REG_S	sp, PCB_CONTEXT+SF_REG_SP(a2)
	REG_S	s8, PCB_CONTEXT+SF_REG_S8(a2)
	REG_S	ra, PCB_CONTEXT+SF_REG_RA(a2)
	REG_S	t0, PCB_CONTEXT+SF_REG_SR(a2)
#if defined(__mips_n32) || defined(__mips_n64)
	REG_S	gp, PCB_CONTEXT+SF_REG_GP(a2)
#endif
#ifdef IPL_ICU_MASK
	INT_L	t0, _C_LABEL(md_imask)
	INT_S	t0, PCB_PPL(a2)
#endif
	REG_EPILOGUE
1:
	move	s6, a0				# s6 = old lwp
	move	MIPS_CURLWP, a1			# s7 = new lwp
	PTR_SUBU sp, CALLFRAME_SIZ
	REG_S	ra, CALLFRAME_RA(sp)
	.mask	0x80000000, -4
/*
 * Switch to new context.
 */
	PTR_L	t2, _C_LABEL(mips_locoresw) + MIPSX_CPU_SWITCH_RESUME
	move	a0, MIPS_CURLWP
	jal	ra, t2
	nop
	PTR_S	MIPS_CURLWP, CPUVAR(CURLWP)

	/* Check for restartable atomic sequences (RAS) */
	PTR_L	t1, L_PROC(MIPS_CURLWP)
	PTR_L	a0, L_PCB(MIPS_CURLWP)		# XXXuvm_lwp_getuarea
	PTR_L	v1, P_RASLIST(t1)
	PTR_ADDU t0, a0, USPACE - FRAME_SIZ
	beq	v1, zero, 1f
	nop
	move	a0, t1
	jal	_C_LABEL(ras_lookup)
	PTR_L	a1, FRAME_EPC(t0)
	PTR_L	a0, L_PCB(MIPS_CURLWP)
	li	v1, -1
	beq	v1, v0, 1f
	PTR_ADDU t0, a0, USPACE - FRAME_SIZ
	PTR_S	v0, FRAME_EPC(t0)
1:
	/* New context is now active */
#ifdef IPL_ICU_MASK
	# restore ICU state
	PTR_L	a0, L_PCB(MIPS_CURLWP)
	INT_L	t0, PCB_PPL(a0)
	INT_S	t0, _C_LABEL(md_imask)
	jal	_C_LABEL(md_imask_update)
	nop
#endif /* IPL_ICU_MASK */
	PTR_L	a0, L_PCB(MIPS_CURLWP)
	move	v0, s6				# Save return value
	REG_PROLOGUE
	REG_L	t0, PCB_CONTEXT+SF_REG_SR(a0)
	DYNAMIC_STATUS_MASK(t0,ra)		# machine dependent masking
	REG_L	ra, PCB_CONTEXT+SF_REG_RA(a0)
	REG_L	s0, PCB_CONTEXT+SF_REG_S0(a0)
	REG_L	s1, PCB_CONTEXT+SF_REG_S1(a0)
	REG_L	s2, PCB_CONTEXT+SF_REG_S2(a0)
	REG_L	s3, PCB_CONTEXT+SF_REG_S3(a0)
	REG_L	s4, PCB_CONTEXT+SF_REG_S4(a0)
	REG_L	s5, PCB_CONTEXT+SF_REG_S5(a0)
	REG_L	s6, PCB_CONTEXT+SF_REG_S6(a0)
	REG_L	s7, PCB_CONTEXT+SF_REG_S7(a0)
#if defined(__mips_n32) || defined(__mips_n64)
	REG_L	gp, PCB_CONTEXT+SF_REG_GP(a0)
#endif
	REG_L	sp, PCB_CONTEXT+SF_REG_SP(a0)
	REG_L	s8, PCB_CONTEXT+SF_REG_S8(a0)
	REG_EPILOGUE
	mtc0	t0, MIPS_COP_0_STATUS
	COP0_SYNC
	j	ra
	nop
END(cpu_switchto)

/*
 * savectx(struct pcb *)
 */
LEAF(savectx)
	mfc0	v0, MIPS_COP_0_STATUS
	REG_PROLOGUE
	REG_S	s0, PCB_CONTEXT+SF_REG_S0(a0)
	REG_S	s1, PCB_CONTEXT+SF_REG_S1(a0)
	REG_S	s2, PCB_CONTEXT+SF_REG_S2(a0)
	REG_S	s3, PCB_CONTEXT+SF_REG_S3(a0)
	REG_S	s4, PCB_CONTEXT+SF_REG_S4(a0)
	REG_S	s5, PCB_CONTEXT+SF_REG_S5(a0)
	REG_S	s6, PCB_CONTEXT+SF_REG_S6(a0)
	REG_S	s7, PCB_CONTEXT+SF_REG_S7(a0)
#if defined(__mips_n32) || defined(__mips_n64)
	REG_S	gp, PCB_CONTEXT+SF_REG_GP(a0)
#endif
	REG_S	sp, PCB_CONTEXT+SF_REG_SP(a0)
	REG_S	s8, PCB_CONTEXT+SF_REG_S8(a0)
	REG_S	ra, PCB_CONTEXT+SF_REG_RA(a0)
	REG_S	v0, PCB_CONTEXT+SF_REG_SR(a0)
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
#if defined(__mips_n32) || defined(__mips_n64)
	REG_S	gp, SF_REG_GP(a0)
#endif
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
#if defined(__mips_n32) || defined(__mips_n64)
	REG_L	gp, SF_REG_GP(a0)
#endif
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
#if 0
	li	a3, ~MIPS_SR_INT_IE
	and	a3, v1
	mtc0	a3, MIPS_COP_0_STATUS		# disable interrupts (2 cycles)
#else
	mtc0	zero, MIPS_COP_0_STATUS		# disable interrupts (2 cycles)
#endif
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
#if 0
	li	a3, ~MIPS_SR_INT_IE
	and	a3, v1
	mtc0	a3, MIPS_COP_0_STATUS		# disable interrupts (2 cycles)
#else
	mtc0	zero, MIPS_COP_0_STATUS		# disable interrupts (2 cycles)
#endif
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
#ifdef _LP64
	li	v0, (MIPS_INT_MASK | MIPS_SR_INT_IE | MIPS_SR_KX)
#else
	li	v0, (MIPS_INT_MASK | MIPS_SR_INT_IE)
#endif
	DYNAMIC_STATUS_MASK(v0,t0)		# machine dependent masking
	mtc0	v0, MIPS_COP_0_STATUS		# enable all sources
	COP0_SYNC
	nop
	j	ra
	nop
END(_splnone)

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
	j	ra
	nop
END(mips_cp0_cause_read)

/*
 * void mips_cp0_cause_write(uint32_t)
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
 * uint32_t mips_cp0_status_read(void)
 *
 *	Return the current value of the CP0 Status register.
 */
LEAF(mips_cp0_status_read)
	mfc0	v0, MIPS_COP_0_STATUS
	j	ra
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
	COP0_SYNC
	nop
	nop
	j	ra
	nop
END(mips_cp0_status_write)

#ifdef MIPS3_PLUS
	.set	push
	.set	mips3
/*
 * uint64_t mips_cp0_tlb_entry_hi_probe(void);
 *
 * Write 1s to the VPN field of Entry_Hi0 to see how many VA bits
 * are implemented.  Assumes that interrupts are disabled.
 */
LEAF(mips_cp0_tlb_entry_hi_probe)
	dmfc0	t0, MIPS_COP_0_TLB_HI
	li	v0, -4096	/* all 1s except low 12 bits */
	dsll	v0, v0, 2	/* except the top 2 */
	dsrl	v0, v0, 2
	dmtc0	v0, MIPS_COP_0_TLB_HI
	COP0_SYNC
	nop
	nop
	dmfc0	v0, MIPS_COP_0_TLB_HI
	dmtc0	t0, MIPS_COP_0_TLB_HI
	COP0_SYNC
	nop
#ifdef __mips_o32
	nop
#if BYTE_ORDER == BIG_ENDIAN
	srl	v1, v0, 0
	dsra	v0, v0, 32
#endif
#if BYTE_ORDER == LITTLE_ENDIAN
	dsra	v1, v0, 32
	srl	v0, v0, 0
#endif
#endif /* __mips_o32 */
	j	ra
	 nop
END(mips_cp0_tlb_entry_hi_probe)

/*
 * uint64_t mips_cp0_tlb_entry_lo_probe(void);
 *
 * Write 1s to the PFN field of Entry_Lo0 to see how many
 * PA bits are implemented.  Assumes that interrupts are disabled.
 */
LEAF(mips_cp0_tlb_entry_lo_probe)
	dmfc0	t0, MIPS_COP_0_TLB_LO0
	li	v0, -64		/* all 1s except low 6 bits */
	dmtc0	v0, MIPS_COP_0_TLB_LO0
	COP0_SYNC
	nop
	nop
	dmfc0	v0, MIPS_COP_0_TLB_LO0
	dmtc0	t0, MIPS_COP_0_TLB_LO0
	COP0_SYNC
	nop
#ifdef __mips_o32
	nop
#if BYTE_ORDER == BIG_ENDIAN
	srl	v1, v0, 0
	dsra	v0, v0, 32
#endif
#if BYTE_ORDER == LITTLE_ENDIAN
	dsra	v1, v0, 32
	srl	v0, v0, 0
#endif
#endif /* __mips_o32 */
	j	ra
	 nop
END(mips_cp0_tlb_entry_lo_probe)
	.set	pop

/*
 * uint32_t mips_cp0_tlb_page_mask_probe(void);
 *
 * Write 1s to the RPN field of Entry_Lo0 to see how many PA bits are implemented.
 * Assumes that interrupts are disabled.
 */
LEAF(mips_cp0_tlb_page_mask_probe)
	mfc0	t0, MIPS_COP_0_TLB_PG_MASK
	lui	v0, 0xffff
	srl	v0, v0, 3
	mtc0	v0, MIPS_COP_0_TLB_PG_MASK
	COP0_SYNC
	nop
	nop
	mfc0	v0, MIPS_COP_0_TLB_PG_MASK
	mtc0	t0, MIPS_COP_0_TLB_PG_MASK
	COP0_SYNC
	nop
	j	ra
	 nop
END(mips_cp0_tlb_page_mask_probe)
#endif /* MIPS3_PLUS */

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
	PTR_SUBU	sp, CALLFRAME_SIZ
	mfc0		t0, MIPS_COP_0_STATUS
	REG_S		ra, CALLFRAME_RA(sp)
	or		t0, t0, MIPS_SR_COP_1_BIT
	mtc0		t0, MIPS_COP_0_STATUS
	COP0_HAZARD_FPUENABLE

	cfc1		t0, MIPS_FPU_CSR	# stall til FP done
	cfc1		t0, MIPS_FPU_CSR	# now get status
	nop
	sll		t2, t0, (31 - 17)	# unimplemented operation?
	bgez		t2, 3f			# no, normal trap
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
	bgez		a1, 1f			# Check the branch delay bit.
	nop
/*
 * The instruction is in the branch delay slot.
 */
	b		2f
	lw		a0, 4(a2)		# a0 = coproc instruction
/*
 * This is not in the branch delay slot so calculate the resulting
 * PC (epc + 4) into v0 and continue to MachEmulateFP().
 */
1:
	lw		a0, 4(a2)		# a0 = coproc instruction
2:
	move		a2, a1

/*
 * Check to see if the instruction to be emulated is a floating-point
 * instruction.
 */
	srl		t0, a0, MIPS_OPCODE_SHIFT
	beq		t0, MIPS_OPCODE_C1, 4f
	nop

/*
 * Send a floating point exception signal to the current LWP.
 */
	li		t0, 0xFFFFFF00
	and		a1, a1, t0
	ori		a1, a1, T_RES_INST << MIPS_CR_EXC_CODE_SHIFT
	REG_PROLOGUE
	REG_S		a1, FRAME_CAUSE(a3)
	REG_EPILOGUE

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
	REG_S		a1, FRAME_CAUSE(a3)
	REG_EPILOGUE

	and		a0, t0, ~MIPS_FPU_EXCEPTION_BITS
	ctc1		a0, MIPS_FPU_CSR

	move		a1, t0			# FPU status
	jal		_C_LABEL(mips_fpuexcept)
	move		a0, MIPS_CURLWP		# get current LWP

	b		FPReturn
	nop

/*
 * Finally, we can call MachEmulateFP() where a0 is the instruction to emulate.
 */
4:
	jal		_C_LABEL(MachEmulateFP)
	move		a1, a3

/*
 * Turn off the floating point coprocessor and return.
 */
FPReturn:
	mfc0		t0, MIPS_COP_0_STATUS
	REG_S		ra, CALLFRAME_RA(sp)
	and		t0, t0, ~MIPS_SR_COP_1_BIT
	mtc0		t0, MIPS_COP_0_STATUS
	COP0_SYNC
	j		ra
	PTR_ADDU	sp, CALLFRAME_SIZ
END(MachFPInterrupt)
#endif /* !defined(NOFPU) && !defined(SOFTFLOAT) */

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
	j	ra
	nop
END(mips_pagecopy)

LEAF(mips_pagezero)
/* We can always safely store a 64-bit zero on MIPS3,4,64 */
	.set	push
#if !defined(MIPS1) && !defined(MIPS32)
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
	j	ra
	nop
#undef XCALLFRAME_RA
#undef XCALLFRAME_SIZ
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
