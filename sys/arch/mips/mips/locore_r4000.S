/*	$NetBSD: locore_r4000.S,v 1.39 1997/07/07 03:54:35 jonathan Exp $	*/

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

/*
 *============================================================================
 *
 *  Mips-III  ISA support, part 1: locore exception vectors.
 *  The following code is copied to the vector locations to which
 *  the CPU jumps in response to an exception or a TLB miss.
 *
 *============================================================================


/*
 *----------------------------------------------------------------------------
 *
 * mips3_TLBMiss --
 * MachTLBMiss --
 *
 *	Vector code for the TLB-miss exception vector 0x80000180
 *	on an r4000.
 *
 * This code is copied to the TLB exception vector address to
 * handle TLB translation misses.
 * NOTE: This code must be relocatable and max 32 instructions!!!
 * Don't check for invalid pte's here. We load them as well and
 * let the processor trap to load the correct value after service.
 *
 *----------------------------------------------------------------------------
 */
	.globl	_C_LABEL(mips3_TLBMiss)
_C_LABEL(mips3_TLBMiss):
	.globl	_C_LABEL(MachTLBMiss)
_C_LABEL(MachTLBMiss):
	.set	noat
	.set	mips3
	dmfc0	k0, MIPS_COP_0_BAD_VADDR	# get the virtual address
	lw	k1, _C_LABEL(curpcb)
	bltz	k0, 1f				# kernel address space ->
	nop
	bnez	k1, 2f
	nop
	la	k1, _C_LABEL(proc0paddr)
2:
	lw	k1, U_PCB_SEGTAB(k1)		# get the current segment table
	srl	k0, k0, SEGSHIFT - 2		# compute segment table index
	andi	k0, k0, 0x7fc			# PMAP_SEGTABSIZ-1
	addu	k1, k1, k0
	dmfc0	k0, MIPS_COP_0_BAD_VADDR	# get the virtual address
	lw	k1, 0(k1)			# get pointer to segment map
	srl	k0, k0, PGSHIFT - 2		# compute segment map index
	andi	k0, k0, ((NPTEPG/2) - 1) << 3
	beq	k1, zero, 2f			# invalid segment map
	addu	k1, k1, k0			# index into segment map
	lw	k0, 0(k1)			# get page PTE
	lw	k1, 4(k1)
	dsll	k0, k0, 34
	dsrl	k0, k0, 34
	dmtc0	k0, MIPS_COP_0_TLB_LO0
	dsll	k1, k1, 34
	dsrl	k1, k1, 34
	dmtc0	k1, MIPS_COP_0_TLB_LO1
	nop
	tlbwr					# update TLB
	nop
	nop
	nop
	nop
	nop
	eret
1:
	j	mips3_TLBMissException
	nop
2:
	j	mips3_SlowFault
	nop

	.globl	_C_LABEL(MachTLBMissEnd)
_C_LABEL(MachTLBMissEnd):
	.globl	_C_LABEL(mips3_TLBMissEnd)
_C_LABEL(mips3_TLBMissEnd):
	.set	at

/*
 *----------------------------------------------------------------------------
 *
 * mips3_execption --
 *
 *	Vector code for the general exception vector 0x80000180
 *	on an r4000 or r4400.
 *
 * This code is copied to the general exception vector address to
 * handle all execptions except RESET and TLBMiss.
 * NOTE: This code must be relocatable!!!
 *----------------------------------------------------------------------------
 */
	.globl	mips3_exception
_C_LABEL(mips3_exception):
/*
 * Find out what mode we came from and jump to the proper handler.
 */
	.set	noat
	mfc0	k0, MIPS_COP_0_STATUS_REG	# Get the status register
	mfc0	k1, MIPS_COP_0_CAUSE_REG	# Get the cause register value.
	and	k0, k0, MIPS3_SR_KSU_USER	# test for user mode
						# sneaky but the bits are
						# with us........
	sll	k0, k0, 3			# shift user bit for cause index
	and	k1, k1, MIPS3_CR_EXC_CODE	# Mask out the cause bits.
	or	k1, k1, k0			# change index to user table
1:
	la	k0, mips3_ExceptionTable	# get base of the jump table
	addu	k0, k0, k1			# Get the address of the
						#  function entry.  Note that
						#  the cause is already
						#  shifted left by 2 bits so
						#  we dont have to shift.
	lw	k0, 0(k0)			# Get the function address
	nop
	j	k0				# Jump to the function.
	nop
	.set	at
	.globl	mips3_exceptionEnd
_C_LABEL(mips3_exceptionEnd):


/*----------------------------------------------------------------------------
 *
 * mips3_SlowFault --
 *
 * Alternate entry point into the mips3_UserGenExceptionor or
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
	mfc0	k0, MIPS_COP_0_STATUS_REG
	nop
	and	k0, k0, MIPS3_SR_KSU_USER
	bne	k0, zero, mips3_UserGenException
	nop
	.set	at
/*
 * Fall though ...
 */

/*----------------------------------------------------------------------------
 *
 * mips3_KernGenException --
 *
 *	Handle an exception from kernel mode.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------------
 */

/*
 * XXX - incorrect now.
 * The kernel exception stack contains 18 saved general registers,
 * the status register and the multiply lo and high registers.
 * In addition, we set this up for linkage conventions.
 */
#define TRAP_FRAME_SIZE	(4 * 38 + 4 * 4 + 4 + 4)
#define TRAP_RA_OFFSET	(TRAP_FRAME_SIZE - 4)
#define TRAP_FRAME_OFFSET (4 * 4)

/*
 * Similar definition for interrupt-exception frames.
 */
#define KINTR_FRAME_SIZE	(4 * 38 + 4 * 4 + 4 + 4)
#define KINTR_RA_OFFSET	STAND_RA_OFFSET
#define KINTR_FRAME_OFFSET STAND_FRAME_SIZE

NNON_LEAF(mips3_KernGenException, TRAP_FRAME_SIZE, ra)
	.set	noat
	subu	sp, sp, TRAP_FRAME_SIZE
	.mask	0x80000000, (TRAP_RA_OFFSET - TRAP_FRAME_SIZE)
/*
 * Save the relevant kernel registers onto the stack.
 * We don't need to save s0 - s8, sp and gp because
 * the compiler does it for us.
 */
	sw	AT, TRAP_FRAME_OFFSET + (AST * 4)(sp)
	sw	v0, TRAP_FRAME_OFFSET + (V0 * 4)(sp)
	sw	v1, TRAP_FRAME_OFFSET + (V1 * 4)(sp)
	sw	a0, TRAP_FRAME_OFFSET + (A0 * 4)(sp)
	mflo	v0
	mfhi	v1
	sw	a1, TRAP_FRAME_OFFSET + (A1 * 4)(sp)
	sw	a2, TRAP_FRAME_OFFSET + (A2 * 4)(sp)
	sw	a3, TRAP_FRAME_OFFSET + (A3 * 4)(sp)
	sw	t0, TRAP_FRAME_OFFSET + (T0 * 4)(sp)
	mfc0	a0, MIPS_COP_0_STATUS_REG	# First arg is the status reg.
	sw	t1, TRAP_FRAME_OFFSET + (T1 * 4)(sp)
	sw	t2, TRAP_FRAME_OFFSET + (T2 * 4)(sp)
	sw	t3, TRAP_FRAME_OFFSET + (T3 * 4)(sp)
	sw	t4, TRAP_FRAME_OFFSET + (T4 * 4)(sp)
	mfc0	a1, MIPS_COP_0_CAUSE_REG	# Second arg is the cause reg.
	sw	t5, TRAP_FRAME_OFFSET + (T5 * 4)(sp)
	sw	t6, TRAP_FRAME_OFFSET + (T6 * 4)(sp)
	sw	t7, TRAP_FRAME_OFFSET + (T7 * 4)(sp)
	sw	t8, TRAP_FRAME_OFFSET + (T8 * 4)(sp)
	mfc0	a2, MIPS_COP_0_BAD_VADDR	# Third arg is the fault addr.
	sw	t9, TRAP_FRAME_OFFSET + (T9 * 4)(sp)
	sw	ra, TRAP_FRAME_OFFSET + (RA * 4)(sp)
	sw	v0, TRAP_FRAME_OFFSET + (MULLO * 4)(sp)
	sw	v1, TRAP_FRAME_OFFSET + (MULHI * 4)(sp)
	mfc0	a3, MIPS_COP_0_EXC_PC		# Fourth arg is the pc.
	sw	a0, TRAP_FRAME_OFFSET + (SR * 4)(sp)

#ifdef DDB
	addu	v0, sp, TRAP_FRAME_SIZE 	#  SP at trap for debugging
	sw	v0, TRAP_FRAME_OFFSET+(SP * 4)(sp)
#else
	nop
#endif DDB
	sw	a3, TRAP_FRAME_OFFSET + (PC * 4)(sp)
	mtc0	zero,MIPS_COP_0_STATUS_REG	# Set kernel no error level
/*
 * Call the exception handler.
 */
	jal	_C_LABEL(trap)
	sw	a3, TRAP_RA_OFFSET(sp)		# for debugging
