/*	$NetBSD: locore_mips3.S,v 1.10.2.3 2000/12/08 09:28:21 bouyer Exp $	*/

/*
 * Copyright (c) 1997 Jonathan Stone (hereinafter referred to as the author)
 * All rights reserved.
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
 *      This product includes software developed by Jonathan R. Stone for
 *      the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

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
#include "opt_cputype.h"
#include "opt_ddb.h"

#include <sys/cdefs.h>

#include <mips/asm.h>
#include <mips/cpuregs.h>
#include <machine/param.h>
#include <machine/endian.h>

#include "assym.h"

/*
 * XXX We need a cleaner way of handling the instruction hazards of
 * the various processors.  Here are the relevant rules for the QED 52XX:
 *	tlbw[ri]	-- two integer ops beforehand
 *	tlbr		-- two integer ops beforehand
 *	tlbp		-- two integer ops beforehand
 *	mtc0	[PageMask,EntryHi,Cp0] -- two integer ops afterwards
 *	changing JTLB	-- two integer ops afterwards
 *	mtc0	[EPC,ErrorEPC,Status] -- two int ops afterwards before eret
 *	config.k0	-- five int ops before kseg0, ckseg0 memref
 *
 * For the IDT R4000, some hazards are:
 *	mtc0/mfc0	one integer op before and after
 *	tlbp		-- one integer op afterwards
 * Obvious solution is to take least common denominator.
 */

/*
 * XXX mips/include
 */
#define	C_IINV_I	0x00
#define	C_IWBINV_D	0x01
#define	C_IINV_SI	0x02
#define	C_IWBINV_SD	0x03

#define	C_ILDTAG_I	0x04
#define	C_ILDTAG_D	0x05
#define	C_ILDTAG_SI	0x06
#define	C_ILDTAG_SD	0x07

#define	C_HINV_I	0x10
#define	C_HINV_D	0x11
#define	C_HINV_SI	0x12
#define	C_HINV_SD	0x13

#define	C_HWBINV_D	0x15
#define	C_HWBINV_SD	0x17

#define	C_HSETV_SI	0x1e
#define	C_HSETV_SD	0x1f

/*
 *============================================================================
 *
 *  MIPS III ISA support, part 1: locore exception vectors.
 *  The following code is copied to the vector locations to which
 *  the CPU jumps in response to an exception or a TLB miss.
 *
 *============================================================================
 */
	.set	noreorder
	.set	mips3

/*
 * TLB handling data.   'segbase' points to the base of the segment
 * table.   this is read and written by C code in mips_machdep.c.
 *
 * XXX: use linear mapped PTs at fixed VA in kseg2 in the future?
 */
	.text


/*
 *----------------------------------------------------------------------------
 *
 * mips3_TLBMiss --
 *
 *	Vector code for the TLB-miss exception vector 0x80000000
 *	on an r4000.
 *
 * This code is copied to the TLB exception vector address to
 * handle TLB translation misses.
 * NOTE: This code should be relocatable and max 32 instructions!!!
 *
 * Don't check for invalid pte's here. We load them as well and
 * let the processor trap to load the correct value after service.
 *----------------------------------------------------------------------------
 */
	.set	noat
VECTOR(mips3_TLBMiss, unknown)
	mfc0	k0, MIPS_COP_0_BAD_VADDR    #00: k0=bad address
	lui	k1, %hi(segbase)	    #01: k1=hi of segbase
	bltz	k0, 4f			    #02: k0<0 -> 4f (kernel fault)
	srl	k0, 20			    #03: k0=seg offset (almost)
	lw	k1, %lo(segbase)(k1)	    #04: k1=segment tab base
	andi	k0, k0, 0xffc		    #05: k0=seg offset (mask 0x3)
	addu	k1, k0, k1		    #06: k1=seg entry address
	lw	k1, 0(k1)		    #07: k1=seg entry
	mfc0	k0, MIPS_COP_0_BAD_VADDR    #08: k0=bad address (again)
	beq	k1, zero, 5f		    #09: ==0 -- no page table
	srl	k0, 10			    #0a: k0=VPN (aka va>>10)
	andi	k0, k0, 0xff8		    #0b: k0=page tab offset
	addu	k1, k1, k0		    #0c: k1=pte address
	lw	k0, 0(k1)		    #0d: k0=lo0 pte
	lw	k1, 4(k1)		    #0e: k1=lo1 pte
	sll	k0, 2			    #0f: chop top 2 bits (part 1a)
	srl	k0, 2			    #10: chop top 2 bits (part 1b)
	mtc0	k0, MIPS_COP_0_TLB_LO0	    #11: lo0 is loaded
	sll	k1, 2			    #12: chop top 2 bits (part 2a)
	srl	k1, 2			    #13: chop top 2 bits (part 2b)
	mtc0	k1, MIPS_COP_0_TLB_LO1      #14: lo1 is loaded
	nop				    #15: standard nop
	nop				    #16: extra nop for QED5230
	tlbwr				    #17: write to tlb
	nop				    #18: standard nop
	nop				    #19: needed by R4000/4400
	nop				    #1a: needed by R4000/4400
	eret				    #1b: return from exception
4:	j _C_LABEL(mips3_TLBMissException)  #1c: kernel exception
	nop				    #1d: branch delay slot
5:	j	mips3_SlowFault		    #1e: no page table present
	nop				    #1f: branch delay slot
VECTOR_END(mips3_TLBMiss)
	.set	at
/*
 * mips3_XTLBMiss routine
 *
 *	Vector code for the XTLB-miss exception vector 0x80000080 on an r4000.
 *
 * This code is copied to the XTLB exception vector address to
 * handle TLB translation misses while in 64-bit mode.
 * NOTE: This code should be relocatable and max 32 instructions!!!
 *
 * Note that we do not support the full size of the PTEs, relying
 * on appropriate truncation/sign extension.
 *
 * Don't check for invalid pte's here. We load them as well and
 * let the processor trap to load the correct value after service.
 */
	.set	noat
VECTOR(mips3_XTLBMiss, unknown)
	dmfc0	k0, MIPS_COP_0_BAD_VADDR    #00: k0=bad address
	lui	k1, %hi(segbase)	    #01: k1=hi of segbase
	bltz	k0, 4f			    #02: k0<0 -> 4f (kernel fault)
	srl	k0, 20			    #03: k0=seg offset (almost)
	lw	k1, %lo(segbase)(k1)	    #04: k1=segment tab base
	andi	k0, k0, 0xffc		    #05: k0=seg offset (mask 0x3)
	addu	k1, k0, k1		    #06: k1=seg entry address
	lw	k1, 0(k1)		    #07: k1=seg entry
	dmfc0	k0, MIPS_COP_0_BAD_VADDR    #08: k0=bad address (again)
	beq	k1, zero, 5f		    #09: ==0 -- no page table
	srl	k0, 10			    #0a: k0=VPN (aka va>>10)
	andi	k0, k0, 0xff8		    #0b: k0=page tab offset
	addu	k1, k1, k0		    #0c: k1=pte address
	lw	k0, 0(k1)		    #0d: k0=lo0 pte
	lw	k1, 4(k1)		    #0e: k1=lo1 pte
	sll	k0, 2			    #0f: chop top 2 bits (part 1a)
	srl	k0, 2			    #10: chop top 2 bits (part 1b)
	mtc0	k0, MIPS_COP_0_TLB_LO0	    #11: lo0 is loaded
	sll	k1, 2			    #12: chop top 2 bits (part 2a)
	srl	k1, 2			    #13: chop top 2 bits (part 2b)
	mtc0	k1, MIPS_COP_0_TLB_LO1      #14: lo1 is loaded
	nop				    #15: standard nop
	nop				    #16: extra nop for QED5230
	tlbwr				    #17: write to tlb
	nop				    #18: standard nop
	nop				    #19: needed by R4000/4400
	nop				    #1a: needed by R4000/4400
	eret				    #1b: return from exception
4:	j _C_LABEL(mips3_TLBMissException)  #1c: kernel exception
	nop				    #1d: branch delay slot
5:	j	mips3_SlowFault		    #1e: no page table present
	nop				    #1f: branch delay slot
VECTOR_END(mips3_XTLBMiss)
	.set	at


/*
 *----------------------------------------------------------------------------
 *
 * mips3_exception --
 *
 *	Vector code for the general exception vector 0x80000180
 *	on an r4000 or r4400.
 *
 * This code is copied to the general exception vector address to
 * handle all exceptions except RESET and TLBMiss.
 * NOTE: This code must be relocatable!!!
 *----------------------------------------------------------------------------
 */
VECTOR(mips3_exception, unknown)
/*
 * Find out what mode we came from and jump to the proper handler.
 */
	.set	noat
	mfc0	k0, MIPS_COP_0_STATUS		# get the status register
	mfc0	k1, MIPS_COP_0_CAUSE		# get the cause register
	and	k0, k0, MIPS3_SR_KSU_USER	# test for user mode
						# sneaky but the bits are
						# with us........
	sll	k0, k0, 3			# shift user bit for cause index
	and	k1, k1, MIPS3_CR_EXC_CODE	# mask out the cause bits.
	or	k1, k1, k0			# change index to user table
1:
	la	k0, _C_LABEL(mips3_excpt_sw)	# get base of the jump table
	addu	k0, k0, k1			# get the address of the
						#  function entry.  Note that
						#  the cause is already
						#  shifted left by 2 bits so
						#  we dont have to shift.
	lw	k0, 0(k0)			# get the function address
	#nop					# -slip-
	j	k0				# jump to the function
	nop
	.set	at
VECTOR_END(mips3_exception)


/*----------------------------------------------------------------------------
 *
 * mips3_SlowFault --
 *
 * Alternate entry point into the mips3_UserGenException or
 * or mips3_user_Kern_exception, when the ULTB miss handler couldn't
 * find a TLB entry.
 *
 * Find out what mode we came from and call the appropriate handler.
 *
 *----------------------------------------------------------------------------
 */

/*
 * We couldn't find a TLB entry.
 * Find out what mode we came from and call the appropriate handler.
 */
mips3_SlowFault:
	.set	noat
	mfc0	k0, MIPS_COP_0_STATUS
	nop
	and	k0, k0, MIPS3_SR_KSU_USER
	bne	k0, zero, _C_LABEL(mips3_UserGenException)
	nop
	.set	at
/*
 * Fall though ...
 */

/*
 * mips3_KernGenException
 *
 * Handle an exception from kernel mode.
 * Build trapframe on stack to hold interrupted kernel context, then
 * call trap() to process the condition.
 *
 * trapframe is pointed to by the 5th arg
 * and a dummy sixth argument is used to avoid alignment problems
 *	{
 *	register_t cf_args[4 + 1];
 *	register_t cf_pad;		(for 8 word alignment)
 *	register_t cf_sp;
 *	register_t cf_ra;
 *	mips_reg_t kf_regs[17];		- trapframe begins here
 * 	mips_reg_t kf_sr;		-
 * 	mips_reg_t kf_mullo;		-
 * 	mips_reg_t kf_mulhi;		-
 * 	mips_reg_t kf_epc;		- may be changed by trap() call
 * };
 */
NESTED_NOPROFILE(mips3_KernGenException, KERNFRAME_SIZ, ra)
	.set	noat
	.mask	0x80000000, -4
