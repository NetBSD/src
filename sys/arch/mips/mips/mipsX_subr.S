/*	$NetBSD: mipsX_subr.S,v 1.26.36.1.2.31 2010/04/28 03:14:45 matt Exp $	*/

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
#include <mips/trap.h>

#include <machine/param.h>
#include <machine/endian.h>

#include "assym.h"

#ifdef _LP64
#define	RESET_EXCEPTION_LEVEL_DISABLE_INTERRUPTS(reg) \
	li reg, MIPS_SR_KX; mtc0 reg, MIPS_COP_0_STATUS
#define	SET_EXCEPTION_LEVEL(reg) \
	li reg, MIPS_SR_EXL | MIPS_SR_KX; mtc0 reg, MIPS_COP_0_STATUS
#else
#define	RESET_EXCEPTION_LEVEL_DISABLE_INTERRUPTS(reg) \
	mtc0 zero, MIPS_COP_0_STATUS
#define	SET_EXCEPTION_LEVEL(reg) \
	li reg, MIPS_SR_EXL; mtc0 reg, MIPS_COP_0_STATUS
#endif

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

#ifdef _LP64
#define	_SLLV		dsllv
#else
#define	_SLLV		sllv
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
 * TLB handling data.   'CPUVAR(PMAP_SEGBASE)' points to the base of the segment
 * table.   this is read and written by C code in mips_machdep.c.
 *
 * XXX: use linear mapped PTs at fixed VA in kseg2 in the future?
 */
	.text


/*
 * some useful labels for debugging
 */
.global	mips_kseg0
.equiv	mips_kseg0,		MIPS_KSEG0_START
.global	mips_kseg1
.equiv	mips_kseg1,		MIPS_KSEG1_START
.global	mips_kseg2
.equiv	mips_kseg2,		MIPS_KSEG2_START
.global	mips_xkphys
.equiv	mips_xkphys,		MIPS_XKPHYS_START
.global	mips_xkphys_u
.equiv	mips_xkphys_u,		MIPS_XKPHYS_UNCACHED
.global	mips_xkphys_cca3
.equiv	mips_xkphys_cca3,	MIPS_XKPHYS_CCA3
.global	mips_xkphys_cca4
.equiv	mips_xkphys_cca4,	MIPS_XKPHYS_CCA4
.global	mips_xkseg
.equiv	mips_xkseg,		MIPS_XKSEG_START


/*
 *----------------------------------------------------------------------------
 *
 * mips3_tlb_miss --
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
VECTOR(MIPSX(tlb_miss), unknown)
	.set	noat
	_MFC0	k0, MIPS_COP_0_BAD_VADDR	#00: k0=bad address
	lui	k1, %hi(CPUVAR(PMAP_SEGBASE))	#01: k1=hi of segbase
	bltz	k0, MIPSX(kernelfault)		#02: k0<0 -> 4f (kernel fault)
	 PTR_SRL k0, (2*PGSHIFT-2-PTR_SCALESHIFT)#03: k0=seg offset (almost)
	PTR_L	k1, %lo(CPUVAR(PMAP_SEGBASE))(k1)#04: k1=segment tab base
	andi	k0, (NBPG-(1<<PTR_SCALESHIFT))	#05: k0=seg offset (mask 0x3)
	PTR_ADDU k1, k0				#06: k1=seg entry address
	PTR_L	k1, 0(k1)			#07: k1=seg entry
	_MFC0	k0, MIPS_COP_0_BAD_VADDR	#08: k0=bad address (again)
	beqz	k1, MIPSX(nopagetable)		#09: ==0 -- no page table
	 PTR_SRL k0, (PGSHIFT-2)		#0a: k0=VPN (aka va>>10)
	andi	k0, (NBPG-8)			#0b: k0=page tab offset
	PTR_ADDU k1, k0				#0c: k1=pte address
	INT_L	k0, 0(k1)			#0d: k0=lo0 pte
	INT_L	k1, 4(k1)			#0e: k1=lo1 pte
	_SLL	k0, WIRED_SHIFT			#0f: chop top 2 bits (part 1a)
	_SRL	k0, WIRED_SHIFT			#10: chop top 2 bits (part 1b)
#ifdef MIPS3_5900
	_MTC0	k0, MIPS_COP_0_TLB_LO0		#11: lo0 is loaded
	sync.p					#12: R5900 cop0 hazard
	_SLL	k1, WIRED_SHIFT			#13: chop top 2 bits (part 2a)
	_SRL	k1, WIRED_SHIFT			#14: chop top 2 bits (part 2b)
	_MTC0	k1, MIPS_COP_0_TLB_LO1		#15: lo1 is loaded
	sync.p					#16: R5900 cop0 hazard
#else /* MIPS3_5900 */
	_MTC0	k0, MIPS_COP_0_TLB_LO0		#11: lo0 is loaded
	_SLL	k1, WIRED_SHIFT			#12: chop top 2 bits (part 2a)
	_SRL	k1, WIRED_SHIFT			#13: chop top 2 bits (part 2b)
	_MTC0	k1, MIPS_COP_0_TLB_LO1		#14: lo1 is loaded
	nop					#15: standard nop
	nop					#16: extra nop for QED5230
#endif /* MIPS3_5900 */
	tlbwr					#17: write to tlb
	nop					#18: standard nop
	nop					#19: needed by R4000/4400
	nop					#1a: needed by R4000/4400
	eret					#1b: return from exception
	.set	at
_VECTOR_END(MIPSX(tlb_miss))

#if defined(USE_64BIT_CP0_FUNCTIONS)
/*
 * mips3_xtlb_miss routine
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
VECTOR(MIPSX(xtlb_miss), unknown)
	.set	noat
	dmfc0	k0, MIPS_COP_0_BAD_VADDR	#00: k0=bad address
	lui	k1, %hi(CPUVAR(PMAP_SEGBASE))	#01: k1=hi of segbase
	bltz	k0, MIPSX(kernelfault)		#02: k0<0 -> 4f (kernel fault)
	 PTR_SRL k0, 2*PGSHIFT-2-PTR_SCALESHIFT	#03: k0=seg offset (almost)
	PTR_L	k1, %lo(CPUVAR(PMAP_SEGBASE))(k1)#04: k1=segment tab base
	andi	k0, NBPG-(1<<PTR_SCALESHIFT)	#05: k0=seg offset (mask 0x3)
	PTR_ADDU k1, k0				#06: k1=seg entry address
	PTR_L	k1, 0(k1)			#07: k1=seg entry
	dmfc0	k0, MIPS_COP_0_BAD_VADDR	#08: k0=bad address (again)
	beqz	k1, MIPSX(nopagetable)		#09: ==0 -- no page table
	 PTR_SRL k0, (PGSHIFT-2)		#0a: k0=VPN (aka va>>10)
	andi	k0, (NBPG-8)			#0b: k0=page tab offset
	PTR_ADDU k1, k0				#0c: k1=pte address
	INT_L	k0, 0(k1)			#0d: k0=lo0 pte
	INT_L	k1, 4(k1)			#0e: k1=lo1 pte
	_SLL	k0, WIRED_SHIFT			#0f: chop top 2 bits (part 1a)
	_SRL	k0, WIRED_SHIFT			#10: chop top 2 bits (part 1b)
	_MTC0	k0, MIPS_COP_0_TLB_LO0		#11: lo0 is loaded
	_SLL	k1, WIRED_SHIFT			#12: chop top 2 bits (part 2a)
	_SRL	k1, WIRED_SHIFT			#13: chop top 2 bits (part 2b)
	_MTC0	k1, MIPS_COP_0_TLB_LO1		#14: lo1 is loaded
	nop					#15: standard nop
	nop					#16: extra nop for QED5230
	tlbwr					#17: write to tlb
	nop					#18: standard nop
	nop					#19: needed by R4000/4400
	nop					#1a: needed by R4000/4400
	eret					#1b: return from exception
	.set	at
_VECTOR_END(MIPSX(xtlb_miss))
#else
	.space	128
#endif /* USE_64BIT_CP0_FUNCTIONS */

/*
 * Vector to real handler in KSEG1.
 */
VECTOR(MIPSX(cache), unknown)
	PTR_LA	k0, _C_LABEL(MIPSX(cache_exception))	#00
	li	k1, MIPS_PHYS_MASK			#02
	and	k0, k1					#03
	li	k1, MIPS_KSEG1_START			#04
	or	k0, k1					#05
	lui	k1, %hi(CPUVAR(CURLWP))			#06: k1=hi of curlwp
	j	k0					#07
	 PTR_L	k1, %lo(CPUVAR(CURLWP))(k1)		#08: k1=lo of curlwp
_VECTOR_END(MIPSX(cache))

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
	and	k0, MIPS3_SR_KSU_USER		#02: test for user mode
						#    sneaky but the bits are
						#    with us........
	sll	k0, 3				#03: shift user bit for cause index
	and	k1, MIPS3_CR_EXC_CODE		#04: mask out the cause bits.
	or	k1, k0				#05: change index to user table
#if PTR_SCALESHIFT > MIPS_CR_EXC_CODE_SHIFT
	PTR_SLL	k1, PTR_SCALESHIFT - MIPS_CR_EXC_CODE_SHIFT
