/*	$NetBSD: mipsX_subr.S,v 1.26.36.1.2.1 2009/08/21 17:50:35 matt Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Simon Burge for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

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
#include "opt_mips3_wired.h"
#include "opt_vmswap.h"

#include <sys/cdefs.h>

#include <mips/asm.h>
#include <mips/cpuregs.h>
#if defined(MIPS3) && !defined(MIPS3_5900)
#include <mips/cache_r4k.h>
#endif

#include <machine/param.h>
#include <machine/endian.h>

#include "assym.h"

/*
 * XXX MIPS3_5900 is still "special" for much of this code.
 */

#if MIPS1
#error This file can not be compiled with MIPS1 defined
#endif

#if MIPS3 + MIPS32 + MIPS64 != 1
# error  Only one of MIPS{3,32,64} can be defined
#endif

/*
 * Use 64bit cp0 instructions?
 */
#if defined(MIPS3)
#define	USE_64BIT_INSTRUCTIONS
#if defined(MIPS3_5900)		/* the 5900 has mips32-like mmu registers */
#undef	USE_64BIT_CP0_FUNCTIONS
#else
#define	USE_64BIT_CP0_FUNCTIONS
#endif
#endif

#if defined(MIPS32)
#undef	USE_64BIT_INSTRUCTIONS
#undef	USE_64BIT_CP0_FUNCTIONS
#endif

#if defined(MIPS64)
#define	USE_64BIT_INSTRUCTIONS
#define	USE_64BIT_CP0_FUNCTIONS
#endif

#if defined(USE_64BIT_CP0_FUNCTIONS)
#define	_SLL		dsll
#define	_SRL		dsrl
#define	WIRED_SHIFT	34
#else
#define	_SLL		sll
#define	_SRL		srl
#define	WIRED_SHIFT	2
#endif

/*
 * Use correct-sized m?c0/dm?c0 opcodes.
 */
#if defined(USE_64BIT_CP0_FUNCTIONS)
#define	_MFC0	dmfc0
#define	_MTC0	dmtc0
#else
#define	_MFC0	mfc0
#define	_MTC0	mtc0
#endif


/*
 * Set ISA level for the assembler.
 */
#if defined(MIPS3)
	.set	mips3
#endif

#if defined(MIPS32)
	.set	mips32
#endif

#if defined(MIPS64)
	.set	mips64
#endif


/*
 * CPP function renaming macros.
 */

#if defined(MIPS3)
#ifdef __STDC__
#define	MIPSX(name)	mips3_ ## name
#else
#define	MIPSX(name)	mips3_/**/name
#endif
#endif

#if defined(MIPS3_5900)
#undef MIPSX
#ifdef __STDC__
#define	MIPSX(name)	mips5900_ ## name
#else
#define	MIPSX(name)	mips5900_/**/name
#endif
#endif
	
#if defined(MIPS32)
#ifdef __STDC__
#define	MIPSX(name)	mips32_ ## name
#else
#define	MIPSX(name)	mips32_/**/name
#endif
#endif

#if defined(MIPS64)
#ifdef __STDC__
#define	MIPSX(name)	mips64_ ## name
#else
#define	MIPSX(name)	mips64_/**/name
#endif
#endif

#define	_VECTOR_END(name)	VECTOR_END(name)

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
 *
 * For the Toshiba R5900, TX79:
 *	mtc0		following sync.p
 *	tlbw[ri], tlbp	following sync.p or eret
 * for those CPU, define COP0_SYNC as sync.p
 */


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
VECTOR(MIPSX(TLBMiss), unknown)
	.set	noat
	mfc0	k0, MIPS_COP_0_BAD_VADDR	#00: k0=bad address
	lui	k1, %hi(segbase)		#01: k1=hi of segbase
	bltz	k0, 4f				#02: k0<0 -> 4f (kernel fault)
	srl	k0, (2*PGSHIFT-2-PTR_SCALESHIFT)#03: k0=seg offset (almost)
	PTR_L	k1, %lo(segbase)(k1)		#04: k1=segment tab base
	andi	k0, k0, (NBPG-(1<<PTR_SCALESHIFT))#05: k0=seg offset (mask 0x3)
	PTR_ADDU k1, k0, k1			#06: k1=seg entry address
	PTR_L	k1, 0(k1)			#07: k1=seg entry
	mfc0	k0, MIPS_COP_0_BAD_VADDR	#08: k0=bad address (again)
	beq	k1, zero, 5f			#09: ==0 -- no page table
	srl	k0, (PGSHIFT-2)			#0a: k0=VPN (aka va>>10)
	andi	k0, k0, (NBPG-8)		#0b: k0=page tab offset
	PTR_ADDU k1, k1, k0			#0c: k1=pte address
	INT_L	k0, 0(k1)			#0d: k0=lo0 pte
	INT_L	k1, 4(k1)			#0e: k1=lo1 pte
	sll	k0, 2				#0f: chop top 2 bits (part 1a)
	srl	k0, 2				#10: chop top 2 bits (part 1b)
#ifdef MIPS3_5900
	mtc0	k0, MIPS_COP_0_TLB_LO0		#11: lo0 is loaded
	sync.p					#12: R5900 cop0 hazard
	sll	k1, 2				#13: chop top 2 bits (part 2a)
	srl	k1, 2				#14: chop top 2 bits (part 2b)
	mtc0	k1, MIPS_COP_0_TLB_LO1		#15: lo1 is loaded
	sync.p					#16: R5900 cop0 hazard
#else /* MIPS3_5900 */
	mtc0	k0, MIPS_COP_0_TLB_LO0		#11: lo0 is loaded
	sll	k1, 2				#12: chop top 2 bits (part 2a)
	srl	k1, 2				#13: chop top 2 bits (part 2b)
	mtc0	k1, MIPS_COP_0_TLB_LO1		#14: lo1 is loaded
	nop					#15: standard nop
	nop					#16: extra nop for QED5230
#endif /* MIPS3_5900 */
	tlbwr					#17: write to tlb
	nop					#18: standard nop
	nop					#19: needed by R4000/4400
	nop					#1a: needed by R4000/4400
	eret					#1b: return from exception
4:	j _C_LABEL(MIPSX(TLBMissException))	#1c: kernel exception
	nop					#1d: branch delay slot
5:	j	slowfault			#1e: no page table present
	nop					#1f: branch delay slot
	.set	at
_VECTOR_END(MIPSX(TLBMiss))

#if defined(USE_64BIT_CP0_FUNCTIONS)
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
VECTOR(MIPSX(XTLBMiss), unknown)
	.set	noat
	dmfc0	k0, MIPS_COP_0_BAD_VADDR	#00: k0=bad address
	lui	k1, %hi(segbase)		#01: k1=hi of segbase
	bltz	k0, 4f				#02: k0<0 -> 4f (kernel fault)
	srl	k0, 2*PGSHIFT-2-PTR_SCALESHIFT	#03: k0=seg offset (almost)
	PTR_L	k1, %lo(segbase)(k1)		#04: k1=segment tab base
	andi	k0, NBPG-(1<<PTR_SCALESHIFT)	#05: k0=seg offset (mask 0x3)
	PTR_ADDU k1, k0, k1			#06: k1=seg entry address
	PTR_L	k1, 0(k1)			#07: k1=seg entry
	dmfc0	k0, MIPS_COP_0_BAD_VADDR	#08: k0=bad address (again)
	beq	k1, zero, 5f			#09: ==0 -- no page table
	srl	k0, (PGSHIFT-2)			#0a: k0=VPN (aka va>>10)
	andi	k0, k0, (NBPG-8)		#0b: k0=page tab offset
	PTR_ADDU k1, k1, k0			#0c: k1=pte address
	INT_L	k0, 0(k1)			#0d: k0=lo0 pte
	INT_L	k1, 4(k1)			#0e: k1=lo1 pte
	sll	k0, 2				#0f: chop top 2 bits (part 1a)
	srl	k0, 2				#10: chop top 2 bits (part 1b)
	mtc0	k0, MIPS_COP_0_TLB_LO0		#11: lo0 is loaded
	sll	k1, 2				#12: chop top 2 bits (part 2a)
	srl	k1, 2				#13: chop top 2 bits (part 2b)
	mtc0	k1, MIPS_COP_0_TLB_LO1		#14: lo1 is loaded
	nop					#15: standard nop
	nop					#16: extra nop for QED5230
	tlbwr					#17: write to tlb
	nop					#18: standard nop
	nop					#19: needed by R4000/4400
	nop					#1a: needed by R4000/4400
	eret					#1b: return from exception
4:	j _C_LABEL(MIPSX(TLBMissException))	#1c: kernel exception
	nop					#1d: branch delay slot
5:	j	slowfault			#1e: no page table present
	nop					#1f: branch delay slot
	.set	at
_VECTOR_END(MIPSX(XTLBMiss))
#endif /* USE_64BIT_CP0_FUNCTIONS */

/*
 * Vector to real handler in KSEG1.
 */
VECTOR(MIPSX(cache), unknown)
	PTR_LA	k0, _C_LABEL(MIPSX(cacheException))
	li	k1, MIPS_PHYS_MASK
	and	k0, k1
	li	k1, MIPS_KSEG1_START
	or	k0, k1
	j	k0
	nop
_VECTOR_END(MIPSX(cache))

/*
 * Handle MIPS32/MIPS64 style interrupt exception vector.
 */