#if defined(DDB) || defined(KGDB)
	la	k0, _C_LABEL(kdbaux)
	REG_S	s0, SF_REG_S0(k0)
	REG_S	s1, SF_REG_S1(k0)
	REG_S	s2, SF_REG_S2(k0)
	REG_S	s3, SF_REG_S3(k0)
	REG_S	s4, SF_REG_S4(k0)
	REG_S	s5, SF_REG_S5(k0)
	REG_S	s6, SF_REG_S6(k0)
	REG_S	s7, SF_REG_S7(k0)
	REG_S	sp, SF_REG_SP(k0)
	REG_S	s8, SF_REG_S8(k0)
	REG_S	gp, SF_REG_RA(k0)
#endif
/*
 * Save the relevant kernel registers onto the stack.
 * We don't need to save s0 - s8, sp and gp because
 * the compiler does it for us.
 */
	subu	sp, sp, KERNFRAME_SIZ
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
	REG_S	t4, TF_BASE+TF_REG_T4(sp)
	REG_S	t5, TF_BASE+TF_REG_T5(sp)
	REG_S	t6, TF_BASE+TF_REG_T6(sp)
	REG_S	t7, TF_BASE+TF_REG_T7(sp)
	mfc0	a2, MIPS_COP_0_BAD_VADDR	# 3rd arg is fault address
	REG_S	t8, TF_BASE+TF_REG_T8(sp)
	REG_S	t9, TF_BASE+TF_REG_T9(sp)
	REG_S	ra, TF_BASE+TF_REG_RA(sp)
	REG_S	a0, TF_BASE+TF_REG_SR(sp)
	mfc0	a3, MIPS_COP_0_EXC_PC		# 4th arg is exception PC
	REG_S	v0, TF_BASE+TF_REG_MULLO(sp)
	REG_S	v1, TF_BASE+TF_REG_MULHI(sp)
	REG_S	a3, TF_BASE+TF_REG_EPC(sp)
	addu	v0, sp, TF_BASE
	sw	v0, KERNFRAME_ARG5(sp)		# 5th arg is p. to trapframe
/*
 * Call the trap handler.
 */
#if defined(DDB) || defined(DEBUG) || defined(KGDB)
	addu	v0, sp, KERNFRAME_SIZ
	sw	v0, KERNFRAME_SP(sp)
#endif
	mtc0	zero, MIPS_COP_0_STATUS		# Set kernel no error level
	nop
	nop
	nop
	jal	_C_LABEL(trap)			#
	sw	a3,	KERNFRAME_RA(sp)	# for debugging

/*
 * Restore registers and return from the exception.
 */
	mtc0	zero, MIPS_COP_0_STATUS		# Make sure int disabled
	nop					# 3 nop delay
	nop
	nop
	REG_L	a0, TF_BASE+TF_REG_SR(sp)	# ??? why differs ???
	REG_L	t0, TF_BASE+TF_REG_MULLO(sp)
	REG_L	t1, TF_BASE+TF_REG_MULHI(sp)
	REG_L	k0, TF_BASE+TF_REG_EPC(sp)	# might be changed inside trap
	mtc0	a0, MIPS_COP_0_STATUS		# restore the SR, disable intrs
	mtlo	t0
	mthi	t1
	dmtc0	k0, MIPS_COP_0_EXC_PC		# set return address
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
	REG_L	t4, TF_BASE+TF_REG_T4(sp)
	REG_L	t5, TF_BASE+TF_REG_T5(sp)
	REG_L	t6, TF_BASE+TF_REG_T6(sp)
	REG_L	t7, TF_BASE+TF_REG_T7(sp)
	REG_L	t8, TF_BASE+TF_REG_T8(sp)
	REG_L	t9, TF_BASE+TF_REG_T9(sp)
	REG_L	ra, TF_BASE+TF_REG_RA(sp)
	addu	sp, sp, KERNFRAME_SIZ
#ifdef DDBnotyet
	la	k0, _C_LABEL(kdbaux)
	REG_L	s0, SF_REG_S0(k0)
	REG_L	s1, SF_REG_S1(k0)
	REG_L	s2, SF_REG_S2(k0)
	REG_L	s3, SF_REG_S3(k0)
	REG_L	s4, SF_REG_S4(k0)
	REG_L	s5, SF_REG_S5(k0)
	REG_L	s6, SF_REG_S6(k0)
	REG_L	s7, SF_REG_S7(k0)
	REG_L	sp, SF_REG_SP(k0)
	REG_L	s8, SF_REG_S8(k0)
	REG_L	gp, SF_REG_RA(k0)
#endif
	eret					# return to interrupted point
	.set	at
END(mips3_KernGenException)

/*
 * mips3_UserGenException
 *
 * Handle an exception from user mode.
 * Save user context atop the kernel stack, then call trap() to process
 * the condition.  The context can be manipulated alternatively via
 * curproc->p_md.md_regs.
 */
NESTED_NOPROFILE(mips3_UserGenException, CALLFRAME_SIZ, ra)
	.set	noat
	.mask	0x80000000, -4
/*
 * Save all of the registers except for the kernel temporaries in u_pcb.
 */
	lw	k1, _C_LABEL(curpcb)
	#nop					# -slip-
	addu	k1, k1, USPACE - FRAME_SIZ
	REG_S	AT, FRAME_AST(k1)
	REG_S	v0, FRAME_V0(k1)
	REG_S	v1, FRAME_V1(k1)
	mflo	v0
	REG_S	a0, FRAME_A0(k1)
	REG_S	a1, FRAME_A1(k1)
	REG_S	a2, FRAME_A2(k1)
	REG_S	a3, FRAME_A3(k1)
	mfhi	v1
	REG_S	t0, FRAME_T0(k1)
	REG_S	t1, FRAME_T1(k1)
	REG_S	t2, FRAME_T2(k1)
	REG_S	t3, FRAME_T3(k1)
	mfc0	a0, MIPS_COP_0_STATUS		# 1st arg is STATUS
	REG_S	t4, FRAME_T4(k1)
	REG_S	t5, FRAME_T5(k1)
	REG_S	t6, FRAME_T6(k1)
	REG_S	t7, FRAME_T7(k1)
	mfc0	a1, MIPS_COP_0_CAUSE		# 2nd arg is CAUSE
	REG_S	s0, FRAME_S0(k1)
	REG_S	s1, FRAME_S1(k1)
	REG_S	s2, FRAME_S2(k1)
	REG_S	s3, FRAME_S3(k1)
	dmfc0	a2, MIPS_COP_0_BAD_VADDR	# 3rd arg is fault address
	REG_S	s4, FRAME_S4(k1)
	REG_S	s5, FRAME_S5(k1)
	REG_S	s6, FRAME_S6(k1)
	REG_S	s7, FRAME_S7(k1)
	dmfc0	a3, MIPS_COP_0_EXC_PC		# 4th arg is exception PC
	REG_S	t8, FRAME_T8(k1)
	REG_S	t9, FRAME_T9(k1)
	REG_S	gp, FRAME_GP(k1)
	REG_S	sp, FRAME_SP(k1)
	REG_S	s8, FRAME_S8(k1)
	REG_S	ra, FRAME_RA(k1)
	REG_S	a0, FRAME_SR(k1)
	REG_S	v0, FRAME_MULLO(k1)
	REG_S	v1, FRAME_MULHI(k1)
	REG_S	a3, FRAME_EPC(k1)
	addu	sp, k1, -CALLFRAME_SIZ	# switch to kernel SP
#ifdef __GP_SUPPORT__
	la	gp, _C_LABEL(_gp)		# switch to kernel GP
#endif
/*
 * Turn off fpu and enter kernel mode
 */
	.set	at
	and	t0, a0, ~(MIPS_SR_COP_1_BIT | MIPS_SR_EXL | MIPS_SR_KSU_MASK | MIPS_SR_INT_IE)
	.set	noat
/*
 * Call the trap handler.
 */
	mtc0	t0, MIPS_COP_0_STATUS
	jal	_C_LABEL(trap)
	sw	a3, CALLFRAME_SIZ-4(sp)		# for debugging
/*
 * Restore user registers and return.
 * First disable interrupts and set exception level.
 */
	mtc0	zero, MIPS_COP_0_STATUS		# disable interrupt
	nop					# 3 clock delay before
	nop					# exceptions blocked
	nop					# for R4X
	li	v0, MIPS_SR_EXL
	mtc0	v0, MIPS_COP_0_STATUS		# set exception level
	nop					# 3 nop delay
	nop
	nop
	addu	a1, sp, CALLFRAME_SIZ
 #	REG_L	a0, FRAME_SR(a1)
	REG_L	t0, FRAME_MULLO(a1)
	REG_L	t1, FRAME_MULHI(a1)
	REG_L	v0, FRAME_EPC(a1)
 #	mtc0	a0, MIPS_COP_0_STATUS		# still exception level
	mtlo	t0
	mthi	t1
	dmtc0	v0, MIPS_COP_0_EXC_PC		# set return address

	move	k1, a1
	REG_L	AT, FRAME_AST(k1)
	REG_L	v0, FRAME_V0(k1)
	REG_L	v1, FRAME_V1(k1)
	REG_L	a0, FRAME_A0(k1)
	REG_L	a1, FRAME_A1(k1)
	REG_L	a2, FRAME_A2(k1)
	REG_L	a3, FRAME_A3(k1)
	REG_L	t0, FRAME_T0(k1)
	REG_L	t1, FRAME_T1(k1)
	REG_L	t2, FRAME_T2(k1)
	REG_L	t3, FRAME_T3(k1)
	REG_L	t4, FRAME_T4(k1)
	REG_L	t5, FRAME_T5(k1)
	REG_L	t6, FRAME_T6(k1)
	REG_L	t7, FRAME_T7(k1)
	REG_L	s0, FRAME_S0(k1)
	REG_L	s1, FRAME_S1(k1)
	REG_L	s2, FRAME_S2(k1)
	REG_L	s3, FRAME_S3(k1)
	REG_L	s4, FRAME_S4(k1)
	REG_L	s5, FRAME_S5(k1)
	REG_L	s6, FRAME_S6(k1)
	REG_L	s7, FRAME_S7(k1)
	REG_L	t8, FRAME_T8(k1)
	REG_L	t9, FRAME_T9(k1)
	REG_L	k0, FRAME_SR(k1)
	DYNAMIC_STATUS_MASK(k0,ra)		# machine dependent masking
	REG_L	gp, FRAME_GP(k1)
	REG_L	sp, FRAME_SP(k1)
	REG_L	s8, FRAME_S8(k1)
	REG_L	ra, FRAME_RA(k1)
	mtc0	k0, MIPS_COP_0_STATUS		# restore status
	nop
	nop
	eret					# return to interrupted point
	.set	at
END(mips3_UserGenException)

/*
 * mips3_SystemCall
 *
 * Save user context in u_pcb, then call syscall() to process a system call.
 * The context can be manipulated alternatively via curproc->p_md.md_regs;
 */