#endif
	PTR_LA	k0, MIPSX(excpt_sw)		#06: get base of the jump table
	PTR_ADDU k0, k1				#08: get the address of the
						#     function entry.  Note that
						#     the cause is already
						#     shifted left by 2 bits so
						#     we dont have to shift.
	PTR_L	k0, 0(k0)			#09: get the function address
	#nop					#    -slip-
	lui	k1, %hi(CPUVAR(CURLWP))		#0a: k1=hi of curlwp
	j	k0				#0b: jump to the function
	 PTR_L	k1, %lo(CPUVAR(CURLWP))(k1)	#0c: k1=lo of curlwp
	nop					#0d
	nop					#0e
#ifndef _LP64
	nop					#0f
#endif
	.p2align 4
MIPSX(kernelfault):
	j	_C_LABEL(MIPSX(kernel_tlb_miss)) #10: kernel exception
	 nop					#11: branch delay slot
	nop
	nop
MIPSX(nopagetable):
	lui	k1, %hi(CPUVAR(CURLWP))		#14: k1=hi of curlwp
	j	MIPSX(slowfault)		#15: no page table present
	 PTR_L	k1, %lo(CPUVAR(CURLWP))(k1)	#16: k1=lo of curlwp
	nop					#17: branch delay slot
	.set	at
_VECTOR_END(MIPSX(exception))

/*
 * Handle MIPS32/MIPS64 style interrupt exception vector.
 */
VECTOR(MIPSX(intr), unknown)
	.set	noat
	mfc0	k1, MIPS_COP_0_STATUS		#00: get the status register
	nop					#01: stall
	and	k1, k1, MIPS3_SR_KSU_USER	#02: test for user mode
	PTR_LA	k0, MIPSX(user_intr)		#03: assume user mode
	bnez	k1, 1f				#05: yep, do it
	 nop					#06:  branch deay
	PTR_LA	k0, MIPSX(kern_intr)		#07: nope, kernel intr
1:
	lui	k1, %hi(CPUVAR(CURLWP))		#09: k1=hi of curlwp
	j	k0				#0a: jump to the function
	 PTR_L	k1, %lo(CPUVAR(CURLWP))(k1)	#0b: k1=lo of curlwp
	.set	at
_VECTOR_END(MIPSX(intr))

/*----------------------------------------------------------------------------
 *
 * MIPSX(slowfault) --
 *
 * Alternate entry point into the mips3_user_gen_exception or
 * mips3_kern_gen_exception, when the ULTB miss handler couldn't
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
	.org	((. + 31) & ~31) + 12
MIPSX(slowfault):
	.set	noat
	mfc0	k0, MIPS_COP_0_STATUS
	nop
	and	k0, k0, MIPS3_SR_KSU_USER
	bnez	k0, _C_LABEL(MIPSX(user_gen_exception))
	nop
	.set	at
/*
 * Fall though ...
 */

/*
 * mips3_kern_gen_exception
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
 *	struct reg cf_tf;
 * };
 */
NESTED_NOPROFILE(MIPSX(kern_gen_exception), KERNFRAME_SIZ, ra)
	.set	noat
	.mask	0x80000000, -4
#ifdef PARANOIA
	PTR_L	k0, L_ADDR(k1)
	slt	k0, k0, sp		# k0 = L_ADDR(k1) < sp
1:	beqz	k0, 1b			# loop forever if false
	 nop
	PTR_L	k0, L_ADDR(k1)
	PTR_ADDU k0, USPACE
	slt	k0, sp, k0		# k0 = sp < L_ADDP(k1) + USPACE
2:	beqz	k0, 2b			# loop forever if false
	 nop
#endif /* PARANOIA
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
	REG_S	t8, TF_BASE+TF_REG_T8(sp)	# will be MIPS_CURLWP
	REG_S	t9, TF_BASE+TF_REG_T9(sp)
	REG_S	ra, TF_BASE+TF_REG_RA(sp)
	REG_S	a0, TF_BASE+TF_REG_SR(sp)
	_MFC0	a3, MIPS_COP_0_EXC_PC		# 4th arg is exception PC
	REG_S	v0, TF_BASE+TF_REG_MULLO(sp)
	REG_S	v1, TF_BASE+TF_REG_MULHI(sp)
	REG_S	a3, TF_BASE+TF_REG_EPC(sp)
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
#if defined(__mips_o32) || defined(__mips_o64)
	PTR_ADDU v0, sp, TF_BASE
	REG_S	v0, KERNFRAME_ARG5(sp)		# 5th arg is p. to trapframe
#endif
#if defined(__mips_n32) || defined(__mips_n64)
	PTR_ADDU a4, sp, TF_BASE		# 5th arg is p. to trapframe
#endif
#ifdef PARANOIA
	/*
	 * save PPL in trapframe
	 */
	PTR_L	t0, L_CPU(MIPS_CURLWP)
	INT_L	t1, CPU_INFO_CPL(t0)		# get current priority level
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
	PTR_L	t9, _C_LABEL(mips_splsw) + SPLSW_SPLCHECK
	jalr	t9
	 nop
#endif /* PARANOIA */

	/*
	 * We need to find out if this was due to a T_BREAK and if so
	 * turn off interrupts in addition to clearing the exception level.
	 */
	li	v1, 1 << T_BREAK		# make a mask of T_BREAK
	sll	t0, a1, MIPS_CR_EXC_CODE_SHIFT	# shift exc code to low 5 bits
	srl	v1, t0				# shift break mask using it
	and	v1, MIPS_SR_INT_IE		# restrict to IE bit
	or	v1, MIPS_SR_EXL			# or in EXL bit
	and	v1, a0				# extract bits from status
	xor	v0, a0, v1 			# generate new status
	mtc0	v0, MIPS_COP_0_STATUS		# update.
	COP0_SYNC
	nop
	nop
	nop
	/*
	 * Call the trap handler.
	 */
	jal	_C_LABEL(trap)			#
	REG_S	a3, KERNFRAME_RA(sp)		# for debugging

/*
 * Restore registers and return from the exception.
 */
	RESET_EXCEPTION_LEVEL_DISABLE_INTERRUPTS(v0)
	COP0_SYNC
	nop					# 3 nop delay
	nop
	nop
	REG_L	a0, TF_BASE+TF_REG_SR(sp)	# get SR with EXL set
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
	bne	t1, t0, 3f
	jal	_C_LABEL(_lock_ras)
	nop
#endif

3:	_MTC0	k1, MIPS_COP_0_EXC_PC		# set return address
	COP0_SYNC

#ifdef PARANOIA
	INT_L	t2, TF_BASE+TF_PPL(sp)		# get saved priority level
	PTR_L	t0, L_CPU(MIPS_CURLWP)
	INT_L	t1, CPU_INFO_CPL(t0)		# get current priority level
11:	bne	t2, t1, 11b			# loop forever if unequal
	 nop

	/*
	 * Verify our existing interrupt level.
	 */
	PTR_L	t9, _C_LABEL(mips_splsw) + SPLSW_SPLCHECK
	jalr	t9
	 nop
#endif /* PARANOIA */

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
	REG_L	t8, TF_BASE+TF_REG_T8(sp)	# was MIPS_CURLWP
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
	PTR_ADDU sp, sp, KERNFRAME_SIZ
	eret					# return to interrupted point
	.set	at
END(MIPSX(kern_gen_exception))

/*
 * mipsN_user_gen_exception
 *
 * Handle an exception from user mode.
 * Save user context atop the kernel stack, then call trap() to process
 * the condition.  The context can be manipulated alternatively via
 * curlwp->l_md.md_regs.
 */
	.p2align 5
NESTED_NOPROFILE(MIPSX(user_gen_exception), CALLFRAME_SIZ, ra)
	.set	noat
	.mask	0x80000000, -4
/*
 * Save all of the registers except for the kernel temporaries in u_pcb.
 */
	/* K1 already has CURLWP */
	PTR_L	k0, L_ADDR(k1)
	PTR_ADDU k0, k0, USPACE - TF_SIZ - CALLFRAME_SIZ
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
	move	sp, k0
	move	MIPS_CURLWP, k1
#ifdef NOFPU
	/*
	 * enter kernel mode
	 */
	and	t0, a0, MIPS_SR_EXL|MIPS_SR_KSU_MASK # bits to clear
	xor	t0, a0				# clear them.
#else
	/*
	 * Turn off FPU and enter kernel mode
	 */
	lui	t0, %hi(~(MIPS_SR_COP_1_BIT|MIPS_SR_EXL|MIPS_SR_KSU_MASK))
	ori	t0, %lo(~(MIPS_SR_COP_1_BIT|MIPS_SR_EXL|MIPS_SR_KSU_MASK))
	and	t0, a0
#endif
/*
 * Call the trap handler.
 */
	mtc0	t0, MIPS_COP_0_STATUS
	COP0_SYNC

	jal	_C_LABEL(trap)
	 REG_S	a3, CALLFRAME_RA(sp)		# for debugging
/*
 * Check pending asynchronous traps.
 */
	INT_L	t0, L_MD_ASTPENDING(MIPS_CURLWP)
	beqz	t0, MIPSX(user_return)
	 nop
/*
 * We have pending asynchronous traps; all the state is already saved.
 */
	PTR_LA	ra, MIPSX(user_return)
	j	_C_LABEL(ast)
	 nop
	.set	at
END(MIPSX(user_gen_exception))

/*
 * mipsN_systemcall
 *
 * Save user context in u_pcb, then call syscall() to process a system call.
 * The context can be manipulated alternatively via curlwp->p_md.md_regs;
 */
	.p2align 5