/*
 * Restore registers and return from the exception.
 */
	mtc0	zero,MIPS_COP_0_STATUS_REG	# Make sure int disabled
	nop
	lw	a0, TRAP_FRAME_OFFSET + (SR * 4)(sp)
	lw	v0, TRAP_FRAME_OFFSET + (PC * 4)(sp)	# might be changed inside trap
	lw	t0, TRAP_FRAME_OFFSET + (MULLO * 4)(sp)
	lw	t1, TRAP_FRAME_OFFSET + (MULHI * 4)(sp)
	nop
	mtc0	a0, MIPS_COP_0_STATUS_REG	# Restore the SR, disable intrs
	mtlo	t0
	mthi	t1
	dmtc0	v0, MIPS_COP_0_EXC_PC		# set return address
	nop

	lw	AT, TRAP_FRAME_OFFSET + (AST * 4)(sp)
	lw	v0, TRAP_FRAME_OFFSET + (V0 * 4)(sp)
	lw	v1, TRAP_FRAME_OFFSET + (V1 * 4)(sp)
	lw	a0, TRAP_FRAME_OFFSET + (A0 * 4)(sp)
	lw	a1, TRAP_FRAME_OFFSET + (A1 * 4)(sp)
	lw	a2, TRAP_FRAME_OFFSET + (A2 * 4)(sp)
	lw	a3, TRAP_FRAME_OFFSET + (A3 * 4)(sp)
	lw	t0, TRAP_FRAME_OFFSET + (T0 * 4)(sp)
	lw	t1, TRAP_FRAME_OFFSET + (T1 * 4)(sp) 
	lw	t2, TRAP_FRAME_OFFSET + (T2 * 4)(sp)
	lw	t3, TRAP_FRAME_OFFSET + (T3 * 4)(sp)
	lw	t4, TRAP_FRAME_OFFSET + (T4 * 4)(sp)
	lw	t5, TRAP_FRAME_OFFSET + (T5 * 4)(sp)
	lw	t6, TRAP_FRAME_OFFSET + (T6 * 4)(sp)
	lw	t7, TRAP_FRAME_OFFSET + (T7 * 4)(sp)
	lw	t8, TRAP_FRAME_OFFSET + (T8 * 4)(sp)
	lw	t9, TRAP_FRAME_OFFSET + (T9 * 4)(sp)	
	lw	ra, TRAP_FRAME_OFFSET + (RA * 4)(sp)

	addu	sp, sp, TRAP_FRAME_SIZE
	eret					#  exception.
	.set	at
END(mips3_KernGenException)

/*----------------------------------------------------------------------------
 *
 * mips3_UserGenException --
 *
 *	Handle an exception from user mode.
 *
 * Results:
 * 	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------------
 */
NNON_LEAF(mips3_UserGenException, STAND_FRAME_SIZE, ra)
	.set	noat
	.mask	0x80000000, (STAND_RA_OFFSET - STAND_FRAME_SIZE)
/*
 * Save all of the registers except for the kernel temporaries in u.u_pcb.
 */
	lw	k1, _C_LABEL(curpcb)
	sw	AT, U_PCB_REGS+(AST * 4)(k1)
	sw	v0, U_PCB_REGS+(V0 * 4)(k1)
	sw	v1, U_PCB_REGS+(V1 * 4)(k1)
	sw	a0, U_PCB_REGS+(A0 * 4)(k1)
	mflo	v0
	sw	a1, U_PCB_REGS+(A1 * 4)(k1)
	sw	a2, U_PCB_REGS+(A2 * 4)(k1)
	sw	a3, U_PCB_REGS+(A3 * 4)(k1)
	sw	t0, U_PCB_REGS+(T0 * 4)(k1)
	mfhi	v1
	sw	t1, U_PCB_REGS+(T1 * 4)(k1)
	sw	t2, U_PCB_REGS+(T2 * 4)(k1)
	sw	t3, U_PCB_REGS+(T3 * 4)(k1)
	sw	t4, U_PCB_REGS+(T4 * 4)(k1)
	mfc0	a0, MIPS_COP_0_STATUS_REG	# First arg is the status reg.
	sw	t5, U_PCB_REGS+(T5 * 4)(k1)
	sw	t6, U_PCB_REGS+(T6 * 4)(k1)
	sw	t7, U_PCB_REGS+(T7 * 4)(k1)
	sw	s0, U_PCB_REGS+(S0 * 4)(k1)
	mfc0	a1, MIPS_COP_0_CAUSE_REG	# Second arg is the cause reg.
	sw	s1, U_PCB_REGS+(S1 * 4)(k1)
	sw	s2, U_PCB_REGS+(S2 * 4)(k1)
	sw	s3, U_PCB_REGS+(S3 * 4)(k1)
	sw	s4, U_PCB_REGS+(S4 * 4)(k1)
	mfc0	a2, MIPS_COP_0_BAD_VADDR	# Third arg is the fault addr
	sw	s5, U_PCB_REGS+(S5 * 4)(k1)
	sw	s6, U_PCB_REGS+(S6 * 4)(k1)
	sw	s7, U_PCB_REGS+(S7 * 4)(k1)
	sw	t8, U_PCB_REGS+(T8 * 4)(k1)
	mfc0	a3, MIPS_COP_0_EXC_PC		# Fourth arg is the pc.
	sw	t9, U_PCB_REGS+(T9 * 4)(k1)
	sw	gp, U_PCB_REGS+(GP * 4)(k1)
	sw	sp, U_PCB_REGS+(SP * 4)(k1)
	sw	s8, U_PCB_REGS+(S8 * 4)(k1)
	li	sp, KERNELSTACK - STAND_FRAME_SIZE	# switch to kernel SP
	sw	ra, U_PCB_REGS+(RA * 4)(k1)
	sw	v0, U_PCB_REGS+(MULLO * 4)(k1)
	sw	v1, U_PCB_REGS+(MULHI * 4)(k1)
	sw	a0, U_PCB_REGS+(SR * 4)(k1)
#ifdef __GP_SUPPORT__
	la	gp, _C_LABEL(_gp)			# switch to kernel GP
#endif
	sw	a3, U_PCB_REGS+(PC * 4)(k1)
	sw	a3, STAND_RA_OFFSET(sp)		# for debugging
	.set	at
# Turn off fpu and enter kernel mode
	and	t0, a0, ~(MIPS_SR_COP_1_BIT | MIPS_SR_EXL | MIPS_SR_KSU_MASK | MIPS_SR_INT_IE)
	.set	noat
/*
 * Call the exception handler.
 */
	jal	_C_LABEL(trap)
	mtc0	t0, MIPS_COP_0_STATUS_REG
/*
 * Restore user registers and return.
 * First disable interrupts and set exeption level.
 */
	mtc0	zero, MIPS_COP_0_STATUS_REG	# disable int
	nop
	nop
	nop

	li	v0, MIPS_SR_EXL
	mtc0	v0, MIPS_COP_0_STATUS_REG	# set exception level
	nop
	nop
	nop

	lw	k1, _C_LABEL(curpcb)
 #	lw	a0, U_PCB_REGS+(SR * 4)(k1)
	lw	t0, U_PCB_REGS+(MULLO * 4)(k1)
	lw	t1, U_PCB_REGS+(MULHI * 4)(k1)
	nop
	nop
 #	mtc0	a0, MIPS_COP_0_STATUS_REG	# still exeption level
	mtlo	t0
	mthi	t1
	lw	a0, U_PCB_REGS+(PC * 4)(k1)
	lw	AT, U_PCB_REGS+(AST * 4)(k1)
	lw	v0, U_PCB_REGS+(V0 * 4)(k1)
	nop
	nop
	dmtc0	a0, MIPS_COP_0_EXC_PC		# set return address
	nop
	nop

	lw	v1, U_PCB_REGS+(V1 * 4)(k1)
	lw	a0, U_PCB_REGS+(A0 * 4)(k1)
	lw	a1, U_PCB_REGS+(A1 * 4)(k1)
	lw	a2, U_PCB_REGS+(A2 * 4)(k1)
	lw	a3, U_PCB_REGS+(A3 * 4)(k1)
	lw	t0, U_PCB_REGS+(T0 * 4)(k1)
	lw	t1, U_PCB_REGS+(T1 * 4)(k1)
	lw	t2, U_PCB_REGS+(T2 * 4)(k1)
	lw	t3, U_PCB_REGS+(T3 * 4)(k1)
	lw	t4, U_PCB_REGS+(T4 * 4)(k1)
	lw	t5, U_PCB_REGS+(T5 * 4)(k1)
	lw	t6, U_PCB_REGS+(T6 * 4)(k1)
	lw	t7, U_PCB_REGS+(T7 * 4)(k1)
	lw	s0, U_PCB_REGS+(S0 * 4)(k1)
	lw	s1, U_PCB_REGS+(S1 * 4)(k1)
	lw	s2, U_PCB_REGS+(S2 * 4)(k1)
	lw	s3, U_PCB_REGS+(S3 * 4)(k1)
	lw	s4, U_PCB_REGS+(S4 * 4)(k1)
	lw	s5, U_PCB_REGS+(S5 * 4)(k1)
	lw	s6, U_PCB_REGS+(S6 * 4)(k1)
	lw	s7, U_PCB_REGS+(S7 * 4)(k1)
	lw	t8, U_PCB_REGS+(T8 * 4)(k1)
	lw	t9, U_PCB_REGS+(T9 * 4)(k1)
	lw	gp, U_PCB_REGS+(GP * 4)(k1)
	lw	sp, U_PCB_REGS+(SP * 4)(k1)
	lw	s8, U_PCB_REGS+(S8 * 4)(k1)
	lw	ra, U_PCB_REGS+(RA * 4)(k1)
	lw	k0, U_PCB_REGS+(SR * 4)(k1)
	nop
	mtc0	k0, MIPS_COP_0_STATUS_REG	# still exeption level

	eret
	.set	at
END(mips3_UserGenException)

/*
 * System call handler
 */