VECTOR(MIPSX(intr), unknown)
	PTR_LA	k0, MIPSX(KernIntr)
	j	k0
	nop
_VECTOR_END(MIPSX(intr))

/*
 *----------------------------------------------------------------------------
 *
 * mipsN_exception --
 *
 *	Vector code for the general exception vector 0x80000180
 *	on an r4000 or r4400.
 *
 * This code is copied to the general exception vector address to
 * handle most exceptions.
 * NOTE: This code should be relocatable and max 32 instructions!!!
 *----------------------------------------------------------------------------
 */
VECTOR(MIPSX(exception), unknown)
/*
 * Find out what mode we came from and jump to the proper handler.
 */
	.set	noat
	mfc0	k0, MIPS_COP_0_STATUS		#00: get the status register
	mfc0	k1, MIPS_COP_0_CAUSE		#01: get the cause register
	and	k0, k0, MIPS3_SR_KSU_USER	#02: test for user mode
						#    sneaky but the bits are
						#    with us........
	sll	k0, k0, 3			#03: shift user bit for cause index
	and	k1, k1, MIPS3_CR_EXC_CODE	#04: mask out the cause bits.
	or	k1, k1, k0			#05: change index to user table
1:
	PTR_LA	k0, MIPSX(excpt_sw)		#06: get base of the jump table
	addu	k0, k0, k1			#07: get the address of the
						#     function entry.  Note that
						#     the cause is already
						#     shifted left by 2 bits so
						#     we dont have to shift.
	PTR_L	k0, 0(k0)			#08: get the function address
	#nop					#    -slip-

	j	k0				#09: jump to the function
	nop					#0a: branch delay slot
	.set	at
_VECTOR_END(MIPSX(exception))

/*----------------------------------------------------------------------------
 *
 * slowfault --
 *
 * Alternate entry point into the mips3_UserGenException or
 * mips3_KernGenException, when the ULTB miss handler couldn't
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
slowfault:
	.set	noat
	mfc0	k0, MIPS_COP_0_STATUS
	nop
	and	k0, k0, MIPS3_SR_KSU_USER
	bne	k0, zero, _C_LABEL(MIPSX(UserGenException))
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
NESTED_NOPROFILE(MIPSX(KernGenException), KERNFRAME_SIZ, ra)
	.set	noat
	.mask	0x80000000, -4
#if defined(DDB) || defined(KGDB)
	PTR_LA	k0, _C_LABEL(kdbaux)
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
	PTR_SUBU sp, sp, KERNFRAME_SIZ
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
	REG_S	t8, TF_BASE+TF_REG_T8(sp)
	REG_S	t9, TF_BASE+TF_REG_T9(sp)
	REG_S	ra, TF_BASE+TF_REG_RA(sp)
	REG_S	a0, TF_BASE+TF_REG_SR(sp)
	_MFC0	a3, MIPS_COP_0_EXC_PC		# 4th arg is exception PC
	REG_S	v0, TF_BASE+TF_REG_MULLO(sp)
	REG_S	v1, TF_BASE+TF_REG_MULHI(sp)
	REG_S	a3, TF_BASE+TF_REG_EPC(sp)
#if defined(__mips_o32) || defined(__mips_o64)
	PTR_ADDU v0, sp, TF_BASE
	REG_S	v0, KERNFRAME_ARG5(sp)		# 5th arg is p. to trapframe
#endif
#if defined(__mips_n32) || defined(__mips_n64)
	PTR_ADDU a4, sp, TF_BASE		# 5th arg is p. to trapframe
#endif
#ifdef IPL_ICU_MASK
	.set at
	INT_L	v0, _C_LABEL(md_imask)
	INT_S	v0, TF_BASE+TF_PPL(sp)
	nop
	.set noat
#endif
/*
 * Call the trap handler.
 */
#if defined(DDB) || defined(DEBUG) || defined(KGDB)
	PTR_ADDU v0, sp, KERNFRAME_SIZ
	REG_S	v0, KERNFRAME_SP(sp)
#endif
	mtc0	zero, MIPS_COP_0_STATUS		# Set kernel no error level
	COP0_SYNC
	nop
	nop
	nop
	jal	_C_LABEL(trap)			#
	REG_S	a3, KERNFRAME_RA(sp)		# for debugging

/*
 * Restore registers and return from the exception.
 */
	mtc0	zero, MIPS_COP_0_STATUS		# Make sure int disabled
	COP0_SYNC
	nop					# 3 nop delay
	nop
	nop
#ifdef IPL_ICU_MASK
	.set at
	INT_L	a0, TF_BASE+TF_PPL(sp)
	INT_S	a0, _C_LABEL(md_imask)
	jal	_C_LABEL(md_imask_update)
	nop
	.set noat
#endif
	REG_L	a0, TF_BASE+TF_REG_SR(sp)	# ??? why differs ???
	REG_L	t0, TF_BASE+TF_REG_MULLO(sp)
	REG_L	t1, TF_BASE+TF_REG_MULHI(sp)
	REG_L	k1, TF_BASE+TF_REG_EPC(sp)	# might be changed inside trap
	mtc0	a0, MIPS_COP_0_STATUS		# restore the SR, disable intrs
	COP0_SYNC
	mtlo	t0
	mthi	t1

#ifdef notyet
	/* Check for restartable sequences. */
	lui	t0, %hi(_C_LABEL(_lock_ras_start))
	ori	t0, zero, %lo(_C_LABEL(_lock_ras_start))
	li	t1, -MIPS_LOCK_RAS_SIZE
	and	t1, t1, k1
	bne	t1, t0, 1f
	jal	_C_LABEL(_lock_ras)
	nop
#endif

1:	_MTC0	k1, MIPS_COP_0_EXC_PC		# set return address
	COP0_SYNC
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
	REG_L	t8, TF_BASE+TF_REG_T8(sp)
	REG_L	t9, TF_BASE+TF_REG_T9(sp)
	REG_L	ra, TF_BASE+TF_REG_RA(sp)
	PTR_ADDU sp, sp, KERNFRAME_SIZ
#ifdef DDBnotyet
	PTR_LA	k0, _C_LABEL(kdbaux)
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
END(MIPSX(KernGenException))

/*
 * mipsN_UserGenException
 *
 * Handle an exception from user mode.
 * Save user context atop the kernel stack, then call trap() to process
 * the condition.  The context can be manipulated alternatively via
 * curlwp->p_md.md_regs.
 */
NESTED_NOPROFILE(MIPSX(UserGenException), CALLFRAME_SIZ, ra)
	.set	noat
	.mask	0x80000000, -4
/*
 * Save all of the registers except for the kernel temporaries in u_pcb.
 */
	PTR_L	k1, CPUVAR(CURLWP)
	PTR_L	k1, L_ADDR(k1)
	PTR_ADDU k1, k1, USPACE - FRAME_SIZ
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
	REG_S	ta0, FRAME_TA0(k1)
	REG_S	ta1, FRAME_TA1(k1)
	REG_S	ta2, FRAME_TA2(k1)
	REG_S	ta3, FRAME_TA3(k1)
	mfc0	a1, MIPS_COP_0_CAUSE		# 2nd arg is CAUSE
	REG_S	s0, FRAME_S0(k1)
	REG_S	s1, FRAME_S1(k1)
	REG_S	s2, FRAME_S2(k1)
	REG_S	s3, FRAME_S3(k1)
	_MFC0	a2, MIPS_COP_0_BAD_VADDR	# 3rd arg is fault address
	REG_S	s4, FRAME_S4(k1)
	REG_S	s5, FRAME_S5(k1)
	REG_S	s6, FRAME_S6(k1)
	REG_S	s7, FRAME_S7(k1)
	_MFC0	a3, MIPS_COP_0_EXC_PC		# 4th arg is exception PC
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
#ifdef IPL_ICU_MASK
	.set at
	INT_L	t0, _C_LABEL(md_imask)
	INT_S	t0, FRAME_PPL(k1)
	.set noat
#endif
	PTR_ADDU sp, k1, -CALLFRAME_SIZ		# switch to kernel SP
#ifdef __GP_SUPPORT__
	PTR_LA	gp, _C_LABEL(_gp)		# switch to kernel GP
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
	PTR_L	MIPS_CURLWP, CPUVAR(CURLWP)
	mtc0	t0, MIPS_COP_0_STATUS
	COP0_SYNC
	jal	_C_LABEL(trap)
	REG_S	a3, CALLFRAME_RA(sp)		# for debugging
/*
 * Check pending asynchronous traps.
 */
	INT_L	t0, L_MD_ASTPENDING(MIPS_CURLWP)
	beq	t0, zero, 1f
	nop
/*
 * We have pending asynchronous traps; all the state is already saved.
 */
	jal	_C_LABEL(ast)
	REG_L	a0, CALLFRAME_SIZ + FRAME_EPC(sp)
1:
/*
 * Restore user registers and return.
 * First disable interrupts and set exception level.
 */
	mtc0	zero, MIPS_COP_0_STATUS		# disable interrupt
	COP0_SYNC
	nop					# 3 clock delay before
	nop					# exceptions blocked
	nop					# for R4X
	li	v0, MIPS_SR_EXL
	mtc0	v0, MIPS_COP_0_STATUS		# set exception level
	COP0_SYNC
	nop					# 3 nop delay
	nop
	nop
	PTR_ADDU a1, sp, CALLFRAME_SIZ