NESTED_NOPROFILE(MIPSX(systemcall), CALLFRAME_SIZ, ra)
	.set	noat
	.mask	0x80000000, -4
	/* k1 already cpulwp */
	PTR_L	k0, L_ADDR(k1)
	#nop					# -slip-
	PTR_ADDU k0, k0, USPACE - TF_SIZ - CALLFRAME_SIZ
	REG_S	AT, CALLFRAME_SIZ+TF_REG_AST(k0)
	REG_S	v0, CALLFRAME_SIZ+TF_REG_V0(k0)		# syscall #
	REG_S	v1, CALLFRAME_SIZ+TF_REG_V1(k0)		# used by syscall()
	mflo	v0
	REG_S	a0, CALLFRAME_SIZ+TF_REG_A0(k0)
	REG_S	a1, CALLFRAME_SIZ+TF_REG_A1(k0)
	REG_S	a2, CALLFRAME_SIZ+TF_REG_A2(k0)
	REG_S	a3, CALLFRAME_SIZ+TF_REG_A3(k0)
	move	a0, k1				# 1st arg is curlwp
	mfhi	v1
	mfc0	a2, MIPS_COP_0_CAUSE		# 3rd arg is CAUSE
	REG_S	s0, CALLFRAME_SIZ+TF_REG_S0(k0)
	REG_S	s1, CALLFRAME_SIZ+TF_REG_S1(k0)
	REG_S	s2, CALLFRAME_SIZ+TF_REG_S2(k0)
	REG_S	s3, CALLFRAME_SIZ+TF_REG_S3(k0)
	_MFC0	a3, MIPS_COP_0_EXC_PC		# 4th arg is PC
	REG_S	s4, CALLFRAME_SIZ+TF_REG_S4(k0)
	REG_S	s5, CALLFRAME_SIZ+TF_REG_S5(k0)
	REG_S	s6, CALLFRAME_SIZ+TF_REG_S6(k0)
	REG_S	s7, CALLFRAME_SIZ+TF_REG_S7(k0)
	REG_S	t0, CALLFRAME_SIZ+TF_REG_T0(k0)
	REG_S	t1, CALLFRAME_SIZ+TF_REG_T1(k0)
	REG_S	t2, CALLFRAME_SIZ+TF_REG_T2(k0)
	REG_S	t3, CALLFRAME_SIZ+TF_REG_T3(k0)		# syscall saved gp for fork
	mfc0	a1, MIPS_COP_0_STATUS		# 2nd arg is STATUS
#if defined(__mips_n32) || defined(__mips_n64)
	REG_S	a4, CALLFRAME_SIZ+TF_REG_A4(k0)
	REG_S	a5, CALLFRAME_SIZ+TF_REG_A5(k0)
	REG_S	a6, CALLFRAME_SIZ+TF_REG_A6(k0)
	REG_S	a7, CALLFRAME_SIZ+TF_REG_A7(k0)
#else
	REG_S	ta0, CALLFRAME_SIZ+TF_REG_TA0(k0)
	REG_S	ta1, CALLFRAME_SIZ+TF_REG_TA1(k0)
	REG_S	ta2, CALLFRAME_SIZ+TF_REG_TA2(k0)
	REG_S	ta3, CALLFRAME_SIZ+TF_REG_TA3(k0)
#endif
	REG_S	t8, CALLFRAME_SIZ+TF_REG_T8(k0)	# was MIPS_CURLWP
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
	move	sp, k0
	move	MIPS_CURLWP, a0			# set curlwp reg
#ifdef __GP_SUPPORT__
	PTR_LA	gp, _C_LABEL(_gp)		# switch to kernel GP
#endif
#if defined(DDB) || defined(DEBUG) || defined(KGDB)
	move	ra, a3
	REG_S	ra, CALLFRAME_RA(sp)
#endif
/*
 * Turn off fpu and enter kernel mode
 */
	PTR_L	t9, P_MD_SYSCALL(t0)		# t1 = syscall
#ifdef NOFPU
	and	t0, a1, MIPS_SR_EXL|MIPS_SR_KSU_MASK
	xor	t0, a1
#else
	lui	t0, %hi(~(MIPS_SR_COP_1_BIT|MIPS_SR_EXL|MIPS_SR_KSU_MASK))
	ori	t0, %lo(~(MIPS_SR_COP_1_BIT|MIPS_SR_EXL|MIPS_SR_KSU_MASK))
	and	t0, a1
#endif
	mtc0	t0, MIPS_COP_0_STATUS		# re-enable interrupts
	COP0_SYNC
/*
 * Call the system call handler.
 */
	jal	t9
	nop
/*
 * Check pending asynchronous traps.
 */
	INT_L	t0, L_MD_ASTPENDING(MIPS_CURLWP)
	beqz	t0, MIPSX(user_return)
	 nop
/*
 * We have pending asynchronous traps; all the state is already saved.
 */
	PTR_LA	ra, MIPSX(user_return)
	j	_C_LABEL(ast)
	 nop
	.set	at
END(MIPSX(systemcall))

/*
 * Panic on cache errors.  A lot more could be done to recover
 * from some types of errors but it is tricky.
 */
	.p2align 5
NESTED_NOPROFILE(MIPSX(cache_exception), KERNFRAME_SIZ, ra)
	.set	noat
	.mask	0x80000000, -4
#ifdef sbmips	/* XXX!  SB-1 needs a real cache error handler */
	eret
	nop
#endif
	PTR_LA	k0, panic			# return to panic
	PTR_LA	a0, 9f				# panicstr
	_MFC0	a1, MIPS_COP_0_ERROR_PC
#if defined(MIPS64_XLS)
	li	k1, 0x309	/* L1D_CACHE_ERROR_LOG */
	mfcr	a2, k1
	li	k1, 0x30b	/* L1D_CACHE_INTERRUPT */
	mfcr	a3, k1
#if defined(__mips_o32)
#error O32 not supported.
#endif
	mfc0	a4, MIPS_COP_0_STATUS
	mfc0	a5, MIPS_COP_0_CAUSE
#else
	mfc0	a2, MIPS_COP_0_ECC
	mfc0	a3, MIPS_COP_0_CACHE_ERR
#endif

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

#if defined(MIPS64_XLS)
	MSG("cache error @ EPC %#lx\nL1D_CACHE_ERROR_LOG %#lx\nL1D_CACHE_INTERRUPT %#lx\nstatus %#x, cause %#x");
#else
	MSG("cache error @ EPC 0x%x ErrCtl 0x%x CacheErr 0x%x");
#endif
	.set	at
END(MIPSX(cache_exception))

/*
 * mipsX_kern_intr
 *
 * Handle an interrupt from kernel mode.
 * Build intrframe on stack to hold interrupted kernel context, then
 * call cpu_intr() to process it.
 *
 */
	.p2align 5
NESTED_NOPROFILE(MIPSX(kern_intr), KERNFRAME_SIZ, ra)
	.set	noat
	.mask	0x80000000, -4
#ifdef PARANOIA
	PTR_L	k0, L_ADDR(k1)
	slt	k0, k0, sp			# k0 = L_ADDR(k1) < sp
1:	beqz	k0, 1b				# loop forever if false
	 nop
	PTR_L	k0, L_ADDR(k1)
	PTR_ADDU k0, USPACE
	slt	k0, sp, k0			# k0 = sp < L_ADDR(k1) + USPACE
2:	beqz	k0, 2b				# loop forever if false
	 nop
	PTR_L	k0, L_CPU(k1)
	INT_L	k0, CPU_INFO_IDEPTH(k0)		# grab interrupt depth
	sltu	k0, k0, 3			# must be < 3
3:	beqz	k0, 3b				# loop forever if false
	 nop
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
	REG_S	t0, TF_BASE+TF_REG_T0(sp)
	REG_S	t1, TF_BASE+TF_REG_T1(sp)
	REG_S	t2, TF_BASE+TF_REG_T2(sp)
	REG_S	t3, TF_BASE+TF_REG_T3(sp)
	REG_S	ta0, TF_BASE+TF_REG_TA0(sp)
	REG_S	ta1, TF_BASE+TF_REG_TA1(sp)
	REG_S	ta2, TF_BASE+TF_REG_TA2(sp)
	REG_S	ta3, TF_BASE+TF_REG_TA3(sp)
	REG_S	s0, TF_BASE+TF_REG_S0(sp)	# s0 is used for saved ipl
	REG_S	s1, TF_BASE+TF_REG_S1(sp)	# s1 is used for initial status
	mfc0	s1, MIPS_COP_0_STATUS
	REG_S	s2, TF_BASE+TF_REG_S2(sp)	# s2 is used for cpu_info
	REG_S	t8, TF_BASE+TF_REG_T8(sp)	# MIPS_CURLWP
	REG_S	t9, TF_BASE+TF_REG_T9(sp)
	REG_S	ra, TF_BASE+TF_REG_RA(sp)
	REG_S	a0, TF_BASE+TF_REG_SR(sp)
	REG_S	v0, TF_BASE+TF_REG_MULLO(sp)
	REG_S	v1, TF_BASE+TF_REG_MULHI(sp)
/*
 * Call the interrupt handler.
 */
	_MFC0	ra, MIPS_COP_0_EXC_PC		# grab exception PC
	REG_S	ra, TF_BASE+TF_REG_EPC(sp)	# and save it

#if defined(DDB) || defined(DEBUG) || defined(KGDB)
	REG_S	ra, KERNFRAME_RA(sp)		# for debugging