NNON_LEAF(mips3_SystemCall, STAND_FRAME_SIZE, ra)
	.set	noat
	.mask	0x80000000, (STAND_RA_OFFSET - STAND_FRAME_SIZE)
	lw	k1, _C_LABEL(curpcb)
	sw	AT, U_PCB_REGS+(AST * 4)(k1)
	sw	v0, U_PCB_REGS+(V0 * 4)(k1)
	sw	v1, U_PCB_REGS+(V1 * 4)(k1)
	mflo	v0
	mfhi	v1
	sw	a0, U_PCB_REGS+(A0 * 4)(k1)
	sw	a1, U_PCB_REGS+(A1 * 4)(k1)
	sw	a2, U_PCB_REGS+(A2 * 4)(k1)
	sw	a3, U_PCB_REGS+(A3 * 4)(k1)
	sw	t0, U_PCB_REGS+(T0 * 4)(k1)
	sw	t1, U_PCB_REGS+(T1 * 4)(k1)
	sw	t2, U_PCB_REGS+(T2 * 4)(k1)
	sw	t3, U_PCB_REGS+(T3 * 4)(k1)
	sw	t4, U_PCB_REGS+(T4 * 4)(k1)
	sw	t5, U_PCB_REGS+(T5 * 4)(k1)
	sw	t6, U_PCB_REGS+(T6 * 4)(k1)
	sw	t7, U_PCB_REGS+(T7 * 4)(k1)
	sw	s0, U_PCB_REGS+(S0 * 4)(k1)
	sw	s1, U_PCB_REGS+(S1 * 4)(k1)
	sw	s2, U_PCB_REGS+(S2 * 4)(k1)
	sw	s3, U_PCB_REGS+(S3 * 4)(k1)
	sw	s4, U_PCB_REGS+(S4 * 4)(k1)
	sw	s5, U_PCB_REGS+(S5 * 4)(k1)
	mfc0	a0, MIPS_COP_0_STATUS_REG	# 1st arg is STATUS
	sw	s6, U_PCB_REGS+(S6 * 4)(k1)
	sw	s7, U_PCB_REGS+(S7 * 4)(k1)
	sw	t8, U_PCB_REGS+(T8 * 4)(k1)
	sw	t9, U_PCB_REGS+(T9 * 4)(k1)
	mfc0	a1, MIPS_COP_0_CAUSE_REG	# 2nd arg is CAUSE
	sw	gp, U_PCB_REGS+(GP * 4)(k1)
	sw	sp, U_PCB_REGS+(SP * 4)(k1)
	sw	s8, U_PCB_REGS+(S8 * 4)(k1)
	sw	ra, U_PCB_REGS+(RA * 4)(k1)
	mfc0	a2, MIPS_COP_0_EXC_PC		# 3rd arg is PC
	la	a3, U_PCB_REGS(k1)		# 4th arg is p. to trapframe
	sw	a0, U_PCB_REGS+(SR * 4)(k1)
	sw	v0, U_PCB_REGS+(MULLO * 4)(k1)
	sw	v1, U_PCB_REGS+(MULHI * 4)(k1)
	sw	a2, U_PCB_REGS+(PC * 4)(k1)
	li	sp, KERNELSTACK - STAND_FRAME_SIZE	# switch to kernel SP
	sw	a2, STAND_RA_OFFSET(sp)		# for debugging
#ifdef __GP_SUPPORT__
	la	gp, _C_LABEL(_gp)		# switch to kernel GP
#endif
	.set	at
# Turn off fpu and enter kernel mode
	and	t0, a0, ~(MIPS_SR_COP_1_BIT | MIPS_SR_EXL | MIPS_SR_KSU_MASK | MIPS_SR_INT_IE)
	.set	noat
	jal	_C_LABEL(syscall)
	mtc0	t0, MIPS_COP_0_STATUS_REG
/*
 * Restore user registers and return.
 * First disable interrupts and set exeption level.
 */
	mtc0	zero, MIPS_COP_0_STATUS_REG	# disable int
	nop
	nop
	nop

	li	v0, MIPS_SR_EXL
	mtc0	v0, MIPS_COP_0_STATUS_REG	# set exception level
	nop
	nop
	nop

/*
 * Restore user registers and return.
 */
	lw	k1, _C_LABEL(curpcb)
 #	lw	a0, U_PCB_REGS+(SR * 4)(k1)
	lw	t0, U_PCB_REGS+(MULLO * 4)(k1)
	lw	t1, U_PCB_REGS+(MULHI * 4)(k1)
 #	mtc0	a0, MIPS_COP_0_STATUS_REG	# this should disable interrupts
	mtlo	t0
	mthi	t1
	lw	k0, U_PCB_REGS+(PC * 4)(k1)	# might be changed in syscall
	dmtc0	k0, MIPS_COP_0_EXC_PC		# set return address
	nop
	nop
	lw	AT, U_PCB_REGS+(AST * 4)(k1)
	lw	v0, U_PCB_REGS+(V0 * 4)(k1)
	lw	v1, U_PCB_REGS+(V1 * 4)(k1)
	lw	a0, U_PCB_REGS+(A0 * 4)(k1)
	lw	a1, U_PCB_REGS+(A1 * 4)(k1)
	lw	a2, U_PCB_REGS+(A2 * 4)(k1)
	lw	a3, U_PCB_REGS+(A3 * 4)(k1)
	lw	t0, U_PCB_REGS+(T0 * 4)(k1)
	lw	t1, U_PCB_REGS+(T1 * 4)(k1)
	lw	t2, U_PCB_REGS+(T2 * 4)(k1)
	lw	t3, U_PCB_REGS+(T3 * 4)(k1)
	lw	t4, U_PCB_REGS+(T4 * 4)(k1)
	lw	t5, U_PCB_REGS+(T5 * 4)(k1)
	lw	t6, U_PCB_REGS+(T6 * 4)(k1)
	lw	t7, U_PCB_REGS+(T7 * 4)(k1)
	lw	s0, U_PCB_REGS+(S0 * 4)(k1)
	lw	s1, U_PCB_REGS+(S1 * 4)(k1)
	lw	s2, U_PCB_REGS+(S2 * 4)(k1)
	lw	s3, U_PCB_REGS+(S3 * 4)(k1)
	lw	s4, U_PCB_REGS+(S4 * 4)(k1)
	lw	s5, U_PCB_REGS+(S5 * 4)(k1)
	lw	s6, U_PCB_REGS+(S6 * 4)(k1)
	lw	s7, U_PCB_REGS+(S7 * 4)(k1)
	lw	t8, U_PCB_REGS+(T8 * 4)(k1)
	lw	t9, U_PCB_REGS+(T9 * 4)(k1)
	lw	gp, U_PCB_REGS+(GP * 4)(k1)
	lw	sp, U_PCB_REGS+(SP * 4)(k1)
	lw	s8, U_PCB_REGS+(S8 * 4)(k1)
	lw	ra, U_PCB_REGS+(RA * 4)(k1)
	lw	k0, U_PCB_REGS+(SR * 4)(k1)
	nop
	mtc0	k0, MIPS_COP_0_STATUS_REG
	nop
	eret
	.set	at
END(mips3_SystemCall)

/*----------------------------------------------------------------------------
 *
 * mips3_KernIntr --
 *
 *	Handle an interrupt from kernel mode.
 *	Interrupts use the standard kernel stack.
 *	switch_exit sets up a kernel stack after exit so interrupts won't fail.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------------
 */
NNON_LEAF(mips3_KernIntr, KINTR_FRAME_SIZE, ra)
	.set	noat
	subu	sp, sp, KINTR_FRAME_SIZE	# allocate stack frame
	.mask	0x80000000, (STAND_RA_OFFSET - KINTR_FRAME_SIZE)
/*
 * Save the relevant kernel registers onto the stack.
 * We don't need to save s0 - s8, sp and gp because
 * the compiler does it for us.
 */
	sw	AT, KINTR_FRAME_OFFSET + (AST * 4)(sp)
	sw	v0, KINTR_FRAME_OFFSET + (V0 * 4)(sp)
	sw	v1, KINTR_FRAME_OFFSET + (V1 * 4)(sp)
	sw	a0, KINTR_FRAME_OFFSET + (A0 * 4)(sp)
	mflo	v0
	mfhi	v1
	sw	a1, KINTR_FRAME_OFFSET + (A1 * 4)(sp)
	sw	a2, KINTR_FRAME_OFFSET + (A2 * 4)(sp)
	sw	a3, KINTR_FRAME_OFFSET + (A3 * 4)(sp)
	sw	t0, KINTR_FRAME_OFFSET + (T0 * 4)(sp)
	mfc0	a0, MIPS_COP_0_STATUS_REG	# First arg is the status reg.
	sw	t1, KINTR_FRAME_OFFSET + (T1 * 4)(sp)
	sw	t2, KINTR_FRAME_OFFSET + (T2 * 4)(sp)
	sw	t3, KINTR_FRAME_OFFSET + (T3 * 4)(sp)
	sw	t4, KINTR_FRAME_OFFSET + (T4 * 4)(sp)
	mfc0	a1, MIPS_COP_0_CAUSE_REG	# Second arg is the cause reg.
	sw	t5, KINTR_FRAME_OFFSET + (T5 * 4)(sp)
	sw	t6, KINTR_FRAME_OFFSET + (T6 * 4)(sp)
	sw	t7, KINTR_FRAME_OFFSET + (T7 * 4)(sp)
	sw	t8, KINTR_FRAME_OFFSET + (T8 * 4)(sp)
	mfc0	a2, MIPS_COP_0_EXC_PC		# Third arg is the pc.
	sw	t9, KINTR_FRAME_OFFSET + (T9 * 4)(sp)
	sw	ra, KINTR_FRAME_OFFSET + (RA * 4)(sp)
	sw	v0, KINTR_FRAME_OFFSET + (MULLO * 4)(sp)
	sw	v1, KINTR_FRAME_OFFSET + (MULHI * 4)(sp)
	sw	a0, KINTR_FRAME_OFFSET + (SR * 4)(sp)
	la	a3, KINTR_FRAME_OFFSET(sp)	# Fourth arg is address of frame

	nop
	mtc0	zero, MIPS_COP_0_STATUS_REG	# Reset exl, trap possible.
/*
 * Call the interrupt handler.
 */
	jal	_C_LABEL(interrupt)
	sw	a2, KINTR_RA_OFFSET(sp)		# for debugging