NESTED_NOPROFILE(mips3_SystemCall, CALLFRAME_SIZ, ra)
	.set	noat
	.mask	0x80000000, -4
	lw	k1, _C_LABEL(curpcb)
	#nop					# -slip-
	addu	k1, k1, USPACE - FRAME_SIZ
	#REG_S	AT, FRAME_AST(k1)
	REG_S	v0, FRAME_V0(k1)		# syscall #
	REG_S	v1, FRAME_V1(k1)		# used by syscall()
	mflo	v0
	REG_S	a0, FRAME_A0(k1)
	REG_S	a1, FRAME_A1(k1)
	REG_S	a2, FRAME_A2(k1)
	REG_S	a3, FRAME_A3(k1)
	mfhi	v1
	#REG_S	t0, FRAME_T0(k1)		# no need to save temp regs
	#REG_S	t1, FRAME_T1(k1)
	#REG_S	t2, FRAME_T2(k1)
	#REG_S	t3, FRAME_T3(k1)
	mfc0	a0, MIPS_COP_0_STATUS		# 1st arg is STATUS
	#REG_S	t4, FRAME_T4(k1)
	#REG_S	t5, FRAME_T5(k1)
	#REG_S	t6, FRAME_T6(k1)
	#REG_S	t7, FRAME_T7(k1)
	mfc0	a1, MIPS_COP_0_CAUSE		# 2nd arg is CAUSE
	REG_S	s0, FRAME_S0(k1)
	REG_S	s1, FRAME_S1(k1)
	REG_S	s2, FRAME_S2(k1)
	REG_S	s3, FRAME_S3(k1)
	mfc0	a2, MIPS_COP_0_EXC_PC		# 3rd arg is PC
	REG_S	s4, FRAME_S4(k1)
	REG_S	s5, FRAME_S5(k1)
	REG_S	s6, FRAME_S6(k1)
	REG_S	s7, FRAME_S7(k1)
	#REG_S	t8, FRAME_T8(k1)
	#REG_S	t9, FRAME_T9(k1)
	REG_S	gp, FRAME_GP(k1)
	REG_S	sp, FRAME_SP(k1)
	REG_S	s8, FRAME_S8(k1)
	REG_S	ra, FRAME_RA(k1)
	REG_S	a0, FRAME_SR(k1)
	REG_S	v0, FRAME_MULLO(k1)
	REG_S	v1, FRAME_MULHI(k1)
	REG_S	a2, FRAME_EPC(k1)
	addu	sp, k1, -CALLFRAME_SIZ
#ifdef __GP_SUPPORT__
	la	gp, _C_LABEL(_gp)		# switch to kernel GP
#endif
/*
 * Turn off fpu and enter kernel mode
 */
	.set	at
	and	t0, a0, ~(MIPS_SR_COP_1_BIT | MIPS_SR_EXL | MIPS_SR_KSU_MASK)
	.set	noat
#if defined(DDB) || defined(DEBUG) || defined(KGDB)
	move	ra, a2
	sw	ra, CALLFRAME_RA(sp)
#endif
/*
 * Call the system call handler.
 */
	mtc0	t0, MIPS_COP_0_STATUS		# re-enable interrupts
	jal	_C_LABEL(syscall)
	nop
/*
 * Restore user registers and return.
 * First disable interrupts and set exception level.
 */
	mtc0	zero, MIPS_COP_0_STATUS		# disable int
	nop					# 3 op delay
	nop
	nop

	li	v0, MIPS_SR_EXL
	mtc0	v0, MIPS_COP_0_STATUS		# set exception level
	nop					# 3 op delay
	nop
	nop
/*
 * Restore user registers and return.
 */
	addu	a1, sp, CALLFRAME_SIZ
 #	REG_L	a0, FRAME_SR(a1)
	REG_L	t0, FRAME_MULLO(a1)
	REG_L	t1, FRAME_MULHI(a1)
	REG_L	v0, FRAME_EPC(a1)		# might be changed in syscall
 #	mtc0	a0, MIPS_COP_0_STATUS		# this should disable interrupts
	mtlo	t0
	mthi	t1
	dmtc0	v0, MIPS_COP_0_EXC_PC		# set return address
	move	k1, a1
	REG_L	AT, FRAME_AST(k1)
	REG_L	v0, FRAME_V0(k1)
	REG_L	v1, FRAME_V1(k1)
	REG_L	a0, FRAME_A0(k1)
	REG_L	a1, FRAME_A1(k1)
	REG_L	a2, FRAME_A2(k1)
	REG_L	a3, FRAME_A3(k1)
	REG_L	t0, FRAME_T0(k1)
	REG_L	t1, FRAME_T1(k1)
	REG_L	t2, FRAME_T2(k1)
	REG_L	t3, FRAME_T3(k1)
	REG_L	t4, FRAME_T4(k1)
	REG_L	t5, FRAME_T5(k1)
	REG_L	t6, FRAME_T6(k1)
	REG_L	t7, FRAME_T7(k1)
	REG_L	s0, FRAME_S0(k1)
	REG_L	s1, FRAME_S1(k1)
	REG_L	s2, FRAME_S2(k1)
	REG_L	s3, FRAME_S3(k1)
	REG_L	s4, FRAME_S4(k1)
	REG_L	s5, FRAME_S5(k1)
	REG_L	s6, FRAME_S6(k1)
	REG_L	s7, FRAME_S7(k1)
	REG_L	t8, FRAME_T8(k1)
	REG_L	t9, FRAME_T9(k1)
	REG_L	k0, FRAME_SR(k1)
	DYNAMIC_STATUS_MASK(k0,ra)		# machine dependent masking
	REG_L	gp, FRAME_GP(k1)
	REG_L	sp, FRAME_SP(k1)
	REG_L	s8, FRAME_S8(k1)
	REG_L	ra, FRAME_RA(k1)
	mtc0	k0, MIPS_COP_0_STATUS
	nop					# 3 nops before eret
	nop
	nop
	eret					# return to syscall point
	.set	at
END(mips3_SystemCall)

/*
 * mips3_KernIntr
 *
 * Handle an interrupt from kernel mode.
 * Build intrframe on stack to hold interrupted kernel context, then
 * call cpu_intr() to process it.
 *
 */
NESTED_NOPROFILE(mips3_KernIntr, KERNFRAME_SIZ, ra)
	.set	noat
	.mask	0x80000000, -4
	subu	sp, sp, KERNFRAME_SIZ
/*
 * Save the relevant kernel registers onto the stack.
 * We don't need to save s0 - s8, sp and gp because
 * the compiler does it for us.
 */
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
	REG_S	t4, TF_BASE+TF_REG_T4(sp)
	REG_S	t5, TF_BASE+TF_REG_T5(sp)
	REG_S	t6, TF_BASE+TF_REG_T6(sp)
	REG_S	t7, TF_BASE+TF_REG_T7(sp)
	mfc0	a2, MIPS_COP_0_EXC_PC		# 3rd arg is exception PC
	REG_S	t8, TF_BASE+TF_REG_T8(sp)
	REG_S	t9, TF_BASE+TF_REG_T9(sp)
	REG_S	ra, TF_BASE+TF_REG_RA(sp)
	REG_S	a0, TF_BASE+TF_REG_SR(sp)
	REG_S	v0, TF_BASE+TF_REG_MULLO(sp)
	REG_S	v1, TF_BASE+TF_REG_MULHI(sp)
	REG_S	a2, TF_BASE+TF_REG_EPC(sp)
/*
 * Call the interrupt handler.
 */
#if defined(DDB) || defined(DEBUG) || defined(KGDB)
	move	ra, a2
	sw	ra, KERNFRAME_RA(sp)		# for debugging
#endif
	mtc0	zero, MIPS_COP_0_STATUS		# Reset exl, trap possible.
	jal	_C_LABEL(cpu_intr)
	and	a3, a0, a1			# 4th is STATUS & CAUSE
/*
 * Restore registers and return from the interrupt.
 */
	mtc0	zero, MIPS_COP_0_STATUS		# Disable interrupt
	nop
	nop
	nop
	REG_L	a0, TF_BASE+TF_REG_SR(sp)	# ??? why differs ???
	DYNAMIC_STATUS_MASK(a0,t0)		# machine dependent masking
	REG_L	t0, TF_BASE+TF_REG_MULLO(sp)
	REG_L	t1, TF_BASE+TF_REG_MULHI(sp)
	REG_L	v0, TF_BASE+TF_REG_EPC(sp)
	mtc0	a0, MIPS_COP_0_STATUS		# restore the SR, disable intrs
	mtlo	t0
	mthi	t1
	dmtc0	v0, MIPS_COP_0_EXC_PC		# set return address

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
	REG_L	t4, TF_BASE+TF_REG_T4(sp)
	REG_L	t5, TF_BASE+TF_REG_T5(sp)
	REG_L	t6, TF_BASE+TF_REG_T6(sp)
	REG_L	t7, TF_BASE+TF_REG_T7(sp)
	REG_L	t8, TF_BASE+TF_REG_T8(sp)
	REG_L	t9, TF_BASE+TF_REG_T9(sp)
	REG_L	ra, TF_BASE+TF_REG_RA(sp)
	addu	sp, sp, KERNFRAME_SIZ		# restore kernel SP
	eret					# return to interrupted point
	.set	at
END(mips3_KernIntr)

/*----------------------------------------------------------------------------
 * XXX this comment block should be updated XXX
 * mips3_UserIntr --
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
NESTED_NOPROFILE(mips3_UserIntr, CALLFRAME_SIZ, ra)
	.set	noat
	.mask	0x80000000, -4
/*
 * Save the relevant user registers into the u_pcb.
 * We don't need to save s0 - s8 because the compiler does it for us.
 */
	lw	k1, _C_LABEL(curpcb)
	#nop					# -slip-
	addu	k1, k1, USPACE - FRAME_SIZ
	REG_S	AT, FRAME_AST(k1)
	REG_S	v0, FRAME_V0(k1)
	REG_S	v1, FRAME_V1(k1)
	mflo	v0
	REG_S	a0, FRAME_A0(k1)
	REG_S	a1, FRAME_A1(k1)
	REG_S	a2, FRAME_A2(k1)
	REG_S	a3, FRAME_A3(k1)
	mfhi	v1
	REG_S	t0, FRAME_T0(k1)
	REG_S	t1, FRAME_T1(k1)
	REG_S	t2, FRAME_T2(k1)
	REG_S	t3, FRAME_T3(k1)
	mfc0	a0, MIPS_COP_0_STATUS		# 1st arg is STATUS
	REG_S	t4, FRAME_T4(k1)
	REG_S	t5, FRAME_T5(k1)
	REG_S	t6, FRAME_T6(k1)
	REG_S	t7, FRAME_T7(k1)
	mfc0	a1, MIPS_COP_0_CAUSE		# 2nd arg is CAUSE
	REG_S	t8, FRAME_T8(k1)
	REG_S	t9, FRAME_T9(k1)
	REG_S	gp, FRAME_GP(k1)
	REG_S	sp, FRAME_SP(k1)
	mfc0	a2, MIPS_COP_0_EXC_PC		# 3rd arg is PC
	REG_S	ra, FRAME_RA(k1)
	REG_S	a0, FRAME_SR(k1)
	REG_S	v0, FRAME_MULLO(k1)
	REG_S	v1, FRAME_MULHI(k1)
	REG_S	a2, FRAME_EPC(k1)
	addu	sp, k1, -CALLFRAME_SIZ		# switch to kernel SP
#ifdef __GP_SUPPORT__
	la	gp, _C_LABEL(_gp)		# switch to kernel GP
#endif
/*
 * Turn off fpu and enter kernel mode
 */
	.set	at
	and	t0, a0, ~(MIPS_SR_COP_1_BIT | MIPS_SR_EXL | MIPS_SR_INT_IE | MIPS_SR_KSU_MASK)
	.set	noat
#if defined(DDB) || defined(DEBUG) || defined(KGDB)
	move	ra, a2
	sw	ra, CALLFRAME_RA(sp)
#endif
/*
 * Call the interrupt handler.
 */
	mtc0	t0, MIPS_COP_0_STATUS
	jal	_C_LABEL(cpu_intr)
	and	a3, a0, a1			# 4th is STATUS & CAUSE
/*
 * Restore registers and return from the interrupt.
 */
	nop
	mtc0	zero, MIPS_COP_0_STATUS
	nop					# 3 nop hazard
	nop
	nop
	li	v0, MIPS_SR_EXL
	mtc0	v0, MIPS_COP_0_STATUS		# set exception level bit.
	nop					# 3 nop hazard
	nop
	nop
	addu	a1, sp, CALLFRAME_SIZ
 #	REG_L	a0, FRAME_SR(a1)
	lw	v0, _C_LABEL(astpending)	# any pending ast?
	nop					# ???
 #	mtc0	a0, MIPS_COP_0_STATUS		# restore the SR, disable intrs