#ifdef IPL_ICU_MASK
	.set at
	INT_L	t0, FRAME_PPL(a1)
	INT_S	t0, _C_LABEL(md_imask)
	jal	_C_LABEL(md_imask_update)
	nop
	PTR_ADDU a1, sp, CALLFRAME_SIZ
	.set noat
#endif
 #	REG_L	a0, FRAME_SR(a1)
	REG_L	t0, FRAME_MULLO(a1)
	REG_L	t1, FRAME_MULHI(a1)
	REG_L	v0, FRAME_EPC(a1)
 #	mtc0	a0, MIPS_COP_0_STATUS		# still exception level
	mtlo	t0
	mthi	t1
	_MTC0	v0, MIPS_COP_0_EXC_PC		# set return address
	COP0_SYNC
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
	REG_L	ta0, FRAME_TA0(k1)
	REG_L	ta1, FRAME_TA1(k1)
	REG_L	ta2, FRAME_TA2(k1)
	REG_L	ta3, FRAME_TA3(k1)
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
	DYNAMIC_STATUS_MASK_TOUSER(k0, ra)	# machine dependent masking
	REG_L	gp, FRAME_GP(k1)
	REG_L	sp, FRAME_SP(k1)
	REG_L	s8, FRAME_S8(k1)
	REG_L	ra, FRAME_RA(k1)
	mtc0	k0, MIPS_COP_0_STATUS		# restore status
	COP0_SYNC
	nop
	nop
	eret					# return to interrupted point
	.set	at
END(MIPSX(UserGenException))

/*
 * mipsN_SystemCall
 *
 * Save user context in u_pcb, then call syscall() to process a system call.
 * The context can be manipulated alternatively via curlwp->p_md.md_regs;
 */
NESTED_NOPROFILE(MIPSX(SystemCall), CALLFRAME_SIZ, ra)
	.set	noat
	.mask	0x80000000, -4
	PTR_L	k1, CPUVAR(CURLWP)
	PTR_L	k1, L_ADDR(k1)
	#nop					# -slip-
	PTR_ADDU k1, k1, USPACE - FRAME_SIZ
	#REG_S	AT, FRAME_AST(k1)
	REG_S	v0, FRAME_V0(k1)		# syscall #
	REG_S	v1, FRAME_V1(k1)		# used by syscall()
	mflo	v0
	REG_S	a0, FRAME_A0(k1)
	REG_S	a1, FRAME_A1(k1)
	REG_S	a2, FRAME_A2(k1)
	REG_S	a3, FRAME_A3(k1)
	PTR_L	a0, CPUVAR(CURLWP)		# 1st arg is curlwp
	mfhi	v1
	#REG_S	t0, FRAME_T0(k1)		# no need to save temp regs
	#REG_S	t1, FRAME_T1(k1)
	#REG_S	t2, FRAME_T2(k1)
	#REG_S	t3, FRAME_T3(k1)
	mfc0	a1, MIPS_COP_0_STATUS		# 2nd arg is STATUS
	#REG_S	ta0, FRAME_TA0(k1)
	#REG_S	ta1, FRAME_TA1(k1)
	#REG_S	ta2, FRAME_TA2(k1)
	#REG_S	ta3, FRAME_TA3(k1)
	mfc0	a2, MIPS_COP_0_CAUSE		# 3rd arg is CAUSE
	REG_S	s0, FRAME_S0(k1)
	REG_S	s1, FRAME_S1(k1)
	REG_S	s2, FRAME_S2(k1)
	REG_S	s3, FRAME_S3(k1)
	_MFC0	a3, MIPS_COP_0_EXC_PC		# 4th arg is PC
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
	REG_S	a1, FRAME_SR(k1)
	REG_S	v0, FRAME_MULLO(k1)
	REG_S	v1, FRAME_MULHI(k1)
	REG_S	a3, FRAME_EPC(k1)
#ifdef IPL_ICU_MASK
	.set at
	INT_L	t0, _C_LABEL(md_imask)
	INT_S	t0, FRAME_PPL(k1)
	.set noat
#endif
	PTR_L	t0, L_PROC(a0)			# curlwp->l_proc (used below)
	move	MIPS_CURLWP, a0			# set curlwp reg
	PTR_ADDU sp, k1, -CALLFRAME_SIZ
#ifdef __GP_SUPPORT__
	PTR_LA	gp, _C_LABEL(_gp)		# switch to kernel GP
#endif
/*
 * Turn off fpu and enter kernel mode
 */
	.set	at
	PTR_L	t1, P_MD_SYSCALL(t0)		# t1 = syscall
	and	t0, a1, ~(MIPS_SR_COP_1_BIT | MIPS_SR_EXL | MIPS_SR_KSU_MASK)
	.set	noat
#if defined(DDB) || defined(DEBUG) || defined(KGDB)
	move	ra, a3
	REG_S	ra, CALLFRAME_RA(sp)
#endif
/*
 * Call the system call handler.
 */
	mtc0	t0, MIPS_COP_0_STATUS		# re-enable interrupts
	COP0_SYNC
	jal	t1
	nop
/*
 * Check pending asynchronous traps.
 */
	INT_L	t0, L_MD_ASTPENDING(MIPS_CURLWP)
	beq	t0, zero, 1f
	nop
/*
 * We have pending asynchronous traps; all the state is already saved.
 */
	jal	_C_LABEL(ast)
	PTR_L	a0, CALLFRAME_SIZ + FRAME_EPC(sp)
1:
/*
 * Restore user registers and return.
 * First disable interrupts and set exception level.
 */
	mtc0	zero, MIPS_COP_0_STATUS		# disable int
	COP0_SYNC
	nop					# 3 op delay
	nop
	nop

	li	v0, MIPS_SR_EXL
	mtc0	v0, MIPS_COP_0_STATUS		# set exception level
	COP0_SYNC
	nop					# 3 op delay
	nop
	nop
/*
 * Restore user registers and return.
 */
	PTR_ADDU a1, sp, CALLFRAME_SIZ
#ifdef IPL_ICU_MASK
	.set at
	INT_L	t0, FRAME_PPL(a1)
	INT_S	t0, _C_LABEL(md_imask)
	jal	_C_LABEL(md_imask_update)
	nop
	PTR_ADDU a1, sp, CALLFRAME_SIZ
	.set noat
#endif
 #	REG_L	a0, FRAME_SR(a1)
	REG_L	t0, FRAME_MULLO(a1)
	REG_L	t1, FRAME_MULHI(a1)
	REG_L	v0, FRAME_EPC(a1)		# might be changed in syscall
 #	mtc0	a0, MIPS_COP_0_STATUS		# this should disable interrupts
	mtlo	t0
	mthi	t1
	_MTC0	v0, MIPS_COP_0_EXC_PC		# set return address
	COP0_SYNC
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
	REG_L	ta0, FRAME_TA0(k1)
	REG_L	ta1, FRAME_TA1(k1)
	REG_L	ta2, FRAME_TA2(k1)
	REG_L	ta3, FRAME_TA3(k1)
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
	DYNAMIC_STATUS_MASK_TOUSER(k0, ra)	# machine dependent masking
	REG_L	gp, FRAME_GP(k1)
	REG_L	sp, FRAME_SP(k1)
	REG_L	s8, FRAME_S8(k1)
	REG_L	ra, FRAME_RA(k1)
	mtc0	k0, MIPS_COP_0_STATUS
	COP0_SYNC
	nop
	nop
	nop

	eret					# return to syscall point
	.set	at
END(MIPSX(SystemCall))

/*
 * Panic on cache errors.  A lot more could be done to recover
 * from some types of errors but it is tricky.
 */
NESTED_NOPROFILE(MIPSX(cacheException), KERNFRAME_SIZ, ra)
	.set	noat
	.mask	0x80000000, -4
#ifdef sbmips	/* XXX!  SB-1 needs a real cache error handler */
	eret
	nop
#endif
	PTR_LA	k0, panic			# return to panic
	PTR_LA	a0, 9f				# panicstr
	_MFC0	a1, MIPS_COP_0_ERROR_PC
	mfc0	a2, MIPS_COP_0_ECC
	mfc0	a3, MIPS_COP_0_CACHE_ERR

	_MTC0	k0, MIPS_COP_0_ERROR_PC		# set return address
	COP0_SYNC

	mfc0	k0, MIPS_COP_0_STATUS		# restore status
	li	k1, MIPS3_SR_DIAG_PE		# ignore further errors
	or	k0, k1
	mtc0	k0, MIPS_COP_0_STATUS		# restore status
	COP0_SYNC
	nop
	nop
	nop

	eret

	MSG("cache error @ EPC 0x%x ErrCtl 0x%x CacheErr 0x%x");
	.set	at
END(MIPSX(cacheException))

/*
 * mipsX_KernIntr
 *
 * Handle an interrupt from kernel mode.
 * Build intrframe on stack to hold interrupted kernel context, then
 * call cpu_intr() to process it.
 *
 */