/*
 * Restore registers and return from the interrupt.
 */
	mtc0	zero, MIPS_COP_0_STATUS_REG	# Disable interrupt
	nop
	lw	a0, KINTR_FRAME_OFFSET + (SR * 4)(sp)
	lw	t0, KINTR_FRAME_OFFSET + (MULLO * 4)(sp)
	lw	t1, KINTR_FRAME_OFFSET + (MULHI * 4)(sp)
	nop
	mtc0	a0, MIPS_COP_0_STATUS_REG	# Restore the SR, disable intrs
	mtlo	t0
	mthi	t1
	lw	a0, KINTR_RA_OFFSET(sp)
	lw	AT, KINTR_FRAME_OFFSET + (AST * 4)(sp)
	lw	v0, KINTR_FRAME_OFFSET + (V0 * 4)(sp)
	nop
	dmtc0	a0, MIPS_COP_0_EXC_PC		# set return address
	nop

	lw	v1, KINTR_FRAME_OFFSET+(V1 * 4)(sp)
	lw	a0, KINTR_FRAME_OFFSET+(A0 * 4)(sp)
	lw	a1, KINTR_FRAME_OFFSET+(A1 * 4)(sp)
	lw	a2, KINTR_FRAME_OFFSET+(A2 * 4)(sp)
	lw	a3, KINTR_FRAME_OFFSET+(A3 * 4)(sp)
	lw	t0, KINTR_FRAME_OFFSET+(T0 * 4)(sp)
	lw	t1, KINTR_FRAME_OFFSET+(T1 * 4)(sp)
	lw	t2, KINTR_FRAME_OFFSET+(T2 * 4)(sp)
	lw	t3, KINTR_FRAME_OFFSET+(T3 * 4)(sp)
	lw	t4, KINTR_FRAME_OFFSET+(T4 * 4)(sp)
	lw	t5, KINTR_FRAME_OFFSET+(T5 * 4)(sp)
	lw	t6, KINTR_FRAME_OFFSET+(T6 * 4)(sp)
	lw	t7, KINTR_FRAME_OFFSET+(T7 * 4)(sp)
	lw	t8, KINTR_FRAME_OFFSET+(T8 * 4)(sp)
	lw	t9, KINTR_FRAME_OFFSET+(T9 * 4)(sp)
 #	lw	gp, KINTR_FRAME_OFFSET+(GP * 4)(sp)
	lw	ra, KINTR_FRAME_OFFSET+(RA * 4)(sp)

	addu	sp, sp, KINTR_FRAME_SIZE
	eret					#  interrupt.
	.set	at
END(mips3_KernIntr)

/*----------------------------------------------------------------------------
 *
 * mips3_UserIntr --
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
NNON_LEAF(mips3_UserIntr, STAND_FRAME_SIZE, ra)
	.set	noat
	.mask	0x80000000, (STAND_RA_OFFSET - STAND_FRAME_SIZE)
/*
 * Save the relevant user registers into the u.u_pcb struct.
 * We don't need to save s0 - s8 because
 * the compiler does it for us.
 */
	lw	k1, _C_LABEL(curpcb)
	sw	AT, U_PCB_REGS+(AST * 4)(k1)
	sw	v0, U_PCB_REGS+(V0 * 4)(k1)
	sw	v1, U_PCB_REGS+(V1 * 4)(k1)
	sw	a0, U_PCB_REGS+(A0 * 4)(k1)
	mflo	v0
	mfhi	v1
	sw	a1, U_PCB_REGS+(A1 * 4)(k1)
	sw	a2, U_PCB_REGS+(A2 * 4)(k1)
	sw	a3, U_PCB_REGS+(A3 * 4)(k1)
	sw	t0, U_PCB_REGS+(T0 * 4)(k1)
	mfc0	a0, MIPS_COP_0_STATUS_REG	# First arg is the status reg.
	sw	t1, U_PCB_REGS+(T1 * 4)(k1)
	sw	t2, U_PCB_REGS+(T2 * 4)(k1)
	sw	t3, U_PCB_REGS+(T3 * 4)(k1)
	sw	t4, U_PCB_REGS+(T4 * 4)(k1)
	mfc0	a1, MIPS_COP_0_CAUSE_REG	# Second arg is the cause reg.
	sw	t5, U_PCB_REGS+(T5 * 4)(k1)
	sw	t6, U_PCB_REGS+(T6 * 4)(k1)
	sw	t7, U_PCB_REGS+(T7 * 4)(k1)
	sw	t8, U_PCB_REGS+(T8 * 4)(k1)
	mfc0	a2, MIPS_COP_0_EXC_PC		# Third arg is the pc.
	sw	t9, U_PCB_REGS+(T9 * 4)(k1)
	sw	gp, U_PCB_REGS+(GP * 4)(k1)
	sw	sp, U_PCB_REGS+(SP * 4)(k1)
	sw	ra, U_PCB_REGS+(RA * 4)(k1)
	la	a3, U_PCB_REGS(k1)
	li	sp, KERNELSTACK - STAND_FRAME_SIZE	# switch to kernel SP
	sw	v0, U_PCB_REGS+(MULLO * 4)(k1)
	sw	v1, U_PCB_REGS+(MULHI * 4)(k1)
	sw	a0, U_PCB_REGS+(SR * 4)(k1)
	sw	a2, U_PCB_REGS+(PC * 4)(k1)
#ifdef __GP_SUPPORT__
	la	gp, _C_LABEL(_gp)			# switch to kernel GP
#endif
# Turn off fpu and enter kernel mode
	.set	at
	and	t0, a0, ~(MIPS_SR_COP_1_BIT | MIPS_SR_EXL | MIPS_SR_INT_IE | MIPS_SR_KSU_MASK)
	.set	noat
	mtc0	t0, MIPS_COP_0_STATUS_REG
/*
 * Call the interrupt handler.
 */
	jal	_C_LABEL(interrupt)
	sw	a2, STAND_RA_OFFSET(sp)		# for debugging
/*
 * Restore registers and return from the interrupt.
 */
	nop
	mtc0	zero, MIPS_COP_0_STATUS_REG
	nop
	nop
	nop
	li	v0, MIPS_SR_EXL
	mtc0	v0, MIPS_COP_0_STATUS_REG	# set exception level bit.
	nop

	lw	k1, _C_LABEL(curpcb)
 #	lw	a0, U_PCB_REGS+(SR * 4)(k1)
	lw	v0, astpending			# any pending interrupts?
	nop
 #	mtc0	a0, MIPS_COP_0_STATUS_REG	# Restore the SR, disable intrs
	bne	v0, zero, 1f			# dont restore, call softintr
	lw	t0, U_PCB_REGS+(MULLO * 4)(k1)
	lw	t1, U_PCB_REGS+(MULHI * 4)(k1)
	lw	a0, U_PCB_REGS+(PC * 4)(k1)
	lw	AT, U_PCB_REGS+(AST * 4)(k1)
	lw	v0, U_PCB_REGS+(V0 * 4)(k1)
	nop
	dmtc0	a0, MIPS_COP_0_EXC_PC		# set return address
	nop
	lw	v1, U_PCB_REGS+(V1 * 4)(k1)
	lw	a0, U_PCB_REGS+(A0 * 4)(k1)
	lw	a1, U_PCB_REGS+(A1 * 4)(k1)
	lw	a2, U_PCB_REGS+(A2 * 4)(k1)
	lw	a3, U_PCB_REGS+(A3 * 4)(k1)
	mtlo	t0
	mthi	t1
	lw	t0, U_PCB_REGS+(T0 * 4)(k1)
	lw	t1, U_PCB_REGS+(T1 * 4)(k1)
	lw	t2, U_PCB_REGS+(T2 * 4)(k1)
	lw	t3, U_PCB_REGS+(T3 * 4)(k1)
	lw	t4, U_PCB_REGS+(T4 * 4)(k1)
	lw	t5, U_PCB_REGS+(T5 * 4)(k1)
	lw	t6, U_PCB_REGS+(T6 * 4)(k1)
	lw	t7, U_PCB_REGS+(T7 * 4)(k1)
	lw	t8, U_PCB_REGS+(T8 * 4)(k1)
	lw	t9, U_PCB_REGS+(T9 * 4)(k1)
	lw	gp, U_PCB_REGS+(GP * 4)(k1)
	lw	sp, U_PCB_REGS+(SP * 4)(k1)
	lw	ra, U_PCB_REGS+(RA * 4)(k1)
	lw	k0, U_PCB_REGS+(SR * 4)(k1)
	nop
	mtc0	k0, MIPS_COP_0_STATUS_REG	# Restore the SR
	nop
	eret					#  interrupt.

1:
/*
 * We have pending software interrupts; save remaining user state in u.u_pcb.
 */
	sw	s0, U_PCB_REGS+(S0 * 4)(k1)
	sw	s1, U_PCB_REGS+(S1 * 4)(k1)
	sw	s2, U_PCB_REGS+(S2 * 4)(k1)
	sw	s3, U_PCB_REGS+(S3 * 4)(k1)
	sw	s4, U_PCB_REGS+(S4 * 4)(k1)
	sw	s5, U_PCB_REGS+(S5 * 4)(k1)
	sw	s6, U_PCB_REGS+(S6 * 4)(k1)
	sw	s7, U_PCB_REGS+(S7 * 4)(k1)
	sw	s8, U_PCB_REGS+(S8 * 4)(k1)
	li	t0, MIPS_HARD_INT_MASK | MIPS_SR_INT_IE
/*
 * Call the software interrupt handler.
 */
	jal	_C_LABEL(ast)
	mtc0	t0, MIPS_COP_0_STATUS_REG	# enable interrupts (spl0)