#endif

	PTR_L	s2, L_CPU(MIPS_CURLWP)
#ifdef PARANOIA
	INT_L	s0, CPU_INFO_CPL(s2)
	INT_S	s0, TF_BASE+TF_PPL(sp)		# save priority level

	/*
	 * Verify the current interrupt level
	 */
	PTR_L	t9, _C_LABEL(mips_splsw) + SPLSW_SPLCHECK
	jalr	t9
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
	or	s0, t1				#   save old interrupt depth
	INT_ADDU t1, 1
	INT_S	t1, CPU_INFO_IDEPTH(s2)		#   store new interrupt depth

	/*
	 * Now we can clear exception level since no interrupts can be delivered
	 */
	mfc0	v1, MIPS_COP_0_STATUS
	and	v0, v1, MIPS_SR_EXL		# grab exception level bit
	xor	v0, v1				# clear it
	mtc0	v0, MIPS_COP_0_STATUS		# write new status
	COP0_SYNC

	/*
	 * Now hard interrupts can be processed.
	 */
	move	a1, ra				# 2nd arg is exception pc
	move	a2, s1				# 3rd arg is status
	jal	_C_LABEL(cpu_intr)		# cpu_intr(ppl, pc, status)
	 srl	a0, s0, 8			# 1st arg is previous pri level

	and	t1, s0, 0xff			# get previous interrupt depth
	INT_S	t1, CPU_INFO_IDEPTH(s2)		# to it previous value

#ifdef PARANOIA
	mfc0	t0, MIPS_COP_0_STATUS		# verify INT_IE is still set
	and	t0, MIPS_SR_INT_IE
3:	beqz	t0, 3b
	 nop
#endif /* PARANOIA */

#ifdef __HAVE_FAST_SOFTINTS
	and	a0, s1, MIPS_SOFT_INT_MASK	# were softints enabled?
	beqz	a0, 4f				#   nope
	 nop
	mfc0	v0, MIPS_COP_0_CAUSE		# grab the pending softints
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
	and	v0, t0, 1 << SOFTINT_KPREEMPT	# do we need a kernel preempt?
	beqz	v0, 4f				#   nop
	 nop
	xor	t0, v0				# clear preempt bit
	INT_S	t0, CPU_INFO_SOFTINTS(s2)	# and save it.
	jal	_C_LABEL(kpreempt)		# kpreempt(pc)
	 PTR_L	a0, TF_BASE+TF_REG_EPC(sp)
#endif /* __HAVE_PREEMPTION */
4:
#endif /* __HAVE_FAST_SOFTINT */
/*
 * Restore registers and return from the interrupt.
 */
	/*
	 * Clear interrupt enable
	 */
	mfc0	v0, MIPS_COP_0_STATUS		# read it 
	xor	v0, MIPS_SR_INT_IE		# disable interrupts
	mtc0	v0, MIPS_COP_0_STATUS		# write it
	COP0_SYNC
	nop
	nop
	nop

	or	v0, MIPS_SR_EXL			# set exception mode
	mtc0	v0, MIPS_COP_0_STATUS		# write it
	COP0_SYNC
	nop
	nop
	nop

	srl	a0, s0, 8			# get previous priority level
#ifdef PARANOIA
	INT_L	t0, TF_BASE+TF_PPL(sp)		# get saved priority level
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
	or	v0, MIPS_SR_INT_IE
5:	bne	v0, s1, 5b
	 nop
#endif /* PARANOIA */

	/*
	 * Restore SR
	 */
	mtc0	s1, MIPS_COP_0_STATUS
	COP0_SYNC
	nop
	nop
	nop

	REG_L	t0, TF_BASE+TF_REG_MULLO(sp)
	REG_L	t1, TF_BASE+TF_REG_MULHI(sp)
	REG_L	v0, TF_BASE+TF_REG_EPC(sp)
	mtlo	t0
	mthi	t1

#ifdef notyet
	/* Check for restartable sequences. */
	lui	t0, %hi(_C_LABEL(_lock_ras_start))
	ori	t0, zero, %lo(_C_LABEL(_lock_ras_start))
	li	t1, -MIPS_LOCK_RAS_SIZE
	and	t1, t1, v0
	bne	t1, t0, 10f
	move	k1, v0
	jal	_C_LABEL(_lock_ras)
	nop
	mov	v0, k1
#endif

10:	_MTC0	v0, MIPS_COP_0_EXC_PC		# set return address
	COP0_SYNC

#ifdef PARANOIA
	INT_L	t2, TF_BASE+TF_PPL(sp)		# get saved priority level
	INT_L	t1, CPU_INFO_CPL(s2)		# get current priority level
11:	bne	t2, t1, 11b			# loop forever is unequal
	 nop

	PTR_L	t9, _C_LABEL(mips_splsw) + SPLSW_SPLCHECK
	jalr	t9
	 nop
#endif /* PARANOIA */

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
	REG_L	s0, TF_BASE+TF_REG_S0(sp)
	REG_L	s1, TF_BASE+TF_REG_S1(sp)
	REG_L	s2, TF_BASE+TF_REG_S2(sp)
	REG_L	t8, TF_BASE+TF_REG_T8(sp)	# MIPS_CURLWP
	REG_L	t9, TF_BASE+TF_REG_T9(sp)
	REG_L	ra, TF_BASE+TF_REG_RA(sp)
	PTR_ADDU sp, sp, KERNFRAME_SIZ		# restore kernel SP
	eret					# return to interrupted point
	.set	at
END(MIPSX(kern_intr))

/*----------------------------------------------------------------------------
 * XXX this comment block should be updated XXX
 * mipsN_user_intr --
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
	.p2align 5
NESTED_NOPROFILE(MIPSX(user_intr), CALLFRAME_SIZ, ra)
	.set	noat
	.mask	0x80000000, -4
/*
 * Save the relevant user registers into the u_pcb.
 * We don't need to save s0 - s8 because the compiler does it for us.
 */
	/* k1 contains curlwp */
	PTR_L	k0, L_ADDR(k1)
	PTR_ADDU k0, k0, USPACE - TF_SIZ - CALLFRAME_SIZ
	REG_S	AT, CALLFRAME_SIZ+TF_REG_AST(k0)	# $1
	REG_S	v0, CALLFRAME_SIZ+TF_REG_V0(k0)		# $2
	REG_S	v1, CALLFRAME_SIZ+TF_REG_V1(k0)		# $3
	mflo	v0
	REG_S	a0, CALLFRAME_SIZ+TF_REG_A0(k0)		# $4
	REG_S	a1, CALLFRAME_SIZ+TF_REG_A1(k0)		# $5
	REG_S	a2, CALLFRAME_SIZ+TF_REG_A2(k0)		# $6
	REG_S	a3, CALLFRAME_SIZ+TF_REG_A3(k0)		# $7
	mfhi	v1
	REG_S	t0, CALLFRAME_SIZ+TF_REG_T0(k0)		# $12
	mfc0	t0, MIPS_COP_0_CAUSE
	REG_S	t1, CALLFRAME_SIZ+TF_REG_T1(k0)		# $13
	REG_S	t2, CALLFRAME_SIZ+TF_REG_T2(k0)		# $14
	REG_S	t3, CALLFRAME_SIZ+TF_REG_T3(k0)		# $15
	REG_S	ta0, CALLFRAME_SIZ+TF_REG_TA0(k0)	# $8
	REG_S	ta1, CALLFRAME_SIZ+TF_REG_TA1(k0)	# $9
	REG_S	ta2, CALLFRAME_SIZ+TF_REG_TA2(k0)	# $10
	REG_S	ta3, CALLFRAME_SIZ+TF_REG_TA3(k0)	# $11
	REG_S	s0, CALLFRAME_SIZ+TF_REG_S0(k0)		# $16
	REG_S	s1, CALLFRAME_SIZ+TF_REG_S1(k0)		# $17
	mfc0	s1, MIPS_COP_0_STATUS
	REG_S	t8, CALLFRAME_SIZ+TF_REG_T8(k0)		# $24 MIPS_CURLWP
	REG_S	t9, CALLFRAME_SIZ+TF_REG_T9(k0)		# $25
	REG_S	gp, CALLFRAME_SIZ+TF_REG_GP(k0)		# $28
	REG_S	sp, CALLFRAME_SIZ+TF_REG_SP(k0)		# $29
	REG_S	ra, CALLFRAME_SIZ+TF_REG_RA(k0)		# $31
	REG_S	s1, CALLFRAME_SIZ+TF_REG_SR(k0)
	_MFC0	ra, MIPS_COP_0_EXC_PC
	REG_S	v0, CALLFRAME_SIZ+TF_REG_MULLO(k0)
	REG_S	v1, CALLFRAME_SIZ+TF_REG_MULHI(k0)
	REG_S	ra, CALLFRAME_SIZ+TF_REG_EPC(k0)
	REG_S	t0, CALLFRAME_SIZ+TF_REG_CAUSE(k0)
	move	sp, k0
	move	MIPS_CURLWP, k1			# set curlwp reg (t8)
#if defined(DDB) || defined(DEBUG) || defined(KGDB)
	REG_S	ra, CALLFRAME_RA(sp)
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
	 * Now we can turn off the FPU, clear exception level, and enter
	 * kernel mode since no interrupts can be delivered
	 */
	mfc0	v1, MIPS_COP_0_STATUS