NESTED_NOPROFILE(MIPSX(KernIntr), KERNFRAME_SIZ, ra)
	.set	noat
	.mask	0x80000000, -4
	PTR_SUBU sp, sp, KERNFRAME_SIZ
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
	REG_S	ta0, TF_BASE+TF_REG_TA0(sp)
	REG_S	ta1, TF_BASE+TF_REG_TA1(sp)
	REG_S	ta2, TF_BASE+TF_REG_TA2(sp)
	REG_S	ta3, TF_BASE+TF_REG_TA3(sp)
	_MFC0	a2, MIPS_COP_0_EXC_PC		# 3rd arg is exception PC
	REG_S	t8, TF_BASE+TF_REG_T8(sp)
	REG_S	t9, TF_BASE+TF_REG_T9(sp)
	REG_S	ra, TF_BASE+TF_REG_RA(sp)
	REG_S	a0, TF_BASE+TF_REG_SR(sp)
	REG_S	v0, TF_BASE+TF_REG_MULLO(sp)
	REG_S	v1, TF_BASE+TF_REG_MULHI(sp)
	REG_S	a2, TF_BASE+TF_REG_EPC(sp)
        REG_S   MIPS_CURLWP, TF_BASE+TF_PAD(sp) # XXX Atheros HAL
/*
 * Call the interrupt handler.
 */
#if defined(DDB) || defined(DEBUG) || defined(KGDB)
	move	ra, a2
	REG_S	ra, KERNFRAME_RA(sp)		# for debugging
#endif
#ifdef IPL_ICU_MASK
	.set at
	INT_L	t0, _C_LABEL(md_imask)
	INT_S	t0, TF_BASE+TF_PPL(sp)
	.set noat
#endif
	mtc0	zero, MIPS_COP_0_STATUS		# Reset exl, trap possible.
	COP0_SYNC
        PTR_L	MIPS_CURLWP, CPUVAR(CURLWP)     # XXX Atheros HAL
	jal	_C_LABEL(cpu_intr)
	and	a3, a0, a1			# 4th is STATUS & CAUSE
/*
 * Restore registers and return from the interrupt.
 */
	mtc0	zero, MIPS_COP_0_STATUS		# Disable interrupt
	COP0_SYNC
	nop
	nop
	nop
#ifdef IPL_ICU_MASK
	.set at
	INT_L	a0, TF_BASE+TF_PPL(sp)
	INT_S	a0, _C_LABEL(md_imask)
	jal	_C_LABEL(md_imask_update)
	nop
	.set noat
#endif
	REG_L	MIPS_CURLWP, TF_BASE+TF_PAD(sp)	# XXX Atheros HAL
	REG_L	a0, TF_BASE+TF_REG_SR(sp)	# ??? why differs ???
	DYNAMIC_STATUS_MASK(a0, t0)		# machine dependent masking
	REG_L	t0, TF_BASE+TF_REG_MULLO(sp)
	REG_L	t1, TF_BASE+TF_REG_MULHI(sp)
	REG_L	v0, TF_BASE+TF_REG_EPC(sp)
	mtc0	a0, MIPS_COP_0_STATUS		# restore the SR, disable intrs
	COP0_SYNC
	mtlo	t0
	mthi	t1

#ifdef notyet
	/* Check for restartable sequences. */
	lui	t0, %hi(_C_LABEL(_lock_ras_start))
	ori	t0, zero, %lo(_C_LABEL(_lock_ras_start))
	li	t1, -MIPS_LOCK_RAS_SIZE
	and	t1, t1, v0
	bne	t1, t0, 1f
	move	k1, v0
	jal	_C_LABEL(_lock_ras)
	nop
	mov	v0, k1
#endif

1:	_MTC0	v0, MIPS_COP_0_EXC_PC		# set return address
	COP0_SYNC

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
	REG_L	t8, TF_BASE+TF_REG_T8(sp)
	REG_L	t9, TF_BASE+TF_REG_T9(sp)
	REG_L	ra, TF_BASE+TF_REG_RA(sp)
	PTR_ADDU sp, sp, KERNFRAME_SIZ		# restore kernel SP
	eret					# return to interrupted point
	.set	at
END(MIPSX(KernIntr))

/*----------------------------------------------------------------------------
 * XXX this comment block should be updated XXX
 * mipsN_UserIntr --
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
NESTED_NOPROFILE(MIPSX(UserIntr), CALLFRAME_SIZ, ra)
	.set	noat
	.mask	0x80000000, -4
/*
 * Save the relevant user registers into the u_pcb.
 * We don't need to save s0 - s8 because the compiler does it for us.
 */
	PTR_L	k1, CPUVAR(CURLWP)
	PTR_L	k1, L_ADDR(k1)
	PTR_ADDU k1, k1, USPACE - FRAME_SIZ
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
	REG_S	ta0, FRAME_TA0(k1)
	REG_S	ta1, FRAME_TA1(k1)
	REG_S	ta2, FRAME_TA2(k1)
	REG_S	ta3, FRAME_TA3(k1)
	mfc0	a1, MIPS_COP_0_CAUSE		# 2nd arg is CAUSE
	REG_S	t8, FRAME_T8(k1)
	REG_S	t9, FRAME_T9(k1)
	REG_S	gp, FRAME_GP(k1)
	REG_S	sp, FRAME_SP(k1)
	_MFC0	a2, MIPS_COP_0_EXC_PC		# 3rd arg is PC
	REG_S	ra, FRAME_RA(k1)
	REG_S	a0, FRAME_SR(k1)
	REG_S	v0, FRAME_MULLO(k1)
	REG_S	v1, FRAME_MULHI(k1)
	REG_S	a2, FRAME_EPC(k1)
#ifdef IPL_ICU_MASK
	.set at
	INT_L	t0, _C_LABEL(md_imask)
	INT_S	t0, FRAME_PPL(k1)
	.set noat
#endif
	PTR_ADDU sp, k1, -CALLFRAME_SIZ		# switch to kernel SP
#ifdef __GP_SUPPORT__
	PTR_LA	gp, _C_LABEL(_gp)		# switch to kernel GP
#endif
	PTR_S	MIPS_CURLWP, MIPS_CURLWP_FRAME(k1)# save curlwp reg
	PTR_L	MIPS_CURLWP, CPUVAR(CURLWP)	# set curlwp reg
/*
 * Turn off fpu and enter kernel mode
 */
	.set	at
	and	t0, a0, ~(MIPS_SR_COP_1_BIT | MIPS_SR_EXL | MIPS_SR_INT_IE | MIPS_SR_KSU_MASK)
	.set	noat
#if defined(DDB) || defined(DEBUG) || defined(KGDB)
	move	ra, a2
	REG_S	ra, CALLFRAME_RA(sp)
#endif
/*
 * Call the interrupt handler.
 */
	mtc0	t0, MIPS_COP_0_STATUS
	COP0_SYNC
	jal	_C_LABEL(cpu_intr)
	and	a3, a0, a1			# 4th is STATUS & CAUSE
/*
 * Restore registers and return from the interrupt.
 */
	nop
	mtc0	zero, MIPS_COP_0_STATUS
	COP0_SYNC
	nop					# 3 nop hazard
	nop
	nop
	li	v0, MIPS_SR_EXL
	mtc0	v0, MIPS_COP_0_STATUS		# set exception level bit.
	COP0_SYNC
	nop					# 3 nop hazard
	nop
	nop
	PTR_ADDU a1, sp, CALLFRAME_SIZ
 #	REG_L	a0, FRAME_SR(a1)
	INT_L	v0, L_MD_ASTPENDING(MIPS_CURLWP)# any pending ast?
 #	mtc0	a0, MIPS_COP_0_STATUS		# restore the SR, disable intrs
	nop
/*
 * Check pending asynchronous traps.
 */
	beq	v0, zero, 1f			# if no, skip ast processing
	PTR_L	MIPS_CURLWP, MIPS_CURLWP_FRAME(a1)# restore curlwp reg
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
	REG_L	a0, FRAME_EPC(a1)		# argument is interrupted PC
	PTR_L	MIPS_CURLWP, CPUVAR(CURLWP)	# set curlwp reg
#ifdef IPL_ICU_MASK
	jal	_C_LABEL(spllowersofthigh);
	nop
#else
	li	t0, MIPS_HARD_INT_MASK | MIPS_SR_INT_IE
	DYNAMIC_STATUS_MASK(t0, t1)		# machine dependent masking
	mtc0	t0, MIPS_COP_0_STATUS		# enable interrupts (spl0)
	COP0_SYNC
#endif
	jal	_C_LABEL(ast)
	nop
/*
 * Restore user registers and return. NOTE: interrupts are enabled.
 */
	mtc0	zero, MIPS_COP_0_STATUS
	COP0_SYNC
	nop					# 3 nop delay
	nop
	nop
	li	v0, MIPS_SR_EXL
	mtc0	v0, MIPS_COP_0_STATUS		# set exception level bit.
	COP0_SYNC
	nop					# 3 nop delay
	nop
	nop

	PTR_ADDU a1, sp, CALLFRAME_SIZ
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
	_MTC0	v0, MIPS_COP_0_EXC_PC		# set return address
	COP0_SYNC
	nop					# ??? how much delay ???
	nop

	move	k1, a1
#ifdef IPL_ICU_MASK
	.set at
	INT_L	t0, FRAME_PPL(k1)
	INT_S	t0, _C_LABEL(md_imask)
	jal	_C_LABEL(md_imask_update)
	nop
	.set noat
#endif
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
	REG_L	ta0, FRAME_TA0(k1)
	REG_L	ta1, FRAME_TA1(k1)
	REG_L	ta2, FRAME_TA2(k1)
	REG_L	ta3, FRAME_TA3(k1)
	REG_L	t8, FRAME_T8(k1)
	REG_L	t9, FRAME_T9(k1)
	REG_L	k0, FRAME_SR(k1)
	DYNAMIC_STATUS_MASK_TOUSER(k0, ra)	# machine dependent masking
	REG_L	gp, FRAME_GP(k1)
	REG_L	sp, FRAME_SP(k1)
	REG_L	ra, FRAME_RA(k1)
	mtc0	k0, MIPS_COP_0_STATUS		# restore the SR
	COP0_SYNC
	nop					# required for QED 5230
	nop
	eret					# return to interrupted point
	.set	at