/*
 * Restore user registers and return. NOTE: interrupts are enabled.
 */
	mtc0	zero, MIPS_COP_0_STATUS_REG
	nop
	nop
	nop
	li	v0, MIPS_SR_EXL
	mtc0	v0, MIPS_COP_0_STATUS_REG	# set exeption level bit.
	nop

	lw	k1, _C_LABEL(curpcb)
 #	lw	a0, U_PCB_REGS+(SR * 4)(k1)
	lw	t0, U_PCB_REGS+(MULLO * 4)(k1)
	lw	t1, U_PCB_REGS+(MULHI * 4)(k1)
	nop
 #	mtc0	a0, MIPS_COP_0_STATUS_REG	# this should disable interrupts
	mtlo	t0
	mthi	t1
	lw	a0, U_PCB_REGS+(PC * 4)(k1)
	lw	AT, U_PCB_REGS+(AST * 4)(k1)
	lw	v0, U_PCB_REGS+(V0 * 4)(k1)
	nop
	dmtc0	a0, MIPS_COP_0_EXC_PC		# set return address
	nop

	lw	v1, U_PCB_REGS+(V1 * 4)(k1)
	lw	a0, U_PCB_REGS+(A0 * 4)(k1)
	lw	a1, U_PCB_REGS+(A1 * 4)(k1)
	lw	a2, U_PCB_REGS+(A2 * 4)(k1)
	lw	a3, U_PCB_REGS+(A3 * 4)(k1)
	lw	t0, U_PCB_REGS+(T0 * 4)(k1)
	lw	t1, U_PCB_REGS+(T1 * 4)(k1)
	lw	t2, U_PCB_REGS+(T2 * 4)(k1)
	lw	t3, U_PCB_REGS+(T3 * 4)(k1)
	lw	t4, U_PCB_REGS+(T4 * 4)(k1)
	lw	t5, U_PCB_REGS+(T5 * 4)(k1)
	lw	t6, U_PCB_REGS+(T6 * 4)(k1)
	lw	t7, U_PCB_REGS+(T7 * 4)(k1)
	lw	t8, U_PCB_REGS+(T8 * 4)(k1)
	lw	t9, U_PCB_REGS+(T9 * 4)(k1)
	lw	gp, U_PCB_REGS+(GP * 4)(k1)
	lw	sp, U_PCB_REGS+(SP * 4)(k1)
	lw	ra, U_PCB_REGS+(RA * 4)(k1)
	lw	k0, U_PCB_REGS+(SR * 4)(k1)
	nop
	mtc0	k0, MIPS_COP_0_STATUS_REG	# Restore the SR
	nop

	eret
	.set	at
END(mips3_UserIntr)


/*----------------------------------------------------------------------------
 *
 *		XXX  START of r4000-specific code  XXX 
 *
 *----------------------------------------------------------------------------
 */


/*----------------------------------------------------------------------------
 *
 *	R4000 TLB exception handlers
 *
 *----------------------------------------------------------------------------
 */


/*----------------------------------------------------------------------------
 *
 * mips3_TLBMInvalidException --
 *
 *	Handle a TLB invalid exception from kernel mode in kernel space.
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
NLEAF(mips3_TLBInvalidException)
	.set	noat
	dmfc0	k0, MIPS_COP_0_BAD_VADDR	# get the fault address
	li	k1, VM_MIN_KERNEL_ADDRESS	# compute index
	subu	k0, k0, k1
	lw	k1, Sysmapsize			# index within range?
	srl	k0, k0, PGSHIFT
	sltu	k1, k0, k1
	beq	k1, zero, sys_stk_chk		# No. check for valid stack
	lw	k1, Sysmap

	sll	k0, k0, 2			# compute offset from index
	tlbp					# Probe the invalid entry
	addu	k1, k1, k0
	and	k0, k0, 4			# check even/odd page
	bne	k0, zero, KernTLBIOdd
	nop

	mfc0	k0, MIPS_COP_0_TLB_INDEX
	nop
	bltz	k0, sys_stk_chk
 #	sltiu	k0, k0, 8

 #	bne	k0, zero, sys_stk_chk
	beqz	k0, sys_stk_chk
	lw	k0, 0(k1)			# get PTE entry

	dsll	k0, k0, 34			# get rid of "wired" bit
	dsrl	k0, k0, 34
	dmtc0	k0, MIPS_COP_0_TLB_LO0		# load PTE entry
	and	k0, k0, MIPS3_PG_V		# check for valid entry
	beq	k0, zero, mips3_KernGenException	# PTE invalid
	lw	k0, 4(k1)			# get odd PTE entry
	dsll	k0, k0, 34
	dsrl	k0, k0, 34
	dmtc0	k0, MIPS_COP_0_TLB_LO1		# load PTE entry
	nop
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
	bltz	k0, sys_stk_chk
 #	sltiu	k0, k0, 8

 #	bne	k0, zero, sys_stk_chk
	beqz	k0, sys_stk_chk
	lw	k0, 0(k1)			# get PTE entry

	dsll	k0, k0, 34			# get rid of wired bit
	dsrl	k0, k0, 34
	dmtc0	k0, MIPS_COP_0_TLB_LO1		# save PTE entry
	and	k0, k0, MIPS3_PG_V		# check for valid entry
	beq	k0, zero, mips3_KernGenException	# PTE invalid
	lw	k0, -4(k1)			# get even PTE entry
	dsll	k0, k0, 34
	dsrl	k0, k0, 34
	dmtc0	k0, MIPS_COP_0_TLB_LO0		# save PTE entry
	nop
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
NLEAF(mips3_TLBMissException)
	.set	noat
	dmfc0	k0, MIPS_COP_0_BAD_VADDR	# get the fault address
	li	k1, VM_MIN_KERNEL_ADDRESS	# compute index
	subu	k0, k0, k1
	lw	k1, Sysmapsize			# index within range?
	srl	k0, k0, PGSHIFT
	sltu	k1, k0, k1
	beq	k1, zero, sys_stk_chk		# No. check for valid stack
	lw	k1, Sysmap
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
	tlbwr					# write TLB
	nop
	nop
	nop
	nop
	nop
	eret

sys_stk_chk:
	subu	k0, sp, UADDR + 0x200		# check to see if we have a
	sltiu	k0, UPAGES*NBPG - 0x200		#  valid kernel stack
	bne	k0, zero, mips3_KernGenException	# Go panic
	nop

	la	a0, start - START_FRAME - 8	# set sp to a valid place
	sw	sp, 24(a0)
	move	sp, a0
	la	a0, 1f
	mfc0	a2, MIPS_COP_0_STATUS_REG
	mfc0	a3, MIPS_COP_0_CAUSE_REG
	dmfc0	a1, MIPS_COP_0_EXC_PC
	sw	a2, 16(sp)
	sw	a3, 20(sp)
	move	a2, ra
	jal	printf
	dmfc0	a3, MIPS_COP_0_BAD_VADDR
	.data
1:
	.asciiz	"ktlbmiss: PC %x RA %x ADR %x\nSR %x CR %x SP %x\n"
	.text

	/* Call mips3_dump_tlb(0, 2, printf) to show the bad kstack TLB */
	la	a2, _C_LABEL(printf)
	li	a0, 0
	jal	mips3_dump_tlb
	li	a1, 2				# BDslot

	la	sp, start - START_FRAME		# set sp to a valid place
#ifdef DEBUG
 #	break	MIPS_BREAK_SOVER_VAL
#endif
	PANIC("kernel stack overflow")
	.set	at
END(mips3_TLBMissException)

/*
 *  Mark where code entreed from exception hander jumptable
 * ends, for stack traceback code.
 */

	.globl	_C_LABEL(mips3_exceptionentry_end)
_C_LABEL(mips3_exceptionentry_end):


/*--------------------------------------------------------------------------
 *
 * mips3_TLBWriteIndexedVPS --
 *
 *	Write the given entry into the TLB at the given index.
 *	Pass full r4000 tlb info icnluding variable page size mask.
 *
 *	mips3_TLBWriteIndexed(index, tlb)
 *		unsigned index;
 *		tlb *tlb;
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	TLB entry set.
 *
 *--------------------------------------------------------------------------
 */
LEAF(mips3_TLBWriteIndexedVPS)
	mfc0	v1, MIPS_COP_0_STATUS_REG	# Save the status register.
	mtc0	zero, MIPS_COP_0_STATUS_REG	# Disable interrupts
	nop
	lw	a2, 8(a1)
	lw	a3, 12(a1)
	dmfc0	t0, MIPS_COP_0_TLB_HI		# Save the current PID.

	dmtc0	a2, MIPS_COP_0_TLB_LO0		# Set up entry low0.
	dmtc0	a3, MIPS_COP_0_TLB_LO1		# Set up entry low1.
	nop
	lw	a2, 0(a1)
	lw	a3, 4(a1)
	nop
	mtc0	a0, MIPS_COP_0_TLB_INDEX	# Set the index.
	dmtc0	a2, MIPS_COP_0_TLB_PG_MASK	# Set up entry mask.
	dmtc0	a3, MIPS_COP_0_TLB_HI		# Set up entry high.
	nop
	tlbwi					# Write the TLB
	nop
	nop
	nop					# Delay for effect
	nop

	dmtc0	t0, MIPS_COP_0_TLB_HI		# Restore the PID.
	nop
	dmtc0	zero, MIPS_COP_0_TLB_PG_MASK	# Default mask value.
	j	ra
	mtc0	v1, MIPS_COP_0_STATUS_REG	# Restore the status register
END(mips3_TLBWriteIndexedVPS)

/*--------------------------------------------------------------------------
 *
 * mips3_TLBWriteIndexed --
 *
 *	Write the given entry into the TLB at the given index.
 *
 *	mips3_TLBWriteIndexed(index, highentry, lowentry0, lowentry1)
 *		unsigned index;
 *		int highEntry;
 *		int lowEntry0;
 *		int lowEntry1;
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	TLB entry set.
 *
 *--------------------------------------------------------------------------
 */