#ifdef NOFPU
	and	v0, v1, MIPS_SR_KSU_MASK|MIPS_SR_EXL
	xor	v0, v1				# clear the bits
#else
	lui	v0, %hi(~(MIPS_SR_COP_1_BIT|MIPS_SR_EXL|MIPS_SR_KSU_MASK))
	ori	v0, %lo(~(MIPS_SR_COP_1_BIT|MIPS_SR_EXL|MIPS_SR_KSU_MASK))
	and	v0, v1 
#endif
	mtc0	v0, MIPS_COP_0_STATUS		# write new status
	COP0_SYNC

	/*
	 * Since we interrupted user mode, the new interrupt depth must be 1.
	 */
	PTR_L	t0, L_CPU(MIPS_CURLWP)
	li	t1, 1
	INT_S	t1, CPU_INFO_IDEPTH(t0)		# store new interrupt depth (1)

	/*
	 * Now hard interrupts can be processed.
	 */
	move	a1, ra				# 2nd arg is exception pc
	move	a2, s1				# 3rd arg is status
	jal	_C_LABEL(cpu_intr)		# cpu_intr(ppl, pc, status)
	 move	a0, s0				# 1st arg is previous pri level

	/*
	 * Interrupt depth is now back to 0.
	 */
	PTR_L	t0, L_CPU(MIPS_CURLWP)
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
	and	v0, v1, MIPS_SR_INT_IE		# clear interrupt enable
	xor	v0, v1
	mtc0	v0, MIPS_COP_0_STATUS		# interrupts are disabled
	COP0_SYNC
	nop
	nop
	nop

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
	beqz	v0, MIPSX(user_intr_return)	# if no, skip ast processing
	 nop
/*
 * We have pending asynchronous traps; save remaining user state in u_pcb.
 */
	#REG_S	s0, CALLFRAME_SIZ+TF_REG_S0(sp)	# $16 saved above
	#REG_S	s1, CALLFRAME_SIZ+TF_REG_S1(sp)	# $17 saved above
	REG_S	s2, CALLFRAME_SIZ+TF_REG_S2(sp)	# $18
	REG_S	s3, CALLFRAME_SIZ+TF_REG_S3(sp)	# $19
	REG_S	s4, CALLFRAME_SIZ+TF_REG_S4(sp)	# $20
	REG_S	s5, CALLFRAME_SIZ+TF_REG_S5(sp)	# $21
	REG_S	s6, CALLFRAME_SIZ+TF_REG_S6(sp)	# $22
	REG_S	s7, CALLFRAME_SIZ+TF_REG_S7(sp)	# $23
	REG_S	s8, CALLFRAME_SIZ+TF_REG_S8(sp)	# $30
	mfc0	t0, MIPS_COP_0_STATUS		# 
	or	t0, MIPS_SR_INT_IE 		# enable interrupts
	DYNAMIC_STATUS_MASK(t0, t1)		# machine dependent masking
	mtc0	t0, MIPS_COP_0_STATUS		# enable interrupts (spl0)
	COP0_SYNC
	nop
	nop
	nop
	PTR_LA	ra, MIPSX(user_return)
	j	_C_LABEL(ast)			# ast()
	 nop
	.set	at
END(MIPSX(user_intr))


/*----------------------------------------------------------------------------
 *
 *	R4000 TLB exception handlers
 *
 *----------------------------------------------------------------------------
 */


/*----------------------------------------------------------------------------
 *
 * mips3_tlb_invalid_exception --
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
LEAF_NOPROFILE(MIPSX(tlb_invalid_exception))
	.set	noat
	_MFC0	k0, MIPS_COP_0_BAD_VADDR	# get the fault address
#if VM_MIN_KERNEL_ADDRESS == MIPS_KSEG2_START
	li	k1, VM_MIN_KERNEL_ADDRESS	# compute index
#else
	li	k1, VM_MIN_KERNEL_ADDRESS>>32	# compute index
	dsll32	k1, k1, 0
#endif
	bgez	k0, _C_LABEL(MIPSX(kern_gen_exception))	# full trap processing
	 move	k1, MIPS_CURLWP
	PTR_SUBU k0, k0, k1
	INT_L	k1, _C_LABEL(Sysmapsize)	# index within range?
	PTR_SRL	k0, k0, PGSHIFT
	sltu	k1, k0, k1
	beqz	k1, _C_LABEL(MIPSX(kern_gen_exception))	# No. Failing beyond...
	 move	k1, MIPS_CURLWP			# - delay slot -
	PTR_L	k1, _C_LABEL(Sysmap)

	PTR_SLL	k0, k0, 2			# compute offset from index
	PTR_ADDU k1, k1, k0
	tlbp					# Probe the invalid entry
	COP0_SYNC
	and	k0, k0, 4			# check even/odd page
	nop					# required for QED 5230
	bnez	k0, MIPSX(kern_tlbi_odd)
	nop

	mfc0	k0, MIPS_COP_0_TLB_INDEX
	nop
	bltz	k0, _C_LABEL(MIPSX(kern_gen_exception)) # ASSERT(TLB entry exists)
	 move	k1, MIPS_CURLWP
	INT_L	k0, 0(k1)			# get PTE entry

	_SLL	k0, k0, WIRED_SHIFT		# get rid of "wired" bit
	_SRL	k0, k0, WIRED_SHIFT
	_MTC0	k0, MIPS_COP_0_TLB_LO0		# load PTE entry
	COP0_SYNC
	and	k0, k0, MIPS3_PG_V		# check for valid entry
	nop					# required for QED5230
	beqz	k0, _C_LABEL(MIPSX(kern_gen_exception))	# PTE invalid
	 move	k1, MIPS_CURLWP
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

MIPSX(kern_tlbi_odd):
	mfc0	k0, MIPS_COP_0_TLB_INDEX
	nop
	bltz	k0, _C_LABEL(MIPSX(kern_gen_exception))	# assert(TLB Entry exists)
	 move	k1, MIPS_CURLWP
	INT_L	k0, 0(k1)			# get PTE entry

	_SLL	k0, k0, WIRED_SHIFT		# get rid of wired bit
	_SRL	k0, k0, WIRED_SHIFT
	_MTC0	k0, MIPS_COP_0_TLB_LO1		# save PTE entry
	COP0_SYNC
	and	k0, k0, MIPS3_PG_V		# check for valid entry
	nop					# required for QED5230
	beqz	k0, _C_LABEL(MIPSX(kern_gen_exception))	# PTE invalid
	 move	k1, MIPS_CURLWP
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
END(MIPSX(tlb_invalid_exception))

/*----------------------------------------------------------------------------
 *
 * mipsN_kernel_tlb_miss --
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
LEAF_NOPROFILE(MIPSX(kernel_tlb_miss))
	.set	noat
	_MFC0	k0, MIPS_COP_0_BAD_VADDR	# get the fault address
#if VM_MIN_KERNEL_ADDRESS == MIPS_KSEG2_START
	li	k1, VM_MIN_KERNEL_ADDRESS	# compute index
#else
	li	k1, VM_MIN_KERNEL_ADDRESS>>32	# compute index
	dsll32	k1, k1, 0
#endif
	PTR_SUBU k0, k0, k1
	INT_L	k1, _C_LABEL(Sysmapsize)	# index within range?
	PTR_SRL	k0, k0, PGSHIFT
	sltu	k1, k0, k1
#ifdef newsmips
	/* news5000 has ROM work area at 0xfff00000. */
	bnez	k1, 1f
	 nop
	j	checkromwork
	 nop					# - delay slot -
1:
#else
	/*
	 * If we are beyond the bounds of Sysmap, let trap panic for us.
	 */
	beqz	k1, _C_LABEL(MIPSX(kern_gen_exception)) # No. Failing beyond. . .
	 move	k1, MIPS_CURLWP			# - delay slot -
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
	.set	at
END(MIPSX(kernel_tlb_miss))

	MSG("TLB out of universe: ksp %p epc %p vaddr %p")

/*
 * Mark where code entered from exception hander jumptable
 * ends, for stack traceback code.
 */

	.globl	_C_LABEL(MIPSX(exceptionentry_end))
_C_LABEL(MIPSX(exceptionentry_end)):

/*--------------------------------------------------------------------------
 *
 * mipsN_tlb_set_asid --
 *
 *	Write the given pid into the TLB pid reg.
 *
 *	mips3_tlb_set_asid(uint32_t pid)
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
LEAF(MIPSX(tlb_set_asid))
	_MTC0	a0, MIPS_COP_0_TLB_HI		# Write the hi reg value
	COP0_SYNC
	/* XXX simonb: lose these nops for mips32/64? */
	nop					# required for QED5230
	nop					# required for QED5230
	j	ra
	nop
END(MIPSX(tlb_set_asid))

#if defined(ENABLE_MIPS3_WIRED_MAP)
/*--------------------------------------------------------------------------
 *
 * mipsN_TLBWriteIndexedVPS --
 *
 *      Write the given entry into the TLB at the given index.
 *      Pass full R4000 style TLB info including variable page size mask.
 *
 *      mipsN_tlb_write_indexed(size_t tlb_index, const struct tlbmask *tlb)
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      TLB entry set.
 *
 *--------------------------------------------------------------------------
 */