/*
 * Check pending asynchronous traps.
 */
	beq	v0, zero, 1f			# if no, skip ast processing
	nop					# -delay slot-
/*
 * We have pending asynchronous traps; save remaining user state in u_pcb.
 */
	REG_S	s0, FRAME_S0(a1)
	REG_S	s1, FRAME_S1(a1)
	REG_S	s2, FRAME_S2(a1)
	REG_S	s3, FRAME_S3(a1)
	REG_S	s4, FRAME_S4(a1)
	REG_S	s5, FRAME_S5(a1)
	REG_S	s6, FRAME_S6(a1)
	REG_S	s7, FRAME_S7(a1)
	REG_S	s8, FRAME_S8(a1)

	REG_L	a0, FRAME_EPC(a1)	# argument is interrupted PC
	li	t0, MIPS_HARD_INT_MASK | MIPS_SR_INT_IE
	DYNAMIC_STATUS_MASK(t0,t1)		# machine dependent masking
	jal	_C_LABEL(ast)
	mtc0	t0, MIPS_COP_0_STATUS		# enable interrupts (spl0)

/*
 * Restore user registers and return. NOTE: interrupts are enabled.
 */
	mtc0	zero, MIPS_COP_0_STATUS
	nop					# 3 nop delay
	nop
	nop
	li	v0, MIPS_SR_EXL
	mtc0	v0, MIPS_COP_0_STATUS		# set exception level bit.
	nop					# 3 nop delay
	nop
	nop

	addu	a1, sp, CALLFRAME_SIZ
 #	REG_L	a0, FRAME_SR(a1)
	REG_L	s0, FRAME_S0(a1)
	REG_L	s1, FRAME_S1(a1)
	REG_L	s2, FRAME_S2(a1)
	REG_L	s3, FRAME_S3(a1)
	REG_L	s4, FRAME_S4(a1)
	REG_L	s5, FRAME_S5(a1)
	REG_L	s6, FRAME_S6(a1)
	REG_L	s7, FRAME_S7(a1)
	REG_L	s8, FRAME_S8(a1)
 #	mtc0	a0, MIPS_COP_0_STATUS		# this should disable interrupts

1:
	REG_L	t0, FRAME_MULLO(a1)
	REG_L	t1, FRAME_MULHI(a1)
	REG_L	v0, FRAME_EPC(a1)
	mtlo	t0
	mthi	t1
	dmtc0	v0, MIPS_COP_0_EXC_PC		# set return address
	nop					# ??? how much delay ???
	nop

	move	k1, a1
	REG_L	AT, FRAME_AST(k1)
	REG_L	v0, FRAME_V0(k1)
	REG_L	v1, FRAME_V1(k1)
	REG_L	a0, FRAME_A0(k1)
	REG_L	a1, FRAME_A1(k1)
	REG_L	a2, FRAME_A2(k1)
	REG_L	a3, FRAME_A3(k1)
	REG_L	t0, FRAME_T0(k1)
	REG_L	t1, FRAME_T1(k1)
	REG_L	t2, FRAME_T2(k1)
	REG_L	t3, FRAME_T3(k1)
	REG_L	t4, FRAME_T4(k1)
	REG_L	t5, FRAME_T5(k1)
	REG_L	t6, FRAME_T6(k1)
	REG_L	t7, FRAME_T7(k1)
	REG_L	t8, FRAME_T8(k1)
	REG_L	t9, FRAME_T9(k1)
	REG_L	k0, FRAME_SR(k1)
	DYNAMIC_STATUS_MASK(k0,ra)		# machine dependent masking
	REG_L	gp, FRAME_GP(k1)
	REG_L	sp, FRAME_SP(k1)
	REG_L	ra, FRAME_RA(k1)
	mtc0	k0, MIPS_COP_0_STATUS		# restore the SR
	nop					# required for QED 5230
	nop
	eret					# return to interrupted point
	.set	at
END(mips3_UserIntr)


/*----------------------------------------------------------------------------
 *
 *	R4000 TLB exception handlers
 *
 *----------------------------------------------------------------------------
 */


/*----------------------------------------------------------------------------
 *
 * mips3_TLBInvalidException --
 *
 *	Handle a TLB invalid exception from kernel mode in kernel space.
 *	The BaddVAddr, Context, and EntryHi registers contain the failed
 *	virtual address.
 *
 *	The case of wired TLB entries is special.  The wired TLB entries
 *	are used to keep the u area TLB's valid.  The PTE entries for these
 *	do not have MIPS3_PG_G set; the kernel instead relies
 *	on the switch_resume function to set these bits.
 *
 *	To preserve this situation, we set PG_G bits on the "other" TLB entries
 *	when they are wired.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------------
 */
LEAF_NOPROFILE(mips3_TLBInvalidException)
	.set	noat
	dmfc0	k0, MIPS_COP_0_BAD_VADDR	# get the fault address
	li	k1, VM_MIN_KERNEL_ADDRESS	# compute index
	bgez	k0, _C_LABEL(mips3_KernGenException)	# full trap processing
	subu	k0, k0, k1
	lw	k1, _C_LABEL(Sysmapsize)	# index within range?
	srl	k0, k0, PGSHIFT
	sltu	k1, k0, k1
	beq	k1, zero, outofworld	# No. Failing beyond. . .
	lw	k1, _C_LABEL(Sysmap)

	sll	k0, k0, 2			# compute offset from index
	addu	k1, k1, k0
	tlbp					# Probe the invalid entry
	and	k0, k0, 4			# check even/odd page
	nop					# required for QED 5230
	bne	k0, zero, KernTLBIOdd
	nop

	mfc0	k0, MIPS_COP_0_TLB_INDEX
	nop
	bltz	k0, outofworld		# ASSERT(TLB entry exists)
	lw	k0, 0(k1)			# get PTE entry

	dsll	k0, k0, 34			# get rid of "wired" bit
	dsrl	k0, k0, 34
	dmtc0	k0, MIPS_COP_0_TLB_LO0		# load PTE entry
	and	k0, k0, MIPS3_PG_V		# check for valid entry
	nop					# required for QED5230
	beq	k0, zero, _C_LABEL(mips3_KernGenException)	# PTE invalid
	lw	k0, 4(k1)			# get odd PTE entry
	dsll	k0, k0, 34
	mfc0	k1, MIPS_COP_0_TLB_INDEX
	dsrl	k0, k0, 34
	sltiu	k1, k1, MIPS3_TLB_WIRED_UPAGES	# Luckily this is MIPS3_PG_G
	or	k1, k1, k0
	dmtc0	k0, MIPS_COP_0_TLB_LO1		# load PTE entry
	nop
	nop					# required for QED5230
	tlbwi					# write TLB
	nop
	nop
	nop
	nop
	nop
	eret

KernTLBIOdd:
	mfc0	k0, MIPS_COP_0_TLB_INDEX
	nop
	bltz	k0, outofworld		# assert(TLB Entry exists)
	lw	k0, 0(k1)			# get PTE entry

	dsll	k0, k0, 34			# get rid of wired bit
	dsrl	k0, k0, 34
	dmtc0	k0, MIPS_COP_0_TLB_LO1		# save PTE entry
	and	k0, k0, MIPS3_PG_V		# check for valid entry
	nop					# required for QED5230
	beq	k0, zero, _C_LABEL(mips3_KernGenException)	# PTE invalid
	lw	k0, -4(k1)			# get even PTE entry
	dsll	k0, k0, 34
	mfc0	k1, MIPS_COP_0_TLB_INDEX
	dsrl	k0, k0, 34
	sltiu	k1, k1, MIPS3_TLB_WIRED_UPAGES	# Luckily this is MIPS3_PG_G
	or	k1, k1, k0
	dmtc0	k0, MIPS_COP_0_TLB_LO0		# save PTE entry
	nop
	nop					# required for QED5230
	tlbwi					# update TLB
	nop
	nop
	nop
	nop
	nop
	eret
END(mips3_TLBInvalidException)

/*----------------------------------------------------------------------------
 *
 * mips3_TLBMissException --
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
LEAF_NOPROFILE(mips3_TLBMissException)
	.set	noat
	dmfc0	k0, MIPS_COP_0_BAD_VADDR	# get the fault address
	li	k1, VM_MIN_KERNEL_ADDRESS	# compute index
	subu	k0, k0, k1
	lw	k1, _C_LABEL(Sysmapsize)	# index within range?
	srl	k0, k0, PGSHIFT
	sltu	k1, k0, k1
#ifdef newsmips
	/* news5000 has ROM work area at 0xfff00000. */
	bne	k1, zero, 1f
	nop
	j	checkromwork
1:
#else
	beq	k1, zero, outofworld	# No. Failing beyond. . .
#endif
	lw	k1, _C_LABEL(Sysmap)
	srl	k0, k0, 1
	sll	k0, k0, 3			# compute offset from index
	addu	k1, k1, k0
	lw	k0, 0(k1)			# get PTE entry
	lw	k1, 4(k1)			# get odd PTE entry
	dsll	k0, k0, 34			# get rid of "wired" bit
	dsrl	k0, k0, 34
	dmtc0	k0, MIPS_COP_0_TLB_LO0		# load PTE entry
	dsll	k1, k1, 34
	dsrl	k1, k1, 34
	dmtc0	k1, MIPS_COP_0_TLB_LO1		# load PTE entry
	nop
	nop					# required for QED5230
	tlbwr					# write TLB
	nop
	nop
	nop
	nop
	nop
	eret

outofworld:
	/* eret to panic so shutdown can use K2.  Try to ensure valid $sp. */
	la	a0,_C_LABEL(panic)
	dmfc0	a2, MIPS_COP_0_EXC_PC
	move	a1, sp
	sll	k0, k0, PGSHIFT
	dmtc0	a0, MIPS_COP_0_EXC_PC		# return to panic
	li	k1, VM_MIN_KERNEL_ADDRESS
	addu	a3, k0, k1
#if defined(DDB)
	bltz	sp, 1f				# for ddb try to keep frame
	nop
#endif
	la	sp, start			# set sp to a valid place
1:	la	a0, 9f				# string
	eret

	MSG("TLB out of universe: ksp %p epc %p vaddr %p")

	.set	at
END(mips3_TLBMissException)

/*
 * Mark where code entered from exception hander jumptable
 * ends, for stack traceback code.
 */

	.globl	_C_LABEL(mips3_exceptionentry_end)
_C_LABEL(mips3_exceptionentry_end):

/*--------------------------------------------------------------------------
 *
 * mips3_SetPID --
 *
 *	Write the given pid into the TLB pid reg.
 *
 *	mips3_SetPID(pid)
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
LEAF(mips3_SetPID)
	dmtc0	a0, MIPS_COP_0_TLB_HI		# Write the hi reg value
	nop					# required for QED5230
	nop					# required for QED5230
	j	ra
	nop
END(mips3_SetPID)

/*--------------------------------------------------------------------------
 *
 * mips3_TLBUpdate --
 *
 *	Update the TLB if highreg is found; otherwise, enter the data.
 *
 *	mips3_TLBUpdate(virpageadr, lowregx)
 *		unsigned virpageadr, lowregx;
 *
 * Results:
 *	< 0 if loaded >= 0 if updated.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------------------
 */