END(MIPSX(UserIntr))


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
LEAF_NOPROFILE(MIPSX(TLBInvalidException))
	.set	noat
	_MFC0	k0, MIPS_COP_0_BAD_VADDR	# get the fault address
	li	k1, VM_MIN_KERNEL_ADDRESS	# compute index
	bgez	k0, _C_LABEL(MIPSX(KernGenException))	# full trap processing
	subu	k0, k0, k1
	INT_L	k1, _C_LABEL(Sysmapsize)	# index within range?
	srl	k0, k0, PGSHIFT
	sltu	k1, k0, k1
	beq	k1, zero, outofworld		# No. Failing beyond. . .
	nop					# - delay slot -
	PTR_L	k1, _C_LABEL(Sysmap)

	sll	k0, k0, 2			# compute offset from index
	addu	k1, k1, k0
	tlbp					# Probe the invalid entry
	COP0_SYNC
	and	k0, k0, 4			# check even/odd page
	nop					# required for QED 5230
	bne	k0, zero, KernTLBIOdd
	nop

	mfc0	k0, MIPS_COP_0_TLB_INDEX
	nop
	bltz	k0, outofworld			# ASSERT(TLB entry exists)
	INT_L	k0, 0(k1)			# get PTE entry

	_SLL	k0, k0, WIRED_SHIFT		# get rid of "wired" bit
	_SRL	k0, k0, WIRED_SHIFT
	mtc0	k0, MIPS_COP_0_TLB_LO0		# load PTE entry
	COP0_SYNC
	and	k0, k0, MIPS3_PG_V		# check for valid entry
	nop					# required for QED5230
	beq	k0, zero, _C_LABEL(MIPSX(KernGenException))	# PTE invalid
	INT_L	k0, 4(k1)			# get odd PTE entry
	_SLL	k0, k0, WIRED_SHIFT
	mfc0	k1, MIPS_COP_0_TLB_INDEX
	_SRL	k0, k0, WIRED_SHIFT
	sltiu	k1, k1, MIPS3_TLB_WIRED_UPAGES	# Luckily this is MIPS3_PG_G
	or	k1, k1, k0
	_MTC0	k0, MIPS_COP_0_TLB_LO1		# load PTE entry
	COP0_SYNC
	nop
	nop					# required for QED5230
	tlbwi					# write TLB
	COP0_SYNC
	nop
	nop
	nop
	nop
	nop
	eret

KernTLBIOdd:
	mfc0	k0, MIPS_COP_0_TLB_INDEX
	nop
	bltz	k0, outofworld			# assert(TLB Entry exists)
	INT_L	k0, 0(k1)			# get PTE entry

	_SLL	k0, k0, WIRED_SHIFT		# get rid of wired bit
	_SRL	k0, k0, WIRED_SHIFT
	_MTC0	k0, MIPS_COP_0_TLB_LO1		# save PTE entry
	COP0_SYNC
	and	k0, k0, MIPS3_PG_V		# check for valid entry
	nop					# required for QED5230
	beq	k0, zero, _C_LABEL(MIPSX(KernGenException))	# PTE invalid
	INT_L	k0, -4(k1)			# get even PTE entry
	_SLL	k0, k0, WIRED_SHIFT
	mfc0	k1, MIPS_COP_0_TLB_INDEX
	_SRL	k0, k0, WIRED_SHIFT
	sltiu	k1, k1, MIPS3_TLB_WIRED_UPAGES	# Luckily this is MIPS3_PG_G
	or	k1, k1, k0
	_MTC0	k0, MIPS_COP_0_TLB_LO0		# save PTE entry
	COP0_SYNC
	nop
	nop					# required for QED5230
	tlbwi					# update TLB
	COP0_SYNC
	nop
	nop
	nop
	nop
	nop
	eret
END(MIPSX(TLBInvalidException))

/*----------------------------------------------------------------------------
 *
 * mipsN_TLBMissException --
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
LEAF_NOPROFILE(MIPSX(TLBMissException))
	.set	noat
	_MFC0	k0, MIPS_COP_0_BAD_VADDR	# get the fault address
	li	k1, VM_MIN_KERNEL_ADDRESS	# compute index
	PTR_SUBU k0, k0, k1
	INT_L	k1, _C_LABEL(Sysmapsize)	# index within range?
	PTR_SRL	k0, k0, PGSHIFT
	sltu	k1, k0, k1
#ifdef newsmips
	/* news5000 has ROM work area at 0xfff00000. */
	bne	k1, zero, 1f
	nop
	j	checkromwork
	nop					# - delay slot -
1:
#else
	beq	k1, zero, outofworld		# No. Failing beyond. . .
	nop					# - delay slot -
#endif
	PTR_L	k1, _C_LABEL(Sysmap)
	PTR_SRL	k0, k0, 1
	PTR_SLL	k0, k0, 3			# compute offset from index
	PTR_ADDU k1, k1, k0
	INT_L	k0, 0(k1)			# get PTE entry
	INT_L	k1, 4(k1)			# get odd PTE entry
	_SLL	k0, k0, WIRED_SHIFT		# get rid of "wired" bit
	_SRL	k0, k0, WIRED_SHIFT
	_MTC0	k0, MIPS_COP_0_TLB_LO0		# load PTE entry
	COP0_SYNC
	_SLL	k1, k1, WIRED_SHIFT
	_SRL	k1, k1, WIRED_SHIFT
	_MTC0	k1, MIPS_COP_0_TLB_LO1		# load PTE entry
	COP0_SYNC
	nop
	nop					# required for QED5230
	tlbwr					# write TLB
	COP0_SYNC
	nop
	nop
	nop
	nop
	nop
	eret

outofworld:
	/* eret to panic so shutdown can use K2.  Try to ensure valid $sp. */
	PTR_LA	a0, _C_LABEL(panic)
	_MFC0	a2, MIPS_COP_0_EXC_PC
	move	a1, sp
	PTR_SLL	k0, k0, PGSHIFT
	_MTC0	a0, MIPS_COP_0_EXC_PC		# return to panic
	COP0_SYNC
	li	k1, VM_MIN_KERNEL_ADDRESS
	PTR_ADDU a3, k0, k1
#if defined(DDB)
	bltz	sp, 1f				# for ddb try to keep frame
	nop
#endif
	PTR_LA	sp, start			# set sp to a valid place
1:	PTR_LA	a0, 9f				# string
	eret

	.set	at
END(MIPSX(TLBMissException))

	MSG("TLB out of universe: ksp %p epc %p vaddr %p")

/*
 * Mark where code entered from exception hander jumptable
 * ends, for stack traceback code.
 */

	.globl	_C_LABEL(MIPSX(exceptionentry_end))
_C_LABEL(MIPSX(exceptionentry_end)):

/*--------------------------------------------------------------------------
 *
 * mipsN_SetPID --
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
LEAF(MIPSX(SetPID))
	_MTC0	a0, MIPS_COP_0_TLB_HI		# Write the hi reg value
	COP0_SYNC
	/* XXX simonb: lose these nops for mips32/64? */
	nop					# required for QED5230
	nop					# required for QED5230
	j	ra
	nop
END(MIPSX(SetPID))

#if defined(ENABLE_MIPS3_WIRED_MAP)
/*--------------------------------------------------------------------------
 *
 * mipsN_TLBWriteIndexedVPS --
 *
 *      Write the given entry into the TLB at the given index.
 *      Pass full R4000 style TLB info including variable page size mask.
 *
 *      mipsN_TLBWriteIndexed(unsigned int index, struct tlb *tlb)
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      TLB entry set.
 *
 *--------------------------------------------------------------------------
 */
LEAF(MIPSX(TLBWriteIndexedVPS))
	mfc0	v1, MIPS_COP_0_STATUS		# Save the status register.
	mtc0	zero, MIPS_COP_0_STATUS		# Disable interrupts
	COP0_SYNC
	nop
	lw	a2, 8(a1)			# fetch tlb->tlb_lo0
	lw	a3, 12(a1)			# fetch tlb->tlb_lo1
	mfc0	v0, MIPS_COP_0_TLB_PG_MASK	# Save current page mask.
	_MFC0	t0, MIPS_COP_0_TLB_HI		# Save the current PID.

	_MTC0	a2, MIPS_COP_0_TLB_LO0		# Set up entry low0.
	COP0_SYNC
	_MTC0	a3, MIPS_COP_0_TLB_LO1		# Set up entry low1.
	COP0_SYNC
	nop
	lw	a2, 0(a1)			# fetch tlb->tlb_mask
	lw	a3, 4(a1)			# fetch tlb->tlb_hi
	nop
	mtc0	a0, MIPS_COP_0_TLB_INDEX	# Set the index.
	COP0_SYNC
	mtc0	a2, MIPS_COP_0_TLB_PG_MASK	# Set up entry pagemask.
	COP0_SYNC
	_MTC0	a3, MIPS_COP_0_TLB_HI		# Set up entry high.
	COP0_SYNC
	nop
	nop
	tlbwi					# Write the TLB
	COP0_SYNC
	nop
	nop
	nop					# Delay for effect
	nop

	_MTC0	t0, MIPS_COP_0_TLB_HI		# Restore the PID.
	COP0_SYNC
	mtc0	v0, MIPS_COP_0_TLB_PG_MASK	# Restore page mask.
	COP0_SYNC
	nop
	nop
	j       ra
	mtc0	v1, MIPS_COP_0_STATUS		# Restore the status register