LEAF(MIPSX(tlb_write_indexed_VPS))
	mfc0	v1, MIPS_COP_0_STATUS		# Save the status register.
	RESET_EXCEPTION_LEVEL_DISABLE_INTERRUPTS(v0)
	COP0_SYNC
	nop
	INT_L	a2, TLBMASK_LO0(a1)		# fetch tlb->tlb_lo0
	INT_L	a3, TLBMASK_LO1(a1)		# fetch tlb->tlb_lo1
	mfc0	v0, MIPS_COP_0_TLB_PG_MASK	# Save current page mask.
	_MFC0	t0, MIPS_COP_0_TLB_HI		# Save the current PID.

	_MTC0	a2, MIPS_COP_0_TLB_LO0		# Set up entry low0.
	COP0_SYNC
	_MTC0	a3, MIPS_COP_0_TLB_LO1		# Set up entry low1.
	COP0_SYNC
	nop
	INT_L	a2, TLBMASK_MASK(a1)		# fetch tlb->tlb_mask
	PTR_L	a3, TLBMASK_HI(a1)		# fetch tlb->tlb_hi
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
END(MIPSX(tlb_write_indexed_VPS))
#endif /* ENABLE_MIPS3_WIRED_MAP */
	
/*--------------------------------------------------------------------------
 *
 * mipsN_tlb_update --
 *
 *	Update the TLB if highreg is found; otherwise do nothing.
 *
 *	long mipsN_tlb_update(vaddr_t va, register_t lowreg)
 *
 * Results:
 *	< 0 if skipped, >= 0 if updated.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------------------
 */
LEAF(MIPSX(tlb_update))
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
	bnez	t1, 1f				# Decide even odd
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
END(MIPSX(tlb_update))

/*--------------------------------------------------------------------------
 *
 * mipsN_tlb_read_indexed --
 *
 *	Read the TLB entry.
 *
 *	void mipsN_tlb_read_indexed(size_t tlb_index, struct tlbmask *tlb);
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	tlb will contain the TLB entry found.
 *
 *--------------------------------------------------------------------------
 */
LEAF(MIPSX(tlb_read_indexed))
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
	PTR_S	t3, TLBMASK_HI(a1)
	INT_S	ta0, TLBMASK_LO0(a1)
	INT_S	ta1, TLBMASK_LO1(a1)
	j	ra
	INT_S	t2, TLBMASK_MASK(a1)
END(MIPSX(tlb_read_indexed))

#if defined(MIPS3) && !defined(MIPS3_5900)
/*----------------------------------------------------------------------------
 *
 * mipsN_VCED --
 *
 *	Handle virtual coherency exceptions.
 *	Called directly from the mips3 exception-table code.
 *	only k0, k1 are available on entry
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Remaps the conflicting address as uncached and returns
 *	from the exception.
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
	PTR_ADDU k1, k0			# get address of count table
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
	PTR_ADDU k1, k0			# get address of count table
	LONG_L	k0, 0(k1)
	PTR_ADDU k0, 1
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
	move	a0, v0
	jal	_C_LABEL(lwp_startup)
	 move	a1, MIPS_CURLWP

	# Call the routine specified by cpu_setfunc()
	jal	s0			
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
#ifdef PARANOIA
	PTR_L	t0, L_CPU(MIPS_CURLWP)
	INT_L	t1, CPU_INFO_CPL(t0)			# get curcpu()->ci_cpl
2:	bnez	t1, 2b
	 nop
#endif
	RESET_EXCEPTION_LEVEL_DISABLE_INTERRUPTS(v0)
	COP0_SYNC
	nop					# 3 op delay
	nop
	nop
	SET_EXCEPTION_LEVEL(v0)			# set exception level
	COP0_SYNC
	nop
	nop
	REG_L	t0, CALLFRAME_SIZ+TF_REG_MULLO(sp)
	REG_L	t1, CALLFRAME_SIZ+TF_REG_MULHI(sp)
	REG_L	v0, CALLFRAME_SIZ+TF_REG_EPC(sp)
	mtlo	t0
	mthi	t1
	_MTC0	v0, MIPS_COP_0_EXC_PC
	COP0_SYNC
	nop
	move	k1, sp
	REG_L	AT, CALLFRAME_SIZ+TF_REG_AST(k1)	# $1
	REG_L	v0, CALLFRAME_SIZ+TF_REG_V0(k1)		# $2
	REG_L	v1, CALLFRAME_SIZ+TF_REG_V1(k1)		# $3
	REG_L	a0, CALLFRAME_SIZ+TF_REG_A0(k1)		# $4
	REG_L	a1, CALLFRAME_SIZ+TF_REG_A1(k1)		# $5
	REG_L	a2, CALLFRAME_SIZ+TF_REG_A2(k1)		# $6
	REG_L	a3, CALLFRAME_SIZ+TF_REG_A3(k1)		# $7
	REG_L	t0, CALLFRAME_SIZ+TF_REG_T0(k1)		# $12 /  $8
	REG_L	t1, CALLFRAME_SIZ+TF_REG_T1(k1)		# $13 /  $9
	REG_L	t2, CALLFRAME_SIZ+TF_REG_T2(k1)		# $14 / $10
	REG_L	t3, CALLFRAME_SIZ+TF_REG_T3(k1)		# $15 / $11
	REG_L	ta0, CALLFRAME_SIZ+TF_REG_TA0(k1)	#  $8 / $12
	REG_L	ta1, CALLFRAME_SIZ+TF_REG_TA1(k1)	#  $9 / $13
	REG_L	ta2, CALLFRAME_SIZ+TF_REG_TA2(k1)	# $10 / $14
	REG_L	ta3, CALLFRAME_SIZ+TF_REG_TA3(k1)	# $11 / $15
	REG_L	t8, CALLFRAME_SIZ+TF_REG_T8(k1)		# $24 MIPS_CURLWP
	REG_L	t9, CALLFRAME_SIZ+TF_REG_T9(k1)		# $25
	REG_L	k0, CALLFRAME_SIZ+TF_REG_SR(k1)		# status register
	DYNAMIC_STATUS_MASK(k0, sp)		# machine dependent masking
	REG_L	gp, CALLFRAME_SIZ+TF_REG_GP(k1)		# $29
	REG_L	sp, CALLFRAME_SIZ+TF_REG_SP(k1)		# $29
	REG_L	ra, CALLFRAME_SIZ+TF_REG_RA(k1)		# $31
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
	# and return directly to user_return
	PTR_LA	ra, MIPSX(user_return)
	jr	s0			
	 move	a0, s1
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
#if VM_MIN_KERNEL_ADDRESS == MIPS_KSEG2_START
	li	t0, VM_MIN_KERNEL_ADDRESS	# compute index
	blt	v0, t0, MIPSX(resume)
	 nop
#else
	li	t0, MIPS_KSEG0_START		# above XKSEG?
	blt	t0, v0, MIPSX(resume)
	 nop
	li	t0, VM_MIN_KERNEL_ADDRESS>>32	# below XKSEG?
	dsll32	t0, t0, 0
	blt	v0, t0, MIPSX(resume)
	 nop
#endif

	and	t0, v0, MIPS3_PG_ODDPG
	beqz	t0, MIPSX(entry0)
	 nop

	PANIC("USPACE sat on odd page boundary")

MIPSX(entry0):
	_MTC0	v0, MIPS_COP_0_TLB_HI		# VPN = va
	COP0_SYNC
	nop
	nop
	tlbp					# probe VPN
	COP0_SYNC
	nop
	nop
	mfc0	t0, MIPS_COP_0_TLB_INDEX
	nop
	bltz	t0, MIPSX(entry0set)
	sll	t0, t0, PGSHIFT + 1		# PAGE_SHIFT + 1
	PTR_LA	t0, MIPS_KSEG0_START(t0)
	_MTC0	t0, MIPS_COP_0_TLB_HI
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
MIPSX(entry0set):
#ifdef MULTIPROCESSOR
	PTR_L	t0, L_CPU(a0)			# get cpu_info
	INT_L	t1, CPU_INFO_KSP_TLB_SLOT(t0)	# get TLB# for KSP
	mtc0	t1, MIPS_COP_0_TLB_INDEX	# TLB entry (virtual)
#else
	mtc0	zero, MIPS_COP_0_TLB_INDEX	# TLB entry #0 (virtual)
#endif
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

MIPSX(resume):
#endif /* !ENABLE_MIPS_16KB_PAGE || VMSWAP_UAREA */
	j	ra
	nop
END(MIPSX(cpu_switch_resume))

/*
 * void mipsN_tlb_invalidate_addr(vaddr_t va)
 *
 * Invalidate a TLB entry which has the given vaddr and ASID if found.
 */
LEAF_NOPROFILE(MIPSX(tlb_invalidate_addr))
	mfc0	v1, MIPS_COP_0_STATUS		# save status register
	mtc0	zero, MIPS_COP_0_STATUS		# disable interrupts
	COP0_SYNC

	li	v0, (MIPS3_PG_HVPN | MIPS3_PG_ASID)
	_MFC0	t0, MIPS_COP_0_TLB_HI		# save current ASID
	mfc0	t3, MIPS_COP_0_TLB_PG_MASK	# save current pgMask
	and	a0, v0				# make sure valid entryHi
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
	PTR_SLL	v0, PGSHIFT + 1			# PAGE_SHIFT + 1
	PTR_ADDU t1, v0
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
END(MIPSX(tlb_invalidate_addr))

/*
 * void mipsN_tlb_invalidate_asids(uint32_t base, uint32_t limit);
 *
 * Invalidate TLB entries belong to per process user spaces with
 * base <= ASIDs < limit while leaving entries for kernel space
 * marked global intact.
 */