LEAF(mips3_TLBUpdate)
	mfc0	v1, MIPS_COP_0_STATUS	# Save the status register.
	mtc0	zero, MIPS_COP_0_STATUS	# Disable interrupts
	and	t1, a0, MIPS3_PG_ODDPG	# t1 = Even/Odd flag
	li	v0, (MIPS3_PG_HVPN | MIPS3_PG_ASID)
	and	a0, a0, v0
	dmfc0	t0, MIPS_COP_0_TLB_HI		# Save current PID
	dmtc0	a0, MIPS_COP_0_TLB_HI		# Init high reg
	and	a2, a1, MIPS3_PG_G		# Copy global bit
	nop
	nop
	tlbp					# Probe for the entry.
	dsll	a1, a1, 34
	dsrl	a1, a1, 34
	bne	t1, zero, 2f			# Decide even odd
	mfc0	v0, MIPS_COP_0_TLB_INDEX	# See what we got
# EVEN
	nop
	bltz	v0, 1f				# index < 0 => !found
	nop
	nop					# required for QED5230

	tlbr					# update, read entry first
	nop
	nop
	nop
	dmtc0	a1, MIPS_COP_0_TLB_LO0		# init low reg0.
	nop
	nop					# required for QED5230
	tlbwi					# update slot found
	nop					# required for QED5230
	nop					# required for QED5230
	b	4f
	nop
1:
#ifdef MIPS3_4100				/* VR4100 core */
	lw	v0, _C_LABEL(default_pg_mask)	# default_pg_mask declared
	mtc0	v0, MIPS_COP_0_TLB_PG_MASK	#	in mips_machdep.c
#else
	mtc0	zero, MIPS_COP_0_TLB_PG_MASK	# init mask.
#endif
	dmtc0	a0, MIPS_COP_0_TLB_HI		# init high reg.
	dmtc0	a1, MIPS_COP_0_TLB_LO0		# init low reg0.
	dmtc0	a2, MIPS_COP_0_TLB_LO1		# init low reg1.
	nop
	nop					# required for QED5230
	tlbwr					# enter into a random slot
	nop					# required for QED5230
	nop					# required for QED5230
	b	4f
	nop
# ODD
2:
	nop
	bltz	v0, 3f				# index < 0 => !found
	nop
	nop					# required for QED5230

	tlbr					# read the entry first
	nop
	nop
	nop
	dmtc0	a1, MIPS_COP_0_TLB_LO1		# init low reg1.
	nop
	nop					# required for QED5230
	tlbwi					# update slot found
	nop					# required for QED5230
	nop					# required for QED5230
	b	4f
	nop
3:
#ifdef MIPS3_4100				/* VR4100 core */
	lw	v0, _C_LABEL(default_pg_mask)	# default_pg_mask declared
	mtc0	v0, MIPS_COP_0_TLB_PG_MASK	#	in mips_machdep.c
#else
	mtc0	zero, MIPS_COP_0_TLB_PG_MASK	# init mask.
#endif
	dmtc0	a0, MIPS_COP_0_TLB_HI		# init high reg.
	dmtc0	a2, MIPS_COP_0_TLB_LO0		# init low reg0.
	dmtc0	a1, MIPS_COP_0_TLB_LO1		# init low reg1.
	nop
	nop					# required for QED5230
	tlbwr					# enter into a random slot

4:						# Make shure pipeline
	nop					# advances before we
	nop					# uses the tlb.
	nop
	nop
	dmtc0	t0, MIPS_COP_0_TLB_HI		# restore PID
	nop					# required for QED5230
	nop					# required for QED5230
	j	ra
	mtc0	v1, MIPS_COP_0_STATUS	# Restore the status register
END(mips3_TLBUpdate)

/*--------------------------------------------------------------------------
 *
 * mips3_TLBRead --
 *
 *	Read the TLB entry.
 *
 *	mips3_TLBRead(entry, tlb)
 *		unsigned entry;
 *		struct tlb *tlb;
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	tlb will contain the TLB entry found.
 *
 *--------------------------------------------------------------------------
 */
LEAF(mips3_TLBRead)
	mfc0	v1, MIPS_COP_0_STATUS		# Save the status register.
	mtc0	zero, MIPS_COP_0_STATUS		# Disable interrupts
	nop
	mfc0	t6, MIPS_COP_0_TLB_PG_MASK	# save current pgMask
	nop
	dmfc0	t0, MIPS_COP_0_TLB_HI		# Get current PID

	mtc0	a0, MIPS_COP_0_TLB_INDEX	# Set the index register
	nop
	nop					# required for QED5230
	tlbr					# Read from the TLB
	nop
	nop
	nop
	mfc0	t2, MIPS_COP_0_TLB_PG_MASK	# fetch the pgMask
	dmfc0	t3, MIPS_COP_0_TLB_HI		# fetch the hi entry
	dmfc0	t4, MIPS_COP_0_TLB_LO0		# See what we got
	dmfc0	t5, MIPS_COP_0_TLB_LO1		# See what we got
	dmtc0	t0, MIPS_COP_0_TLB_HI		# restore PID
	mtc0	t6, MIPS_COP_0_TLB_PG_MASK	# restore pgMask
	nop
	nop
	nop					# wait for PID active
	mtc0	v1, MIPS_COP_0_STATUS		# Restore the status register
	nop
	sw	t2, 0(a1)
	sw	t3, 4(a1)
	sw	t4, 8(a1)
	j	ra
	sw	t5, 12(a1)
END(mips3_TLBRead)

/*----------------------------------------------------------------------------
 *
 * mips3_FlushCache --
 *
 *	Flush the caches. Assumes a line size of 16 bytes for speed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The contents of the caches is flushed.
 *
 *----------------------------------------------------------------------------
 */
LEAF(mips3_FlushCache)
	lw	t1, mips_L1ICacheSize
	lw	t2, mips_L1DCacheSize
 #	lw	t3, mips_L1ICacheLSize
 #	lw	t4, mips_L1DCacheLSize
/*
 * Flush the instruction cache.
 */
	li	t0, MIPS_KSEG0_START
	addu	t1, t0, t1		# End address
	subu	t1, t1, 128
1:
	cache	C_IINV_I, 0(t0)
	cache	C_IINV_I, 16(t0)
	cache	C_IINV_I, 32(t0)
	cache	C_IINV_I, 48(t0)
	cache	C_IINV_I, 64(t0)
	cache	C_IINV_I, 80(t0)
	cache	C_IINV_I, 96(t0)
	cache	C_IINV_I, 112(t0)
	bne	t0, t1, 1b
	addu	t0, t0, 128

/*
 * Flush the data cache.
 */
	li	t0, MIPS_KSEG0_START
	addu	t1, t0, t2		# End address
	subu	t1, t1, 128
1:
	cache	C_IWBINV_D, 0(t0)
	cache	C_IWBINV_D, 16(t0)
	cache	C_IWBINV_D, 32(t0)
	cache	C_IWBINV_D, 48(t0)
	cache	C_IWBINV_D, 64(t0)
	cache	C_IWBINV_D, 80(t0)
	cache	C_IWBINV_D, 96(t0)
	cache	C_IWBINV_D, 112(t0)
	bne	t0, t1, 1b
	addu	t0, t0, 128

	lw	t2, mips_L2CacheSize
	beq	t2, zero, 2f
	nop
	li	t0, MIPS_KSEG0_START
	addu	t1, t0, t2
	subu	t1, t1, 128
1:
	cache	C_IWBINV_SD, 0(t0)
	cache	C_IWBINV_SD, 32(t0)
	cache	C_IWBINV_SD, 64(t0)
	cache	C_IWBINV_SD, 96(t0)
	bne	t0, t1, 1b
	addu	t0, t0, 128
2:
	j	ra
	nop
END(mips3_FlushCache)

/*----------------------------------------------------------------------------
 *
 * mips3_FlushICache --
 *
 *	void mips3_FlushICache(addr, len)
 *		vaddr_t addr; vsize_t len;
 *
 *	Flush instruction cache for range of addr to addr + len - 1.
 *	The address is reduced to a kseg0 index.
 *	Assumes a cache line size of 16 bytes for speed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The contents of the cache is flushed.
 *	Must not touch v0.
 *
 *----------------------------------------------------------------------------
 */
LEAF(mips3_FlushICache)
	lw	a2, mips_L1ICacheSize
	addu	a2, -1
	and	a0, a0, a2	# get index into primary cache
	addu	a1, 127		# Align
	li	a2, MIPS_KSEG0_START
	addu	a0, a0, a2
	srl	a1, a1, 7	# Number of unrolled loops
1:
	cache	C_IINV_I, 0(a0)
	cache	C_IINV_I, 16(a0)
	cache	C_IINV_I, 32(a0)
	cache	C_IINV_I, 48(a0)
	cache	C_IINV_I, 64(a0)
	cache	C_IINV_I, 80(a0)
	cache	C_IINV_I, 96(a0)
	cache	C_IINV_I, 112(a0)
	addu	a1, -1
	bne	a1, zero, 1b
	addu	a0, 128

	j	ra
	nop
END(mips3_FlushICache)

/*----------------------------------------------------------------------------
 *
 * mips3_FlushDCache --
 *
 *	void mips3_FlushDCache(addr, len)
 *		vaddr_t addr; vsize_t len;
 *
 *	Flush data cache for index range of addr to addr + len - 1.
 *	The address is reduced to a kseg0 index.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The contents of the cache is written back to primary memory.
 *	The cache line is invalidated.
 *
 *----------------------------------------------------------------------------
 */
LEAF(mips3_FlushDCache)
	lw	a2, mips_L1DCacheSize
	addu	a2, -1
	move	t0, a0		# copy start address
	and	a0, a0, a2	# get index into primary cache
	addu	a1, 127		# Align
	li	a2, MIPS_KSEG0_START
	addu	a0, a0, a2
	addu	a1, a1, a0
	and	a0, a0, -128
	subu	a1, a1, a0
	srl	a1, a1, 7	# Compute number of cache lines
	move	t1, a1		# copy length
1:
	cache	C_IWBINV_D, 0(a0)
	cache	C_IWBINV_D, 16(a0)
	cache	C_IWBINV_D, 32(a0)
	cache	C_IWBINV_D, 48(a0)
	cache	C_IWBINV_D, 64(a0)
	cache	C_IWBINV_D, 80(a0)
	cache	C_IWBINV_D, 96(a0)
	cache	C_IWBINV_D, 112(a0)
	addu	a1, -1
	bne	a1, zero, 1b
	addu	a0, 128

	lw	a2, mips_L2CacheSize
	beq	a2, zero, 2f	# no secondary cache
	addu	a2, -1
	and	t0,t0,a2	# secondary cache index
	li	a0, MIPS_KSEG0_START
	addu	a0, a0, t0	# reduce to kseg0 address
1:
	cache	C_IWBINV_SD, 0(a0)
	cache	C_IWBINV_SD, 32(a0)
	cache	C_IWBINV_SD, 64(a0)
	cache	C_IWBINV_SD, 96(a0)
	addu	t1, -1
	bne	t1, zero, 1b
	addu	a0, 128
2:

	j	ra
	nop
END(mips3_FlushDCache)

/*----------------------------------------------------------------------------
 *
 * mips3_HitFlushDCache --
 *
 *	void mips3_HitFlushDCache(addr, len)
 *		vaddr_t addr, len;
 *
 *	Flush data cache for range of addr to addr + len - 1.
 *	The address can be any valid virtual address as long
 *	as no TLB invalid traps occur. Only lines with matching
 *	addr is flushed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The contents of the cache is written back to primary memory.
 *	The cache line is invalidated.
 *
 *----------------------------------------------------------------------------
 */