END(MIPSX(TLBWriteIndexedVPS))
#endif /* ENABLE_MIPS3_WIRED_MAP */
	
/*--------------------------------------------------------------------------
 *
 * mipsN_TLBUpdate --
 *
 *	Update the TLB if highreg is found; otherwise do nothing.
 *
 *	mips3_TLBUpdate(virpageadr, lowregx)
 *		unsigned virpageadr, lowregx;
 *
 * Results:
 *	< 0 if skipped, >= 0 if updated.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------------------
 */
LEAF(MIPSX(TLBUpdate))
	mfc0	v1, MIPS_COP_0_STATUS	# Save the status register.
	mtc0	zero, MIPS_COP_0_STATUS	# Disable interrupts
	COP0_SYNC
	and	t1, a0, MIPS3_PG_ODDPG	# t1 = Even/Odd flag
	li	v0, (MIPS3_PG_HVPN | MIPS3_PG_ASID)
	and	a0, a0, v0
	_MFC0	t0, MIPS_COP_0_TLB_HI		# Save current PID
	_MTC0	a0, MIPS_COP_0_TLB_HI		# Init high reg
	COP0_SYNC
	and	a2, a1, MIPS3_PG_G		# Copy global bit
	nop
	nop
	tlbp					# Probe for the entry.
	COP0_SYNC
	_SLL	a1, a1, WIRED_SHIFT		# Clear top 34 bits of EntryLo
	_SRL	a1, a1, WIRED_SHIFT
	bne	t1, zero, 1f			# Decide even odd
	mfc0	v0, MIPS_COP_0_TLB_INDEX	# See what we got
# EVEN
	nop
	bltz	v0, 1f				# index < 0 => !found
	nop
	nop					# required for QED5230

	tlbr					# update, read entry first
	COP0_SYNC
	nop
	nop
	nop
	_MTC0	a1, MIPS_COP_0_TLB_LO0		# init low reg0.
	COP0_SYNC
	nop
	nop					# required for QED5230
	tlbwi					# update slot found
	COP0_SYNC
	nop					# required for QED5230
	nop					# required for QED5230
	b	4f
	nop
1:
# ODD
	nop
	bltz	v0, 4f				# index < 0 => !found
	nop
	nop					# required for QED5230

	tlbr					# read the entry first
	COP0_SYNC
	nop
	nop
	nop
	_MTC0	a1, MIPS_COP_0_TLB_LO1		# init low reg1.
	COP0_SYNC
	nop
	nop					# required for QED5230
	tlbwi					# update slot found
	COP0_SYNC
	nop					# required for QED5230
	nop					# required for QED5230
	nop
4:
	nop					# Make sure pipeline
	nop					# advances before we
	nop					# use the TLB.
	nop
	_MTC0	t0, MIPS_COP_0_TLB_HI		# restore PID
	COP0_SYNC
	nop					# required for QED5230
	nop					# required for QED5230
	j	ra
	mtc0	v1, MIPS_COP_0_STATUS		# Restore the status register
	COP0_SYNC				# XXXX - not executed!!
END(MIPSX(TLBUpdate))

/*--------------------------------------------------------------------------
 *
 * mipsN_TLBRead --
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
LEAF(MIPSX(TLBRead))
	mfc0	v1, MIPS_COP_0_STATUS		# Save the status register.
	mtc0	zero, MIPS_COP_0_STATUS		# Disable interrupts
	COP0_SYNC
	nop
	mfc0	ta2, MIPS_COP_0_TLB_PG_MASK	# save current pgMask
	nop
	_MFC0	t0, MIPS_COP_0_TLB_HI		# Get current PID

	mtc0	a0, MIPS_COP_0_TLB_INDEX	# Set the index register
	COP0_SYNC
	nop
	nop					# required for QED5230
	tlbr					# Read from the TLB
	COP0_SYNC
	nop
	nop
	nop
	mfc0	t2, MIPS_COP_0_TLB_PG_MASK	# fetch the pgMask
	_MFC0	t3, MIPS_COP_0_TLB_HI		# fetch the hi entry
	_MFC0	ta0, MIPS_COP_0_TLB_LO0		# See what we got
	_MFC0	ta1, MIPS_COP_0_TLB_LO1		# See what we got
	_MTC0	t0, MIPS_COP_0_TLB_HI		# restore PID
	COP0_SYNC
	mtc0	ta2, MIPS_COP_0_TLB_PG_MASK	# restore pgMask
	COP0_SYNC
	nop
	nop
	nop					# wait for PID active
	mtc0	v1, MIPS_COP_0_STATUS		# Restore the status register
	COP0_SYNC
	nop
	sw	t2, 0(a1)
	sw	t3, 4(a1)
	sw	ta0, 8(a1)
	j	ra
	sw	ta1, 12(a1)
END(MIPSX(TLBRead))

#if defined(MIPS3) && !defined(MIPS3_5900)
/*----------------------------------------------------------------------------
 *
 * mips3_VCED --
 *
 *	Handle virtual coherency exceptions.
 *	Called directly from the mips3 execption-table code.
 *	only k0, k1 are available on entry
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
LEAF_NOPROFILE(MIPSX(VCED))
	.set	noat
	_MFC0	k0, MIPS_COP_0_BAD_VADDR	# fault addr.
	li	k1, -16
	and	k0, k1
	cache	(CACHE_R4K_SD | CACHEOP_R4K_HIT_WB_INV), 0(k0)
	cache	(CACHE_R4K_D | CACHEOP_R4K_HIT_INV), 0(k0)
#ifdef DEBUG
	_MFC0	k0, MIPS_COP_0_BAD_VADDR
	PTR_LA	k1, VCED_vaddr
	PTR_S	k0, 0(k1)
	_MFC0	k0, MIPS_COP_0_EXC_PC
	PTR_LA	k1, VCED_epc
	PTR_S	k0, 0(k1)
	PTR_LA	k1, VCED_count		# count number of exceptions
	PTR_SRL	k0, k0, 26		# position upper 4 bits of VA
	and	k0, k0, 0x3c		# mask it off
	add	k1, k0			# get address of count table
	LONG_L	k0, 0(k1)
	LONG_ADDU k0, 1
	LONG_S	k0, 0(k1)
#endif
	eret
	.set	at

#ifdef DEBUG
	.data
	.globl	_C_LABEL(VCED_count)
_C_LABEL(VCED_count):
	LONG_WORD	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	.globl	_C_LABEL(VCED_epc)
_C_LABEL(VCED_epc):
	PTR_WORD	0
	.globl	_C_LABEL(VCED_vaddr)
_C_LABEL(VCED_vaddr):
	PTR_WORD	0
	.text
#endif
END(MIPSX(VCED))

LEAF_NOPROFILE(MIPSX(VCEI))
	.set	noat
	_MFC0	k0, MIPS_COP_0_BAD_VADDR	# fault addr.
	cache	(CACHE_R4K_SD | CACHEOP_R4K_HIT_WB_INV), 0(k0)
	cache	(CACHE_R4K_I | CACHEOP_R4K_HIT_INV), 0(k0)
#ifdef DEBUG
	_MFC0	k0, MIPS_COP_0_BAD_VADDR
	PTR_LA	k1, VCEI_vaddr
	PTR_S	k0, 0(k1)
	PTR_LA	k1, VCEI_count		# count number of exceptions
	PTR_SRL	k0, k0, 26		# position upper 4 bits of VA
	and	k0, k0, 0x3c		# mask it off
	add	k1, k0			# get address of count table
	LONG_L	k0, 0(k1)
	addu	k0, 1
	LONG_S	k0, 0(k1)
#endif
	eret
	.set	at

#ifdef DEBUG
	.data
	.globl	_C_LABEL(VCEI_count)
_C_LABEL(VCEI_count):
	LONG_WORD	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	.globl	_C_LABEL(VCEI_vaddr)
_C_LABEL(VCEI_vaddr):
	PTR_WORD	0
	.text
#endif
END(MIPSX(VCEI))
#endif /* MIPS3 && !MIPS3_5900 */

/*
 * mipsN_lwp_trampoline()
 *
 * Arrange for a function to be invoked neatly, after a cpu_switch().
 * Call the service function with one argument, specified by the s0
 * and s1 respectively.  There is no need register save operation.
 */
LEAF(MIPSX(lwp_trampoline))
	PTR_ADDU sp, sp, -CALLFRAME_SIZ

	# Call lwp_startup(), with args from cpu_switchto()/cpu_setfunc()
	PTR_LA	t0, _C_LABEL(lwp_startup)
	move	a0, v0
	jal	ra, t0
	move	a1, s7

	# Call the routine specified by cpu_setfunc()
	jal	ra, s0			
	move	a0, s1

	#
	# Return to user (won't happen if a kernel thread)
	#
	# Make sure to disable interrupts here, as otherwise
	# we can take an interrupt *after* EXL is set, and
	# end up returning to a bogus PC since the PC is not
	# saved if EXL=1.
	#
	.set	noat