LEAF_NOPROFILE(MIPSX(tlb_invalidate_asids))
	mfc0	v1, MIPS_COP_0_STATUS		# save status register
	mtc0	zero, MIPS_COP_0_STATUS		# disable interrupts
	COP0_SYNC

	_MFC0	t0, MIPS_COP_0_TLB_HI		# Save the current PID.
	mfc0	t1, MIPS_COP_0_TLB_WIRED
	li	v0, MIPS_KSEG0_START		# invalid address
	INT_L	t2, _C_LABEL(mips_options) + MO_NUM_TLB_ENTRIES
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
	and	a0, MIPS3_PG_G			# check to see it has G bit
	bnez	a0, 2f				# yep, skip this one.
	 nop
	_MFC0	a0, MIPS_COP_0_TLB_HI		# get VA and ASID
	and	a0, MIPS3_PG_ASID		# focus on ASID
	sltu	a3, a0, a1			# asid < base?
	bnez	a3, 2f				# yes, skip this entry.
	 nop
	sltu	a3, a0, a2			# asid < limit
	beqz	a3, 2f				# nope, skip this entry.
	 nop
	PTR_ADDU ta0, v0

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
	addu	t1, 1
	bne	t1, t2, 1b
	 nop

	_MFC0	t0, MIPS_COP_0_TLB_HI		# restore PID.
	COP0_SYNC
	mtc0	t3, MIPS_COP_0_TLB_PG_MASK	# restore pgMask
	COP0_SYNC
	/* XXX simonb: lose these nops for mips32/64? */
	nop
	nop
	j	ra				# new ASID will be set soon
	 mtc0	v1, MIPS_COP_0_STATUS		# restore status register
	COP0_SYNC				# XXXX - not executed!!
END(MIPSX(tlb_invalidate_asids))

/*
 * u_int mipsN_tlb_record_asids(u_long *bitmap, uint32_t asid_mask);
 *
 * Record all the ASIDs in use in the TLB and return the number of different
 * ASIDs present.
 */
LEAF_NOPROFILE(MIPSX(tlb_record_asids))

	_MFC0	a3, MIPS_COP_0_TLB_HI		# Save the current PID.
	mfc0	ta0, MIPS_COP_0_TLB_WIRED
	INT_L	ta1, _C_LABEL(mips_options) + MO_NUM_TLB_ENTRIES
	move	ta2, zero
	li	ta3, 1
	move	v0, zero

	mfc0	v1, MIPS_COP_0_STATUS		# save status register
#ifdef _LP64
	and	t0, v1, MIPS_SR_INT_IE
	xor	t0, v1
	mtc0	t0, MIPS_COP_0_STATUS		# disable interrupts
#else
	mtc0	zero, MIPS_COP_0_STATUS		# disable interrupts
#endif
	COP0_SYNC

	# do {} while (ta0 < ta1)
1:
	mtc0	ta0, MIPS_COP_0_TLB_INDEX	# set index
	COP0_SYNC
	nop
	nop
	/* XXX simonb: lose this nop for mips32/64? */
	nop
	tlbr					# obtain an entry
	COP0_SYNC
	/* XXX simonb: lose these nops for mips32/64? */
	nop
	nop
	nop
	_MFC0	t0, MIPS_COP_0_TLB_LO1
	and	t0, MIPS3_PG_G			# check to see it has G bit
	bnez	t0, 4f				# yep, skip this one.
	 nop
	_MFC0	t0, MIPS_COP_0_TLB_HI		# get VA and ASID
	and	t0, a1				# focus on ASID

	srl	a2, t0, 3 + LONG_SCALESHIFT	# drop low 5 or 6 bits
	sll	a2, LONG_SCALESHIFT		# make an index for the bitmap
	_SLLV	t0, ta3, t0			# t0 is mask (ta3 == 1)

	PTR_ADDU a2, a0				# index into the bitmap 
	beq	a2, ta2, 3f			# is the desired cell loaded?
	 nop					#    yes, don't reload it
	beqz	ta2, 2f				# have we ever loaded it?
	 nop					#   nope, so don't save it.

	LONG_S	t2, 0(ta2)			# save the updated value.
2:
	move	ta2, a2				# remember the new cell's addr
	LONG_L	t2, 0(ta2)			# and load it
3:
	and	t1, t2, t0			# t1 = t2 & t0
	sltu	t1, t1, ta3			# t1 = t1 < 1 (aka t1 == 0)
	addu	v0, t1				# v0 += t1
	or	t2, t0				# or in the new ASID bits
4:
	addu	ta0, 1				# increment TLB entry #
	bne	ta0, ta1, 1b			# keep lookup if not limit
	 nop

	beqz	ta2, 5f				# do we have a cell to write?
	 nop					#   nope.  nothing

	LONG_S	t2, 0(ta2)			# save the updated value.
5:
	_MTC0	a3, MIPS_COP_0_TLB_HI		# restore ASID
	COP0_SYNC

	mtc0	v1, MIPS_COP_0_STATUS		# restore status register
	j	ra
	 COP0_SYNC				# XXXX - not executed!!
END(MIPSX(tlb_record_asids))

/*
 * void mipsN_tlb_invalidate_globals(void);
 *
 * Invalidate the non-wired TLB entries belonging to kernel space while
 * leaving entries for user space (not marked global) intact.
 */
LEAF_NOPROFILE(MIPSX(tlb_invalidate_globals))
	mfc0	v1, MIPS_COP_0_STATUS		# save status register
	mtc0	zero, MIPS_COP_0_STATUS		# disable interrupts
	COP0_SYNC

	mfc0	t1, MIPS_COP_0_TLB_WIRED
	li	v0, MIPS_KSEG0_START		# invalid address
	INT_L	t2, _C_LABEL(mips_options) + MO_NUM_TLB_ENTRIES
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
	and	a0, MIPS3_PG_G			# check to see it has G bit
	beqz	a0, 2f				# no, skip this entry
	 nop
	PTR_ADDU ta0, v0

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
	addu	t1, 1
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
END(MIPSX(tlb_invalidate_globals))

/*
 * void mipsN_tlb_invalidate_all(void);
 *
 * Invalidate all of non-wired TLB entries.
 */
LEAF_NOPROFILE(MIPSX(tlb_invalidate_all))
	mfc0	v1, MIPS_COP_0_STATUS		# save status register
	mtc0	zero, MIPS_COP_0_STATUS		# disable interrupts
	COP0_SYNC

	INT_L	a0, _C_LABEL(mips_options) + MO_NUM_TLB_ENTRIES

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
	PTR_ADDU ta0, v0
	_MTC0	ta0, MIPS_COP_0_TLB_HI		# make entryHi invalid
	COP0_SYNC
	nop
	nop
	tlbwi					# clear the entry
	COP0_SYNC
	addu	t1, 1				# increment index
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
END(MIPSX(tlb_invalidate_all))

/*
 * mipsX_tlb_enter(size_t tlb_index, vaddr_t va, uint32_t pte);
 */
LEAF(MIPSX(tlb_enter))
	.set	noat
	mfc0	ta0, MIPS_COP_0_TLB_HI		# save EntryHi

	and	a3, a1, MIPS3_PG_ODDPG		# select odd page bit
	xor	a3, a1				# clear it.
	mtc0	a3, MIPS_COP_0_TLB_HI		# set the VA for tlbp
	COP0_SYNC
	nop
	nop

	and	t2, a2, MIPS3_PG_G		# make prototype tlb_lo0
	and	t3, a2, MIPS3_PG_G		# make prototype tlb_lo1

	tlbp					# is va in TLB?
	nop
	nop
	nop

	mfc0	v0, MIPS_COP_0_TLB_INDEX	# was it in the TLB?
	bltz	v0, 1f				# nope
	 nop

	/*
	 * Get the existing tlb_lo's because we need to replace one of them.
	 */
	mfc0	t2, MIPS_COP_0_TLB_LO0		# save for update
	mfc0	t3, MIPS_COP_0_TLB_LO1		# save for update

	/*
	 * If it's already where we want, no reason to invalidate it.
	 */
	beq	v0, a0, 2f			# already where we want it?
	 nop					

	/*
	 * Make an
	 */
	sll	t1, 1 + PGSHIFT			# make a fake addr for the entry
	lui	v1, %hi(MIPS_KSEG0_START)
	or	t1, v1
	mtc0	t1, MIPS_COP_0_TLB_HI
	COP0_SYNC
	nop
	nop

	and	t0, a2, MIPS3_PG_G		# make prototype tlb_lo
	mtc0	t0, MIPS_COP_0_TLB_LO0		# use an invalid tlb_lo
	COP0_SYNC
	mtc0	t0, MIPS_COP_0_TLB_LO1		# use an invalid tlb_lo
	COP0_SYNC

	tlbwi					# now write the invalid TLB
	COP0_SYNC

	mtc0	a3, MIPS_COP_0_TLB_HI		# restore the addr for new TLB
	COP0_SYNC
	nop
	nop
1:
	mtc0	a0, MIPS_COP_0_TLB_INDEX	# set the index
	COP0_SYNC