LEAF(mips3_HitFlushDCache)
	beq	a1, zero, 2f
	addu	a1, 127		# Align
	addu	a1, a1, a0
	and	a0, a0, -128
	subu	a1, a1, a0
	srl	a1, a1, 7	# Compute number of cache lines
1:
	cache	C_HWBINV_D, 0(a0)
	cache	C_HWBINV_D, 16(a0)
	cache	C_HWBINV_D, 32(a0)
	cache	C_HWBINV_D, 48(a0)
	cache	C_HWBINV_D, 64(a0)
	cache	C_HWBINV_D, 80(a0)
	cache	C_HWBINV_D, 96(a0)
	cache	C_HWBINV_D, 112(a0)
#if 1
	cache	C_HWBINV_SD, 0(a0)
	cache	C_HWBINV_SD, 32(a0)
	cache	C_HWBINV_SD, 64(a0)
	cache	C_HWBINV_SD, 96(a0)
#endif
	addu	a1, -1
	bne	a1, zero, 1b
	addu	a0, 128

2:
	j	ra
	nop
END(mips3_HitFlushDCache)

/*----------------------------------------------------------------------------
 *
 * mips3_InvalidateDCache --
 *
 *	void mips3_InvalidateDCache(addr, len)
 *		vaddr_t addr, len;
 *
 *	Flush data cache for range of addr to addr + len - 1.
 *	The address can be any valid address as long as no TLB misses occur.
 *	(Be sure to use cached K0SEG kernel addresses or mapped addresses)
 * Results:
 *	None.
 *
 * Side effects:
 *	The cache line is invalidated.
 *
 *----------------------------------------------------------------------------
 */
LEAF(mips3_InvalidateDCache)
	addu	a1, a1, a0			# compute ending address
1:
	cache	C_HINV_SD, 0(a0)
	addu	a0, a0, 4
	bne	a0, a1, 1b
	cache	C_HINV_D,-4(a0)

	j	ra
	nop
END(mips3_InvalidateDCache)

/*----------------------------------------------------------------------------
 *
 * mips3_FlushCache_2way --
 *
 *	Flush the caches. Assumes a line size of 32 bytes for speed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The contents of the caches is flushed.
 *
 *----------------------------------------------------------------------------
 */
LEAF(mips3_FlushCache_2way)
	lw	t1, mips_L1ICacheSize
	lw	t2, mips_L1DCacheSize

	/*
	 * Flush the instruction cache.
	 */
	li	t0, MIPS_KSEG0_START
	srl	t1, 1			# Two way set assoc
	or	t3, t1, t0		# Second way address
	addu	t1, t0, t1		# End address
1:
	cache	C_IINV_I,  0(t0)
	cache	C_IINV_I,  0(t3)
	cache	C_IINV_I, 32(t0)
	cache	C_IINV_I, 32(t3)
	cache	C_IINV_I, 64(t0)
	cache	C_IINV_I, 64(t3)
	cache	C_IINV_I, 96(t0)
	addu	t0, t0, 128
	cache	C_IINV_I, 96(t3)
	bne	t0, t1, 1b
	addu	t3, t3, 128		# Branch delay slot

	/*
	 * Flush the data cache.
	 */
	li	t0, MIPS_KSEG0_START
	srl	t2, 1			# Two way set assoc
	or	t3, t2, t0		# Second way address
	addu	t1, t0, t2		# End address
1:
	cache	C_IWBINV_D,  0(t0)
	cache	C_IWBINV_D,  0(t3)
	cache	C_IWBINV_D, 32(t0)
	cache	C_IWBINV_D, 32(t3)
	cache	C_IWBINV_D, 64(t0)
	cache	C_IWBINV_D, 64(t3)
	cache	C_IWBINV_D, 96(t0)
	addu	t0, t0, 128
	cache	C_IWBINV_D, 96(t3)
	bne	t0, t1, 1b
	addu	t3, t3, 128		# Branch delay slot

        lw      t2, mips_L2CacheSize
        beq     t2, zero, 2f
        nop
        li      t0, MIPS_KSEG0_START
        addu    t1, t0, t2
        subu    t1, t1, 128
1:
        cache   C_IWBINV_SD, 0(t0)
        cache   C_IWBINV_SD, 32(t0)
        cache   C_IWBINV_SD, 64(t0)
        cache   C_IWBINV_SD, 96(t0)
        bne     t0, t1, 1b
        addu    t0, t0, 128
2:      
        j       ra 
        nop
END(mips3_FlushCache_2way)

/*----------------------------------------------------------------------------
 *
 * mips3_FlushICache_2way --
 *
 *	void mips3_FlushICache_2way(addr, len)
 *		vaddr_t addr; vsize_t len;
 *
 *	Flush instruction cache for range of addr to addr + len - 1.
 *	The address can be any valid address so long as no TLB misses occur.
 *
 *	Assumes a cache line size of 32 bytes for speed.
 *
 *	Addr should be a KSEG0 address, but it can be called with a KUSEG
 *	address.  To handle flushing both ways here we | in KSEG0 to avoid
 *	invalid TLB misses.  With a virtually indexed cache this does the
 *	correct thing.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The contents of the cache is flushed.
 *	Must not touch v0.
 *
 *----------------------------------------------------------------------------
 */
LEAF(mips3_FlushICache_2way)
	lw	t0, mips_L1ICacheSize
	li	t1, MIPS_KSEG0_START
	addu	a1, 127		# Align (I $ inval of partials is ok)
	srl	t0,1		# Two way set assoc offset
	addu	t2, t0, -1	# Cache index mask
	and	a0, a0, t2	# Only keep index bits (avoid KSEG2, way 0)
	or	a0, a0, t1	# Make KSEG0
	srl	a1, a1, 7	# Number of unrolled loops
	or	t0, t0, a0	# Way 1 index offset.
1:
	cache	C_IINV_I,  0(a0)
	cache	C_IINV_I,  0(t0)	# other way
	cache	C_IINV_I, 32(a0)
	cache	C_IINV_I, 32(t0)	# other way
	addu	a1, -1
	cache	C_IINV_I, 64(a0)
	cache	C_IINV_I, 64(t0)	# other way
	cache	C_IINV_I, 96(a0)
	addu	a0, 128
	cache	C_IINV_I, 96(t0)	# other way
	bgt	a1, zero, 1b
	addu	t0, 128		# Branch delay slot

	j	ra
	nop
END(mips3_FlushICache_2way)

/*----------------------------------------------------------------------------
 *
 * mips3_FlushDCache_2way --
 *
 *	void mips3_FlushDCache_2way(paddr_t addr, len)
 *
 *	Flush data cache for index range of addr to addr + len - 1.
 *	The address is reduced to a kseg0 index.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The contents of the cache is written back to primary memory.
 *	The cache line is invalidated.
 *
 *----------------------------------------------------------------------------
 */
LEAF(mips3_FlushDCache_2way)
	lw	a2, mips_L1DCacheSize
	srl	a3, a2, 1	# two way set associative cache
	addu	a2, a3, -1	# offset mask
	and	a0, a0, a2	# get index into primary cache
	addu	a1, a1, a0	# add offset to length
	and	a0, a0, -128	# block align address
	subu	a1, a1, a0	# subtract aligned offset -> inc len by align
	addu	a1, 127		# tail align length
	li	a2, MIPS_KSEG0_START
	addu	a0, a0, a2	# K0(vindex)
	addu	a3, a0, a3	# second set of two way cache
	srl	a1, a1, 7	# Compute number of cache blocks
1:
	cache	C_IWBINV_D, 0(a0)
	cache	C_IWBINV_D, 0(a3)	# second way
	cache	C_IWBINV_D, 32(a0)
	cache	C_IWBINV_D, 32(a3)
	addu	a1, -1
	cache	C_IWBINV_D, 64(a0)
	cache	C_IWBINV_D, 64(a3)
	cache	C_IWBINV_D, 96(a0)
	cache	C_IWBINV_D, 96(a3)
	addu	a0, 128
	bgtz	a1, 1b
	addu	a3, 128		# Branch delay slot

	j	ra
	nop
END(mips3_FlushDCache_2way)

/*----------------------------------------------------------------------------
 *
 * mips3_HitFlushDCache_2way --
 *
 *	void mips3_HitFlushDCache_2way(addr, len)
 *		vaddr_t addr, len;
 *
 *	Flush data cache for range of addr to addr + len - 1.
 *	The address can be any valid virtual address as long
 *	as no TLB invalid traps occur. Only lines with matching
 *	addr is flushed.
 *
 *	Assumes a cache line size of 32 bytes for speed.
 *	Does not currently support a second level cache.
 *
 *	Other MIPS code does 128B per loop.  W/o a secondary
 *	cache, I think it's better to just operate on what
 *	cache lines were asked to be cleared in the hit case.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The contents of the cache is written back to primary memory.
 *	The cache line is invalidated.
 *
 *----------------------------------------------------------------------------
 */
LEAF(mips3_HitFlushDCache_2way)
	beq	a1, zero, 2f
	addu	a1, 31		# Branch delay slot; align length
	and	a0, a0, -32
	srl	a1, a1, 5	# Compute number of cache lines
1:
	addu	a1, -1
	cache	C_HWBINV_D,0(a0)
	bne	a1, zero, 1b
	addu	a0, 32		# Branch delay slot
2:
	j	ra
	nop
END(mips3_HitFlushDCache_2way)

/*----------------------------------------------------------------------------
 *
 * mips3_InvalidateDCache_2way --
 *
 *	void mips3_InvalidateDCache_2way(addr, len)
 *		vaddr_t addr, len;
 *
 *	Flush data cache for range of addr to addr + len - 1.
 *	The address can be any valid address as long as no TLB misses occur.
 *	(Be sure to use cached K0SEG kernel addresses or mapped addresses)
 *
 *	Caller must be sure the whole cache line is to be invalidated, and
 *	the call does not corrupt bordering data.
 *
 *	Does not currently support a second level cache.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The cache line is invalidated.
 *
 *----------------------------------------------------------------------------
 */
LEAF(mips3_InvalidateDCache_2way)
	addu	a1, a1, a0		# compute ending address
1:
	cache	C_HINV_D,0(a0)
	addu	a0, a0, 32
	bltu	a0, a1, 1b
	nop				# Branch delay slot

	j	ra
	nop
END(mips3_InvalidateDCache_2way)

/*----------------------------------------------------------------------------
 *
 * mips3_VCED --
 *
 *	Handle virtual coherency exceptions.
 *	Called directly from the mips3 execption-table code.
 *	only k0, k1 are avaiable on entry
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Remaps the conflicting address as uncached and returns
 *	from the execption.
 *
 *	NB: cannot be profiled, all registers are user registers on entry.
 *
 *----------------------------------------------------------------------------
 */
LEAF_NOPROFILE(mips3_VCED)
XLEAF(mips3_VCEI)	/* XXXX */
	.set	noat
	move	k0, AT
	.set	at
	sw	k0, vce_saveat
	mfc0	k0, MIPS_COP_0_BAD_VADDR	# fault addr.
	nop
	nop
	and	k0, -16
	sw	k0, vce_savek0			# save virtual address
	cache	C_IWBINV_D, 0(k0)		# writeback primary line
	nop
	nop
	cache	C_ILDTAG_SD, 0(k0)		# read L2Cache tag
	and	k0, PGOFSET
	mfc0	k1, MIPS_COP_0_TAG_LO
	and	k1, 0x00000380			# VIndex[9..7]
	sll	k1, k1, 5			# [14..12] <---
	or	k0, k0, k1
	or	k0, MIPS_KSEG0_START		# physical K0SEG address
	lw	k1, _C_LABEL(mips_L2CacheLSize)
	beq	k1, zero, 2f			# XXX needed?
	subu	k1, zero, k1
	and	k0, k0, k1			# align to L2CacheLSize