1:
	mtc0	zero, MIPS_COP_0_STATUS		# disable int
	COP0_SYNC
	nop					# 3 op delay
	nop
	nop
	li	a0, MIPS_SR_EXL			# set exception level
	mtc0	a0, MIPS_COP_0_STATUS
	COP0_SYNC
	nop
	nop
	PTR_ADDU a1, sp, CALLFRAME_SIZ
 #	REG_L	a0, FRAME_SR(a1)
	REG_L	t0, FRAME_MULLO(a1)
	REG_L	t1, FRAME_MULHI(a1)
	REG_L	v0, FRAME_EPC(a1)
	mtlo	t0
	mthi	t1
	_MTC0	v0, MIPS_COP_0_EXC_PC
	COP0_SYNC
	nop
	move	k1, a1
#ifdef IPL_ICU_MASK
	.set at
	INT_L	t0, FRAME_PPL(k1)
	INT_S	t0, _C_LABEL(md_imask)
	jal	_C_LABEL(md_imask_update)
	nop
	.set noat
#endif
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
	REG_L	ta0, FRAME_TA0(k1)
	REG_L	ta1, FRAME_TA1(k1)
	REG_L	ta2, FRAME_TA2(k1)
	REG_L	ta3, FRAME_TA3(k1)
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
	DYNAMIC_STATUS_MASK(k0, sp)		# machine dependent masking
	REG_L	gp, FRAME_GP(k1)
	REG_L	s8, FRAME_S8(k1)
	REG_L	ra, FRAME_RA(k1)
	REG_L	sp, FRAME_SP(k1)
	mtc0	k0, MIPS_COP_0_STATUS
	COP0_SYNC
	nop
	nop
	eret
	.set	at
END(MIPSX(lwp_trampoline))

/*
 * Like lwp_trampoline, but do not call lwp_startup
 */
LEAF(MIPSX(setfunc_trampoline))
	PTR_ADDU sp, sp, -CALLFRAME_SIZ

	# Call the routine specified by cpu_setfunc()
	jal	ra, s0			
	move	a0, s1

	j	1b
	nop

END(MIPSX(setfunc_trampoline))


/*
 * void mipsN_cpu_switch_resume(struct lwp *newlwp)
 *
 * Wiredown the USPACE of newproc in TLB entry#0.  Check whether target
 * USPACE is already in another place of TLB before that, and make
 * sure TBIS(it) in the case.
 */
LEAF_NOPROFILE(MIPSX(cpu_switch_resume))
#if !defined(ENABLE_MIPS_16KB_PAGE) || defined(VMSWAP_UAREA)
	INT_L	a1, L_MD_UPTE_0(a0)		# a1 = upte[0]
	INT_L	a2, L_MD_UPTE_1(a0)		# a2 = upte[1]
	PTR_L	v0, L_ADDR(a0)			# va = l->l_addr
	li	s0, MIPS_KSEG2_START
	blt	v0, s0, resume
	nop

	and	s0, v0, MIPS3_PG_ODDPG
	beq	s0, zero, entry0
	nop

	PANIC("USPACE sat on odd page boundary")

entry0:
	_MTC0	v0, MIPS_COP_0_TLB_HI		# VPN = va
	COP0_SYNC
	nop
	nop
	tlbp					# probe VPN
	COP0_SYNC
	nop
	nop
	mfc0	s0, MIPS_COP_0_TLB_INDEX
	nop
	bltz	s0, entry0set
	sll	s0, s0, PGSHIFT + 1		# PAGE_SHIFT + 1
	PTR_LA	s0, MIPS_KSEG0_START(s0)
	_MTC0	s0, MIPS_COP_0_TLB_HI
	COP0_SYNC
	_MTC0	zero, MIPS_COP_0_TLB_LO0
	COP0_SYNC
	_MTC0	zero, MIPS_COP_0_TLB_LO1
	COP0_SYNC
	nop
	nop
	tlbwi
	COP0_SYNC
	nop
	nop
	_MTC0	v0, MIPS_COP_0_TLB_HI		# set VPN again
	COP0_SYNC
entry0set:
	mtc0	zero, MIPS_COP_0_TLB_INDEX	# TLB entry #0
	COP0_SYNC
	or	a1, MIPS3_PG_G
	_MTC0	a1, MIPS_COP_0_TLB_LO0		# upte[0] | PG_G
	COP0_SYNC
	or	a2, MIPS3_PG_G
	_MTC0	a2, MIPS_COP_0_TLB_LO1		# upte[1] | PG_G
	COP0_SYNC
	nop
	nop
	tlbwi					# set TLB entry #0
	COP0_SYNC
	nop
	nop

resume:
#endif /* !ENABLE_MIPS_16KB_PAGE || VMSWAP_UAREA */
	j	ra
	nop
END(MIPSX(cpu_switch_resume))

/*
 * void mipsN_TBIS(vaddr_t va)
 *
 * Invalidate a TLB entry which has the given vaddr and ASID if found.
 */
LEAF_NOPROFILE(MIPSX(TBIS))
	mfc0	v1, MIPS_COP_0_STATUS		# save status register
	mtc0	zero, MIPS_COP_0_STATUS		# disable interrupts
	COP0_SYNC

	li	v0, (MIPS3_PG_HVPN | MIPS3_PG_ASID)
	_MFC0	t0, MIPS_COP_0_TLB_HI		# save current ASID
	mfc0	t3, MIPS_COP_0_TLB_PG_MASK	# save current pgMask
	and	a0, a0, v0			# make sure valid entryHi
	_MTC0	a0, MIPS_COP_0_TLB_HI		# look for the vaddr & ASID
	COP0_SYNC
	nop
	nop
	tlbp					# probe the entry in question
	COP0_SYNC
	nop
	nop
	mfc0	v0, MIPS_COP_0_TLB_INDEX	# see what we got
	#nop					# -slip-
	#nop					# -slip-
	bltz	v0, 1f				# index < 0 then skip
	li	t1, MIPS_KSEG0_START		# invalid address
	sll	v0, v0, PGSHIFT + 1		# PAGE_SHIFT + 1
	addu	t1, t1, v0
	_MTC0	t1, MIPS_COP_0_TLB_HI		# make entryHi invalid
	COP0_SYNC
	_MTC0	zero, MIPS_COP_0_TLB_LO0	# zero out entryLo0
	COP0_SYNC
	_MTC0	zero, MIPS_COP_0_TLB_LO1	# zero out entryLo1
	COP0_SYNC
#if 1
	nop
#else
	mtc0	zero, MIPS_COP_0_TLB_PG_MASK	# zero out pageMask
#endif
	COP0_SYNC
	nop
	nop

	tlbwi
	COP0_SYNC
	nop
	nop
1:
	_MTC0	t0, MIPS_COP_0_TLB_HI		# restore current ASID
	COP0_SYNC
	mtc0	t3, MIPS_COP_0_TLB_PG_MASK	# restore pgMask
	COP0_SYNC
	nop
	nop
	j	ra
	mtc0	v1, MIPS_COP_0_STATUS		# restore status register
	COP0_SYNC				# XXXX - not executed!!
END(MIPSX(TBIS))

/*
 * void mips3_TBIAP(int sizeofTLB)
 *
 * Invalidate TLB entries belong to per process user spaces while
 * leaving entries for kernel space marked global intact.
 */
LEAF_NOPROFILE(MIPSX(TBIAP))
	mfc0	v1, MIPS_COP_0_STATUS		# save status register
	mtc0	zero, MIPS_COP_0_STATUS		# disable interrupts
	COP0_SYNC

	move	t2, a0
	mfc0	t1, MIPS_COP_0_TLB_WIRED
	li	v0, MIPS_KSEG0_START		# invalid address
	mfc0	t3, MIPS_COP_0_TLB_PG_MASK	# save current pgMask

	# do {} while (t1 < t2)
1:
	mtc0	t1, MIPS_COP_0_TLB_INDEX	# set index
	COP0_SYNC
	sll	ta0, t1, PGSHIFT + 1		# PAGE_SHIFT + 1
	nop
	/* XXX simonb: lose this nop for mips32/64? */
	nop
	tlbr					# obtain an entry
	COP0_SYNC
	/* XXX simonb: lose these nops for mips32/64? */
	nop
	nop
	nop
	_MFC0	a0, MIPS_COP_0_TLB_LO1
	and	a0, a0, MIPS3_PG_G		# check to see it has G bit
	bnez	a0, 2f
	addu	ta0, ta0, v0

	_MTC0	ta0, MIPS_COP_0_TLB_HI		# make entryHi invalid
	COP0_SYNC
	_MTC0	zero, MIPS_COP_0_TLB_LO0	# zero out entryLo0
	COP0_SYNC
	_MTC0	zero, MIPS_COP_0_TLB_LO1	# zero out entryLo1
	COP0_SYNC
	mtc0	zero, MIPS_COP_0_TLB_PG_MASK	# zero out mask entry
	COP0_SYNC
	/* XXX simonb: lose these nops for mips32/64? */
	nop
	nop
	tlbwi					# invalidate the TLB entry
	COP0_SYNC
2:
	addu	t1, t1, 1
	bne	t1, t2, 1b
	nop

	mtc0	t3, MIPS_COP_0_TLB_PG_MASK	# restore pgMask
	COP0_SYNC
	/* XXX simonb: lose these nops for mips32/64? */
	nop
	nop
	j	ra				# new ASID will be set soon
	mtc0	v1, MIPS_COP_0_STATUS		# restore status register
	COP0_SYNC				# XXXX - not executed!!