2:
	and	v1, a1, MIPS3_PG_ODDPG		# odd or even page
	sll	v1, 31 - PGSHIFT		# move to MSB
	sra	v1, 31				# v1 a mask (0/~0 = even/odd)
	not	v0, v1				# v0 a mask (~0/0 = even/odd)

	and	ta2, t2, v1
	and	ta3, a2, v0
	or	t2, ta2, ta3			# t2 = (v1 & t2) | (~v1 & a2)
	and	ta2, t3, v0
	and	ta3, a2, v1
	or	t3, ta2, ta3			# t3 = (~v1 & t3) | (v1 & a2)

	mtc0	t2, MIPS_COP_0_TLB_LO0		# set tlb_lo0 (even)
	COP0_SYNC
	mtc0	t3, MIPS_COP_0_TLB_LO1		# set tlb_lo1 (odd)
	COP0_SYNC

	tlbwi					# enter it into the TLB
	nop
	nop
	nop

	mtc0	ta1, MIPS_COP_0_TLB_HI		# restore EntryHi
	COP0_SYNC

	j	ra
	 nop
	.set	at
END(MIPSX(tlb_enter))

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

	.globl _C_LABEL(MIPSX(locore_vec))
_C_LABEL(MIPSX(locore_vec)):
	PTR_WORD _C_LABEL(MIPSX(tlb_set_asid))
	PTR_WORD _C_LABEL(MIPSX(tlb_invalidate_asids))
	PTR_WORD _C_LABEL(MIPSX(tlb_invalidate_addr))
	PTR_WORD _C_LABEL(MIPSX(tlb_invalidate_globals))
	PTR_WORD _C_LABEL(MIPSX(tlb_invalidate_all))
	PTR_WORD _C_LABEL(MIPSX(tlb_record_asids))
	PTR_WORD _C_LABEL(MIPSX(tlb_update))
	PTR_WORD _C_LABEL(MIPSX(tlb_enter))
	PTR_WORD _C_LABEL(MIPSX(tlb_read_indexed))
	PTR_WORD _C_LABEL(MIPSX(wbflush))

	.globl _C_LABEL(MIPSX(locoresw))
_C_LABEL(MIPSX(locoresw)):
	PTR_WORD _C_LABEL(MIPSX(cpu_switch_resume))
	PTR_WORD _C_LABEL(MIPSX(lwp_trampoline))
	PTR_WORD _C_LABEL(nullop)	# cpu_idle
	PTR_WORD _C_LABEL(MIPSX(setfunc_trampoline))
	PTR_WORD _C_LABEL(nullop)	# send ipi
	PTR_WORD _C_LABEL(nullop)	# cpu_offline_md
	PTR_WORD _C_LABEL(nullop)	# spare
	PTR_WORD _C_LABEL(nullop)	# spare

MIPSX(excpt_sw):
	####
	#### The kernel exception handlers.
	####
	PTR_WORD _C_LABEL(MIPSX(kern_intr))		#  0 external interrupt
	PTR_WORD _C_LABEL(MIPSX(kern_gen_exception))	#  1 TLB modification
	PTR_WORD _C_LABEL(MIPSX(tlb_invalid_exception))	#  2 TLB miss (LW/I-fetch)
	PTR_WORD _C_LABEL(MIPSX(tlb_invalid_exception))	#  3 TLB miss (SW)
	PTR_WORD _C_LABEL(MIPSX(kern_gen_exception))	#  4 address error (LW/I-fetch)
	PTR_WORD _C_LABEL(MIPSX(kern_gen_exception))	#  5 address error (SW)
	PTR_WORD _C_LABEL(MIPSX(kern_gen_exception))	#  6 bus error (I-fetch)
	PTR_WORD _C_LABEL(MIPSX(kern_gen_exception))	#  7 bus error (load or store)
	PTR_WORD _C_LABEL(MIPSX(kern_gen_exception))	#  8 system call
	PTR_WORD _C_LABEL(MIPSX(kern_gen_exception))	#  9 breakpoint
	PTR_WORD _C_LABEL(MIPSX(kern_gen_exception))	# 10 reserved instruction
	PTR_WORD _C_LABEL(MIPSX(kern_gen_exception))	# 11 coprocessor unusable
	PTR_WORD _C_LABEL(MIPSX(kern_gen_exception))	# 12 arithmetic overflow
	PTR_WORD _C_LABEL(MIPSX(kern_gen_exception))	# 13 r4k trap exception
#if defined(MIPS3) && !defined(MIPS3_5900)
	PTR_WORD _C_LABEL(mips3_VCEI)			# 14 r4k virt coherence
#else
	PTR_WORD _C_LABEL(MIPSX(kern_gen_exception))	# 14 reserved
#endif
	PTR_WORD _C_LABEL(MIPSX(kern_gen_exception))	# 15 r4k FP exception
	PTR_WORD _C_LABEL(MIPSX(kern_gen_exception))	# 16 reserved
	PTR_WORD _C_LABEL(MIPSX(kern_gen_exception))	# 17 reserved
	PTR_WORD _C_LABEL(MIPSX(kern_gen_exception))	# 18 reserved
	PTR_WORD _C_LABEL(MIPSX(kern_gen_exception))	# 19 reserved
	PTR_WORD _C_LABEL(MIPSX(kern_gen_exception))	# 20 reserved
	PTR_WORD _C_LABEL(MIPSX(kern_gen_exception))	# 21 reserved
	PTR_WORD _C_LABEL(MIPSX(kern_gen_exception))	# 22 reserved
	PTR_WORD _C_LABEL(MIPSX(kern_gen_exception))	# 23 watch exception
	PTR_WORD _C_LABEL(MIPSX(kern_gen_exception))	# 24 reserved
	PTR_WORD _C_LABEL(MIPSX(kern_gen_exception))	# 25 reserved
	PTR_WORD _C_LABEL(MIPSX(kern_gen_exception))	# 26 reserved
	PTR_WORD _C_LABEL(MIPSX(kern_gen_exception))	# 27 reserved
	PTR_WORD _C_LABEL(MIPSX(kern_gen_exception))	# 28 reserved
	PTR_WORD _C_LABEL(MIPSX(kern_gen_exception))	# 29 reserved
	PTR_WORD _C_LABEL(MIPSX(kern_gen_exception))	# 30 reserved
#if defined(MIPS3) && !defined(MIPS3_5900)
	PTR_WORD _C_LABEL(mips3_VCED)		# 31 v. coherence exception data
#else
	PTR_WORD _C_LABEL(MIPSX(kern_gen_exception))	# 31 reserved
#endif
	#####
	##### The user exception handlers.
	#####
	PTR_WORD _C_LABEL(MIPSX(user_intr))		#  0
	PTR_WORD _C_LABEL(MIPSX(user_gen_exception))	#  1
	PTR_WORD _C_LABEL(MIPSX(user_gen_exception))	#  2
	PTR_WORD _C_LABEL(MIPSX(user_gen_exception))	#  3
	PTR_WORD _C_LABEL(MIPSX(user_gen_exception))	#  4
	PTR_WORD _C_LABEL(MIPSX(user_gen_exception))	#  5
	PTR_WORD _C_LABEL(MIPSX(user_gen_exception))	#  6
	PTR_WORD _C_LABEL(MIPSX(user_gen_exception))	#  7
	PTR_WORD _C_LABEL(MIPSX(systemcall))		#  8
	PTR_WORD _C_LABEL(MIPSX(user_gen_exception))	#  9
	PTR_WORD _C_LABEL(MIPSX(user_gen_exception))	# 10
	PTR_WORD _C_LABEL(MIPSX(user_gen_exception))	# 11
	PTR_WORD _C_LABEL(MIPSX(user_gen_exception))	# 12
	PTR_WORD _C_LABEL(MIPSX(user_gen_exception))	# 13
#if defined(MIPS3) && !defined(MIPS3_5900)
	PTR_WORD _C_LABEL(mips3_VCEI)			# 14
#else
	PTR_WORD _C_LABEL(MIPSX(user_gen_exception))	# 14
#endif
	PTR_WORD _C_LABEL(MIPSX(user_gen_exception))	# 15
	PTR_WORD _C_LABEL(MIPSX(user_gen_exception))	# 16
	PTR_WORD _C_LABEL(MIPSX(user_gen_exception))	# 17
	PTR_WORD _C_LABEL(MIPSX(user_gen_exception))	# 18
	PTR_WORD _C_LABEL(MIPSX(user_gen_exception))	# 19
	PTR_WORD _C_LABEL(MIPSX(user_gen_exception))	# 20
	PTR_WORD _C_LABEL(MIPSX(user_gen_exception))	# 21
	PTR_WORD _C_LABEL(MIPSX(user_gen_exception))	# 22
	PTR_WORD _C_LABEL(MIPSX(user_gen_exception))	# 23
	PTR_WORD _C_LABEL(MIPSX(user_gen_exception))	# 24
	PTR_WORD _C_LABEL(MIPSX(user_gen_exception))	# 25
	PTR_WORD _C_LABEL(MIPSX(user_gen_exception))	# 26
	PTR_WORD _C_LABEL(MIPSX(user_gen_exception))	# 27
	PTR_WORD _C_LABEL(MIPSX(user_gen_exception))	# 28
	PTR_WORD _C_LABEL(MIPSX(user_gen_exception))	# 29
	PTR_WORD _C_LABEL(MIPSX(user_gen_exception))	# 30
#if defined(MIPS3) && !defined(MIPS3_5900)
	PTR_WORD _C_LABEL(mips3_VCED)			# 31 v. coherence exception data
#else
	PTR_WORD _C_LABEL(MIPSX(user_gen_exception))	# 31
#endif