1:
	cache	C_IWBINV_D, 0(k0)		# flush 32 bytes
	cache	C_IWBINV_D, 16(k0)
	addu	k1, 32
	bltz	k1, 1b
	addu	k0, 32
2:
	lw	k0, vce_savek0			# get original address
	cache	C_HSETV_SD, 0(k0)
	nop
#ifdef DEBUG
	mfc0	k1, MIPS_COP_0_EXC_PC
	sw	k0, VCE_vaddr
	sw	k1, VCE_epc
	la	k1, VCE_count		# count number of exceptions
	srl	k0, k0, 26		# position upper 4 bits of VA
	and	k0, k0, 0x3c		# mask it off
	add	k1, k0			# get address of count table
	lw	k0, 0(k1)
	addu	k0, 1
	sw	k0, 0(k1)
#endif
	lw	k0, vce_saveat
	.set	noat
	move	AT, k0
	.set	at
	eret
	.align	3			# needs to be aligned?
vce_saveat:
	.word	0
	.word	0
vce_savek0:
	.word	0
	.word	0
	.globl	_C_LABEL(VCE_count)
_C_LABEL(VCE_count):
	.word	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	.globl	_C_LABEL(VCE_epc)
_C_LABEL(VCE_epc):
	.word	0
	.globl	_C_LABEL(VCE_vaddr)
_C_LABEL(VCE_vaddr):
	.word	0
END(mips3_VCED)

/*----------------------------------------------------------------------------
 *
 * mips3_wbflush --
 *
 *	Return when the write buffer is empty.
 *
 *	mips3_wbflush()
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------------
 */
LEAF(mips3_wbflush)
	nop
	sync
	j	ra
	nop
END(mips3_wbflush)

/*
 * mips3_proc_trampoline()
 *
 * Arrange for a function to be invoked neatly, after a cpu_switch().
 * Call the service function with one argument, specified by the s0
 * and s1 respectively.  There is no need register save operation.
 */
LEAF(mips3_proc_trampoline)
	jal	ra, s0
	move	a0, s1
	.set	noat
	li	a0, MIPS_SR_EXL			# set exception level
	mtc0	a0, MIPS_COP_0_STATUS
	nop
	nop
	addu	a1, sp, CALLFRAME_SIZ
 #	REG_L	a0, FRAME_SR(a1)
	REG_L	t0, FRAME_MULLO(a1)
	REG_L	t1, FRAME_MULHI(a1)
	REG_L	v0, FRAME_EPC(a1)
	mtlo	t0
	mthi	t1
	dmtc0	v0, MIPS_COP_0_EXC_PC
	nop
	move	k1, a1
	REG_L	AT, FRAME_AST(k1)
	REG_L	v0, FRAME_V0(k1)
	REG_L	v1, FRAME_V1(k1)
	REG_L	a0, FRAME_A0(k1)
	REG_L	a1, FRAME_A1(k1)
	REG_L	a2, FRAME_A2(k1)
	REG_L	a3, FRAME_A3(k1)
	REG_L	t0, FRAME_T0(k1)
	REG_L	t1, FRAME_T1(k1)
	REG_L	t2, FRAME_T2(k1)
	REG_L	t3, FRAME_T3(k1)
	REG_L	t4, FRAME_T4(k1)
	REG_L	t5, FRAME_T5(k1)
	REG_L	t6, FRAME_T6(k1)
	REG_L	t7, FRAME_T7(k1)
	REG_L	s0, FRAME_S0(k1)
	REG_L	s1, FRAME_S1(k1)
	REG_L	s2, FRAME_S2(k1)
	REG_L	s3, FRAME_S3(k1)
	REG_L	s4, FRAME_S4(k1)
	REG_L	s5, FRAME_S5(k1)
	REG_L	s6, FRAME_S6(k1)
	REG_L	s7, FRAME_S7(k1)
	REG_L	t8, FRAME_T8(k1)
	REG_L	t9, FRAME_T9(k1)
	REG_L	k0, FRAME_SR(k1)
	DYNAMIC_STATUS_MASK(k0,sp)		# machine dependent masking
	REG_L	gp, FRAME_GP(k1)
	REG_L	s8, FRAME_S8(k1)
	REG_L	ra, FRAME_RA(k1)
	REG_L	sp, FRAME_SP(k1)
	mtc0	k0, MIPS_COP_0_STATUS
	nop
	nop
	eret
	.set	at
END(mips3_proc_trampoline)

/*
 * void mips3_cpu_switch_resume(struct proc *newproc)
 *
 * Wiredown the USPACE of newproc in TLB entry#0.  Check whether target
 * USPACE is already in another place of TLB before that, and make
 * sure TBIS(it) in the case.
 */
LEAF_NOPROFILE(mips3_cpu_switch_resume)
	lw	a1, P_MD_UPTE_0(a0)		# a1 = upte[0]
	lw	a2, P_MD_UPTE_1(a0)		# a2 = upte[1]
	lw	v0, P_ADDR(a0)			# va = p->p_addr
	li	s0, MIPS_KSEG2_START
	blt	v0, s0, resume
	nop

	and	s0, v0, MIPS3_PG_ODDPG
	beq	s0, zero, entry0
	nop

	PANIC("USPACE sat on odd page boundary")

entry0:
	dmtc0	v0, MIPS_COP_0_TLB_HI		# VPN = va
	nop
	nop
	tlbp					# probe VPN
	nop
	nop
	mfc0	s0, MIPS_COP_0_TLB_INDEX
	nop
	bltz	s0, entry0set
	li	s0, MIPS_KSEG0_START
	dmtc0	s0, MIPS_COP_0_TLB_HI
	dmtc0	zero, MIPS_COP_0_TLB_LO0
	dmtc0	zero, MIPS_COP_0_TLB_LO1
	nop
	nop
	tlbwi
	nop
	nop
	dmtc0	v0, MIPS_COP_0_TLB_HI		# set VPN again
entry0set:
	mtc0	zero, MIPS_COP_0_TLB_INDEX	# TLB entry #0
	or	a1, MIPS3_PG_G
	dmtc0	a1, MIPS_COP_0_TLB_LO0		# upte[0] | PG_G
	or	a2, MIPS3_PG_G
	dmtc0	a2, MIPS_COP_0_TLB_LO1		# upte[1] | PG_G
	nop
	nop
	tlbwi					# set TLB entry #0
	nop
	nop

resume:
	j	ra
	nop
	END(mips3_cpu_switch_resume)

/*
 * void mips3_TBIS(vaddr_t va)
 *
 * Invalidate a TLB entry which has the given vaddr and ASID if found.
 */
LEAF_NOPROFILE(mips3_TBIS)
	mfc0	v1, MIPS_COP_0_STATUS		# save status register
	mtc0	zero, MIPS_COP_0_STATUS		# disable interrupts

	li	v0, (MIPS3_PG_HVPN | MIPS3_PG_ASID)
	dmfc0	t0, MIPS_COP_0_TLB_HI		# save current ASID
	mfc0	t3, MIPS_COP_0_TLB_PG_MASK	# save current pgMask
	and	a0, a0, v0			# make sure valid entryHi
	dmtc0	a0, MIPS_COP_0_TLB_HI		# look for the vaddr & ASID
	nop
	nop
	tlbp					# probe the entry in question
	nop
	nop
	mfc0	v0, MIPS_COP_0_TLB_INDEX	# see what we got
	#nop					# -slip-
	#nop					# -slip-
	bltz	v0, 1f				# index < 0 then skip
	li	t1, MIPS_KSEG0_START		# invalid address
	dmtc0	t1, MIPS_COP_0_TLB_HI		# make entryHi invalid
	dmtc0	zero, MIPS_COP_0_TLB_LO0	# zero out entryLo0
	dmtc0	zero, MIPS_COP_0_TLB_LO1	# zero out entryLo1
	mtc0	zero, MIPS_COP_0_TLB_PG_MASK	# zero out pageMask
	nop
	nop
	tlbwi
	nop
	nop
1:
	dmtc0	t0, MIPS_COP_0_TLB_HI		# restore current ASID
	mtc0	t3, MIPS_COP_0_TLB_PG_MASK	# restore pgMask
	nop
	nop
	j	ra
	mtc0	v1, MIPS_COP_0_STATUS		# restore status register
	END(mips3_TBIS)

/*
 * void mips3_TBIAP(int sizeofTLB)
 *
 * Invalidate TLB entries belong to per process user spaces while
 * leaving entries for kernel space marked global intact.
 */
LEAF_NOPROFILE(mips3_TBIAP)
	mfc0	v1, MIPS_COP_0_STATUS		# save status register
	mtc0	zero, MIPS_COP_0_STATUS		# disable interrupts

	move	t2, a0
	mfc0	t1, MIPS_COP_0_TLB_WIRED
	li	v0, MIPS_KSEG0_START		# invalid address
	mfc0	t3, MIPS_COP_0_TLB_PG_MASK	# save current pgMask

	# do {} while (t1 < t2)
1:
	mtc0	t1, MIPS_COP_0_TLB_INDEX	# set index
	nop
	nop
	tlbr					# obtain an entry
	nop
	nop
	nop
	dmfc0	a0, MIPS_COP_0_TLB_LO1
	#nop					# -slip-
	#nop					# -slip-
	and	a0, a0, MIPS3_PG_G		# check to see it has G bit
	bnez	a0, 2f
	nop

	dmtc0	v0, MIPS_COP_0_TLB_HI		# make entryHi invalid
	dmtc0	zero, MIPS_COP_0_TLB_LO0	# zero out entryLo0
	dmtc0	zero, MIPS_COP_0_TLB_LO1	# zero out entryLo1
	mtc0	zero, MIPS_COP_0_TLB_PG_MASK	# zero out mask entry
	nop
	nop
	tlbwi					# invalidate the TLB entry
	#nop
	#nop
2:
	addu	t1, t1, 1
	bne	t1, t2, 1b
	nop

	mtc0	t3, MIPS_COP_0_TLB_PG_MASK	# restore pgMask
	nop
	nop
	j	ra				# new ASID will be set soon
	mtc0	v1, MIPS_COP_0_STATUS		# restore status register
	END(mips3_TBIAP)

/*
 * void mips3_TBIA(int sizeofTLB)
 *
 * Invalidate all of non-wired TLB entries.
 */
LEAF_NOPROFILE(mips3_TBIA)
	mfc0	v1, MIPS_COP_0_STATUS		# save status register
	mtc0	zero, MIPS_COP_0_STATUS		# disable interrupts

	li	v0, MIPS_KSEG0_START		# invalid address
	dmfc0	t0, MIPS_COP_0_TLB_HI		# save current ASID
	mfc0	t1, MIPS_COP_0_TLB_WIRED
	mfc0	t2, MIPS_COP_0_TLB_PG_MASK	# save current pgMask

	dmtc0	v0, MIPS_COP_0_TLB_HI		# make entryHi invalid
	dmtc0	zero, MIPS_COP_0_TLB_LO0	# zero out entryLo0
	dmtc0	zero, MIPS_COP_0_TLB_LO1	# zero out entryLo1
	mtc0	zero, MIPS_COP_0_TLB_PG_MASK	# zero out pageMask

	# do {} while (t1 < a0)