END(MIPSX(TBIAP))

/*
 * void mipsN_TBIA(int sizeofTLB)
 *
 * Invalidate all of non-wired TLB entries.
 */
LEAF_NOPROFILE(MIPSX(TBIA))
	mfc0	v1, MIPS_COP_0_STATUS		# save status register
	mtc0	zero, MIPS_COP_0_STATUS		# disable interrupts
	COP0_SYNC

	li	v0, MIPS_KSEG0_START		# invalid address
	_MFC0	t0, MIPS_COP_0_TLB_HI		# save current ASID
	mfc0	t1, MIPS_COP_0_TLB_WIRED
	mfc0	t2, MIPS_COP_0_TLB_PG_MASK	# save current pgMask

	_MTC0	zero, MIPS_COP_0_TLB_LO0	# zero out entryLo0
	COP0_SYNC
	_MTC0	zero, MIPS_COP_0_TLB_LO1	# zero out entryLo1
	COP0_SYNC
	mtc0	zero, MIPS_COP_0_TLB_PG_MASK	# zero out pageMask
	COP0_SYNC

	# do {} while (t1 < a0)
1:
	mtc0	t1, MIPS_COP_0_TLB_INDEX	# set TLBindex
	COP0_SYNC
	sll	ta0, t1, PGSHIFT + 1		# PAGE_SHIFT + 1
	add	ta0, v0, ta0
	_MTC0	ta0, MIPS_COP_0_TLB_HI		# make entryHi invalid
	COP0_SYNC
	nop
	nop
	tlbwi					# clear the entry
	COP0_SYNC
	addu	t1, t1, 1			# increment index
	bne	t1, a0, 1b
	nop

	_MTC0	t0, MIPS_COP_0_TLB_HI		# restore ASID
	COP0_SYNC
	mtc0	t2, MIPS_COP_0_TLB_PG_MASK	# restore pgMask
	COP0_SYNC
	nop
	nop
	j	ra
	mtc0	v1, MIPS_COP_0_STATUS		# restore status register
	COP0_SYNC				# XXXX - not executed!!
END(MIPSX(TBIA))

#ifdef USE_64BIT_INSTRUCTIONS
LEAF(MIPSX(pagezero))
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
	bgtz	a1, 1b
	addu	a0, 64

	j	ra
	nop
END(MIPSX(pagezero))
#endif /* USE_64BIT_INSTRUCTIONS */

	.rdata

	.globl _C_LABEL(MIPSX(locoresw))
_C_LABEL(MIPSX(locoresw)):
	PTR_WORD _C_LABEL(MIPSX(cpu_switch_resume))
	PTR_WORD _C_LABEL(MIPSX(lwp_trampoline))
	PTR_WORD _C_LABEL(nullop)
	PTR_WORD _C_LABEL(MIPSX(setfunc_trampoline))

MIPSX(excpt_sw):
	####
	#### The kernel exception handlers.
	####
	PTR_WORD _C_LABEL(MIPSX(KernIntr))		#  0 external interrupt
	PTR_WORD _C_LABEL(MIPSX(KernGenException))	#  1 TLB modification
	PTR_WORD _C_LABEL(MIPSX(TLBInvalidException))# 2 TLB miss (LW/I-fetch)
	PTR_WORD _C_LABEL(MIPSX(TLBInvalidException))# 3 TLB miss (SW)
	PTR_WORD _C_LABEL(MIPSX(KernGenException))	#  4 address error (LW/I-fetch)
	PTR_WORD _C_LABEL(MIPSX(KernGenException))	#  5 address error (SW)
	PTR_WORD _C_LABEL(MIPSX(KernGenException))	#  6 bus error (I-fetch)
	PTR_WORD _C_LABEL(MIPSX(KernGenException))	#  7 bus error (load or store)
	PTR_WORD _C_LABEL(MIPSX(KernGenException))	#  8 system call
	PTR_WORD _C_LABEL(MIPSX(KernGenException))	#  9 breakpoint
	PTR_WORD _C_LABEL(MIPSX(KernGenException))	# 10 reserved instruction
	PTR_WORD _C_LABEL(MIPSX(KernGenException))	# 11 coprocessor unusable
	PTR_WORD _C_LABEL(MIPSX(KernGenException))	# 12 arithmetic overflow
	PTR_WORD _C_LABEL(MIPSX(KernGenException))	# 13 r4k trap exception
#if defined(MIPS3) && !defined(MIPS3_5900)
	PTR_WORD _C_LABEL(mips3_VCEI)			# 14 r4k virt coherence
#else
	PTR_WORD _C_LABEL(MIPSX(KernGenException))	# 14 reserved
#endif
	PTR_WORD _C_LABEL(MIPSX(KernGenException))	# 15 r4k FP exception
	PTR_WORD _C_LABEL(MIPSX(KernGenException))	# 16 reserved
	PTR_WORD _C_LABEL(MIPSX(KernGenException))	# 17 reserved
	PTR_WORD _C_LABEL(MIPSX(KernGenException))	# 18 reserved
	PTR_WORD _C_LABEL(MIPSX(KernGenException))	# 19 reserved
	PTR_WORD _C_LABEL(MIPSX(KernGenException))	# 20 reserved
	PTR_WORD _C_LABEL(MIPSX(KernGenException))	# 21 reserved
	PTR_WORD _C_LABEL(MIPSX(KernGenException))	# 22 reserved
	PTR_WORD _C_LABEL(MIPSX(KernGenException))	# 23 watch exception
	PTR_WORD _C_LABEL(MIPSX(KernGenException))	# 24 reserved
	PTR_WORD _C_LABEL(MIPSX(KernGenException))	# 25 reserved
	PTR_WORD _C_LABEL(MIPSX(KernGenException))	# 26 reserved
	PTR_WORD _C_LABEL(MIPSX(KernGenException))	# 27 reserved
	PTR_WORD _C_LABEL(MIPSX(KernGenException))	# 28 reserved
	PTR_WORD _C_LABEL(MIPSX(KernGenException))	# 29 reserved
	PTR_WORD _C_LABEL(MIPSX(KernGenException))	# 30 reserved
#if defined(MIPS3) && !defined(MIPS3_5900)
	PTR_WORD _C_LABEL(mips3_VCED)		# 31 v. coherence exception data
#else
	PTR_WORD _C_LABEL(MIPSX(KernGenException))	# 31 reserved
#endif
	#####
	##### The user exception handlers.
	#####
	PTR_WORD _C_LABEL(MIPSX(UserIntr))		#  0
	PTR_WORD _C_LABEL(MIPSX(UserGenException))	#  1
	PTR_WORD _C_LABEL(MIPSX(UserGenException))	#  2
	PTR_WORD _C_LABEL(MIPSX(UserGenException))	#  3
	PTR_WORD _C_LABEL(MIPSX(UserGenException))	#  4
	PTR_WORD _C_LABEL(MIPSX(UserGenException))	#  5
	PTR_WORD _C_LABEL(MIPSX(UserGenException))	#  6
	PTR_WORD _C_LABEL(MIPSX(UserGenException))	#  7
	PTR_WORD _C_LABEL(MIPSX(SystemCall))		#  8
	PTR_WORD _C_LABEL(MIPSX(UserGenException))	#  9
	PTR_WORD _C_LABEL(MIPSX(UserGenException))	# 10
	PTR_WORD _C_LABEL(MIPSX(UserGenException))	# 11
	PTR_WORD _C_LABEL(MIPSX(UserGenException))	# 12
	PTR_WORD _C_LABEL(MIPSX(UserGenException))	# 13
#if defined(MIPS3) && !defined(MIPS3_5900)
	PTR_WORD _C_LABEL(mips3_VCEI)			# 14
#else
	PTR_WORD _C_LABEL(MIPSX(UserGenException))	# 14
#endif
	PTR_WORD _C_LABEL(MIPSX(UserGenException))	# 15
	PTR_WORD _C_LABEL(MIPSX(UserGenException))	# 16
	PTR_WORD _C_LABEL(MIPSX(UserGenException))	# 17
	PTR_WORD _C_LABEL(MIPSX(UserGenException))	# 18
	PTR_WORD _C_LABEL(MIPSX(UserGenException))	# 19
	PTR_WORD _C_LABEL(MIPSX(UserGenException))	# 20
	PTR_WORD _C_LABEL(MIPSX(UserGenException))	# 21
	PTR_WORD _C_LABEL(MIPSX(UserGenException))	# 22
	PTR_WORD _C_LABEL(MIPSX(UserGenException))	# 23
	PTR_WORD _C_LABEL(MIPSX(UserGenException))	# 24
	PTR_WORD _C_LABEL(MIPSX(UserGenException))	# 25
	PTR_WORD _C_LABEL(MIPSX(UserGenException))	# 26
	PTR_WORD _C_LABEL(MIPSX(UserGenException))	# 27
	PTR_WORD _C_LABEL(MIPSX(UserGenException))	# 28
	PTR_WORD _C_LABEL(MIPSX(UserGenException))	# 29
	PTR_WORD _C_LABEL(MIPSX(UserGenException))	# 30
#if defined(MIPS3) && !defined(MIPS3_5900)
	PTR_WORD _C_LABEL(mips3_VCED)			# 31 v. coherence exception data
#else
	PTR_WORD _C_LABEL(MIPSX(UserGenException))	# 31
#endif