LEAF(mips3_TLBWriteIndexed)
	mfc0	v1, MIPS_COP_0_STATUS_REG	# Save the status register.
	mtc0	zero, MIPS_COP_0_STATUS_REG	# Disable interrupts
	nop
	lw	a2, 8(a1)
	lw	a3, 12(a1)
	dmfc0	t0, MIPS_COP_0_TLB_HI		# Save the current PID.

	dmtc0	a2, MIPS_COP_0_TLB_LO0		# Set up entry low0.
	dmtc0	a3, MIPS_COP_0_TLB_LO1		# Set up entry low1.
	nop
	lw	a2, 0(a1)
	lw	a3, 4(a1)
	nop
	mtc0	a0, MIPS_COP_0_TLB_INDEX	# Set the index.
	dmtc0	a2, MIPS_COP_0_TLB_PG_MASK	# Set up entry mask.
	dmtc0	a3, MIPS_COP_0_TLB_HI		# Set up entry high.
	nop
	tlbwi					# Write the TLB
	nop
	nop
	nop					# Delay for effect
	nop

	dmtc0	t0, MIPS_COP_0_TLB_HI		# Restore the PID.
	nop
	dmtc0	zero, MIPS_COP_0_TLB_PG_MASK	# Default mask value.
	j	ra
	mtc0	v1, MIPS_COP_0_STATUS_REG	# Restore the status register
END(mips3_TLBWriteIndexed)

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
	j	ra
	nop
END(mips3_SetPID)

/*--------------------------------------------------------------------------
 *
 * mips3_SetWIRED --
 *
 *	Write the given value into the TLB wired reg.
 *
 *	mips3_SetPID(wired)
 *		int wired;
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	WIRED set in the wired register.
 *
 *--------------------------------------------------------------------------
 */
LEAF(mips3_SetWIRED)
	mtc0	a0, MIPS_COP_0_TLB_WIRED
	j	ra
	nop
END(mips3_SetWIRED)

/*--------------------------------------------------------------------------
 *
 * mips3_GetWIRED --
 *
 *	Get the value from the TLB wired reg.
 *
 *	mips3_GetWIRED(void)
 *
 * Results:
 *	Value of wired reg.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------------------
 */
LEAF(mips3_GetWIRED)
	mfc0	v0, MIPS_COP_0_TLB_WIRED
	j	ra
	nop
END(mips3_GetWIRED)

/*--------------------------------------------------------------------------
 *
 * mips3_TLBFlush --
 *
 *	Flush the "random" entries from the TLB.
 *	Uses "wired" register to determine what register to start with.
 *
 *	mips3_TLBFlush()
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The TLB is flushed.
 *
 *--------------------------------------------------------------------------
 */
LEAF(mips3_TLBFlush)
	mfc0	v1, MIPS_COP_0_STATUS_REG	# Save the status register.
	mtc0	zero, MIPS_COP_0_STATUS_REG	# Disable interrupts
	mfc0	t1, MIPS_COP_0_TLB_WIRED
	li	t2, MIPS3_TLB_NUM_TLB_ENTRIES
	li	v0, MIPS_KSEG0_START		# invalid address
	dmfc0	t0, MIPS_COP_0_TLB_HI		# Save the PID

	dmtc0	v0, MIPS_COP_0_TLB_HI		# Mark entry high as invalid
	dmtc0	zero, MIPS_COP_0_TLB_LO0	# Zero out low entry0.
	dmtc0	zero, MIPS_COP_0_TLB_LO1	# Zero out low entry1.
	mtc0	zero, MIPS_COP_0_TLB_PG_MASK	# Zero out mask entry.
/*
 * Align the starting value (t1) and the upper bound (t2).
 */
1:
	mtc0	t1, MIPS_COP_0_TLB_INDEX	# Set the index register.
	addu	t1, t1, 1			# Increment index.
	tlbwi					# Write the TLB entry.
	nop
	nop
	bne	t1, t2, 1b
	nop

	dmtc0	t0, MIPS_COP_0_TLB_HI		# Restore the PID
	j	ra
	mtc0	v1, MIPS_COP_0_STATUS_REG	# Restore the status register
END(mips3_TLBFlush)


/*--------------------------------------------------------------------------
 *
 * mips3_TLBFlushAddr --
 *
 *	Flush any TLB entries for the given address and TLB PID.
 *
 *	mips3_TLBFlushAddr(TLBhi)
 *		unsigned TLBhi;
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The process's page is flushed from the TLB.
 *
 *--------------------------------------------------------------------------
 */
LEAF(mips3_TLBFlushAddr)
	mfc0	v1, MIPS_COP_0_STATUS_REG	# Save the status register.
	mtc0	zero, MIPS_COP_0_STATUS_REG	# Disable interrupts
	nop
	li	v0, (MIPS3_PG_HVPN | MIPS3_PG_ASID)
	and	a0, a0, v0			# Make shure valid hi value.
	dmfc0	t0, MIPS_COP_0_TLB_HI		# Get current PID
	dmtc0	a0, MIPS_COP_0_TLB_HI		# look for addr & PID
	nop
	nop
	nop
	tlbp					# Probe for the entry.
	nop
	nop					# Delay for effect
	nop
	mfc0	v0, MIPS_COP_0_TLB_INDEX	# See what we got
	li	t1, MIPS_KSEG0_START		# Load invalid entry.
	bltz	v0, 1f				# index < 0 => !found
	nop
	dmtc0	t1, MIPS_COP_0_TLB_HI		# Mark entry high as invalid

	dmtc0	zero, MIPS_COP_0_TLB_LO0	# Zero out low entry.
	dmtc0	zero, MIPS_COP_0_TLB_LO1	# Zero out low entry.
	nop
	tlbwi
	nop
	nop
	nop
	nop
1:
	dmtc0	t0, MIPS_COP_0_TLB_HI		# restore PID
	j	ra
	mtc0	v1, MIPS_COP_0_STATUS_REG	# Restore the status register
END(mips3_TLBFlushAddr)

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
	mfc0	v1, MIPS_COP_0_STATUS_REG	# Save the status register.
	mtc0	zero, MIPS_COP_0_STATUS_REG	# Disable interrupts
	and	t1, a0, 0x1000			# t1 = Even/Odd flag
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

	tlbr					# update, read entry first
	nop
	nop
	nop
	dmtc0	a1, MIPS_COP_0_TLB_LO0		# init low reg0.
	nop
	tlbwi					# update slot found
	b	4f
	nop
1:
	mtc0	zero, MIPS_COP_0_TLB_PG_MASK	# init mask.
	dmtc0	a0, MIPS_COP_0_TLB_HI		# init high reg.
	dmtc0	a1, MIPS_COP_0_TLB_LO0		# init low reg0.
	dmtc0	a2, MIPS_COP_0_TLB_LO1		# init low reg1.
	nop
	tlbwr					# enter into a random slot
	b	4f
	nop
# ODD
2:
	nop
	bltz	v0, 3f				# index < 0 => !found
	nop

	tlbr					# read the entry first
	nop
	nop
	nop
	dmtc0	a1, MIPS_COP_0_TLB_LO1		# init low reg1.
	nop
	tlbwi					# update slot found
	b	4f
	nop
3:
	mtc0	zero, MIPS_COP_0_TLB_PG_MASK	# init mask.
	dmtc0	a0, MIPS_COP_0_TLB_HI		# init high reg.
	dmtc0	a2, MIPS_COP_0_TLB_LO0		# init low reg0.
	dmtc0	a1, MIPS_COP_0_TLB_LO1		# init low reg1.
	nop
	tlbwr					# enter into a random slot

4:						# Make shure pipeline
	nop					# advances before we
	nop					# uses the tlb.
	nop
	nop
	dmtc0	t0, MIPS_COP_0_TLB_HI		# restore PID
	j	ra
	mtc0	v1, MIPS_COP_0_STATUS_REG	# Restore the status register
END(mips3_TLBUpdate)

/*--------------------------------------------------------------------------
 *
 * mips3_TLBReadVPS --
 *
 *	Read the TLB entry, including variable-page-size mask.
 *
 *	mips3_TLBReadVPS(entry, tlb)
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
LEAF(mips3_TLBReadVPS)
	mfc0	v1, MIPS_COP_0_STATUS_REG	# Save the status register.
	mtc0	zero, MIPS_COP_0_STATUS_REG	# Disable interrupts
	nop
	nop
	nop
	dmfc0	t0, MIPS_COP_0_TLB_HI		# Get current PID

	mtc0	a0, MIPS_COP_0_TLB_INDEX	# Set the index register
	nop
	tlbr					# Read from the TLB
	nop
	nop
	nop
	mfc0	t2, MIPS_COP_0_TLB_PG_MASK	# fetch the hi entry
	dmfc0	t3, MIPS_COP_0_TLB_HI		# fetch the hi entry
	dmfc0	t4, MIPS_COP_0_TLB_LO0		# See what we got
	dmfc0	t5, MIPS_COP_0_TLB_LO1		# See what we got
	dmtc0	t0, MIPS_COP_0_TLB_HI		# restore PID
	nop
	nop
	nop					# wait for PID active
	mtc0	v1, MIPS_COP_0_STATUS_REG	# Restore the status register
	nop
	sw	t2, 0(a1)
	sw	t3, 4(a1)
	sw	t4, 8(a1)
	j	ra
	sw	t5, 12(a1)
END(mips3_TLBReadVPS)

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
	mfc0	v1, MIPS_COP_0_STATUS_REG	# Save the status register.
	mtc0	zero, MIPS_COP_0_STATUS_REG	# Disable interrupts
	nop
	nop
	nop
	dmfc0	t0, MIPS_COP_0_TLB_HI		# Get current PID

	mtc0	a0, MIPS_COP_0_TLB_INDEX	# Set the index register
	nop
	tlbr					# Read from the TLB
	nop
	nop
	nop
	mfc0	t2, MIPS_COP_0_TLB_PG_MASK	# fetch the hi entry
	dmfc0	t3, MIPS_COP_0_TLB_HI		# fetch the hi entry
	dmfc0	t4, MIPS_COP_0_TLB_LO0		# See what we got
	dmfc0	t5, MIPS_COP_0_TLB_LO1		# See what we got
	dmtc0	t0, MIPS_COP_0_TLB_HI		# restore PID
	nop
	nop
	nop					# wait for PID active
	mtc0	v1, MIPS_COP_0_STATUS_REG	# Restore the status register
	nop
	sw	t2, 0(a1)
	sw	t3, 4(a1)
	sw	t4, 8(a1)
	j	ra
	sw	t5, 12(a1)
END(mips3_TLBRead)

/*--------------------------------------------------------------------------
 *
 * mips3_TLBGetPID --
 *
 *	mips3_TLBGetPID()
 *
 * Results:
 *	Returns the current TLB pid reg.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------------------
 */