1:
	mtc0	t1, MIPS_COP_0_TLB_INDEX	# set TLBindex
	nop
	nop
	tlbwi					# clear the entry
	#nop
	#nop
	addu	t1, t1, 1			# increment index
	bne	t1, a0, 1b
	nop

	dmtc0	t0, MIPS_COP_0_TLB_HI		# restore ASID
	mtc0	t2, MIPS_COP_0_TLB_PG_MASK	# restore pgMask
	nop
	nop
	j	ra
	mtc0	v1, MIPS_COP_0_STATUS		# restore status register
	END(mips3_TBIA)

LEAF(mips3_FetchIcache)
	lw	t1, mips_L1ICacheSize
	lw	t2, mips_L1ICacheLSize
	li	v0, 0
	li	t0, MIPS_KSEG0_START
	addu	t3, t1, t0
1:
	cache	C_ILDTAG_I, 0(t0)
	mfc0	t4, MIPS_COP_0_TAG_LO
	sw	t4, 0(a0)
	addu	a0, 4
	addu	v0, 1
	addu	t0, t2
	blt	t0, t3, 1b
	nop
	j	ra
	nop
END(mips3_FetchIcache)

LEAF(mips3_FetchDcache)
	lw	t1, mips_L1DCacheSize
	lw	t2, mips_L1DCacheLSize
	li	v0, 0
	li	t0, MIPS_KSEG0_START
	addu	t3, t1, t0
1:
	cache	C_ILDTAG_D, 0(t0)
	mfc0	t4, MIPS_COP_0_TAG_LO
	sw	t4, 0(a0)
	addu	a0, 4
	addu	v0, 1
	addu	t0, t2
	blt	t0, t3, 1b
	nop
	j	ra
	nop
END(mips3_FetchDcache)


/*
 * u_int32_t mips3_cp0_compare_read(void)
 *
 *	Return the current value of the CP0 Compare register.
 */
LEAF(mips3_cp0_compare_read)
	mfc0	v0, MIPS_COP_0_COMPARE
	j	ra
	nop
END(mips3_cp0_compare_read)

/*
 * void mips3_cp0_compare_write(u_int32_t)
 *
 *	Set the value of the CP0 Compare register.
 */
LEAF(mips3_cp0_compare_write)
	mtc0	a0, MIPS_COP_0_COMPARE
	nop
	nop
	j	ra
	nop
END(mips3_cp0_compare_write)


/*
 * u_int32_t mips3_cp0_config_read(void)
 *
 *	Return the current value of the CP0 Config register.
 */
LEAF(mips3_cp0_config_read)
	mfc0	v0, MIPS_COP_0_CONFIG
	j	ra
	nop
END(mips3_cp0_config_read)

/*
 * void mips3_cp0_config_write(u_int32_t)
 *
 *	Set the value of the CP0 Config register.
 */
LEAF(mips3_cp0_config_write)
	mtc0	a0, MIPS_COP_0_CONFIG
	nop
	nop
	j	ra
	nop
END(mips3_cp0_config_write)


/*
 * u_int32_t mips3_cp0_count_read(void)
 *
 *	Return the current value of the CP0 Count register.
 */
LEAF(mips3_cp0_count_read)
	mfc0	v0, MIPS_COP_0_COUNT
	j	ra
	nop
END(mips3_cp0_count_read)

/*
 * void mips3_cp0_count_write(u_int32_t)
 *
 *	Set the value of the CP0 Count register.
 */
LEAF(mips3_cp0_count_write)
	mtc0	a0, MIPS_COP_0_COUNT
	nop
	nop
	j	ra
	nop
END(mips3_cp0_count_write)


/*
 * u_int32_t mips3_cp0_wired_read(void)
 *
 *	Return the current value of the CP0 Wired register.
 */
LEAF(mips3_cp0_wired_read)
	mfc0	v0, MIPS_COP_0_TLB_WIRED
	j	ra
	nop
END(mips3_cp0_wired_read)

/*
 * void mips3_cp0_wired_write(u_int32_t)
 *
 *	Set the value of the CP0 Wired register.
 */
LEAF(mips3_cp0_wired_write)
	mtc0	a0, MIPS_COP_0_TLB_WIRED
	nop
	nop
	j	ra
	nop
END(mips3_cp0_wired_write)


#if defined(_MIPS_BSD_API) && \
    (_MIPS_BSD_API == _MIPS_BSD_API_N32 || _MIPS_BSD_API == _MIPS_BSD_API_LP64)
#error mips3_ld and mips3_sd should be adjusted for N32 or LP64
#endif

LEAF(mips3_ld)
#if !defined(_MIPS_BSD_API) || _MIPS_BSD_API == _MIPS_BSD_API_LP32
	mfc0	t0, MIPS_COP_0_STATUS		# turn of interrupts
	and	t1, t0, ~(MIPS_SR_INT_IE)
	mtc0	t1, MIPS_COP_0_STATUS
	nop
	nop
	nop
#endif

	ld	v0, 0(a0)
#if _BYTE_ORDER == _BIG_ENDIAN
	addu	v1, v0, 0			# low word in v1
	dsra	v0, v0, 32			# high word in v0
#else
	dsra	v1, v0, 32			# high word in v1
	addu	v0, v0, 0			# low word in v0
#endif

#if !defined(_MIPS_BSD_API) || _MIPS_BSD_API == _MIPS_BSD_API_LP32
	mtc0	t0, MIPS_COP_0_STATUS		# restore intr status.
	nop
#endif

	jr	ra
	nop
END(mips3_ld)

LEAF(mips3_sd)
#if !defined(_MIPS_BSD_API) || _MIPS_BSD_API == _MIPS_BSD_API_LP32
	mfc0	t0, MIPS_COP_0_STATUS		# turn of interrupts
	and	t1, t0, ~(MIPS_SR_INT_IE)
	mtc0	t1, MIPS_COP_0_STATUS
	nop
	nop
	nop
#endif

	# NOTE: a1 is padding!

#if _BYTE_ORDER == _BIG_ENDIAN
	dsll	a2, a2, 32			# high word in a2
	dsll	a3, a3, 32			# low word in a3
	dsrl	a3, a3, 32
#else
	dsll	a2, a2, 32			# low word in a2
	dsrl	a2, a2, 32
	dsll	a3, a3, 32			# high word in a3
#endif
	or	a1, a2, a3
	sd	a1, 0(a0)

#if !defined(_MIPS_BSD_API) || _MIPS_BSD_API == _MIPS_BSD_API_LP32
	mtc0	t0, MIPS_COP_0_STATUS		# restore intr status.
	nop
#endif

	jr	ra
	nop
END(mips3_sd)


/*
 * The variables below are used to communicate the cache handling
 * to higher-level software.
 */
	.sdata

	.globl	_C_LABEL(mips3_L1TwoWayCache)
_C_LABEL(mips3_L1TwoWayCache):
	.word	0

	.data

	.globl _C_LABEL(mips3_locoresw)
_C_LABEL(mips3_locoresw):
	.word _C_LABEL(mips3_cpu_switch_resume)
	.word _C_LABEL(mips3_proc_trampoline)
	.word _C_LABEL(mips_idle)

_C_LABEL(mips3_excpt_sw):
	####
	#### The kernel exception handlers.
	####
	.word _C_LABEL(mips3_KernIntr)		# 0 external interrupt
	.word _C_LABEL(mips3_KernGenException)	# 1 TLB modification
	.word _C_LABEL(mips3_TLBInvalidException) # 2 TLB miss (LW/I-fetch)
	.word _C_LABEL(mips3_TLBInvalidException) # 3 TLB miss (SW)
	.word _C_LABEL(mips3_KernGenException)	# 4 address error (LW/I-fetch)
	.word _C_LABEL(mips3_KernGenException)	# 5 address error (SW)
	.word _C_LABEL(mips3_KernGenException)	# 6 bus error (I-fetch)
	.word _C_LABEL(mips3_KernGenException)	# 7 bus error (load or store)
	.word _C_LABEL(mips3_KernGenException)	# 8 system call
	.word _C_LABEL(mips3_KernGenException)	# 9 breakpoint
	.word _C_LABEL(mips3_KernGenException)	# 10 reserved instruction
	.word _C_LABEL(mips3_KernGenException)	# 11 coprocessor unusable
	.word _C_LABEL(mips3_KernGenException)	# 12 arithmetic overflow
	.word _C_LABEL(mips3_KernGenException)	# 13 r4k trap exception
	.word _C_LABEL(mips3_VCEI)		# 14 r4k virt coherence
	.word _C_LABEL(mips3_KernGenException)	# 15 r4k FP exception
	.word _C_LABEL(mips3_KernGenException)	# 16 reserved
	.word _C_LABEL(mips3_KernGenException)	# 17 reserved
	.word _C_LABEL(mips3_KernGenException)	# 18 reserved
	.word _C_LABEL(mips3_KernGenException)	# 19 reserved
	.word _C_LABEL(mips3_KernGenException)	# 20 reserved
	.word _C_LABEL(mips3_KernGenException)	# 21 reserved
	.word _C_LABEL(mips3_KernGenException)	# 22 reserved
	.word _C_LABEL(mips3_KernGenException)	# 23 watch exception
	.word _C_LABEL(mips3_KernGenException)	# 24 reserved
	.word _C_LABEL(mips3_KernGenException)	# 25 reserved
	.word _C_LABEL(mips3_KernGenException)	# 26 reserved
	.word _C_LABEL(mips3_KernGenException)	# 27 reserved
	.word _C_LABEL(mips3_KernGenException)	# 28 reserved
	.word _C_LABEL(mips3_KernGenException)	# 29 reserved
	.word _C_LABEL(mips3_KernGenException)	# 30 reserved
	.word _C_LABEL(mips3_VCED)		# 31 v. coherence exception data
	#####
	##### The user exception handlers.
	#####
	.word _C_LABEL(mips3_UserIntr)		#  0
	.word _C_LABEL(mips3_UserGenException)	#  1
	.word _C_LABEL(mips3_UserGenException)	#  2
	.word _C_LABEL(mips3_UserGenException)	#  3
	.word _C_LABEL(mips3_UserGenException)	#  4
	.word _C_LABEL(mips3_UserGenException)	#  5
	.word _C_LABEL(mips3_UserGenException)	#  6
	.word _C_LABEL(mips3_UserGenException)	#  7
	.word _C_LABEL(mips3_SystemCall)	#  8
	.word _C_LABEL(mips3_UserGenException)	#  9
	.word _C_LABEL(mips3_UserGenException)	# 10
	.word _C_LABEL(mips3_UserGenException)	# 11
	.word _C_LABEL(mips3_UserGenException)	# 12
	.word _C_LABEL(mips3_UserGenException)	# 13
	.word _C_LABEL(mips3_VCEI)		# 14
	.word _C_LABEL(mips3_UserGenException)	# 15
	.word _C_LABEL(mips3_UserGenException)	# 16
	.word _C_LABEL(mips3_UserGenException)	# 17
	.word _C_LABEL(mips3_UserGenException)	# 18
	.word _C_LABEL(mips3_UserGenException)	# 19
	.word _C_LABEL(mips3_UserGenException)	# 20
	.word _C_LABEL(mips3_UserGenException)	# 21
	.word _C_LABEL(mips3_UserGenException)	# 22
	.word _C_LABEL(mips3_UserGenException)	# 23
	.word _C_LABEL(mips3_UserGenException)	# 24
	.word _C_LABEL(mips3_UserGenException)	# 25
	.word _C_LABEL(mips3_UserGenException)	# 26
	.word _C_LABEL(mips3_UserGenException)	# 27
	.word _C_LABEL(mips3_UserGenException)	# 28
	.word _C_LABEL(mips3_UserGenException)	# 29
	.word _C_LABEL(mips3_UserGenException)	# 20
	.word _C_LABEL(mips3_VCED)		# 31 v. coherence exception data