LEAF(mips3_TLBGetPID)
	dmfc0	v0, MIPS_COP_0_TLB_HI		# get PID
	j	ra
	and	v0, v0, MIPS3_TLB_PID		# mask off PID
END(mips3_TLBGetPID)



/*----------------------------------------------------------------------------
 *
 *	R4000 cache sizing and flushing code.
 *
 *----------------------------------------------------------------------------
 */


/*----------------------------------------------------------------------------
 *
 * mips3_ConfigCache --
 *
 *	Size the caches.
 *	NOTE: should only be called from mach_init().
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *	The size of the data cache is stored into mips_L1DataCacheSize.
 *	The size of instruction cache is stored into mips_L1InstCacheSize.
 *	Alignment mask for cache aliasing test is stored in mips_CacheAliasMask.
 *
 *----------------------------------------------------------------------------
 */
LEAF(mips3_ConfigCache)
	mfc0	v0, MIPS_COP_0_CONFIG		# Get configuration register
	nop
	srl	t1, v0, 9			# Get D cache size.
	and	t1, 7				# ???
	li	t2, 4096
	sllv	t2, t2, t1
	sw	t2, mips_L1DataCacheSize
	addiu	t2, -1
	and	t2, ~(NBPG - 1)
	sw	t2, mips_CacheAliasMask

	and	t2, v0, 0x20
	srl	t2, t2, 1
	addu	t2, t2, 16
	sw	t2, mips_L1DataCacheLSize

	srl	t1, v0, 6			# Get I cache size.
	and	t1, 7				# ???
	li	t2, 4096
	sllv	t2, t2, t1
	sw	t2, mips_L1InstCacheSize

	and	t2, v0, 0x10
	addu	t2, t2, 16
	sw	t2, mips_L1InstCacheLSize

	lui	t1, 2
	and	t1, t1, v0
	bne	t1, zero, 1f
	nop
	lui	t1, 0x10
	sw	t1, mips_L2CacheSize
	lui	t1, 0xc0
	and	t1, t1, v0
	srl	t1, 22
	li	t2, 16
	sllv	t2, t2, t1
	sw	t2, mips_L2CacheLSize
1:
	j	ra
	nop
END(mips3_ConfigCache)

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
	lw	t1, mips_L1InstCacheSize
	lw	t2, mips_L1DataCacheSize
 #	lw	t3, mips_L1InstCacheLSize
 #	lw	t4, mips_L1DataCacheLSize
/*
 * Flush the instruction cache.
 */
	li	t0, MIPS_KSEG0_START
	addu	t1, t0, t1		# End address
	subu	t1, t1, 128
1:
	cache	0, 0(t0)
	cache	0, 16(t0)
	cache	0, 32(t0)
	cache	0, 48(t0)
	cache	0, 64(t0)
	cache	0, 80(t0)
	cache	0, 96(t0)
	cache	0, 112(t0)
	bne	t0, t1, 1b
	addu	t0, t0, 128

/*
 * Flush the data cache.
 */
	li	t0, MIPS_KSEG0_START
	addu	t1, t0, t2		# End address
	subu	t1, t1, 128
1:
	cache	1, 0(t0)
	cache	1, 16(t0)
	cache	1, 32(t0)
	cache	1, 48(t0)
	cache	1, 64(t0)
	cache	1, 80(t0)
	cache	1, 96(t0)
	cache	1, 112(t0)
	bne	t0, t1, 1b
	addu	t0, t0, 128

#if 1
	lw	t2, mips_L2CacheSize
	beq	t2, zero, 2f
	nop
	li	t0, MIPS_KSEG0_START
	addu	t1, t0, t2
	subu	t1, t1, 128
1:
	cache	3, 0(t0)
	cache	3, 32(t0)
	cache	3, 64(t0)
	cache	3, 96(t0)
	bne	t0, t1, 1b
	addu	t0, t0, 128
2:
#endif

	j	ra
	nop
END(mips3_FlushCache)

/*----------------------------------------------------------------------------
 *
 * mips3_FlushICache --
 *
 *	void mips3_FlushICache(addr, len)
 *		vm_offset_t addr, len;
 *
 *	Flush instruction cache for range of addr to addr + len - 1.
 *	The address can be any valid address so long as no TLB misses occur.
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
	addu	a1, 127		# Align
	srl	a1, a1, 7	# Number of unrolled loops
1:
	cache	0, 0(a0)
	cache	0, 16(a0)
	cache	0, 32(a0)
	cache	0, 48(a0)
	cache	0, 64(a0)
	cache	0, 80(a0)
	cache	0, 96(a0)
	cache	0, 112(a0)
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
 *		vm_offset_t addr, len;
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
	lw	a2, mips_L1DataCacheSize
	addiu	a2, -1
	move	t0, a0		# copy start address
	and	a0, a0, a2	# get index into primary cache
	addu	a1, 127		# Align
	li	a2, 0x80000000
	addu	a0, a0, a2
	addu	a1, a1, a0
	and	a0, a0, -128
	subu	a1, a1, a0
	srl	a1, a1, 7	# Compute number of cache lines
	move	t1, a1		# copy length
1:
	cache	1, 0(a0)
	cache	1, 16(a0)
	cache	1, 32(a0)
	cache	1, 48(a0)
	cache	1, 64(a0)
	cache	1, 80(a0)
	cache	1, 96(a0)
	cache	1, 112(a0)
	addu	a1, -1
	bne	a1, zero, 1b
	addu	a0, 128

#if 1
	lw	a2, mips_L2CacheSize
	beq	a2, zero, 2f	# no secondary cache
	addiu	a2, -1
	and	t0,t0,a2	# secondary cache index
	li	a0, 0x80000000
	addu	a0, a0, t0	# reduce to kseg0 address
1:
	cache	3, 0(a0)
	cache	3, 32(a0)
	cache	3, 64(a0)
	cache	3, 96(a0)
	addu	t1, -1
	bne	t1, zero, 1b
	addu	a0, 128
2:
#endif

	j	ra
	nop
END(mips3_FlushDCache)

/*----------------------------------------------------------------------------
 *
 * mips3_HitFlushDCache --
 *
 *	void mips3_HitFlushDCache(addr, len)
 *		vm_offset_t addr, len;
 *
 *	Flush data cache for range of addr to addr + len - 1.
 *	The address can be any valid viritual address as long
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
	cache	0x15, 0(a0)
	cache	0x15, 16(a0)
	cache	0x15, 32(a0)
	cache	0x15, 48(a0)
	cache	0x15, 64(a0)
	cache	0x15, 80(a0)
	cache	0x15, 96(a0)
	cache	0x15, 112(a0)
#if 1
	cache	0x17, 0(a0)
	cache	0x17, 32(a0)
	cache	0x17, 64(a0)
	cache	0x17, 96(a0)
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
 *	void mips3_FlushDCache(addr, len)
 *		vm_offset_t addr, len;
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
	cache	0x13, 0(a0)
	addu	a0, a0, 4
	bne	a0, a1, 1b
	cache	0x11,-4(a0)

	j	ra
	nop
END(mips3_InvalidateDCache)

/*----------------------------------------------------------------------------
 *
 * mips3_VCED --
 *
 *	Handle virtual coherency exceptions
 *
 *----------------------------------------------------------------------------
 */
LEAF(mips3_VCED)
ALEAF(mips3_VCEI)	/* XXXX */
	.set	noat
	move	k0, AT
	.set	at
	sw	k0, vce_saveat
	mfc0	k0, MIPS_COP_0_BAD_VADDR	# fault addr.
	nop
	nop
	and	k0, -16
	sw	k0, vce_savek0			# save virtual address
	cache	1, 0(k0)			# writeback primary line
	lw	k1, mips_L1DataCacheSize
	addiu	k1, -1
	and	k0, k0, k1			# mask to cache index
	or	k0, 0x80000000			# physical K0SEG address
	xor	k0, 0x1000			# other page of cache
	cache	1, 0(k0)
	xor	k0, 0x10			# other half of cache line
	cache	1, 0(k0)
	xor	k0, 0x1000			# other half in same page
	cache	1, 0(k0)
	sltiu	k1, k1, 0x3fff
	bnez	k1, 1f				# not R4400
	xor	k0, 0x2000			# clear out same cache lines
	cache	1, 0(k0)			#   in the other half of the
	xor	k0, 0x1000			#   R4400 primary cache
	cache	1, 0(k0)
	xor	k0, 0x10
	cache	1, 0(k0)
	xor	k0, 0x1000
	cache	1, 0(k0)
 # end R4400
1:
	lw	k0, vce_savek0			# get original address
	cache	31, 0(k0)
	nop
#ifdef DEBUG
	mfc0	k1, MIPS_COP_0_EXC_PC
	sw	k0, VCE_vaddr
	sw	k1, VCE_epc
	la	k1, VCE_count		# count number of exceptions
	srl	k0, k0, 26		# position upper 4 bits of VA
	andi	k0, k0, 0x3c		# mask it off
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
 * Invoke the service function with one argument, specified by the s0
 * and s1 respectively.  There is no need register save operation.
 */
LEAF(mips3_proc_trampoline) 
	jal	ra, s0
	move	a0, s1
	.set	noat
	li	a0, MIPS_SR_EXL			# set exception level
	mtc0	a0, MIPS_COP_0_STATUS_REG
	nop
	nop
	lw	a1, _C_LABEL(curpcb)
	nop
	lw	a0, U_PCB_REGS + (SR * 4)(a1)
	lw	t0, U_PCB_REGS + (MULLO * 4)(a1)
	lw	t1, U_PCB_REGS + (MULHI * 4)(a1)
	mtlo	t0
	mthi	t1
	nop
	move	k1, a1
	lw	AT, U_PCB_REGS + (AST * 4)(k1)
	lw	a0, U_PCB_REGS + (PC * 4)(k1)
	nop
	dmtc0	a0, MIPS_COP_0_EXC_PC
	nop
	lw	v0, U_PCB_REGS + (V0 * 4)(k1)
	lw	v1, U_PCB_REGS + (V1 * 4)(k1)
	lw	a0, U_PCB_REGS + (A0 * 4)(k1)
	lw	a1, U_PCB_REGS + (A1 * 4)(k1)
	lw	a2, U_PCB_REGS + (A2 * 4)(k1)
	lw	a3, U_PCB_REGS + (A3 * 4)(k1)
	lw	t0, U_PCB_REGS + (T0 * 4)(k1)
	lw	t1, U_PCB_REGS + (T1 * 4)(k1)
	lw	t2, U_PCB_REGS + (T2 * 4)(k1)
	lw	t3, U_PCB_REGS + (T3 * 4)(k1)
	lw	t4, U_PCB_REGS + (T4 * 4)(k1)
	lw	t5, U_PCB_REGS + (T5 * 4)(k1)
	lw	t6, U_PCB_REGS + (T6 * 4)(k1)
	lw	t7, U_PCB_REGS + (T7 * 4)(k1)
	lw	s0, U_PCB_REGS + (S0 * 4)(k1)
	lw	s1, U_PCB_REGS + (S1 * 4)(k1)
	lw	s2, U_PCB_REGS + (S2 * 4)(k1)
	lw	s3, U_PCB_REGS + (S3 * 4)(k1)
	lw	s4, U_PCB_REGS + (S4 * 4)(k1)
	lw	s5, U_PCB_REGS + (S5 * 4)(k1)
	lw	s6, U_PCB_REGS + (S6 * 4)(k1)
	lw	s7, U_PCB_REGS + (S7 * 4)(k1)
	lw	t8, U_PCB_REGS + (T8 * 4)(k1)
	lw	t9, U_PCB_REGS + (T9 * 4)(k1)
	lw	gp, U_PCB_REGS + (GP * 4)(k1)
	lw	s8, U_PCB_REGS + (S8 * 4)(k1)
	lw	ra, U_PCB_REGS + (RA * 4)(k1)
	lw	sp, U_PCB_REGS + (SP * 4)(k1)
	lw	k0, U_PCB_REGS + (SR * 4)(k1)
	nop
	mtc0	k0, MIPS_COP_0_STATUS_REG
	nop
	nop
	eret
	.set	at
END(mips3_proc_trampoline)

/*
 * mips3_switch_exit(struct proc *)
 * Make the named process exit.  Switch SP to nullproc stack, free the
 * exiting proc's USPACE, then jump into the middle of cpu_switch().
 * MUST BE CALLED AT SPLHIGH.
 */
LEAF(mips3_switch_exit)
	la	v1, _C_LABEL(nullproc)		# !! SP runs on p->p_addr !!
	lw	t0, P_MD_UPTE+0(v1)		# t0 = first u. pte
	lw	t1, P_MD_UPTE+4(v1)		# t1 = 2nd u. pte
	li	v0, MIPS_KSEG0_START		# clear wired entries for pcb
	mtc0	v0, MIPS_COP_0_TLB_HI
	mtc0	zero, MIPS_COP_0_TLB_LO0
	mtc0	zero, MIPS_COP_0_TLB_LO1
	li	v0, 1				# clear wired entry 1
	mtc0	v0, MIPS_COP_0_TLB_INDEX
	tlbwi
	li	v0, 2				# clear wired entry 2
	mtc0	v0, MIPS_COP_0_TLB_INDEX
	tlbwi
	li	v0, UADDR			# still using UADDR for kstack
	nop
	mtc0	zero, MIPS_COP_0_TLB_INDEX	# set the index register
	mtc0	v0, MIPS_COP_0_TLB_HI		# init high entry
	mtc0	t0, MIPS_COP_0_TLB_LO0		# init low entry 0
	mtc0	t1, MIPS_COP_0_TLB_LO1		# init low entry 1
	nop
	tlbwi					# Write the TLB entry.

	addu	sp, v0, USPACE - START_FRAME
	li	a2, USPACE
	lw	a1, P_ADDR(a0)
	lw	a0, kernel_map
	la	ra, sw1				# goto cpu_switch()
	j	kmem_free			# free exiting USPACE
END(mips3_switch_exit)

/*
 * Resume process indicated by the pte's for its u struct
 * NOTE: This is hard coded to UPAGES == 2.
 * Also, there should be no TLB faults at this point.
 *
 * Entered from CPU-common cpu_switch()
 * v0 = Address Space Identifier (TLB PID) for this process
 * t0 = First u.pte
 * t1 = Second u.pte
 */
/*
 * XXXX - almost done with the UADDR mapping; only the kernel stack should
 * now be using UADDR.  The first TLB entry is used to map UADDR to the
 * u-area.  Then the following TLB entries are used to map the current u-area.
 * This gets a bit messy on the R4000:  it can take one or two TLB
 * entries to map the u-area, depending upon whether the u-area
 * begins on an even or odd page.  Also, any current mapping for the u-area
 * address space needs to be flushed as well.
 */
LEAF(mips3_cpu_switch_resume)
	or	v0, v0, UADDR			# v0 = first HI entry
	mtc0	zero, MIPS_COP_0_TLB_INDEX	# set the index register
	mtc0	v0, MIPS_COP_0_TLB_HI		# init high entry
	mtc0	t0, MIPS_COP_0_TLB_LO0		# init low entry 0
	mtc0	t1, MIPS_COP_0_TLB_LO1		# init low entry 1
	nop
	nop
	tlbwi					# Write the TLB entry.
	nop
	nop
	nop
	nop
# now map the p_addr pages
	andi	v0, PG_ASID
	or	v0, s0
	ori	t0, MIPS3_PG_G		# XXX set PG_G
	ori	t1, MIPS3_PG_G		# XXX set PG_G
	li	s1, MIPS3_PG_ODDPG
	and	s1, v0				# does p_addr start on odd page
	beq	s1, zero, 1f			# no, only one TLB entry needed
# p_addr starts on an odd page, need to set up 2 TLB entries
	addu	v0, v0, MIPS3_PG_ODDPG		# map second page
# see if this address currently exists in TLB
	mtc0	v0, MIPS_COP_0_TLB_HI
	nop
	tlbp					# probe for existing entry
	nop
	nop
	mfc0	s1, MIPS_COP_0_TLB_INDEX
	nop
	slti	s1, s1, 8			# if found in random entry
	bnez	s1, 2f
	mtc0	zero, MIPS_COP_0_TLB_LO0	# flush it
	mtc0	zero, MIPS_COP_0_TLB_LO1
	li	s1, MIPS_KSEG0_START
	mtc0	s1, MIPS_COP_0_TLB_HI
	nop
	tlbwi
	nop
	mtc0	v0, MIPS_COP_0_TLB_HI		# restore high entry
2:
	mtc0	t1, MIPS_COP_0_TLB_LO0		# even page to LO0
	li	s1, MIPS3_PG_G		# XXX set PG_G
	mtc0	s1, MIPS_COP_0_TLB_LO1		# invalid page to LO1
	li	s1, 2				# use TLB entry 2 for even page
	mtc0	s1, MIPS_COP_0_TLB_INDEX
	nop
	tlbwi					# write odd page TLB entry
	move	t1, t0				# odd page to LO1
	li	t0, MIPS3_PG_G		# XXX PG_G	# clear LO0
	addi	v0, v0, -NBPG * 2		# backup to odd page mapping
# set up TLB entry 1
1:
# see if this address currently exists in TLB
	mtc0	v0, MIPS_COP_0_TLB_HI
	nop
	tlbp					# probe for existing entry
	nop
	nop
	mfc0	s1, MIPS_COP_0_TLB_INDEX
	nop
	slti	s1, s1, 8			# if found in random entry
	bnez	s1, 2f
	mtc0	zero, MIPS_COP_0_TLB_LO0	# flush it
	mtc0	zero, MIPS_COP_0_TLB_LO1
	li	s1, MIPS_KSEG0_START
	mtc0	s1, MIPS_COP_0_TLB_HI
	nop
	tlbwi
	nop
	mtc0	v0, MIPS_COP_0_TLB_HI		# restore high entry
2:
	mtc0	t0, MIPS_COP_0_TLB_LO0		# first page
	mtc0	t1, MIPS_COP_0_TLB_LO1		# second page
	li	s1, 1				# use TLB entry 1
	mtc0	s1, MIPS_COP_0_TLB_INDEX
	nop
	tlbwi					# write TLB entry
/*
 * Now running on new u struct.
 * Restore registers and return.
 */
	lw	t0, _C_LABEL(curpcb)
	nop
	lw	v0, U_PCB_CONTEXT+44(t0)	# restore kernel context
	lw	ra, U_PCB_CONTEXT+40(t0)
	lw	s0, U_PCB_CONTEXT+0(t0)
	lw	s1, U_PCB_CONTEXT+4(t0)
	lw	s2, U_PCB_CONTEXT+8(t0)
	lw	s3, U_PCB_CONTEXT+12(t0)
	lw	s4, U_PCB_CONTEXT+16(t0)
	lw	s5, U_PCB_CONTEXT+20(t0)
	lw	s6, U_PCB_CONTEXT+24(t0)
	lw	s7, U_PCB_CONTEXT+28(t0)
	lw	sp, U_PCB_CONTEXT+32(t0)
	lw	s8, U_PCB_CONTEXT+36(t0)
	nop
	mtc0	v0, MIPS_COP_0_STATUS_REG
	j	ra
	li	v0, 1				# possible return to 'savectx()'
END(mips3_cpu_switch_resume)

/*----------------------------------------------------------------------------
 *
 *			XXX  END of r4000-specific code  XXX 
 *
 *----------------------------------------------------------------------------
 */
	.set	mips2
