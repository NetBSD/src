/*	$NetBSD: fp.S,v 1.18.2.1 2000/06/22 17:01:32 minoura Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell.
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
 *	@(#)fp.s	8.1 (Berkeley) 6/10/93
 */
#include <machine/cdefs.h>

#include <mips/asm.h>
#include <mips/cpuregs.h>
#include <mips/trap.h>

#include "assym.h"

#define SEXP_INF	0xff
#define DEXP_INF	0x7ff
#define SEXP_BIAS	127
#define DEXP_BIAS	1023
#define SEXP_MIN	-126
#define DEXP_MIN	-1022
#define SEXP_MAX	127
#define DEXP_MAX	1023
#define WEXP_MAX	30		/* maximum unbiased exponent for int */
#define WEXP_MIN	-1		/* minimum unbiased exponent for int */
#define SFRAC_BITS	23
#define DFRAC_BITS	52
#define SIMPL_ONE	0x00800000
#define DIMPL_ONE	0x00100000
#define SLEAD_ZEROS	31 - 23
#define DLEAD_ZEROS	31 - 20
#define STICKYBIT	1
#define GUARDBIT	0x80000000
#define SSIGNAL_NAN	0x00400000
#define DSIGNAL_NAN	0x00080000
#define SQUIET_NAN	0x003fffff
#define DQUIET_NAN0	0x0007ffff
#define DQUIET_NAN1	0xffffffff
#define INT_MIN		0x80000000
#define INT_MAX		0x7fffffff

#define COND_UNORDERED	0x1
#define COND_EQUAL	0x2
#define COND_LESS	0x4
#define COND_SIGNAL	0x8

#if SZREG == 8
#define SZREG_SHFT	3
#define SZREG_MASK	0x00f8
#else
#define SZREG_SHFT	2
#define SZREG_MASK	0x007c
#endif

/* insns are reordered in the way as MIPS architecture imposes */
	.set	reorder

/*----------------------------------------------------------------------------
 *
 * MachEmulateFP --
 *
 *	Emulate unimplemented floating point operations.
 *	This routine should only be called by MachFPInterrupt().
 *
 *	MachEmulateFP(instr, frame, cause)
 *		u_int32_t instr;
 *		struct frame *frame;
 *		u_int32_t cause;
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Floating point registers are modified according to instruction.
 *
 *----------------------------------------------------------------------------
 */
NESTED(MachEmulateFP, CALLFRAME_SIZ, ra)
	subu	sp, sp, CALLFRAME_SIZ
	sw	ra, CALLFRAME_RA(sp)
	sw	a2, CALLFRAME_SIZ + 8(sp)
/*
 * Decode the FMT field (bits 25-21) and FUNCTION field (bits 5-0).
 */
	srl	v0, a0, 21 - 2			# get FMT field
	andi	v0, v0, 0x1F << 2		# mask FMT field
#ifdef SOFTFLOAT
	lw	t0, _C_LABEL(curpcb)		# get pcb of current process
	lw	a3, fmt_tbl(v0)			# switch on FUNC & FMT
	lw	a2, U_PCB_FPREGS+FRAME_FSR(t0)
#else
	cfc1	a2, MIPS_FPU_CSR		# get exception register
	lw	a3, fmt_tbl(v0)			# switch on FUNC & FMT
	and	a2, a2, ~MIPS_FPU_EXCEPTION_UNIMPL	# clear exception
	ctc1	a2, MIPS_FPU_CSR
#endif
	j	a3

single_op:
	andi	v0, a0, 0x3F		# get FUNC field
	sll	v0, v0, 2
	lw	v0, func_single_tbl(v0)
	j	v0
double_op:
	andi	v0, a0, 0x3F		# get FUNC field
	sll	v0, v0, 2
	lw	v0, func_double_tbl(v0)
	j	v0
single_fixed_op:
	andi	v0, a0, 0x3F		# get FUNC field
	sll	v0, v0, 2
	lw	v0, func_single_fixed_tbl(v0)
	j	v0
long_fixed_op:
	andi	v0, a0, 0x3F		# get FUNC field
	sll	v0, v0, 2
	lw	v0, func_long_fixed_tbl(v0)
	j	v0

	.rdata
fmt_tbl:
#ifdef SOFTFLOAT
	.word	mfromc1		# sub 0
#else
	.word	ill		# sub 0
#endif
	.word	ill		# sub 1
#ifdef SOFTFLOAT
	.word	cfromc1		# sub 2
#else
	.word	ill		# sub 2
#endif
	.word	ill		# sub 3
#ifdef SOFTFLOAT
	.word	mtoc1		# sub 4
#else
	.word	ill		# sub 4
#endif
	.word	ill		# sub 5
#ifdef SOFTFLOAT
	.word	ctoc1		# sub 6
#else
	.word	ill		# sub 6
#endif
	.word	ill		# sub 7
#ifdef SOFTFLOAT
	.word	branchc1	# sub 8
#else
	.word	ill		# sub 8
#endif
	.word	ill		# sub 9
	.word	ill		# sub 10
	.word	ill		# sub 11
	.word	ill		# sub 12
	.word	ill		# sub 13
	.word	ill		# sub 14
	.word	ill		# sub 15
	.word	single_op	# sub 16
	.word	double_op	# sub 17
	.word	ill		# sub 18
	.word	ill		# sub 19
	.word	single_fixed_op	# sub 20
	.word	long_fixed_op	# sub 21
	.word	ill		# sub 22
	.word	ill		# sub 23
	.word	ill		# sub 24
	.word	ill		# sub 25
	.word	ill		# sub 26
	.word	ill		# sub 27
	.word	ill		# sub 28
	.word	ill		# sub 29
	.word	ill		# sub 30
	.word	ill		# sub 31

func_single_tbl:
	.word	add_s		# func 0
	.word	sub_s		# func 1
	.word	mul_s		# func 2
	.word	div_s		# func 3
#ifdef MIPS3
	.word	sqrt_s		# func 4
#else
	.word	ill		# func 4
#endif
	.word	abs_s		# func 5
	.word	mov_s		# func 6
	.word	neg_s		# func 7
	.word	ill		# func 8
	.word	ill		# func 9
	.word	ill		# func 10
	.word	ill		# func 11
#ifdef MIPS3
	.word	round_w_s	# func 12 /* MIPS2 FP instructions */
	.word	trunc_w_s	# func 13
	.word	ceil_w_s	# func 14
	.word	floor_w_s	# func 15
#else
	.word	ill		# func 12
	.word	ill		# func 13
	.word	ill		# func 14
	.word	ill		# func 15
#endif
	.word	ill		# func 16
	.word	ill		# func 17
	.word	ill		# func 18
	.word	ill		# func 19
	.word	ill		# func 20
	.word	ill		# func 21
	.word	ill		# func 22
	.word	ill		# func 23
	.word	ill		# func 24
	.word	ill		# func 25
	.word	ill		# func 26
	.word	ill		# func 27
	.word	ill		# func 28
	.word	ill		# func 29
	.word	ill		# func 30
	.word	ill		# func 31
	.word	ill		# func 32
	.word	cvt_d_s		# func 33
	.word	ill		# func 34
	.word	ill		# func 35
	.word	cvt_w_s		# func 36
	.word	ill		# func 37
	.word	ill		# func 38
	.word	ill		# func 39
	.word	ill		# func 40
	.word	ill		# func 41
	.word	ill		# func 42
	.word	ill		# func 43
	.word	ill		# func 44
	.word	ill		# func 45
	.word	ill		# func 46
	.word	ill		# func 47
	.word	cmp_s		# func 48
	.word	cmp_s		# func 49
	.word	cmp_s		# func 50
	.word	cmp_s		# func 51
	.word	cmp_s		# func 52
	.word	cmp_s		# func 53
	.word	cmp_s		# func 54
	.word	cmp_s		# func 55
	.word	cmp_s		# func 56
	.word	cmp_s		# func 57
	.word	cmp_s		# func 58
	.word	cmp_s		# func 59
	.word	cmp_s		# func 60
	.word	cmp_s		# func 61
	.word	cmp_s		# func 62
	.word	cmp_s		# func 63

func_double_tbl:
	.word	add_d		# func 0
	.word	sub_d		# func 1
	.word	mul_d		# func 2
	.word	div_d		# func 3
#ifdef MIPS3
	.word	sqrt_d		# func 4
#else
	.word	ill		# func 4
#endif
	.word	abs_d		# func 5
	.word	mov_d		# func 6
	.word	neg_d		# func 7
	.word	ill		# func 8
	.word	ill		# func 9
	.word	ill		# func 10
	.word	ill		# func 11
#ifdef MIPS3
	.word	round_w_d	# func 12 /* MIPS2 FP instructions */
	.word	trunc_w_d	# func 13
	.word	ceil_w_d	# func 14
	.word	floor_w_d	# func 15
#else
	.word	ill		# func 12
	.word	ill		# func 13
	.word	ill		# func 14
	.word	ill		# func 15
#endif
	.word	ill		# func 16
	.word	ill		# func 17
	.word	ill		# func 18
	.word	ill		# func 19
	.word	ill		# func 20
	.word	ill		# func 21
	.word	ill		# func 22
	.word	ill		# func 23
	.word	ill		# func 24
	.word	ill		# func 25
	.word	ill		# func 26
	.word	ill		# func 27
	.word	ill		# func 28
	.word	ill		# func 29
	.word	ill		# func 30
	.word	ill		# func 31
	.word	cvt_s_d		# func 32
	.word	ill		# func 33
	.word	ill		# func 34
	.word	ill		# func 35
	.word	cvt_w_d		# func 36
	.word	ill		# func 37
	.word	ill		# func 38
	.word	ill		# func 39
	.word	ill		# func 40
	.word	ill		# func 41
	.word	ill		# func 42
	.word	ill		# func 43
	.word	ill		# func 44
	.word	ill		# func 45
	.word	ill		# func 46
	.word	ill		# func 47
	.word	cmp_d		# func 48
	.word	cmp_d		# func 49
	.word	cmp_d		# func 50
	.word	cmp_d		# func 51
	.word	cmp_d		# func 52
	.word	cmp_d		# func 53
	.word	cmp_d		# func 54
	.word	cmp_d		# func 55
	.word	cmp_d		# func 56
	.word	cmp_d		# func 57
	.word	cmp_d		# func 58
	.word	cmp_d		# func 59
	.word	cmp_d		# func 60
	.word	cmp_d		# func 61
	.word	cmp_d		# func 62
	.word	cmp_d		# func 63

func_single_fixed_tbl:
	.word	ill		# func 0
	.word	ill		# func 1
	.word	ill		# func 2
	.word	ill		# func 3
	.word	ill		# func 4
	.word	ill		# func 5
	.word	ill		# func 6
	.word	ill		# func 7
	.word	ill		# func 8
	.word	ill		# func 9
	.word	ill		# func 10
	.word	ill		# func 11
	.word	ill		# func 12
	.word	ill		# func 13
	.word	ill		# func 14
	.word	ill		# func 15
	.word	ill		# func 16
	.word	ill		# func 17
	.word	ill		# func 18
	.word	ill		# func 19
	.word	ill		# func 20
	.word	ill		# func 21
	.word	ill		# func 22
	.word	ill		# func 23
	.word	ill		# func 24
	.word	ill		# func 25
	.word	ill		# func 26
	.word	ill		# func 27
	.word	ill		# func 28
	.word	ill		# func 29
	.word	ill		# func 30
	.word	ill		# func 31
	.word	cvt_s_w		# func 32
	.word	cvt_d_w		# func 33
	.word	ill		# func 34
	.word	ill		# func 35
	.word	ill		# func 36
	.word	ill		# func 37
	.word	ill		# func 38
	.word	ill		# func 39
	.word	ill		# func 40
	.word	ill		# func 41
	.word	ill		# func 42
	.word	ill		# func 43
	.word	ill		# func 44
	.word	ill		# func 45
	.word	ill		# func 46
	.word	ill		# func 47
	.word	ill		# func 48
	.word	ill		# func 49
	.word	ill		# func 50
	.word	ill		# func 51
	.word	ill		# func 52
	.word	ill		# func 53
	.word	ill		# func 54
	.word	ill		# func 55
	.word	ill		# func 56
	.word	ill		# func 57
	.word	ill		# func 58
	.word	ill		# func 59
	.word	ill		# func 60
	.word	ill		# func 61
	.word	ill		# func 62
	.word	ill		# func 63

func_long_fixed_tbl:
	.word	ill		# func 0
	.word	ill		# func 1
	.word	ill		# func 2
	.word	ill		# func 3
	.word	ill		# func 4
	.word	ill		# func 5
	.word	ill		# func 6
	.word	ill		# func 7
	.word	ill		# func 8
	.word	ill		# func 9
	.word	ill		# func 10
	.word	ill		# func 11
	.word	ill		# func 12
	.word	ill		# func 13
	.word	ill		# func 14
	.word	ill		# func 15
	.word	ill		# func 16
	.word	ill		# func 17
	.word	ill		# func 18
	.word	ill		# func 19
	.word	ill		# func 20
	.word	ill		# func 21
	.word	ill		# func 22
	.word	ill		# func 23
	.word	ill		# func 24
	.word	ill		# func 25
	.word	ill		# func 26
	.word	ill		# func 27
	.word	ill		# func 28
	.word	ill		# func 29
	.word	ill		# func 30
	.word	ill		# func 31
	.word	ill		# func 32
	.word	ill		# func 33
	.word	ill		# func 34
	.word	ill		# func 35
	.word	ill		# func 36
	.word	ill		# func 37
	.word	ill		# func 38
	.word	ill		# func 39
	.word	ill		# func 40
	.word	ill		# func 41
	.word	ill		# func 42
	.word	ill		# func 43
	.word	ill		# func 44
	.word	ill		# func 45
	.word	ill		# func 46
	.word	ill		# func 47
	.word	ill		# func 48
	.word	ill		# func 49
	.word	ill		# func 50
	.word	ill		# func 51
	.word	ill		# func 52
	.word	ill		# func 53
	.word	ill		# func 54
	.word	ill		# func 55
	.word	ill		# func 56
	.word	ill		# func 57
	.word	ill		# func 58
	.word	ill		# func 59
	.word	ill		# func 60
	.word	ill		# func 61
	.word	ill		# func 62
	.word	ill		# func 63

	.text

#ifdef SOFTFLOAT
mfromc1:
	srl	t1, a0, 11-2
	lw	t0, _C_LABEL(curpcb)		# get pcb of current process
	andi	t1, t1, 0x007C
	addu	t0, t0, t1

	lw	v0, U_PCB_FPREGS+FRAME_FP0(t0)

	srl	t0, a0, 16-SZREG_SHFT
	andi	t0, t0, SZREG_MASK
	addu	t0, t0, a1

	REG_PROLOGUE
	REG_S	v0, FRAME_ZERO(t0)
	REG_EPILOGUE

	b	done

mtoc1:
	REG_PROLOGUE
	REG_S	zero, FRAME_ZERO(a1)		# ensure zero has value 0
	srl	t0, a0, 16-SZREG_SHFT
	andi	t0, t0, SZREG_MASK
	addu	v0, a1, t0
	REG_L	v0, FRAME_ZERO(v0)
	REG_EPILOGUE

	srl	t1, a0, 11-2
	lw	t0, _C_LABEL(curpcb)		# get pcb of current process
	andi	t1, t1, 0x007C
	addu	t0, t0, t1

	sw	v0, U_PCB_FPREGS+FRAME_FP0(t0)

	b	done

cfromc1:
	srl	t1, a0, 11
	lw	t0, _C_LABEL(curpcb)		# get pcb of current process
	andi	t1, t1, 0x001F
	li	t2, 0x1F
	move	v0, zero
	bne	t1, t2, cfinvalid

	lw	v0, U_PCB_FPREGS+FRAME_FSR(t0)

cfinvalid:

	srl	t0, a0, 16-SZREG_SHFT
	andi	t0, t0, SZREG_MASK
	addu	t0, t0, a1

	REG_PROLOGUE
	REG_S	v0, FRAME_ZERO(t0)
	REG_EPILOGUE

	b	done

ctoc1:
	REG_PROLOGUE
	REG_S	zero, FRAME_ZERO(a1)		# ensure zero has value 0
	REG_EPILOGUE

	srl	t0, a0, 11
	andi	t0, t0, 0x001F
	li	t1, 0x1F
	bne	t0, t1, done

	srl	t0, a0, 16-SZREG_SHFT
	andi	t0, t0, SZREG_MASK
	addu	v0, a1, t0
	REG_PROLOGUE
	REG_L	v0, FRAME_ZERO(v0)
	REG_EPILOGUE
	lw	t0, _C_LABEL(curpcb)		# get pcb of current process
	#nop
	sw	v0, U_PCB_FPREGS+FRAME_FSR(t0)

	b	done

branchc1:
	srl	v0, a0, 16-2
	andi	v0, v0, 0x007C
	lw	v0, branchc1_tbl(v0)
	j	v0

	.rdata
branchc1_tbl:
	.word	bcfalse		# br 0
	.word	bctrue		# br 1
	.word	bcfalse_l	# br 2
	.word	bctrue_l	# br 3
	.word	ill		# br 4
	.word	ill		# br 5
	.word	ill		# br 6
	.word	ill		# br 7
	.word	ill		# br 8
	.word	ill		# br 9
	.word	ill		# br 10
	.word	ill		# br 11
	.word	ill		# br 12
	.word	ill		# br 13
	.word	ill		# br 14
	.word	ill		# br 15
	.word	ill		# br 16
	.word	ill		# br 17
	.word	ill		# br 18
	.word	ill		# br 19
	.word	ill		# br 20
	.word	ill		# br 21
	.word	ill		# br 22
	.word	ill		# br 23
	.word	ill		# br 24
	.word	ill		# br 25
	.word	ill		# br 26
	.word	ill		# br 27
	.word	ill		# br 28
	.word	ill		# br 29
	.word	ill		# br 30
	.word	ill		# br 31

	.text

bcfalse:
	li	v0, MIPS_FPU_COND_BIT
	and	v0, v0, a2
	beq	v0, zero, bcemul_branch
	b	done
bctrue:
	li	v0, MIPS_FPU_COND_BIT
	and	v0, v0, a2
	bne	v0, zero, bcemul_branch
	b	done
bcfalse_l:
	li	v0, MIPS_FPU_COND_BIT
	and	v0, v0, a2
	beq	v0, zero, bcemul_branch
	REG_PROLOGUE
	REG_L	v0, FRAME_EPC(a1)
	addiu	v0, v0, 4
	REG_S	v0, FRAME_EPC(a1)
	REG_EPILOGUE
	b	done
bctrue_l:
	li	v0, MIPS_FPU_COND_BIT
	and	v0, v0, a2
	bne	v0, zero, bcemul_branch
	REG_PROLOGUE
	REG_L	v0, FRAME_EPC(a1)
	addiu	v0, v0, 4
	REG_S	v0, FRAME_EPC(a1)
	REG_EPILOGUE
	b	done

bcemul_branch:
	/* Fetch delay slot instruction */
	sw	a1, CALLFRAME_SIZ + 4(sp)
	REG_PROLOGUE
	REG_L	a0, FRAME_EPC(a1)
	REG_EPILOGUE
	addiu	a0, a0, 4
	jal	_C_LABEL(fuiword)

	move	a0, v0
	lw	a1, CALLFRAME_SIZ + 4(sp)
	lw	a2, CALLFRAME_SIZ + 8(sp)

	/* Update cause */
	li	t0, MIPS_CR_BR_DELAY
	or	a2, a2, t0

	/* Free MachEmulateFP call frame */
	lw	ra, CALLFRAME_RA(sp)
	addu	sp, sp, CALLFRAME_SIZ

	j	_C_LABEL(bcemul_delay_slot)
#endif

/*
 * Single precision subtract.
 */
sub_s:
	jal	_C_LABEL(get_ft_fs_s)
	xor	t4, t4, 1			# negate FT sign bit
	b	add_sub_s
/*
 * Single precision add.
 */
add_s:
	jal	_C_LABEL(get_ft_fs_s)
add_sub_s:
	bne	t1, SEXP_INF, 1f		# is FS an infinity?
	bne	t5, SEXP_INF, result_fs_s	# if FT is not inf, result=FS
	bne	t2, zero, result_fs_s		# if FS is NAN, result is FS
	bne	t6, zero, result_ft_s		# if FT is NAN, result is FT
	bne	t0, t4, invalid_s		# both infinities same sign?
	b	result_fs_s			# result is in FS
1:
	beq	t5, SEXP_INF, result_ft_s	# if FT is inf, result=FT
	bne	t1, zero, 4f			# is FS a denormalized num?
	beq	t2, zero, 3f			# is FS zero?
	bne	t5, zero, 2f			# is FT a denormalized num?
	beq	t6, zero, result_fs_s		# FT is zero, result=FS
	jal	_C_LABEL(renorm_fs_s)
	jal	_C_LABEL(renorm_ft_s)
	b	5f
2:
	jal	_C_LABEL(renorm_fs_s)
	subu	t5, t5, SEXP_BIAS		# unbias FT exponent
	or	t6, t6, SIMPL_ONE		# set implied one bit
	b	5f
3:
	bne	t5, zero, result_ft_s		# if FT != 0, result=FT
	bne	t6, zero, result_ft_s
	and	v0, a2, MIPS_FPU_ROUNDING_BITS	# get rounding mode
	bne	v0, MIPS_FPU_ROUND_RM, 1f	# round to -infinity?
	or	t0, t0, t4			# compute result sign
	b	result_fs_s
1:
	and	t0, t0, t4			# compute result sign
	b	result_fs_s
4:
	bne	t5, zero, 2f			# is FT a denormalized num?
	beq	t6, zero, result_fs_s		# FT is zero, result=FS
	subu	t1, t1, SEXP_BIAS		# unbias FS exponent
	or	t2, t2, SIMPL_ONE		# set implied one bit
	jal	_C_LABEL(renorm_ft_s)
	b	5f
2:
	subu	t1, t1, SEXP_BIAS		# unbias FS exponent
	or	t2, t2, SIMPL_ONE		# set implied one bit
	subu	t5, t5, SEXP_BIAS		# unbias FT exponent
	or	t6, t6, SIMPL_ONE		# set implied one bit
/*
 * Perform the addition.
 */
5:
	move	t8, zero			# no shifted bits (sticky reg)
	beq	t1, t5, 4f			# no shift needed
	subu	v0, t1, t5			# v0 = difference of exponents
	move	v1, v0				# v1 = abs(difference)
	bge	v0, zero, 1f
	negu	v1
1:
	ble	v1, SFRAC_BITS+2, 2f		# is difference too great?
	li	t8, STICKYBIT			# set the sticky bit
	bge	v0, zero, 1f			# check which exp is larger
	move	t1, t5				# result exp is FTs
	move	t2, zero			# FSs fraction shifted is zero
	b	4f
1:
	move	t6, zero			# FTs fraction shifted is zero
	b	4f
2:
	li	t9, 32				# compute 32 - abs(exp diff)
	subu	t9, t9, v1
	bgt	v0, zero, 3f			# if FS > FT, shift FTs frac
	move	t1, t5				# FT > FS, result exp is FTs
	sll	t8, t2, t9			# save bits shifted out
	srl	t2, t2, v1			# shift FSs fraction
	b	4f
3:
	sll	t8, t6, t9			# save bits shifted out
	srl	t6, t6, v1			# shift FTs fraction
4:
	bne	t0, t4, 1f			# if signs differ, subtract
	addu	t2, t2, t6			# add fractions
	b	norm_s
1:
	blt	t2, t6, 3f			# subtract larger from smaller
	bne	t2, t6, 2f			# if same, result=0
	move	t1, zero			# result=0
	move	t2, zero
	and	v0, a2, MIPS_FPU_ROUNDING_BITS	# get rounding mode
	bne	v0, MIPS_FPU_ROUND_RM, 1f	# round to -infinity?
	or	t0, t0, t4			# compute result sign
	b	result_fs_s
1:
	and	t0, t0, t4			# compute result sign
	b	result_fs_s
2:
	sltu	t9, zero, t8			# compute t2:zero - t6:t8
	subu	t8, zero, t8
	subu	t2, t2, t6			# subtract fractions
	subu	t2, t2, t9			# subtract barrow
	b	norm_s
3:
	move	t0, t4				# sign of result = FTs
	sltu	t9, zero, t8			# compute t6:zero - t2:t8
	subu	t8, zero, t8
	subu	t2, t6, t2			# subtract fractions
	subu	t2, t2, t9			# subtract barrow
	b	norm_s

/*
 * Double precision subtract.
 */
sub_d:
	jal	_C_LABEL(get_ft_fs_d)
	xor	t4, t4, 1			# negate sign bit
	b	add_sub_d
/*
 * Double precision add.
 */
add_d:
	jal	_C_LABEL(get_ft_fs_d)
add_sub_d:
	bne	t1, DEXP_INF, 1f		# is FS an infinity?
	bne	t5, DEXP_INF, result_fs_d	# if FT is not inf, result=FS
	bne	t2, zero, result_fs_d		# if FS is NAN, result is FS
	bne	t3, zero, result_fs_d
	bne	t6, zero, result_ft_d		# if FT is NAN, result is FT
	bne	t7, zero, result_ft_d
	bne	t0, t4, invalid_d		# both infinities same sign?
	b	result_fs_d			# result is in FS
1:
	beq	t5, DEXP_INF, result_ft_d	# if FT is inf, result=FT
	bne	t1, zero, 4f			# is FS a denormalized num?
	bne	t2, zero, 1f			# is FS zero?
	beq	t3, zero, 3f
1:
	bne	t5, zero, 2f			# is FT a denormalized num?
	bne	t6, zero, 1f
	beq	t7, zero, result_fs_d		# FT is zero, result=FS
1:
	jal	_C_LABEL(renorm_fs_d)
	jal	_C_LABEL(renorm_ft_d)
	b	5f
2:
	jal	_C_LABEL(renorm_fs_d)
	subu	t5, t5, DEXP_BIAS		# unbias FT exponent
	or	t6, t6, DIMPL_ONE		# set implied one bit
	b	5f
3:
	bne	t5, zero, result_ft_d		# if FT != 0, result=FT
	bne	t6, zero, result_ft_d
	bne	t7, zero, result_ft_d
	and	v0, a2, MIPS_FPU_ROUNDING_BITS	# get rounding mode
	bne	v0, MIPS_FPU_ROUND_RM, 1f	# round to -infinity?
	or	t0, t0, t4			# compute result sign
	b	result_fs_d
1:
	and	t0, t0, t4			# compute result sign
	b	result_fs_d
4:
	bne	t5, zero, 2f			# is FT a denormalized num?
	bne	t6, zero, 1f
	beq	t7, zero, result_fs_d		# FT is zero, result=FS
1:
	subu	t1, t1, DEXP_BIAS		# unbias FS exponent
	or	t2, t2, DIMPL_ONE		# set implied one bit
	jal	_C_LABEL(renorm_ft_d)
	b	5f
2:
	subu	t1, t1, DEXP_BIAS		# unbias FS exponent
	or	t2, t2, DIMPL_ONE		# set implied one bit
	subu	t5, t5, DEXP_BIAS		# unbias FT exponent
	or	t6, t6, DIMPL_ONE		# set implied one bit
/*
 * Perform the addition.
 */
5:
	move	t8, zero			# no shifted bits (sticky reg)
	beq	t1, t5, 4f			# no shift needed
	subu	v0, t1, t5			# v0 = difference of exponents
	move	v1, v0				# v1 = abs(difference)
	bge	v0, zero, 1f
	negu	v1
1:
	ble	v1, DFRAC_BITS+2, 2f		# is difference too great?
	li	t8, STICKYBIT			# set the sticky bit
	bge	v0, zero, 1f			# check which exp is larger
	move	t1, t5				# result exp is FTs
	move	t2, zero			# FSs fraction shifted is zero
	move	t3, zero
	b	4f
1:
	move	t6, zero			# FTs fraction shifted is zero
	move	t7, zero
	b	4f
2:
	li	t9, 32
	bge	v0, zero, 3f			# if FS > FT, shift FTs frac
	move	t1, t5				# FT > FS, result exp is FTs
	blt	v1, t9, 1f			# shift right by < 32?
	subu	v1, v1, t9
	subu	t9, t9, v1
	sll	t8, t2, t9			# save bits shifted out
	sltu	t9, zero, t3			# dont lose any one bits
	or	t8, t8, t9			# save sticky bit
	srl	t3, t2, v1			# shift FSs fraction
	move	t2, zero
	b	4f
1:
	subu	t9, t9, v1
	sll	t8, t3, t9			# save bits shifted out
	srl	t3, t3, v1			# shift FSs fraction
	sll	t9, t2, t9			# save bits shifted out of t2
	or	t3, t3, t9			# and put into t3
	srl	t2, t2, v1
	b	4f
3:
	blt	v1, t9, 1f			# shift right by < 32?
	subu	v1, v1, t9
	subu	t9, t9, v1
	sll	t8, t6, t9			# save bits shifted out
	srl	t7, t6, v1			# shift FTs fraction
	move	t6, zero
	b	4f
1:
	subu	t9, t9, v1
	sll	t8, t7, t9			# save bits shifted out
	srl	t7, t7, v1			# shift FTs fraction
	sll	t9, t6, t9			# save bits shifted out of t2
	or	t7, t7, t9			# and put into t3
	srl	t6, t6, v1
4:
	bne	t0, t4, 1f			# if signs differ, subtract
	addu	t3, t3, t7			# add fractions
	sltu	t9, t3, t7			# compute carry
	addu	t2, t2, t6			# add fractions
	addu	t2, t2, t9			# add carry
	b	norm_d
1:
	blt	t2, t6, 3f			# subtract larger from smaller
	bne	t2, t6, 2f
	bltu	t3, t7, 3f
	bne	t3, t7, 2f			# if same, result=0
	move	t1, zero			# result=0
	move	t2, zero
	move	t3, zero
	and	v0, a2, MIPS_FPU_ROUNDING_BITS	# get rounding mode
	bne	v0, MIPS_FPU_ROUND_RM, 1f	# round to -infinity?
	or	t0, t0, t4			# compute result sign
	b	result_fs_d
1:
	and	t0, t0, t4			# compute result sign
	b	result_fs_d
2:
	beq	t8, zero, 1f			# compute t2:t3:zero - t6:t7:t8
	subu	t8, zero, t8
	sltu	v0, t3, 1			# compute barrow out
	subu	t3, t3, 1			# subtract barrow
	subu	t2, t2, v0
1:
	sltu	v0, t3, t7
	subu	t3, t3, t7			# subtract fractions
	subu	t2, t2, t6			# subtract fractions
	subu	t2, t2, v0			# subtract barrow
	b	norm_d
3:
	move	t0, t4				# sign of result = FTs
	beq	t8, zero, 1f			# compute t6:t7:zero - t2:t3:t8
	subu	t8, zero, t8
	sltu	v0, t7, 1			# compute barrow out
	subu	t7, t7, 1			# subtract barrow
	subu	t6, t6, v0
1:
	sltu	v0, t7, t3
	subu	t3, t7, t3			# subtract fractions
	subu	t2, t6, t2			# subtract fractions
	subu	t2, t2, v0			# subtract barrow
	b	norm_d

/*
 * Single precision multiply.
 */
mul_s:
	jal	_C_LABEL(get_ft_fs_s)
	xor	t0, t0, t4			# compute sign of result
	move	t4, t0
	bne	t1, SEXP_INF, 2f		# is FS an infinity?
	bne	t2, zero, result_fs_s		# if FS is a NAN, result=FS
	bne	t5, SEXP_INF, 1f		# FS is inf, is FT an infinity?
	bne	t6, zero, result_ft_s		# if FT is a NAN, result=FT
	b	result_fs_s			# result is infinity
1:
	bne	t5, zero, result_fs_s		# inf * zero? if no, result=FS
	bne	t6, zero, result_fs_s
	b	invalid_s			# infinity * zero is invalid
2:
	bne	t5, SEXP_INF, 1f		# FS != inf, is FT an infinity?
	bne	t1, zero, result_ft_s		# zero * inf? if no, result=FT
	bne	t2, zero, result_ft_s
	bne	t6, zero, result_ft_s		# if FT is a NAN, result=FT
	b	invalid_s			# zero * infinity is invalid
1:
	bne	t1, zero, 1f			# is FS zero?
	beq	t2, zero, result_fs_s		# result is zero
	jal	_C_LABEL(renorm_fs_s)
	b	2f
1:
	subu	t1, t1, SEXP_BIAS		# unbias FS exponent
	or	t2, t2, SIMPL_ONE		# set implied one bit
2:
	bne	t5, zero, 1f			# is FT zero?
	beq	t6, zero, result_ft_s		# result is zero
	jal	_C_LABEL(renorm_ft_s)
	b	2f
1:
	subu	t5, t5, SEXP_BIAS		# unbias FT exponent
	or	t6, t6, SIMPL_ONE		# set implied one bit
2:
	addu	t1, t1, t5			# compute result exponent
	addu	t1, t1, 9			# account for binary point
	multu	t2, t6				# multiply fractions
	mflo	t8
	mfhi	t2
	b	norm_s

/*
 * Double precision multiply.
 */
mul_d:
	jal	_C_LABEL(get_ft_fs_d)
	xor	t0, t0, t4			# compute sign of result
	move	t4, t0
	bne	t1, DEXP_INF, 2f		# is FS an infinity?
	bne	t2, zero, result_fs_d		# if FS is a NAN, result=FS
	bne	t3, zero, result_fs_d
	bne	t5, DEXP_INF, 1f		# FS is inf, is FT an infinity?
	bne	t6, zero, result_ft_d		# if FT is a NAN, result=FT
	bne	t7, zero, result_ft_d
	b	result_fs_d			# result is infinity
1:
	bne	t5, zero, result_fs_d		# inf * zero? if no, result=FS
	bne	t6, zero, result_fs_d
	bne	t7, zero, result_fs_d
	b	invalid_d			# infinity * zero is invalid
2:
	bne	t5, DEXP_INF, 1f		# FS != inf, is FT an infinity?
	bne	t1, zero, result_ft_d		# zero * inf? if no, result=FT
	bne	t2, zero, result_ft_d		# if FS is a NAN, result=FS
	bne	t3, zero, result_ft_d
	bne	t6, zero, result_ft_d		# if FT is a NAN, result=FT
	bne	t7, zero, result_ft_d
	b	invalid_d			# zero * infinity is invalid
1:
	bne	t1, zero, 2f			# is FS zero?
	bne	t2, zero, 1f
	beq	t3, zero, result_fs_d		# result is zero
1:
	jal	_C_LABEL(renorm_fs_d)
	b	3f
2:
	subu	t1, t1, DEXP_BIAS		# unbias FS exponent
	or	t2, t2, DIMPL_ONE		# set implied one bit
3:
	bne	t5, zero, 2f			# is FT zero?
	bne	t6, zero, 1f
	beq	t7, zero, result_ft_d		# result is zero
1:
	jal	_C_LABEL(renorm_ft_d)
	b	3f
2:
	subu	t5, t5, DEXP_BIAS		# unbias FT exponent
	or	t6, t6, DIMPL_ONE		# set implied one bit
3:
	addu	t1, t1, t5			# compute result exponent
	addu	t1, t1, 12			# ???
	multu	t3, t7				# multiply fractions (low * low)
	move	t4, t2				# free up t2,t3 for result
	move	t5, t3
	mflo	a3				# save low order bits
	mfhi	t8
	not	v0, t8
	multu	t4, t7				# multiply FS(high) * FT(low)
	mflo	v1
	mfhi	t3				# init low result
	sltu	v0, v0, v1			# compute carry
	addu	t8, v1
	multu	t5, t6				# multiply FS(low) * FT(high)
	addu	t3, t3, v0			# add carry
	not	v0, t8
	mflo	v1
	mfhi	t2
	sltu	v0, v0, v1
	addu	t8, v1
	multu	t4, t6				# multiply FS(high) * FT(high)
	addu	t3, v0
	not	v1, t3
	sltu	v1, v1, t2
	addu	t3, t2
	not	v0, t3
	mfhi	t2
	addu	t2, v1
	mflo	v1
	sltu	v0, v0, v1
	addu	t2, v0
	addu	t3, v1
	sltu	a3, zero, a3			# reduce t8,a3 to just t8
	or	t8, a3
	b	norm_d

/*
 * Single precision divide.
 */
div_s:
	jal	_C_LABEL(get_ft_fs_s)
	xor	t0, t0, t4			# compute sign of result
	move	t4, t0
	bne	t1, SEXP_INF, 1f		# is FS an infinity?
	bne	t2, zero, result_fs_s		# if FS is NAN, result is FS
	bne	t5, SEXP_INF, result_fs_s	# is FT an infinity?
	bne	t6, zero, result_ft_s		# if FT is NAN, result is FT
	b	invalid_s			# infinity/infinity is invalid
1:
	bne	t5, SEXP_INF, 1f		# is FT an infinity?
	bne	t6, zero, result_ft_s		# if FT is NAN, result is FT
	move	t1, zero			# x / infinity is zero
	move	t2, zero
	b	result_fs_s
1:
	bne	t1, zero, 2f			# is FS zero?
	bne	t2, zero, 1f
	bne	t5, zero, result_fs_s		# FS=zero, is FT zero?
	beq	t6, zero, invalid_s		# 0 / 0
	b	result_fs_s			# result = zero
1:
	jal	_C_LABEL(renorm_fs_s)
	b	3f
2:
	subu	t1, t1, SEXP_BIAS		# unbias FS exponent
	or	t2, t2, SIMPL_ONE		# set implied one bit
3:
	bne	t5, zero, 2f			# is FT zero?
	bne	t6, zero, 1f
	or	a2, a2, MIPS_FPU_EXCEPTION_DIV0 | MIPS_FPU_STICKY_DIV0
	and	v0, a2, MIPS_FPU_ENABLE_DIV0	# trap enabled?
	bne	v0, zero, fpe_trap
#ifdef SOFTFLOAT
	lw	t1, _C_LABEL(curpcb)		# get pcb of current process
	#nop
	sw	a2, U_PCB_FPREGS+FRAME_FSR(t1)
#else
	ctc1	a2, MIPS_FPU_CSR		# save exceptions
#endif
	li	t1, SEXP_INF			# result is infinity
	move	t2, zero
	b	result_fs_s
1:
	jal	_C_LABEL(renorm_ft_s)
	b	3f
2:
	subu	t5, t5, SEXP_BIAS		# unbias FT exponent
	or	t6, t6, SIMPL_ONE		# set implied one bit
3:
	subu	t1, t1, t5			# compute exponent
	subu	t1, t1, 3			# compensate for result position
	li	v0, SFRAC_BITS+3		# number of bits to divide
	move	t8, t2				# init dividend
	move	t2, zero			# init result
1:
	bltu	t8, t6, 3f			# is dividend >= divisor?
2:
	subu	t8, t8, t6			# subtract divisor from dividend
	or	t2, t2, 1			# remember that we did
	bne	t8, zero, 3f			# if not done, continue
	sll	t2, t2, v0			# shift result to final position
	b	norm_s
3:
	sll	t8, t8, 1			# shift dividend
	sll	t2, t2, 1			# shift result
	subu	v0, v0, 1			# are we done?
	bne	v0, zero, 1b			# no, continue
	b	norm_s

/*
 * Double precision divide.
 */
div_d:
	jal	_C_LABEL(get_ft_fs_d)
	xor	t0, t0, t4			# compute sign of result
	move	t4, t0
	bne	t1, DEXP_INF, 1f		# is FS an infinity?
	bne	t2, zero, result_fs_d		# if FS is NAN, result is FS
	bne	t3, zero, result_fs_d
	bne	t5, DEXP_INF, result_fs_d	# is FT an infinity?
	bne	t6, zero, result_ft_d		# if FT is NAN, result is FT
	bne	t7, zero, result_ft_d
	b	invalid_d			# infinity/infinity is invalid
1:
	bne	t5, DEXP_INF, 1f		# is FT an infinity?
	bne	t6, zero, result_ft_d		# if FT is NAN, result is FT
	bne	t7, zero, result_ft_d
	move	t1, zero			# x / infinity is zero
	move	t2, zero
	move	t3, zero
	b	result_fs_d
1:
	bne	t1, zero, 2f			# is FS zero?
	bne	t2, zero, 1f
	bne	t3, zero, 1f
	bne	t5, zero, result_fs_d		# FS=zero, is FT zero?
	bne	t6, zero, result_fs_d
	beq	t7, zero, invalid_d		# 0 / 0
	b	result_fs_d			# result = zero
1:
	jal	_C_LABEL(renorm_fs_d)
	b	3f
2:
	subu	t1, t1, DEXP_BIAS		# unbias FS exponent
	or	t2, t2, DIMPL_ONE		# set implied one bit
3:
	bne	t5, zero, 2f			# is FT zero?
	bne	t6, zero, 1f
	bne	t7, zero, 1f
	or	a2, a2, MIPS_FPU_EXCEPTION_DIV0 | MIPS_FPU_STICKY_DIV0
	and	v0, a2, MIPS_FPU_ENABLE_DIV0	# trap enabled?
	bne	v0, zero, fpe_trap
#ifdef SOFTFLOAT
	lw	t1, _C_LABEL(curpcb)		# get pcb of current process
	#nop
	sw	a2, U_PCB_FPREGS+FRAME_FSR(t1)
#else
	ctc1	a2, MIPS_FPU_CSR		# save exceptions
#endif
	li	t1, DEXP_INF			# result is infinity
	move	t2, zero
	move	t3, zero
	b	result_fs_d
1:
	jal	_C_LABEL(renorm_ft_d)
	b	3f
2:
	subu	t5, t5, DEXP_BIAS		# unbias FT exponent
	or	t6, t6, DIMPL_ONE		# set implied one bit
3:
	subu	t1, t1, t5			# compute exponent
	subu	t1, t1, 3			# compensate for result position
	li	v0, DFRAC_BITS+3		# number of bits to divide
	move	t8, t2				# init dividend
	move	t9, t3
	move	t2, zero			# init result
	move	t3, zero
1:
	bltu	t8, t6, 3f			# is dividend >= divisor?
	bne	t8, t6, 2f
	bltu	t9, t7, 3f
2:
	sltu	v1, t9, t7			# subtract divisor from dividend
	subu	t9, t9, t7
	subu	t8, t8, t6
	subu	t8, t8, v1
	or	t3, t3, 1			# remember that we did
	bne	t8, zero, 3f			# if not done, continue
	bne	t9, zero, 3f
	li	v1, 32				# shift result to final position
	blt	v0, v1, 2f			# shift < 32 bits?
	subu	v0, v0, v1			# shift by > 32 bits
	sll	t2, t3, v0			# shift upper part
	move	t3, zero
	b	norm_d
2:
	subu	v1, v1, v0			# shift by < 32 bits
	sll	t2, t2, v0			# shift upper part
	srl	t9, t3, v1			# save bits shifted out
	or	t2, t2, t9			# and put into upper part
	sll	t3, t3, v0
	b	norm_d
3:
	sll	t8, t8, 1			# shift dividend
	srl	v1, t9, 31			# save bit shifted out
	or	t8, t8, v1			# and put into upper part
	sll	t9, t9, 1
	sll	t2, t2, 1			# shift result
	srl	v1, t3, 31			# save bit shifted out
	or	t2, t2, v1			# and put into upper part
	sll	t3, t3, 1
	subu	v0, v0, 1			# are we done?
	bne	v0, zero, 1b			# no, continue
	sltu	v0, zero, t9			# be sure to save any one bits
	or	t8, t8, v0			# from the lower remainder
	b	norm_d

#ifdef MIPS3
sqrt_s:
	jal	_C_LABEL(get_fs_s)

	/* Take care of zero, negative, inf, and NaN special cases */
	or	v0, t1, t2			# sqrt(+-0) == +-0
	beq	v0, zero, result_fs_s		# ...
	bne	t0, zero, 1f			# sqrt(-val) == sNaN
	bne	t1, SEXP_INF, 2f		# skip forward if not infinity
	b	result_fs_s			# sqrt(NaN,+inf) == itself
1:	move	t0, zero			# result is a quiet NAN
	li	t1, SEXP_INF			# sqrt(-inf,-val) == sNaN
	li	t2, SQUIET_NAN
	b	result_fs_s
2:
	/* normalize FS if needed */
	bne	t1, zero, 2f
	jal	_C_LABEL(renorm_fs_s)
2:	and	t2, t2, (SIMPL_ONE-1)		# ix &= 0x007fffff;
	or	t2, t2, SIMPL_ONE		# ix |= 0x00800000;
	and	v0, t1, 1			# if (m & 1)
	beq	v0, zero, 1f			# ...
	add	t2, t2, t2			#	ix += ix;
1:	sra	t1, t1, 1			# m = m / 2;

	/* generate sqrt(FS) bit by bit */
	add	t2, t2, t2			# ix += ix;
	move	t4, zero			# q = 0; (result)
	li	t8, SIMPL_ONE<<1		# r = 0x01000000;
	move	t6, zero			# s = 0;
1:	beq	t8, zero, 3f			# while (r != 0) {
	add	t9, t6, t8			#	t = s + r;
	bgt	t9, t2, 2f			#	if (t <= ix)
	add	t6, t9, t8			#		s = t + r;
	sub	t2, t2, t9			#		ix -= t;
	add	t4, t4, t8			#		q += r;
2:	add	t2, t2, t2			#	ix += ix;
	srl	t8, t8, 1			# 	r >>= 1;
	b	1b				# }
3:
	/* rounding -- all mips rounding modes use the same rounding here */
	beq	t2, zero, 1f			# if (ix != 0)
	and	v0, t4, 1			# q += q&1;
	add	t4, t4, v0			# ...

	/* calculate result */
1:	srl	t2, t4, 1			# ix = (q >> 1);
	add	t1, t1, SEXP_BIAS 		# m += 127; (re-bias)
	li	v1, SIMPL_ONE
	and	v0, t2, v1			# keep extra exponent bit
	bne	v0, zero, 1f			# if it is there.
	sub	t1, t1, 1			# ...
1:
	nor	v1, v1, v1			# ~SIMP_ONE
	and	t2, t2, v1			# ix &= ~SIMPL_ONE
	b	result_fs_s			# store result (already normal)

sqrt_d:
	jal	_C_LABEL(get_fs_d)

	/* Take care of zero, negative, inf, and NaN special cases */
	or	v0, t1, t2			# sqrt(+-0) == +- 0
	or	v0, v0, t3			# ...
	beq	v0, zero, result_fs_d		# ...
	bne	t0, zero, 1f			# sqrt(-val) == sNaN
	bne	t1, DEXP_INF, 2f		# skip forward if not infinity
	b	result_fs_d			# sqrt(NaN,+inf) == itself
1:	move	t0, zero			# sqrt(-inf,-val) == sNaN
	li	t1, DEXP_INF
	li	t2, DQUIET_NAN0
	li	t3, DQUIET_NAN1
	b	result_fs_d
2:
	/* normalize FS if needed */
	bne	t1, zero, 2f
	jal	_C_LABEL(renorm_fs_d)
2:	and	t2, t2, (DIMPL_ONE-1)		# ix0 &= 0x000fffff
	or	t2, t2, DIMPL_ONE		# ix0 |= 0x00100000
	and	v0, t1, 1			# if (m & 1)
	beq	v0, zero, 1f			# ...
	add	t2, t2, t2			# ix0 += ix0
	srl	v0, t3, 31			# ix0 += (ix1&sign)>>31)
	and	v0, v0, 1			# ...
	add	t2, t2, v0			# ...
	addu	t3, t3, t3			# ix1 += ix1;
1:	sra	t1, t1, 1			# m = m / 2;

	/* generate sqrt(FS) bit by bit -- first upper */
	addu	t2, t2, t2			# ix0 += ix0;
	srl	v0, t3, 31			# ix0 += (ix1&sign)>>31)
	and	v0, v0, 1			# ...
	add	t2, t2, v0			# ...
	addu	t3, t3, t3			# ix1 += ix1;

	move	t4, zero			# q = 0;	(result)
	move	t5, zero			# q1 = 0;	(result)
	move	t6, zero			# s0 = 0;
	move	t7, zero			# s1 = 0;
	li	t8, DIMPL_ONE<<1		# t = 0x00200000;
1:	beq	t8, zero, 3f			# while (r != 0) {
	add	t9, t6, t8			#	t = s0+r;
	bgt	t9, t2, 2f			#	if (t <= ix0)
	add	t6, t9, t8			#		s0 = t + r;
	sub	t2, t2, t9			#		ix0 -= t;
	add	t4, t4, t8			#		q += r;
2:	add	t2, t2, t2			#	ix0 += ix0;
	srl	v0, t3, 31			# 	ix0 += (ix1&sign)>>31)
	and	v0, v0, 1			# 	...
	add	t2, t2, v0			# 	...
	addu	t3, t3, t3			#	ix1 += ix1;
	srl	t8, t8, 1			#	r >>= 1;
	b	1b				# }
3:
	/* then lower bits */
	li	t8, 1<<31			# r = sign;
1:	beq	t8, zero, 4f			# while (r != 0) {
	addu	v1, t7, t8			#    t1 = s1 + r;
	move	t9, t6				#    t = s0;
	blt	t9, t2, 2f			#    if ( (t<ix0) ||
	bne	t9, t2, 3f			#         ((t == ix0) &&
	bgtu	v1, t3, 3f			#          (t1 <= ix1)))
2:	addu	t7, v1, t8			#	s1 = t1 + r;
	srl	v0, v1, 31			#	if (((t1&sign)==sign) &&
	and	v0, v0, 1			#	...
	beq	v0, zero, 2f			#	...
	srl	v0, t7, 31			#	    (s1&sign) == 0)
	and	v0, v0, 1			#	    ...
	bne	v0, zero, 2f			#	    ...
	add	t6, t6, 1			#	    s0 += 1;
2:	sub	t2, t2, t9			#	ix0 -= t;
	bgeu	t3, v1, 2f			#	if (ix1 < t1)
	sub	t2, t2, 1			#	    ix0 -= 1;
2:	subu	t3, t3, v1			#	ix1 -= t1;
	addu	t5, t5, t8			#	q1 += r;
3:	add	t2, t2, t2			#    ix0 += ix0;
	srl	v0, t3, 31			#    ix0 += (ix1&sign)>>31)
	and	v0, v0, 1			#    ...
	add	t2, t2, v0			#    ...
	addu	t3, t3, t3			#    ix1 += ix1;
	srl	t8, t8, 1			#    r >>= 1;
	b	1b				# }
4:

	/* rounding -- all mips rounding modes use the same rounding here */
	or	v0, t2, t3			# if (ix0 | ix1)
	beq	v0, zero, 2f			# ...
	li	v0, 0xffffffff			#    if (q1 == 0xffffffff)
	and	v1, t2, v0			#    ...
	bne	v1, v0, 1f			#    ...
	move	t5, zero			#	q1 = 0;
	add	t4, t4, 1			#	q += 1;
	b	2f				#    else
1:	and	v0, t5, 1			#       q1 += q1 & 1;
	addu	t5, t5, v0			#       ...

	/* calculate result */
2:	srl	t2, t4, 1			# ix0 = q >> 1;
	srl	t3, t5, 1			# ix1 = q1 >> 1;
	and	v0, t4, 1			# if ((q & 1) == 1)
	beq	v0, zero, 1f			# ...
	or	t3, (1<<31)			#	ix1 |= sign;
1:	add	t1, t1, DEXP_BIAS		# m += 1023;
	li	v1, DIMPL_ONE
	and	v0, t2, v1			# keep extra exponent bit
	bne	v0, zero, 1f			# if it is there.
	sub	t1, t1, 1			# ...
1:
	nor	v1, v1, v1			# ~DIMPL_ONE
	and	t2, t2, v1			# ix0 &= ~DIMPL_ONE
	b	result_fs_d			# store result (already normal)
#endif	/* MIPS3 */

/*
 * Single precision absolute value.
 */
abs_s:
	jal	_C_LABEL(get_fs_s)
	move	t0, zero			# set sign positive
	b	result_fs_s

/*
 * Double precision absolute value.
 */
abs_d:
	jal	_C_LABEL(get_fs_d)
	move	t0, zero			# set sign positive
	b	result_fs_d

/*
 * Single precision move.
 */
mov_s:
	jal	_C_LABEL(get_fs_s)
	b	result_fs_s

/*
 * Double precision move.
 */
mov_d:
	jal	_C_LABEL(get_fs_d)
	b	result_fs_d

/*
 * Single precision negate.
 */
neg_s:
	jal	_C_LABEL(get_fs_s)
	xor	t0, t0, 1			# reverse sign
	b	result_fs_s

/*
 * Double precision negate.
 */
neg_d:
	jal	_C_LABEL(get_fs_d)
	xor	t0, t0, 1			# reverse sign
	b	result_fs_d

/*
 * Single precision mips2 rounding.  Explicit case of cvt_w_s.
 */
round_w_s:
	li	v1,0
	b	_cvt_w_s
trunc_w_s:
	li	v1,1
	b	_cvt_w_s
ceil_w_s:
	li	v1,2
	b	_cvt_w_s
floor_w_s:
	li	v1,3
	b	_cvt_w_s

/*
 * Double precision mips2 rounding.  Explicit case of cvt_w_d.
 */
round_w_d:
	li	v1,0
	b	_cvt_w_d
trunc_w_d:
	li	v1,1
	b	_cvt_w_d
ceil_w_d:
	li	v1,2
	b	_cvt_w_d
floor_w_d:
	li	v1,3
	b	_cvt_w_d

/*
 * Convert double to single.
 */
cvt_s_d:
	jal	_C_LABEL(get_fs_d)
	bne	t1, DEXP_INF, 1f		# is FS an infinity?
	li	t1, SEXP_INF			# convert to single
	sll	t2, t2, 3			# convert D fraction to S
	srl	t8, t3, 32 - 3
	or	t2, t2, t8
	b	result_fs_s
1:
	bne	t1, zero, 2f			# is FS zero?
	bne	t2, zero, 1f
	beq	t3, zero, result_fs_s		# result=0
1:
	jal	_C_LABEL(renorm_fs_d)
	subu	t1, t1, 3			# correct exp for shift below
	b	3f
2:
	subu	t1, t1, DEXP_BIAS		# unbias exponent
	or	t2, t2, DIMPL_ONE		# add implied one bit
3:
	sll	t2, t2, 3			# convert D fraction to S
	srl	t8, t3, 32 - 3
	or	t2, t2, t8
	sll	t8, t3, 3
	b	norm_noshift_s

/*
 * Convert integer to single.
 */
cvt_s_w:
	jal	_C_LABEL(get_fs_int)
	bne	t2, zero, 1f			# check for zero
	move	t1, zero
	b	result_fs_s
/*
 * Find out how many leading zero bits are in t2 and put in t9.
 */
1:
	move	v0, t2
	move	t9, zero
	srl	v1, v0, 16
	bne	v1, zero, 1f
	addu	t9, 16
	sll	v0, 16
1:
	srl	v1, v0, 24
	bne	v1, zero, 1f
	addu	t9, 8
	sll	v0, 8
1:
	srl	v1, v0, 28
	bne	v1, zero, 1f
	addu	t9, 4
	sll	v0, 4
1:
	srl	v1, v0, 30
	bne	v1, zero, 1f
	addu	t9, 2
	sll	v0, 2
1:
	srl	v1, v0, 31
	bne	v1, zero, 1f
	addu	t9, 1
/*
 * Now shift t2 the correct number of bits.
 */
1:
	subu	t9, t9, SLEAD_ZEROS		# dont count leading zeros
	li	t1, 23				# init exponent
	subu	t1, t1, t9			# compute exponent
	beq	t9, zero, 1f
	li	v0, 32
	blt	t9, zero, 2f			# if shift < 0, shift right
	subu	v0, v0, t9
	sll	t2, t2, t9			# shift left
1:
	add	t1, t1, SEXP_BIAS		# bias exponent
	and	t2, t2, ~SIMPL_ONE		# clear implied one bit
	b	result_fs_s
2:
	negu	t9				# shift right by t9
	subu	v0, v0, t9
	sll	t8, t2, v0			# save bits shifted out
	srl	t2, t2, t9
	b	norm_noshift_s

/*
 * Convert single to double.
 */
cvt_d_s:
	jal	_C_LABEL(get_fs_s)
	move	t3, zero
	bne	t1, SEXP_INF, 1f		# is FS an infinity?
	li	t1, DEXP_INF			# convert to double
	b	result_fs_d
1:
	bne	t1, zero, 2f			# is FS denormalized or zero?
	beq	t2, zero, result_fs_d		# is FS zero?
	jal	_C_LABEL(renorm_fs_s)
	move	t8, zero
	b	norm_d
2:
	addu	t1, t1, DEXP_BIAS - SEXP_BIAS	# bias exponent correctly
	sll	t3, t2, 32 - 3			# convert S fraction to D
	srl	t2, t2, 3
	b	result_fs_d

/*
 * Convert integer to double.
 */
cvt_d_w:
	jal	_C_LABEL(get_fs_int)
	bne	t2, zero, 1f			# check for zero
	move	t1, zero			# result=0
	move	t3, zero
	b	result_fs_d
/*
 * Find out how many leading zero bits are in t2 and put in t9.
 */
1:
	move	v0, t2
	move	t9, zero
	srl	v1, v0, 16
	bne	v1, zero, 1f
	addu	t9, 16
	sll	v0, 16
1:
	srl	v1, v0, 24
	bne	v1, zero, 1f
	addu	t9, 8
	sll	v0, 8
1:
	srl	v1, v0, 28
	bne	v1, zero, 1f
	addu	t9, 4
	sll	v0, 4
1:
	srl	v1, v0, 30
	bne	v1, zero, 1f
	addu	t9, 2
	sll	v0, 2
1:
	srl	v1, v0, 31
	bne	v1, zero, 1f
	addu	t9, 1
/*
 * Now shift t2 the correct number of bits.
 */
1:
	subu	t9, t9, DLEAD_ZEROS		# dont count leading zeros
	li	t1, DEXP_BIAS + 20		# init exponent
	subu	t1, t1, t9			# compute exponent
	beq	t9, zero, 1f
	li	v0, 32
	blt	t9, zero, 2f			# if shift < 0, shift right
	subu	v0, v0, t9
	sll	t2, t2, t9			# shift left
1:
	and	t2, t2, ~DIMPL_ONE		# clear implied one bit
	move	t3, zero
	b	result_fs_d
2:
	negu	t9				# shift right by t9
	subu	v0, v0, t9
	sll	t3, t2, v0
	srl	t2, t2, t9
	and	t2, t2, ~DIMPL_ONE		# clear implied one bit
	b	result_fs_d

/*
 * Convert single to integer.
 */
cvt_w_s:
	and	v1, a2, MIPS_FPU_ROUNDING_BITS	# get rounding mode
_cvt_w_s:
	jal	_C_LABEL(get_fs_s)
	bne	t1, SEXP_INF, 1f		# is FS an infinity?
	bne	t2, zero, invalid_w		# invalid conversion
1:
	bne	t1, zero, 1f			# is FS zero?
	beq	t2, zero, result_fs_w		# result is zero
	move	t2, zero			# result is an inexact zero
	b	inexact_w
1:
	subu	t1, t1, SEXP_BIAS		# unbias exponent
	or	t2, t2, SIMPL_ONE		# add implied one bit
	sll	t3, t2, 32 - 3			# convert S fraction to D
	srl	t2, t2, 3
	b	cvt_w

/*
 * Convert double to integer.
 */
cvt_w_d:
	and	v1, a2, MIPS_FPU_ROUNDING_BITS	# get rounding mode
_cvt_w_d:
	jal	_C_LABEL(get_fs_d)
	bne	t1, DEXP_INF, 1f		# is FS an infinity?
	bne	t2, zero, invalid_w		# invalid conversion
	bne	t3, zero, invalid_w		# invalid conversion
1:
	bne	t1, zero, 2f			# is FS zero?
	bne	t2, zero, 1f
	beq	t3, zero, result_fs_w		# result is zero
1:
	move	t2, zero			# result is an inexact zero
	b	inexact_w
2:
	subu	t1, t1, DEXP_BIAS		# unbias exponent
	or	t2, t2, DIMPL_ONE		# add implied one bit
cvt_w:
#if 0
	blt	t1, WEXP_MIN, underflow_w	# is exponent too small?
#else
	bge	t1, WEXP_MIN, 3f		# is exponent too small?
	beq	v1, MIPS_FPU_ROUND_RP, 1f	# round to +infinity
	beq	v1, MIPS_FPU_ROUND_RM, 2f	# round to -infinity

	move	t2, zero
	b	result_fs_w
1:
	xori	t2, t0, 1
	b	result_fs_w
2:
	sll	t2, t0, 31
	sra	t2, t2, 31
	b	result_fs_w

3:
#endif
	li	v0, WEXP_MAX+1
	bgt	t1, v0, overflow_w		# is exponent too large?
	bne	t1, v0, 1f			# special check for INT_MIN
	beq	t0, zero, overflow_w		# if positive, overflow
	bne	t2, DIMPL_ONE, overflow_w
	bne	t3, zero, overflow_w
	li	t2, INT_MIN			# result is INT_MIN
	b	result_fs_w
1:
	subu	v0, t1, 20			# compute amount to shift
	beq	v0, zero, 2f			# is shift needed?
	li	v1, 32
	blt	v0, zero, 1f			# if shift < 0, shift right
	subu	v1, v1, v0			# shift left
	sll	t2, t2, v0
	srl	t9, t3, v1			# save bits shifted out of t3
	or	t2, t2, t9			# and put into t2
	sll	t3, t3, v0			# shift FSs fraction
	b	2f
1:
	negu	v0				# shift right by v0
	subu	v1, v1, v0
	sll	t8, t3, v1			# save bits shifted out
	sltu	t8, zero, t8			# dont lose any ones
	srl	t3, t3, v0			# shift FSs fraction
	or	t3, t3, t8
	sll	t9, t2, v1			# save bits shifted out of t2
	or	t3, t3, t9			# and put into t3
	srl	t2, t2, v0
/*
 * round result (t0 is sign, t2 is integer part, t3 is fractional part).
 */
2:
	and	v0, a2, MIPS_FPU_ROUNDING_BITS	# get rounding mode
	beq	v0, MIPS_FPU_ROUND_RN, 3f	# round to nearest
	beq	v0, MIPS_FPU_ROUND_RZ, 5f	# round to zero (truncate)
	beq	v0, MIPS_FPU_ROUND_RP, 1f	# round to +infinity
	beq	t0, zero, 5f			# if sign is positive, truncate
	b	2f
1:
	bne	t0, zero, 5f			# if sign is negative, truncate
2:
	beq	t3, zero, 5f			# if no fraction bits, continue
	addu	t2, t2, 1			# add rounding bit
	blt	t2, zero, overflow_w		# overflow?
	b	5f
3:
	li	v0, GUARDBIT			# load guard bit for rounding
	addu	v0, v0, t3			# add remainder
	sltu	v1, v0, t3			# compute carry out
	beq	v1, zero, 4f			# if no carry, continue
	addu	t2, t2, 1			# add carry to result
	blt	t2, zero, overflow_w		# overflow?
4:
	bne	v0, zero, 5f			# if rounded remainder is zero
	and	t2, t2, ~1			#  clear LSB (round to nearest)
5:
	beq	t0, zero, 1f			# result positive?
	negu	t2				# convert to negative integer
1:
	beq	t3, zero, result_fs_w		# is result exact?
/*
 * Handle inexact exception.
 */
inexact_w:
	or	a2, a2, MIPS_FPU_EXCEPTION_INEXACT | MIPS_FPU_STICKY_INEXACT
	and	v0, a2, MIPS_FPU_ENABLE_INEXACT
	bne	v0, zero, fpe_trap
#ifdef SOFTFLOAT
	lw	v0, _C_LABEL(curpcb)		# get pcb of current process
	#nop
	sw	a2, U_PCB_FPREGS+FRAME_FSR(v0)
#else
	ctc1	a2, MIPS_FPU_CSR		# save exceptions
#endif
	b	result_fs_w

/*
 * Conversions to integer which overflow will trap (if enabled),
 * or generate an inexact trap (if enabled),
 * or generate an invalid exception.
 */
overflow_w:
	or	a2, a2, MIPS_FPU_EXCEPTION_OVERFLOW | MIPS_FPU_STICKY_OVERFLOW
	and	v0, a2, MIPS_FPU_ENABLE_OVERFLOW
	bne	v0, zero, fpe_trap
	and	v0, a2, MIPS_FPU_ENABLE_INEXACT
	bne	v0, zero, inexact_w		# inexact traps enabled?
	b	invalid_w

/*
 * Conversions to integer which underflow will trap (if enabled),
 * or generate an inexact trap (if enabled),
 * or generate an invalid exception.
 */
underflow_w:
	or	a2, a2, MIPS_FPU_EXCEPTION_UNDERFLOW | MIPS_FPU_STICKY_UNDERFLOW
	and	v0, a2, MIPS_FPU_ENABLE_UNDERFLOW
	bne	v0, zero, fpe_trap
	and	v0, a2, MIPS_FPU_ENABLE_INEXACT
	bne	v0, zero, inexact_w		# inexact traps enabled?
	b	invalid_w

/*
 * Compare single.
 */
cmp_s:
	jal	_C_LABEL(get_cmp_s)
	bne	t1, SEXP_INF, 1f		# is FS an infinity?
	bne	t2, zero, unordered		# FS is a NAN
1:
	bne	t5, SEXP_INF, 2f		# is FT an infinity?
	bne	t6, zero, unordered		# FT is a NAN
2:
	sll	t1, t1, 23			# reassemble exp & frac
	or	t1, t1, t2
	sll	t5, t5, 23			# reassemble exp & frac
	or	t5, t5, t6
	beq	t0, zero, 1f			# is FS positive?
	negu	t1
1:
	beq	t4, zero, 1f			# is FT positive?
	negu	t5
1:
	li	v0, COND_LESS
	blt	t1, t5, test_cond		# is FS < FT?
	li	v0, COND_EQUAL
	beq	t1, t5, test_cond		# is FS == FT?
	move	v0, zero			# FS > FT
	b	test_cond

/*
 * Compare double.
 */
cmp_d:
	jal	_C_LABEL(get_cmp_d)
	bne	t1, DEXP_INF, 1f		# is FS an infinity?
	bne	t2, zero, unordered
	bne	t3, zero, unordered		# FS is a NAN
1:
	bne	t5, DEXP_INF, 2f		# is FT an infinity?
	bne	t6, zero, unordered
	bne	t7, zero, unordered		# FT is a NAN
2:
	sll	t1, t1, 20			# reassemble exp & frac
	or	t1, t1, t2
	sll	t5, t5, 20			# reassemble exp & frac
	or	t5, t5, t6
	beq	t0, zero, 1f			# is FS positive?
	not	t3				# negate t1,t3
	not	t1
	addu	t3, t3, 1
	seq	v0, t3, zero			# compute carry
	addu	t1, t1, v0
1:
	beq	t4, zero, 1f			# is FT positive?
	not	t7				# negate t5,t7
	not	t5
	addu	t7, t7, 1
	seq	v0, t7, zero			# compute carry
	addu	t5, t5, v0
1:
	li	v0, COND_LESS
	blt	t1, t5, test_cond		# is FS(MSW) < FT(MSW)?
	move	v0, zero
	bne	t1, t5, test_cond		# is FS(MSW) > FT(MSW)?
	li	v0, COND_LESS
	bltu	t3, t7, test_cond		# is FS(LSW) < FT(LSW)?
	li	v0, COND_EQUAL
	beq	t3, t7, test_cond		# is FS(LSW) == FT(LSW)?
	move	v0, zero			# FS > FT
test_cond:
	and	v0, v0, a0			# condition match instruction?
set_cond:
	bne	v0, zero, 1f
	and	a2, a2, ~MIPS_FPU_COND_BIT	# clear condition bit
	b	2f
1:
	or	a2, a2, MIPS_FPU_COND_BIT	# set condition bit
2:
#ifdef SOFTFLOAT
	lw	v0, _C_LABEL(curpcb)		# get pcb of current process
	#nop
	sw	a2, U_PCB_FPREGS+FRAME_FSR(v0)
#else
	ctc1	a2, MIPS_FPU_CSR		# save condition bit
#endif
	b	done

unordered:
	and	v0, a0, COND_UNORDERED		# this cmp match unordered?
	bne	v0, zero, 1f
	and	a2, a2, ~MIPS_FPU_COND_BIT	# clear condition bit
	b	2f
1:
	or	a2, a2, MIPS_FPU_COND_BIT	# set condition bit
2:
	and	v0, a0, COND_SIGNAL
	beq	v0, zero, 1f			# is this a signaling cmp?
	or	a2, a2, MIPS_FPU_EXCEPTION_INVALID | MIPS_FPU_STICKY_INVALID
	and	v0, a2, MIPS_FPU_ENABLE_INVALID
	bne	v0, zero, fpe_trap
1:
#ifdef SOFTFLOAT
	lw	v0, _C_LABEL(curpcb)		# get pcb of current process
	#nop
	sw	a2, U_PCB_FPREGS+FRAME_FSR(v0)
#else
	ctc1	a2, MIPS_FPU_CSR		# save condition bit
#endif
	b	done

/*
 * Determine the amount to shift the fraction in order to restore the
 * normalized position. After that, round and handle exceptions.
 */
norm_s:
	move	v0, t2
	move	t9, zero			# t9 = num of leading zeros
	bne	t2, zero, 1f
	move	v0, t8
	addu	t9, 32
1:
	srl	v1, v0, 16
	bne	v1, zero, 1f
	addu	t9, 16
	sll	v0, 16
1:
	srl	v1, v0, 24
	bne	v1, zero, 1f
	addu	t9, 8
	sll	v0, 8
1:
	srl	v1, v0, 28
	bne	v1, zero, 1f
	addu	t9, 4
	sll	v0, 4
1:
	srl	v1, v0, 30
	bne	v1, zero, 1f
	addu	t9, 2
	sll	v0, 2
1:
	srl	v1, v0, 31
	bne	v1, zero, 1f
	addu	t9, 1
/*
 * Now shift t2,t8 the correct number of bits.
 */
1:
	subu	t9, t9, SLEAD_ZEROS		# dont count leading zeros
	subu	t1, t1, t9			# adjust the exponent
	beq	t9, zero, norm_noshift_s
	li	v1, 32
	blt	t9, zero, 1f			# if shift < 0, shift right
	subu	v1, v1, t9
	sll	t2, t2, t9			# shift t2,t8 left
	srl	v0, t8, v1			# save bits shifted out
	or	t2, t2, v0
	sll	t8, t8, t9
	b	norm_noshift_s
1:
	negu	t9				# shift t2,t8 right by t9
	subu	v1, v1, t9
	sll	v0, t8, v1			# save bits shifted out
	sltu	v0, zero, v0			# be sure to save any one bits
	srl	t8, t8, t9
	or	t8, t8, v0
	sll	v0, t2, v1			# save bits shifted out
	or	t8, t8, v0
	srl	t2, t2, t9
norm_noshift_s:
	move	t5, t1				# save unrounded exponent
	move	t6, t2				# save unrounded fraction
	and	v0, a2, MIPS_FPU_ROUNDING_BITS	# get rounding mode
	beq	v0, MIPS_FPU_ROUND_RN, 3f	# round to nearest
	beq	v0, MIPS_FPU_ROUND_RZ, 5f	# round to zero (truncate)
	beq	v0, MIPS_FPU_ROUND_RP, 1f	# round to +infinity
	beq	t0, zero, 5f			# if sign is positive, truncate
	b	2f
1:
	bne	t0, zero, 5f			# if sign is negative, truncate
2:
	beq	t8, zero, 5f			# if exact, continue
	addu	t2, t2, 1			# add rounding bit
	bne	t2, SIMPL_ONE<<1, 5f		# need to adjust exponent?
	addu	t1, t1, 1			# adjust exponent
	srl	t2, t2, 1			# renormalize fraction
	b	5f
3:
	li	v0, GUARDBIT			# load guard bit for rounding
	addu	v0, v0, t8			# add remainder
	sltu	v1, v0, t8			# compute carry out
	beq	v1, zero, 4f			# if no carry, continue
	addu	t2, t2, 1			# add carry to result
	bne	t2, SIMPL_ONE<<1, 4f		# need to adjust exponent?
	addu	t1, t1, 1			# adjust exponent
	srl	t2, t2, 1			# renormalize fraction
4:
	bne	v0, zero, 5f			# if rounded remainder is zero
	and	t2, t2, ~1			#  clear LSB (round to nearest)
5:
	bgt	t1, SEXP_MAX, overflow_s	# overflow?
	blt	t1, SEXP_MIN, underflow_s	# underflow?
	bne	t8, zero, inexact_s		# is result inexact?
	addu	t1, t1, SEXP_BIAS		# bias exponent
	and	t2, t2, ~SIMPL_ONE		# clear implied one bit
	b	result_fs_s

/*
 * Handle inexact exception.
 */
inexact_s:
	addu	t1, t1, SEXP_BIAS		# bias exponent
	and	t2, t2, ~SIMPL_ONE		# clear implied one bit
inexact_nobias_s:
	jal	_C_LABEL(set_fd_s)		# save result
	or	a2, a2, MIPS_FPU_EXCEPTION_INEXACT | MIPS_FPU_STICKY_INEXACT
	and	v0, a2, MIPS_FPU_ENABLE_INEXACT
	bne	v0, zero, fpe_trap
#ifdef SOFTFLOAT
	lw	v0, _C_LABEL(curpcb)		# get pcb of current process
	#nop
	sw	a2, U_PCB_FPREGS+FRAME_FSR(v0)
#else
	ctc1	a2, MIPS_FPU_CSR		# save exceptions
#endif
	b	done

/*
 * Overflow will trap (if enabled),
 * or generate an inexact trap (if enabled),
 * or generate an infinity.
 */
overflow_s:
	or	a2, a2, MIPS_FPU_EXCEPTION_OVERFLOW | MIPS_FPU_STICKY_OVERFLOW
	and	v0, a2, MIPS_FPU_ENABLE_OVERFLOW
	beq	v0, zero, 1f
	subu	t1, t1, 192			# bias exponent
	and	t2, t2, ~SIMPL_ONE		# clear implied one bit
	jal	_C_LABEL(set_fd_s)		# save result
	b	fpe_trap
1:
	and	v0, a2, MIPS_FPU_ROUNDING_BITS	# get rounding mode
	beq	v0, MIPS_FPU_ROUND_RN, 3f	# round to nearest
	beq	v0, MIPS_FPU_ROUND_RZ, 1f	# round to zero (truncate)
	beq	v0, MIPS_FPU_ROUND_RP, 2f	# round to +infinity
	bne	t0, zero, 3f
1:
	li	t1, SEXP_MAX			# result is max finite
	li	t2, 0x007fffff
	b	inexact_s
2:
	bne	t0, zero, 1b
3:
	li	t1, SEXP_MAX + 1		# result is infinity
	move	t2, zero
	b	inexact_s

/*
 * In this implementation, "tininess" is detected "after rounding" and
 * "loss of accuracy" is detected as "an inexact result".
 */
underflow_s:
	and	v0, a2, MIPS_FPU_ENABLE_UNDERFLOW
	beq	v0, zero, 1f
/*
 * Underflow is enabled so compute the result and trap.
 */
	addu	t1, t1, 192			# bias exponent
	and	t2, t2, ~SIMPL_ONE		# clear implied one bit
	jal	_C_LABEL(set_fd_s)		# save result
	or	a2, a2, MIPS_FPU_EXCEPTION_UNDERFLOW | MIPS_FPU_STICKY_UNDERFLOW
	b	fpe_trap
/*
 * Underflow is not enabled so compute the result,
 * signal inexact result (if it is) and trap (if enabled).
 */
1:
	move	t1, t5				# get unrounded exponent
	move	t2, t6				# get unrounded fraction
	li	t9, SEXP_MIN			# compute shift amount
	subu	t9, t9, t1			# shift t2,t8 right by t9
	blt	t9, SFRAC_BITS+2, 3f		# shift all the bits out?
	move	t1, zero			# result is inexact zero
	move	t2, zero
	or	a2, a2, MIPS_FPU_EXCEPTION_UNDERFLOW | MIPS_FPU_STICKY_UNDERFLOW
/*
 * Now round the zero result.
 * Only need to worry about rounding to +- infinity when the sign matches.
 */
	and	v0, a2, MIPS_FPU_ROUNDING_BITS	# get rounding mode
	beq	v0, MIPS_FPU_ROUND_RN, inexact_nobias_s	# round to nearest
	beq	v0, MIPS_FPU_ROUND_RZ, inexact_nobias_s	# round to zero
	beq	v0, MIPS_FPU_ROUND_RP, 1f	# round to +infinity
	beq	t0, zero, inexact_nobias_s	# if sign is positive, truncate
	b	2f
1:
	bne	t0, zero, inexact_nobias_s	# if sign is negative, truncate
2:
	addu	t2, t2, 1			# add rounding bit
	b	inexact_nobias_s
3:
	li	v1, 32
	subu	v1, v1, t9
	sltu	v0, zero, t8			# be sure to save any one bits
	sll	t8, t2, v1			# save bits shifted out
	or	t8, t8, v0			# include sticky bits
	srl	t2, t2, t9
/*
 * Now round the denormalized result.
 */
	and	v0, a2, MIPS_FPU_ROUNDING_BITS	# get rounding mode
	beq	v0, MIPS_FPU_ROUND_RN, 3f	# round to nearest
	beq	v0, MIPS_FPU_ROUND_RZ, 5f	# round to zero (truncate)
	beq	v0, MIPS_FPU_ROUND_RP, 1f	# round to +infinity
	beq	t0, zero, 5f			# if sign is positive, truncate
	b	2f
1:
	bne	t0, zero, 5f			# if sign is negative, truncate
2:
	beq	t8, zero, 5f			# if exact, continue
	addu	t2, t2, 1			# add rounding bit
	b	5f
3:
	li	v0, GUARDBIT			# load guard bit for rounding
	addu	v0, v0, t8			# add remainder
	sltu	v1, v0, t8			# compute carry out
	beq	v1, zero, 4f			# if no carry, continue
	addu	t2, t2, 1			# add carry to result
4:
	bne	v0, zero, 5f			# if rounded remainder is zero
	and	t2, t2, ~1			#  clear LSB (round to nearest)
5:
	move	t1, zero			# denorm or zero exponent
	jal	_C_LABEL(set_fd_s)		# save result
	beq	t8, zero, done			# check for exact result
	or	a2, a2, MIPS_FPU_EXCEPTION_UNDERFLOW | MIPS_FPU_STICKY_UNDERFLOW
	or	a2, a2, MIPS_FPU_EXCEPTION_INEXACT | MIPS_FPU_STICKY_INEXACT
	and	v0, a2, MIPS_FPU_ENABLE_INEXACT
	bne	v0, zero, fpe_trap
#ifdef SOFTFLOAT
	lw	v0, _C_LABEL(curpcb)		# get pcb of current process
	#nop
	sw	a2, U_PCB_FPREGS+FRAME_FSR(v0)
#else
	ctc1	a2, MIPS_FPU_CSR		# save exceptions
#endif
	b	done

/*
 * Determine the amount to shift the fraction in order to restore the
 * normalized position. After that, round and handle exceptions.
 */
norm_d:
	move	v0, t2
	move	t9, zero			# t9 = num of leading zeros
	bne	t2, zero, 1f
	move	v0, t3
	addu	t9, 32
	bne	t3, zero, 1f
	move	v0, t8
	addu	t9, 32
1:
	srl	v1, v0, 16
	bne	v1, zero, 1f
	addu	t9, 16
	sll	v0, 16
1:
	srl	v1, v0, 24
	bne	v1, zero, 1f
	addu	t9, 8
	sll	v0, 8
1:
	srl	v1, v0, 28
	bne	v1, zero, 1f
	addu	t9, 4
	sll	v0, 4
1:
	srl	v1, v0, 30
	bne	v1, zero, 1f
	addu	t9, 2
	sll	v0, 2
1:
	srl	v1, v0, 31
	bne	v1, zero, 1f
	addu	t9, 1
/*
 * Now shift t2,t3,t8 the correct number of bits.
 */
1:
	subu	t9, t9, DLEAD_ZEROS		# dont count leading zeros
	subu	t1, t1, t9			# adjust the exponent
	beq	t9, zero, norm_noshift_d
	li	v1, 32
	blt	t9, zero, 2f			# if shift < 0, shift right
	blt	t9, v1, 1f			# shift by < 32?
	subu	t9, t9, v1			# shift by >= 32
	subu	v1, v1, t9
	sll	t2, t3, t9			# shift left by t9
	srl	v0, t8, v1			# save bits shifted out
	or	t2, t2, v0
	sll	t3, t8, t9
	move	t8, zero
	b	norm_noshift_d
1:
	subu	v1, v1, t9
	sll	t2, t2, t9			# shift left by t9
	srl	v0, t3, v1			# save bits shifted out
	or	t2, t2, v0
	sll	t3, t3, t9
	srl	v0, t8, v1			# save bits shifted out
	or	t3, t3, v0
	sll	t8, t8, t9
	b	norm_noshift_d
2:
	negu	t9				# shift right by t9
	subu	v1, v1, t9			#  (known to be < 32 bits)
	sll	v0, t8, v1			# save bits shifted out
	sltu	v0, zero, v0			# be sure to save any one bits
	srl	t8, t8, t9
	or	t8, t8, v0
	sll	v0, t3, v1			# save bits shifted out
	or	t8, t8, v0
	srl	t3, t3, t9
	sll	v0, t2, v1			# save bits shifted out
	or	t3, t3, v0
	srl	t2, t2, t9
norm_noshift_d:
	move	t5, t1				# save unrounded exponent
	move	t6, t2				# save unrounded fraction (MS)
	move	t7, t3				# save unrounded fraction (LS)
	and	v0, a2, MIPS_FPU_ROUNDING_BITS	# get rounding mode
	beq	v0, MIPS_FPU_ROUND_RN, 3f	# round to nearest
	beq	v0, MIPS_FPU_ROUND_RZ, 5f	# round to zero (truncate)
	beq	v0, MIPS_FPU_ROUND_RP, 1f	# round to +infinity
	beq	t0, zero, 5f			# if sign is positive, truncate
	b	2f
1:
	bne	t0, zero, 5f			# if sign is negative, truncate
2:
	beq	t8, zero, 5f			# if exact, continue
	addu	t3, t3, 1			# add rounding bit
	bne	t3, zero, 5f			# branch if no carry
	addu	t2, t2, 1			# add carry
	bne	t2, DIMPL_ONE<<1, 5f		# need to adjust exponent?
	addu	t1, t1, 1			# adjust exponent
	srl	t2, t2, 1			# renormalize fraction
	b	5f
3:
	li	v0, GUARDBIT			# load guard bit for rounding
	addu	v0, v0, t8			# add remainder
	sltu	v1, v0, t8			# compute carry out
	beq	v1, zero, 4f			# branch if no carry
	addu	t3, t3, 1			# add carry
	bne	t3, zero, 4f			# branch if no carry
	addu	t2, t2, 1			# add carry to result
	bne	t2, DIMPL_ONE<<1, 4f		# need to adjust exponent?
	addu	t1, t1, 1			# adjust exponent
	srl	t2, t2, 1			# renormalize fraction
4:
	bne	v0, zero, 5f			# if rounded remainder is zero
	and	t3, t3, ~1			#  clear LSB (round to nearest)
5:
	bgt	t1, DEXP_MAX, overflow_d	# overflow?
	blt	t1, DEXP_MIN, underflow_d	# underflow?
	bne	t8, zero, inexact_d		# is result inexact?
	addu	t1, t1, DEXP_BIAS		# bias exponent
	and	t2, t2, ~DIMPL_ONE		# clear implied one bit
	b	result_fs_d

/*
 * Handle inexact exception.
 */
inexact_d:
	addu	t1, t1, DEXP_BIAS		# bias exponent
	and	t2, t2, ~DIMPL_ONE		# clear implied one bit
inexact_nobias_d:
	jal	_C_LABEL(set_fd_d)		# save result
	or	a2, a2, MIPS_FPU_EXCEPTION_INEXACT | MIPS_FPU_STICKY_INEXACT
	and	v0, a2, MIPS_FPU_ENABLE_INEXACT
	bne	v0, zero, fpe_trap
#ifdef SOFTFLOAT
	lw	v0, _C_LABEL(curpcb)		# get pcb of current process
	#nop
	sw	a2, U_PCB_FPREGS+FRAME_FSR(v0)
#else
	ctc1	a2, MIPS_FPU_CSR		# save exceptions
#endif
	b	done

/*
 * Overflow will trap (if enabled),
 * or generate an inexact trap (if enabled),
 * or generate an infinity.
 */
overflow_d:
	or	a2, a2, MIPS_FPU_EXCEPTION_OVERFLOW | MIPS_FPU_STICKY_OVERFLOW
	and	v0, a2, MIPS_FPU_ENABLE_OVERFLOW
	beq	v0, zero, 1f
	subu	t1, t1, 1536			# bias exponent
	and	t2, t2, ~DIMPL_ONE		# clear implied one bit
	jal	_C_LABEL(set_fd_d)		# save result
	b	fpe_trap
1:
	and	v0, a2, MIPS_FPU_ROUNDING_BITS	# get rounding mode
	beq	v0, MIPS_FPU_ROUND_RN, 3f	# round to nearest
	beq	v0, MIPS_FPU_ROUND_RZ, 1f	# round to zero (truncate)
	beq	v0, MIPS_FPU_ROUND_RP, 2f	# round to +infinity
	bne	t0, zero, 3f
1:
	li	t1, DEXP_MAX			# result is max finite
	li	t2, 0x000fffff
	li	t3, 0xffffffff
	b	inexact_d
2:
	bne	t0, zero, 1b
3:
	li	t1, DEXP_MAX + 1		# result is infinity
	move	t2, zero
	move	t3, zero
	b	inexact_d

/*
 * In this implementation, "tininess" is detected "after rounding" and
 * "loss of accuracy" is detected as "an inexact result".
 */
underflow_d:
	and	v0, a2, MIPS_FPU_ENABLE_UNDERFLOW
	beq	v0, zero, 1f
/*
 * Underflow is enabled so compute the result and trap.
 */
	addu	t1, t1, 1536			# bias exponent
	and	t2, t2, ~DIMPL_ONE		# clear implied one bit
	jal	_C_LABEL(set_fd_d)		# save result
	or	a2, a2, MIPS_FPU_EXCEPTION_UNDERFLOW | MIPS_FPU_STICKY_UNDERFLOW
	b	fpe_trap
/*
 * Underflow is not enabled so compute the result,
 * signal inexact result (if it is) and trap (if enabled).
 */
1:
	move	t1, t5				# get unrounded exponent
	move	t2, t6				# get unrounded fraction (MS)
	move	t3, t7				# get unrounded fraction (LS)
	li	t9, DEXP_MIN			# compute shift amount
	subu	t9, t9, t1			# shift t2,t8 right by t9
	blt	t9, DFRAC_BITS+2, 3f		# shift all the bits out?
	move	t1, zero			# result is inexact zero
	move	t2, zero
	move	t3, zero
	or	a2, a2, MIPS_FPU_EXCEPTION_UNDERFLOW | MIPS_FPU_STICKY_UNDERFLOW
/*
 * Now round the zero result.
 * Only need to worry about rounding to +- infinity when the sign matches.
 */
	and	v0, a2, MIPS_FPU_ROUNDING_BITS	# get rounding mode
	beq	v0, MIPS_FPU_ROUND_RN, inexact_nobias_d	# round to nearest
	beq	v0, MIPS_FPU_ROUND_RZ, inexact_nobias_d	# round to zero
	beq	v0, MIPS_FPU_ROUND_RP, 1f	# round to +infinity
	beq	t0, zero, inexact_nobias_d	# if sign is positive, truncate
	b	2f
1:
	bne	t0, zero, inexact_nobias_d	# if sign is negative, truncate
2:
	addu	t3, t3, 1			# add rounding bit
	b	inexact_nobias_d
3:
	li	v1, 32
	blt	t9, v1, 1f			# shift by < 32?
	subu	t9, t9, v1			# shift right by >= 32
	subu	v1, v1, t9
	sltu	v0, zero, t8			# be sure to save any one bits
	sll	t8, t2, v1			# save bits shifted out
	or	t8, t8, v0			# include sticky bits
	srl	t3, t2, t9
	move	t2, zero
	b	2f
1:
	subu	v1, v1, t9			# shift right by t9
	sltu	v0, zero, t8			# be sure to save any one bits
	sll	t8, t3, v1			# save bits shifted out
	or	t8, t8, v0			# include sticky bits
	srl	t3, t3, t9
	sll	v0, t2, v1			# save bits shifted out
	or	t3, t3, v0
	srl	t2, t2, t9
/*
 * Now round the denormalized result.
 */
2:
	and	v0, a2, MIPS_FPU_ROUNDING_BITS	# get rounding mode
	beq	v0, MIPS_FPU_ROUND_RN, 3f	# round to nearest
	beq	v0, MIPS_FPU_ROUND_RZ, 5f	# round to zero (truncate)
	beq	v0, MIPS_FPU_ROUND_RP, 1f	# round to +infinity
	beq	t0, zero, 5f			# if sign is positive, truncate
	b	2f
1:
	bne	t0, zero, 5f			# if sign is negative, truncate
2:
	beq	t8, zero, 5f			# if exact, continue
	addu	t3, t3, 1			# add rounding bit
	bne	t3, zero, 5f			# if no carry, continue
	addu	t2, t2, 1			# add carry
	b	5f
3:
	li	v0, GUARDBIT			# load guard bit for rounding
	addu	v0, v0, t8			# add remainder
	sltu	v1, v0, t8			# compute carry out
	beq	v1, zero, 4f			# if no carry, continue
	addu	t3, t3, 1			# add rounding bit
	bne	t3, zero, 4f			# if no carry, continue
	addu	t2, t2, 1			# add carry
4:
	bne	v0, zero, 5f			# if rounded remainder is zero
	and	t3, t3, ~1			#  clear LSB (round to nearest)
5:
	move	t1, zero			# denorm or zero exponent
	jal	_C_LABEL(set_fd_d)		# save result
	beq	t8, zero, done			# check for exact result
	or	a2, a2, MIPS_FPU_EXCEPTION_UNDERFLOW | MIPS_FPU_STICKY_UNDERFLOW
	or	a2, a2, MIPS_FPU_EXCEPTION_INEXACT | MIPS_FPU_STICKY_INEXACT
	and	v0, a2, MIPS_FPU_ENABLE_INEXACT
	bne	v0, zero, fpe_trap
#ifdef SOFTFLOAT
	lw	v0, _C_LABEL(curpcb)		# get pcb of current process
	#nop
	sw	a2, U_PCB_FPREGS+FRAME_FSR(v0)
#else
	ctc1	a2, MIPS_FPU_CSR		# save exceptions
#endif
	b	done

/*
 * Signal an invalid operation if the trap is enabled; otherwise,
 * the result is a quiet NAN.
 */
invalid_s:					# trap invalid operation
	or	a2, a2, MIPS_FPU_EXCEPTION_INVALID | MIPS_FPU_STICKY_INVALID
	and	v0, a2, MIPS_FPU_ENABLE_INVALID
	bne	v0, zero, fpe_trap
#ifdef SOFTFLOAT
	lw	v0, _C_LABEL(curpcb)		# get pcb of current process
	#nop
	sw	a2, U_PCB_FPREGS+FRAME_FSR(v0)
#else
	ctc1	a2, MIPS_FPU_CSR		# save exceptions
#endif
	move	t0, zero			# result is a quiet NAN
	li	t1, SEXP_INF
	li	t2, SQUIET_NAN
	jal	_C_LABEL(set_fd_s)		# save result (in t0,t1,t2)
	b	done

/*
 * Signal an invalid operation if the trap is enabled; otherwise,
 * the result is a quiet NAN.
 */
invalid_d:					# trap invalid operation
	or	a2, a2, MIPS_FPU_EXCEPTION_INVALID | MIPS_FPU_STICKY_INVALID
	and	v0, a2, MIPS_FPU_ENABLE_INVALID
	bne	v0, zero, fpe_trap
#ifdef SOFTFLOAT
	lw	v0, _C_LABEL(curpcb)		# get pcb of current process
	#nop
	sw	a2, U_PCB_FPREGS+FRAME_FSR(v0)
#else
	ctc1	a2, MIPS_FPU_CSR		# save exceptions
#endif
	move	t0, zero			# result is a quiet NAN
	li	t1, DEXP_INF
	li	t2, DQUIET_NAN0
	li	t3, DQUIET_NAN1
	jal	_C_LABEL(set_fd_d)		# save result (in t0,t1,t2,t3)
	b	done

/*
 * Signal an invalid operation if the trap is enabled; otherwise,
 * the result is INT_MAX or INT_MIN.
 */
invalid_w:					# trap invalid operation
	or	a2, a2, MIPS_FPU_EXCEPTION_INVALID | MIPS_FPU_STICKY_INVALID
	and	v0, a2, MIPS_FPU_ENABLE_INVALID
	bne	v0, zero, fpe_trap
#ifdef SOFTFLOAT
	lw	v0, _C_LABEL(curpcb)		# get pcb of current process
	#nop
	sw	a2, U_PCB_FPREGS+FRAME_FSR(v0)
#else
	ctc1	a2, MIPS_FPU_CSR		# save exceptions
#endif
	bne	t0, zero, 1f
	li	t2, INT_MAX			# result is INT_MAX
	b	result_fs_w
1:
	li	t2, INT_MIN			# result is INT_MIN
	b	result_fs_w

/*
 * Trap if the hardware should have handled this case.
 */
fpe_trap:
#ifdef SOFTFLOAT
	lw	v0, _C_LABEL(curpcb)		# get pcb of current process
	#nop
	sw	a2, U_PCB_FPREGS+FRAME_FSR(v0)
#else
	/*
	 * ctc1 with fpe bits set causes FPE in kernel mode panic on 5231.
	 */
	sw	a2, CALLFRAME_SIZ + 12(sp)
	lw	a0, _C_LABEL(fpcurproc)
	jal	_C_LABEL(savefpregs)		# on RM5231

	lw	a2, CALLFRAME_SIZ + 12(sp)

	lw	v0, _C_LABEL(curpcb)		# get pcb of current process
	#nop
	sw	a2, U_PCB_FPREGS+FRAME_FSR(v0)
#endif
	lw	a2, CALLFRAME_SIZ + 8(sp)
	lw	ra, CALLFRAME_RA(sp)
	addu	sp, sp, CALLFRAME_SIZ
	j	_C_LABEL(fpemul_sigfpe)

/*
 * Send an illegal instruction signal to the current process.
 */
ill:
#ifdef SOFTFLOAT
	lw	v0, _C_LABEL(curpcb)		# get pcb of current process
	#nop
	sw	a2, U_PCB_FPREGS+FRAME_FSR(v0)
#else
	ctc1	a2, MIPS_FPU_CSR		# save exceptions
#endif
	lw	a2, CALLFRAME_SIZ + 8(sp)
	lw	ra, CALLFRAME_RA(sp)
	addu	sp, sp, CALLFRAME_SIZ
	j	_C_LABEL(fpemul_sigill)

result_ft_s:
	move	t0, t4				# result is FT
	move	t1, t5
	move	t2, t6
result_fs_s:					# result is FS
	jal	_C_LABEL(set_fd_s)		# save result (in t0,t1,t2)
	b	done

result_fs_w:
	jal	_C_LABEL(set_fd_word)		# save result (in t2)
	b	done

result_ft_d:
	move	t0, t4				# result is FT
	move	t1, t5
	move	t2, t6
	move	t3, t7
result_fs_d:					# result is FS
	jal	_C_LABEL(set_fd_d)		# save result (in t0,t1,t2,t3)

done:
/*
 * Succeeded to emulate instruction with no error
 * so compute the next PC.
 */
	lw	t0, CALLFRAME_SIZ + 8(sp)
	REG_PROLOGUE
	REG_L	v0, FRAME_EPC(a1)
	REG_EPILOGUE
	bgez	t0, 1f				# Check the branch delay bit.
/*
 * The instruction is in the branch delay slot so the branch will have to
 * be emulated to get the resulting PC.
 */
	sw	a1, CALLFRAME_SIZ + 4(sp)
	move	a0, a1				# 1st arg is p. to trapframe
	move	a1, v0				# 2nd arg is instruction PC
						# 3rd arg is FP CSR
	move	a3, zero			# 4th arg is FALSE
	jal	_C_LABEL(MachEmulateBranch)	# compute PC after branch

	lw	a1, CALLFRAME_SIZ + 4(sp)
	b	2f
/*
 * This is not in the branch delay slot so calculate the resulting
 * PC (epc + 4) into v0.
 */
1:
	addiu	v0, v0, 4			# v0 = next pc
2:
	REG_PROLOGUE
	REG_S	v0, FRAME_EPC(a1)		# save new pc
	REG_EPILOGUE

	lw	ra, CALLFRAME_RA(sp)
	addu	sp, sp, CALLFRAME_SIZ
	j	ra
END(MachEmulateFP)

/*----------------------------------------------------------------------------
 * get_fs_int --
 *
 *	Read (integer) the FS register (bits 15-11).
 *	This is an internal routine used by MachEmulateFP only.
 *
 * Results:
 *	t0	contains the sign
 *	t2	contains the fraction
 *
 *----------------------------------------------------------------------------
 */
LEAF(get_fs_int)
#ifdef SOFTFLOAT
	srl	t2, a0, 11-2
	lw	t0, _C_LABEL(curpcb)		# get pcb of current process
	andi	t2, t2, 0x0078			# Even regs only
	addu	t0, t0, t2

	lw	t2, U_PCB_FPREGS+FRAME_FP0(t0)

	srl	t0, t2, 31		# init the sign bit
	bge	t2, zero, 1f
	negu	t2
1:
	j	ra
#else
	srl	a3, a0, 12 - 2			# get FS field (even regs only)
	and	a3, a3, 0xF << 2		# mask FS field
	lw	a3, get_fs_int_tbl(a3)		# switch on register number
	j	a3

	.rdata
get_fs_int_tbl:
	.word	get_fs_int_f0
	.word	get_fs_int_f2
	.word	get_fs_int_f4
	.word	get_fs_int_f6
	.word	get_fs_int_f8
	.word	get_fs_int_f10
	.word	get_fs_int_f12
	.word	get_fs_int_f14
	.word	get_fs_int_f16
	.word	get_fs_int_f18
	.word	get_fs_int_f20
	.word	get_fs_int_f22
	.word	get_fs_int_f24
	.word	get_fs_int_f26
	.word	get_fs_int_f28
	.word	get_fs_int_f30
	.text

get_fs_int_f0:
	mfc1	t2, $f0
	b	get_fs_int_done
get_fs_int_f2:
	mfc1	t2, $f2
	b	get_fs_int_done
get_fs_int_f4:
	mfc1	t2, $f4
	b	get_fs_int_done
get_fs_int_f6:
	mfc1	t2, $f6
	b	get_fs_int_done
get_fs_int_f8:
	mfc1	t2, $f8
	b	get_fs_int_done
get_fs_int_f10:
	mfc1	t2, $f10
	b	get_fs_int_done
get_fs_int_f12:
	mfc1	t2, $f12
	b	get_fs_int_done
get_fs_int_f14:
	mfc1	t2, $f14
	b	get_fs_int_done
get_fs_int_f16:
	mfc1	t2, $f16
	b	get_fs_int_done
get_fs_int_f18:
	mfc1	t2, $f18
	b	get_fs_int_done
get_fs_int_f20:
	mfc1	t2, $f20
	b	get_fs_int_done
get_fs_int_f22:
	mfc1	t2, $f22
	b	get_fs_int_done
get_fs_int_f24:
	mfc1	t2, $f24
	b	get_fs_int_done
get_fs_int_f26:
	mfc1	t2, $f26
	b	get_fs_int_done
get_fs_int_f28:
	mfc1	t2, $f28
	b	get_fs_int_done
get_fs_int_f30:
	mfc1	t2, $f30
get_fs_int_done:
	srl	t0, t2, 31		# init the sign bit
	bge	t2, zero, 1f
	negu	t2
1:
	j	ra
#endif
END(get_fs_int)

/*----------------------------------------------------------------------------
 * get_ft_fs_s --
 *
 *	Read (single precision) the FT register (bits 20-16) and
 *	the FS register (bits 15-11) and break up into fields.
 *	This is an internal routine used by MachEmulateFP only.
 *
 * Results:
 *	t0	contains the FS sign
 *	t1	contains the FS (biased) exponent
 *	t2	contains the FS fraction
 *	t4	contains the FT sign
 *	t5	contains the FT (biased) exponent
 *	t6	contains the FT fraction
 *
 *----------------------------------------------------------------------------
 */
LEAF(get_ft_fs_s)
#ifdef SOFTFLOAT
	srl	t4, a0, 16-2
	lw	t5, _C_LABEL(curpcb)		# get pcb of current process
	andi	t4, t4, 0x0078			# Even regs only
	addu	t5, t5, t4

	lw	t4, U_PCB_FPREGS+FRAME_FP0(t5)

	srl	t5, t4, 23			# get exponent
	and	t5, t5, 0xFF
	and	t6, t4, 0x7FFFFF		# get fraction
	srl	t4, t4, 31			# get sign
	bne	t5, SEXP_INF, 1f		# is it a signaling NAN?
	and	v0, t6, SSIGNAL_NAN
	bne	v0, zero, invalid_s
1:
	/* fall through to get FS */
#else
	srl	a3, a0, 17 - 2			# get FT field (even regs only)
	and	a3, a3, 0xF << 2		# mask FT field
	lw	a3, get_ft_s_tbl(a3)		# switch on register number
	j	a3

	.rdata
get_ft_s_tbl:
	.word	get_ft_s_f0
	.word	get_ft_s_f2
	.word	get_ft_s_f4
	.word	get_ft_s_f6
	.word	get_ft_s_f8
	.word	get_ft_s_f10
	.word	get_ft_s_f12
	.word	get_ft_s_f14
	.word	get_ft_s_f16
	.word	get_ft_s_f18
	.word	get_ft_s_f20
	.word	get_ft_s_f22
	.word	get_ft_s_f24
	.word	get_ft_s_f26
	.word	get_ft_s_f28
	.word	get_ft_s_f30
	.text

get_ft_s_f0:
	mfc1	t4, $f0
	b	get_ft_s_done
get_ft_s_f2:
	mfc1	t4, $f2
	b	get_ft_s_done
get_ft_s_f4:
	mfc1	t4, $f4
	b	get_ft_s_done
get_ft_s_f6:
	mfc1	t4, $f6
	b	get_ft_s_done
get_ft_s_f8:
	mfc1	t4, $f8
	b	get_ft_s_done
get_ft_s_f10:
	mfc1	t4, $f10
	b	get_ft_s_done
get_ft_s_f12:
	mfc1	t4, $f12
	b	get_ft_s_done
get_ft_s_f14:
	mfc1	t4, $f14
	b	get_ft_s_done
get_ft_s_f16:
	mfc1	t4, $f16
	b	get_ft_s_done
get_ft_s_f18:
	mfc1	t4, $f18
	b	get_ft_s_done
get_ft_s_f20:
	mfc1	t4, $f20
	b	get_ft_s_done
get_ft_s_f22:
	mfc1	t4, $f22
	b	get_ft_s_done
get_ft_s_f24:
	mfc1	t4, $f24
	b	get_ft_s_done
get_ft_s_f26:
	mfc1	t4, $f26
	b	get_ft_s_done
get_ft_s_f28:
	mfc1	t4, $f28
	b	get_ft_s_done
get_ft_s_f30:
	mfc1	t4, $f30
get_ft_s_done:
	srl	t5, t4, 23			# get exponent
	and	t5, t5, 0xFF
	and	t6, t4, 0x7FFFFF		# get fraction
	srl	t4, t4, 31			# get sign
	bne	t5, SEXP_INF, 1f		# is it a signaling NAN?
	and	v0, t6, SSIGNAL_NAN
	bne	v0, zero, invalid_s
1:
	/* fall through to get FS */
#endif

/*----------------------------------------------------------------------------
 * get_fs_s --
 *
 *	Read (single precision) the FS register (bits 15-11) and
 *	break up into fields.
 *	This is an internal routine used by MachEmulateFP only.
 *
 * Results:
 *	t0	contains the sign
 *	t1	contains the (biased) exponent
 *	t2	contains the fraction
 *
 *----------------------------------------------------------------------------
 */
XLEAF(get_fs_s)
#ifdef SOFTFLOAT
	srl	t0, a0, 11-2
	lw	t1, _C_LABEL(curpcb)		# get pcb of current process
	andi	t0, t0, 0x0078			# Even regs only
	addu	t1, t1, t0

	lw	t0, U_PCB_FPREGS+FRAME_FP0(t1)

	srl	t1, t0, 23			# get exponent
	and	t1, t1, 0xFF
	and	t2, t0, 0x7FFFFF		# get fraction
	srl	t0, t0, 31			# get sign
	bne	t1, SEXP_INF, 1f		# is it a signaling NAN?
	and	v0, t2, SSIGNAL_NAN
	bne	v0, zero, invalid_s
1:
	j	ra
#else
	srl	a3, a0, 12 - 2			# get FS field (even regs only)
	and	a3, a3, 0xF << 2		# mask FS field
	lw	a3, get_fs_s_tbl(a3)		# switch on register number
	j	a3

	.rdata
get_fs_s_tbl:
	.word	get_fs_s_f0
	.word	get_fs_s_f2
	.word	get_fs_s_f4
	.word	get_fs_s_f6
	.word	get_fs_s_f8
	.word	get_fs_s_f10
	.word	get_fs_s_f12
	.word	get_fs_s_f14
	.word	get_fs_s_f16
	.word	get_fs_s_f18
	.word	get_fs_s_f20
	.word	get_fs_s_f22
	.word	get_fs_s_f24
	.word	get_fs_s_f26
	.word	get_fs_s_f28
	.word	get_fs_s_f30
	.text

get_fs_s_f0:
	mfc1	t0, $f0
	b	get_fs_s_done
get_fs_s_f2:
	mfc1	t0, $f2
	b	get_fs_s_done
get_fs_s_f4:
	mfc1	t0, $f4
	b	get_fs_s_done
get_fs_s_f6:
	mfc1	t0, $f6
	b	get_fs_s_done
get_fs_s_f8:
	mfc1	t0, $f8
	b	get_fs_s_done
get_fs_s_f10:
	mfc1	t0, $f10
	b	get_fs_s_done
get_fs_s_f12:
	mfc1	t0, $f12
	b	get_fs_s_done
get_fs_s_f14:
	mfc1	t0, $f14
	b	get_fs_s_done
get_fs_s_f16:
	mfc1	t0, $f16
	b	get_fs_s_done
get_fs_s_f18:
	mfc1	t0, $f18
	b	get_fs_s_done
get_fs_s_f20:
	mfc1	t0, $f20
	b	get_fs_s_done
get_fs_s_f22:
	mfc1	t0, $f22
	b	get_fs_s_done
get_fs_s_f24:
	mfc1	t0, $f24
	b	get_fs_s_done
get_fs_s_f26:
	mfc1	t0, $f26
	b	get_fs_s_done
get_fs_s_f28:
	mfc1	t0, $f28
	b	get_fs_s_done
get_fs_s_f30:
	mfc1	t0, $f30
get_fs_s_done:
	srl	t1, t0, 23			# get exponent
	and	t1, t1, 0xFF
	and	t2, t0, 0x7FFFFF		# get fraction
	srl	t0, t0, 31			# get sign
	bne	t1, SEXP_INF, 1f		# is it a signaling NAN?
	and	v0, t2, SSIGNAL_NAN
	bne	v0, zero, invalid_s
1:
	j	ra
#endif
END(get_ft_fs_s)

/*----------------------------------------------------------------------------
 * get_ft_fs_d --
 *
 *	Read (double precision) the FT register (bits 20-16) and
 *	the FS register (bits 15-11) and break up into fields.
 *	This is an internal routine used by MachEmulateFP only.
 *
 * Results:
 *	t0	contains the FS sign
 *	t1	contains the FS (biased) exponent
 *	t2	contains the FS fraction
 *	t3	contains the FS remaining fraction
 *	t4	contains the FT sign
 *	t5	contains the FT (biased) exponent
 *	t6	contains the FT fraction
 *	t7	contains the FT remaining fraction
 *
 *----------------------------------------------------------------------------
 */
LEAF(get_ft_fs_d)
#ifdef SOFTFLOAT
	srl	t7, a0, 16-2
	lw	t4, _C_LABEL(curpcb)		# get pcb of current process
	andi	t7, t7, 0x0078			# Even regs only
	addu	t4, t4, t7

	lw	t7, U_PCB_FPREGS+FRAME_FP0(t4)
	lw	t4, U_PCB_FPREGS+FRAME_FP0+4(t4)

	srl	t5, t4, 20			# get exponent
	and	t5, t5, 0x7FF
	and	t6, t4, 0xFFFFF			# get fraction
	srl	t4, t4, 31			# get sign
	bne	t5, DEXP_INF, 1f		# is it a signaling NAN?
	and	v0, t6, DSIGNAL_NAN
	bne	v0, zero, invalid_d
1:
	/* fall through to get FS */
#else
	srl	a3, a0, 17 - 2			# get FT field (even regs only)
	and	a3, a3, 0xF << 2		# mask FT field
	lw	a3, get_ft_d_tbl(a3)		# switch on register number
	j	a3

	.rdata
get_ft_d_tbl:
	.word	get_ft_d_f0
	.word	get_ft_d_f2
	.word	get_ft_d_f4
	.word	get_ft_d_f6
	.word	get_ft_d_f8
	.word	get_ft_d_f10
	.word	get_ft_d_f12
	.word	get_ft_d_f14
	.word	get_ft_d_f16
	.word	get_ft_d_f18
	.word	get_ft_d_f20
	.word	get_ft_d_f22
	.word	get_ft_d_f24
	.word	get_ft_d_f26
	.word	get_ft_d_f28
	.word	get_ft_d_f30
	.text

get_ft_d_f0:
	mfc1	t7, $f0
	mfc1	t4, $f1
	b	get_ft_d_done
get_ft_d_f2:
	mfc1	t7, $f2
	mfc1	t4, $f3
	b	get_ft_d_done
get_ft_d_f4:
	mfc1	t7, $f4
	mfc1	t4, $f5
	b	get_ft_d_done
get_ft_d_f6:
	mfc1	t7, $f6
	mfc1	t4, $f7
	b	get_ft_d_done
get_ft_d_f8:
	mfc1	t7, $f8
	mfc1	t4, $f9
	b	get_ft_d_done
get_ft_d_f10:
	mfc1	t7, $f10
	mfc1	t4, $f11
	b	get_ft_d_done
get_ft_d_f12:
	mfc1	t7, $f12
	mfc1	t4, $f13
	b	get_ft_d_done
get_ft_d_f14:
	mfc1	t7, $f14
	mfc1	t4, $f15
	b	get_ft_d_done
get_ft_d_f16:
	mfc1	t7, $f16
	mfc1	t4, $f17
	b	get_ft_d_done
get_ft_d_f18:
	mfc1	t7, $f18
	mfc1	t4, $f19
	b	get_ft_d_done
get_ft_d_f20:
	mfc1	t7, $f20
	mfc1	t4, $f21
	b	get_ft_d_done
get_ft_d_f22:
	mfc1	t7, $f22
	mfc1	t4, $f23
	b	get_ft_d_done
get_ft_d_f24:
	mfc1	t7, $f24
	mfc1	t4, $f25
	b	get_ft_d_done
get_ft_d_f26:
	mfc1	t7, $f26
	mfc1	t4, $f27
	b	get_ft_d_done
get_ft_d_f28:
	mfc1	t7, $f28
	mfc1	t4, $f29
	b	get_ft_d_done
get_ft_d_f30:
	mfc1	t7, $f30
	mfc1	t4, $f31
get_ft_d_done:
	srl	t5, t4, 20			# get exponent
	and	t5, t5, 0x7FF
	and	t6, t4, 0xFFFFF			# get fraction
	srl	t4, t4, 31			# get sign
	bne	t5, DEXP_INF, 1f		# is it a signaling NAN?
	and	v0, t6, DSIGNAL_NAN
	bne	v0, zero, invalid_d
1:
	/* fall through to get FS */
#endif

/*----------------------------------------------------------------------------
 * get_fs_d --
 *
 *	Read (double precision) the FS register (bits 15-11) and
 *	break up into fields.
 *	This is an internal routine used by MachEmulateFP only.
 *
 * Results:
 *	t0	contains the sign
 *	t1	contains the (biased) exponent
 *	t2	contains the fraction
 *	t3	contains the remaining fraction
 *
 *----------------------------------------------------------------------------
 */
XLEAF(get_fs_d)
#ifdef SOFTFLOAT
	srl	t3, a0, 11-2
	lw	t0, _C_LABEL(curpcb)		# get pcb of current process
	andi	t3, t3, 0x0078			# Even regs only
	addu	t0, t0, t3

	lw	t3, U_PCB_FPREGS+FRAME_FP0(t0)
	lw	t0, U_PCB_FPREGS+FRAME_FP0+4(t0)

	srl	t1, t0, 20			# get exponent
	and	t1, t1, 0x7FF
	and	t2, t0, 0xFFFFF			# get fraction
	srl	t0, t0, 31			# get sign
	bne	t1, DEXP_INF, 1f		# is it a signaling NAN?
	and	v0, t2, DSIGNAL_NAN
	bne	v0, zero, invalid_d
1:
	j	ra
#else
	srl	a3, a0, 12 - 2			# get FS field (even regs only)
	and	a3, a3, 0xF << 2		# mask FS field
	lw	a3, get_fs_d_tbl(a3)		# switch on register number
	j	a3

	.rdata
get_fs_d_tbl:
	.word	get_fs_d_f0
	.word	get_fs_d_f2
	.word	get_fs_d_f4
	.word	get_fs_d_f6
	.word	get_fs_d_f8
	.word	get_fs_d_f10
	.word	get_fs_d_f12
	.word	get_fs_d_f14
	.word	get_fs_d_f16
	.word	get_fs_d_f18
	.word	get_fs_d_f20
	.word	get_fs_d_f22
	.word	get_fs_d_f24
	.word	get_fs_d_f26
	.word	get_fs_d_f28
	.word	get_fs_d_f30
	.text

get_fs_d_f0:
	mfc1	t3, $f0
	mfc1	t0, $f1
	b	get_fs_d_done
get_fs_d_f2:
	mfc1	t3, $f2
	mfc1	t0, $f3
	b	get_fs_d_done
get_fs_d_f4:
	mfc1	t3, $f4
	mfc1	t0, $f5
	b	get_fs_d_done
get_fs_d_f6:
	mfc1	t3, $f6
	mfc1	t0, $f7
	b	get_fs_d_done
get_fs_d_f8:
	mfc1	t3, $f8
	mfc1	t0, $f9
	b	get_fs_d_done
get_fs_d_f10:
	mfc1	t3, $f10
	mfc1	t0, $f11
	b	get_fs_d_done
get_fs_d_f12:
	mfc1	t3, $f12
	mfc1	t0, $f13
	b	get_fs_d_done
get_fs_d_f14:
	mfc1	t3, $f14
	mfc1	t0, $f15
	b	get_fs_d_done
get_fs_d_f16:
	mfc1	t3, $f16
	mfc1	t0, $f17
	b	get_fs_d_done
get_fs_d_f18:
	mfc1	t3, $f18
	mfc1	t0, $f19
	b	get_fs_d_done
get_fs_d_f20:
	mfc1	t3, $f20
	mfc1	t0, $f21
	b	get_fs_d_done
get_fs_d_f22:
	mfc1	t3, $f22
	mfc1	t0, $f23
	b	get_fs_d_done
get_fs_d_f24:
	mfc1	t3, $f24
	mfc1	t0, $f25
	b	get_fs_d_done
get_fs_d_f26:
	mfc1	t3, $f26
	mfc1	t0, $f27
	b	get_fs_d_done
get_fs_d_f28:
	mfc1	t3, $f28
	mfc1	t0, $f29
	b	get_fs_d_done
get_fs_d_f30:
	mfc1	t3, $f30
	mfc1	t0, $f31
get_fs_d_done:
	srl	t1, t0, 20			# get exponent
	and	t1, t1, 0x7FF
	and	t2, t0, 0xFFFFF			# get fraction
	srl	t0, t0, 31			# get sign
	bne	t1, DEXP_INF, 1f		# is it a signaling NAN?
	and	v0, t2, DSIGNAL_NAN
	bne	v0, zero, invalid_d
1:
	j	ra
#endif
END(get_ft_fs_d)

/*----------------------------------------------------------------------------
 * get_cmp_s --
 *
 *	Read (single precision) the FS register (bits 15-11) and
 *	the FT register (bits 20-16) and break up into fields.
 *	This is an internal routine used by MachEmulateFP only.
 *
 * Results:
 *	t0	contains the sign
 *	t1	contains the (biased) exponent
 *	t2	contains the fraction
 *	t4	contains the sign
 *	t5	contains the (biased) exponent
 *	t6	contains the fraction
 *
 *----------------------------------------------------------------------------
 */
LEAF(get_cmp_s)
#ifdef SOFTFLOAT
	srl	t1, a0, 11-2
	lw	t6, _C_LABEL(curpcb)		# get pcb of current process
	andi	t1, t1, 0x0078			# Even regs only
	addu	t0, t6, t1

	lw	t0, U_PCB_FPREGS+FRAME_FP0(t0)

	srl	t1, t0, 23			# get exponent
	and	t1, t1, 0xFF
	and	t2, t0, 0x7FFFFF		# get fraction
	srl	t0, t0, 31			# get sign

	srl	t4, a0, 16-2
	andi	t4, t4, 0x0078			# Even regs only
	addu	t6, t6, t4

	lw	t4, U_PCB_FPREGS+FRAME_FP0(t6)

	srl	t5, t4, 23			# get exponent
	and	t5, t5, 0xFF
	and	t6, t4, 0x7FFFFF		# get fraction
	srl	t4, t4, 31			# get sign
	j	ra
#else
	srl	a3, a0, 12 - 2			# get FS field (even regs only)
	and	a3, a3, 0xF << 2		# mask FS field
	lw	a3, cmp_fs_s_tbl(a3)		# switch on register number
	j	a3

	.rdata
cmp_fs_s_tbl:
	.word	cmp_fs_s_f0
	.word	cmp_fs_s_f2
	.word	cmp_fs_s_f4
	.word	cmp_fs_s_f6
	.word	cmp_fs_s_f8
	.word	cmp_fs_s_f10
	.word	cmp_fs_s_f12
	.word	cmp_fs_s_f14
	.word	cmp_fs_s_f16
	.word	cmp_fs_s_f18
	.word	cmp_fs_s_f20
	.word	cmp_fs_s_f22
	.word	cmp_fs_s_f24
	.word	cmp_fs_s_f26
	.word	cmp_fs_s_f28
	.word	cmp_fs_s_f30
	.text

cmp_fs_s_f0:
	mfc1	t0, $f0
	b	cmp_fs_s_done
cmp_fs_s_f2:
	mfc1	t0, $f2
	b	cmp_fs_s_done
cmp_fs_s_f4:
	mfc1	t0, $f4
	b	cmp_fs_s_done
cmp_fs_s_f6:
	mfc1	t0, $f6
	b	cmp_fs_s_done
cmp_fs_s_f8:
	mfc1	t0, $f8
	b	cmp_fs_s_done
cmp_fs_s_f10:
	mfc1	t0, $f10
	b	cmp_fs_s_done
cmp_fs_s_f12:
	mfc1	t0, $f12
	b	cmp_fs_s_done
cmp_fs_s_f14:
	mfc1	t0, $f14
	b	cmp_fs_s_done
cmp_fs_s_f16:
	mfc1	t0, $f16
	b	cmp_fs_s_done
cmp_fs_s_f18:
	mfc1	t0, $f18
	b	cmp_fs_s_done
cmp_fs_s_f20:
	mfc1	t0, $f20
	b	cmp_fs_s_done
cmp_fs_s_f22:
	mfc1	t0, $f22
	b	cmp_fs_s_done
cmp_fs_s_f24:
	mfc1	t0, $f24
	b	cmp_fs_s_done
cmp_fs_s_f26:
	mfc1	t0, $f26
	b	cmp_fs_s_done
cmp_fs_s_f28:
	mfc1	t0, $f28
	b	cmp_fs_s_done
cmp_fs_s_f30:
	mfc1	t0, $f30
cmp_fs_s_done:
	srl	t1, t0, 23			# get exponent
	and	t1, t1, 0xFF
	and	t2, t0, 0x7FFFFF		# get fraction
	srl	t0, t0, 31			# get sign

	srl	a3, a0, 17 - 2			# get FT field (even regs only)
	and	a3, a3, 0xF << 2		# mask FT field
	lw	a3, cmp_ft_s_tbl(a3)		# switch on register number
	j	a3

	.rdata
cmp_ft_s_tbl:
	.word	cmp_ft_s_f0
	.word	cmp_ft_s_f2
	.word	cmp_ft_s_f4
	.word	cmp_ft_s_f6
	.word	cmp_ft_s_f8
	.word	cmp_ft_s_f10
	.word	cmp_ft_s_f12
	.word	cmp_ft_s_f14
	.word	cmp_ft_s_f16
	.word	cmp_ft_s_f18
	.word	cmp_ft_s_f20
	.word	cmp_ft_s_f22
	.word	cmp_ft_s_f24
	.word	cmp_ft_s_f26
	.word	cmp_ft_s_f28
	.word	cmp_ft_s_f30
	.text

cmp_ft_s_f0:
	mfc1	t4, $f0
	b	cmp_ft_s_done
cmp_ft_s_f2:
	mfc1	t4, $f2
	b	cmp_ft_s_done
cmp_ft_s_f4:
	mfc1	t4, $f4
	b	cmp_ft_s_done
cmp_ft_s_f6:
	mfc1	t4, $f6
	b	cmp_ft_s_done
cmp_ft_s_f8:
	mfc1	t4, $f8
	b	cmp_ft_s_done
cmp_ft_s_f10:
	mfc1	t4, $f10
	b	cmp_ft_s_done
cmp_ft_s_f12:
	mfc1	t4, $f12
	b	cmp_ft_s_done
cmp_ft_s_f14:
	mfc1	t4, $f14
	b	cmp_ft_s_done
cmp_ft_s_f16:
	mfc1	t4, $f16
	b	cmp_ft_s_done
cmp_ft_s_f18:
	mfc1	t4, $f18
	b	cmp_ft_s_done
cmp_ft_s_f20:
	mfc1	t4, $f20
	b	cmp_ft_s_done
cmp_ft_s_f22:
	mfc1	t4, $f22
	b	cmp_ft_s_done
cmp_ft_s_f24:
	mfc1	t4, $f24
	b	cmp_ft_s_done
cmp_ft_s_f26:
	mfc1	t4, $f26
	b	cmp_ft_s_done
cmp_ft_s_f28:
	mfc1	t4, $f28
	b	cmp_ft_s_done
cmp_ft_s_f30:
	mfc1	t4, $f30
cmp_ft_s_done:
	srl	t5, t4, 23			# get exponent
	and	t5, t5, 0xFF
	and	t6, t4, 0x7FFFFF		# get fraction
	srl	t4, t4, 31			# get sign
	j	ra
#endif
END(get_cmp_s)

/*----------------------------------------------------------------------------
 * get_cmp_d --
 *
 *	Read (double precision) the FS register (bits 15-11) and
 *	the FT register (bits 20-16) and break up into fields.
 *	This is an internal routine used by MachEmulateFP only.
 *
 * Results:
 *	t0	contains the sign
 *	t1	contains the (biased) exponent
 *	t2	contains the fraction
 *	t3	contains the remaining fraction
 *	t4	contains the sign
 *	t5	contains the (biased) exponent
 *	t6	contains the fraction
 *	t7	contains the remaining fraction
 *
 *----------------------------------------------------------------------------
 */
LEAF(get_cmp_d)
#ifdef SOFTFLOAT
	srl	t1, a0, 11-2
	lw	t6, _C_LABEL(curpcb)		# get pcb of current process
	andi	t1, t1, 0x0078			# Even regs only
	addu	t0, t6, t1

	lw	t3, U_PCB_FPREGS+FRAME_FP0(t0)
	lw	t0, U_PCB_FPREGS+FRAME_FP0+4(t0)

	srl	t1, t0, 20			# get exponent
	and	t1, t1, 0x7FF
	and	t2, t0, 0xFFFFF			# get fraction
	srl	t0, t0, 31			# get sign

	srl	t4, a0, 16-2
	andi	t4, t4, 0x0078			# Even regs only
	addu	t6, t6, t4

	lw	t7, U_PCB_FPREGS+FRAME_FP0(t6)
	lw	t4, U_PCB_FPREGS+FRAME_FP0+4(t6)

	srl	t5, t4, 20			# get exponent
	and	t5, t5, 0x7FF
	and	t6, t4, 0xFFFFF			# get fraction
	srl	t4, t4, 31			# get sign
	j	ra
#else
	srl	a3, a0, 12 - 2			# get FS field (even regs only)
	and	a3, a3, 0xF << 2		# mask FS field
	lw	a3, cmp_fs_d_tbl(a3)		# switch on register number
	j	a3

	.rdata
cmp_fs_d_tbl:
	.word	cmp_fs_d_f0
	.word	cmp_fs_d_f2
	.word	cmp_fs_d_f4
	.word	cmp_fs_d_f6
	.word	cmp_fs_d_f8
	.word	cmp_fs_d_f10
	.word	cmp_fs_d_f12
	.word	cmp_fs_d_f14
	.word	cmp_fs_d_f16
	.word	cmp_fs_d_f18
	.word	cmp_fs_d_f20
	.word	cmp_fs_d_f22
	.word	cmp_fs_d_f24
	.word	cmp_fs_d_f26
	.word	cmp_fs_d_f28
	.word	cmp_fs_d_f30
	.text

cmp_fs_d_f0:
	mfc1	t3, $f0
	mfc1	t0, $f1
	b	cmp_fs_d_done
cmp_fs_d_f2:
	mfc1	t3, $f2
	mfc1	t0, $f3
	b	cmp_fs_d_done
cmp_fs_d_f4:
	mfc1	t3, $f4
	mfc1	t0, $f5
	b	cmp_fs_d_done
cmp_fs_d_f6:
	mfc1	t3, $f6
	mfc1	t0, $f7
	b	cmp_fs_d_done
cmp_fs_d_f8:
	mfc1	t3, $f8
	mfc1	t0, $f9
	b	cmp_fs_d_done
cmp_fs_d_f10:
	mfc1	t3, $f10
	mfc1	t0, $f11
	b	cmp_fs_d_done
cmp_fs_d_f12:
	mfc1	t3, $f12
	mfc1	t0, $f13
	b	cmp_fs_d_done
cmp_fs_d_f14:
	mfc1	t3, $f14
	mfc1	t0, $f15
	b	cmp_fs_d_done
cmp_fs_d_f16:
	mfc1	t3, $f16
	mfc1	t0, $f17
	b	cmp_fs_d_done
cmp_fs_d_f18:
	mfc1	t3, $f18
	mfc1	t0, $f19
	b	cmp_fs_d_done
cmp_fs_d_f20:
	mfc1	t3, $f20
	mfc1	t0, $f21
	b	cmp_fs_d_done
cmp_fs_d_f22:
	mfc1	t3, $f22
	mfc1	t0, $f23
	b	cmp_fs_d_done
cmp_fs_d_f24:
	mfc1	t3, $f24
	mfc1	t0, $f25
	b	cmp_fs_d_done
cmp_fs_d_f26:
	mfc1	t3, $f26
	mfc1	t0, $f27
	b	cmp_fs_d_done
cmp_fs_d_f28:
	mfc1	t3, $f28
	mfc1	t0, $f29
	b	cmp_fs_d_done
cmp_fs_d_f30:
	mfc1	t3, $f30
	mfc1	t0, $f31
cmp_fs_d_done:
	srl	t1, t0, 20			# get exponent
	and	t1, t1, 0x7FF
	and	t2, t0, 0xFFFFF			# get fraction
	srl	t0, t0, 31			# get sign

	srl	a3, a0, 17 - 2			# get FT field (even regs only)
	and	a3, a3, 0xF << 2		# mask FT field
	lw	a3, cmp_ft_d_tbl(a3)		# switch on register number
	j	a3

	.rdata
cmp_ft_d_tbl:
	.word	cmp_ft_d_f0
	.word	cmp_ft_d_f2
	.word	cmp_ft_d_f4
	.word	cmp_ft_d_f6
	.word	cmp_ft_d_f8
	.word	cmp_ft_d_f10
	.word	cmp_ft_d_f12
	.word	cmp_ft_d_f14
	.word	cmp_ft_d_f16
	.word	cmp_ft_d_f18
	.word	cmp_ft_d_f20
	.word	cmp_ft_d_f22
	.word	cmp_ft_d_f24
	.word	cmp_ft_d_f26
	.word	cmp_ft_d_f28
	.word	cmp_ft_d_f30
	.text

cmp_ft_d_f0:
	mfc1	t7, $f0
	mfc1	t4, $f1
	b	cmp_ft_d_done
cmp_ft_d_f2:
	mfc1	t7, $f2
	mfc1	t4, $f3
	b	cmp_ft_d_done
cmp_ft_d_f4:
	mfc1	t7, $f4
	mfc1	t4, $f5
	b	cmp_ft_d_done
cmp_ft_d_f6:
	mfc1	t7, $f6
	mfc1	t4, $f7
	b	cmp_ft_d_done
cmp_ft_d_f8:
	mfc1	t7, $f8
	mfc1	t4, $f9
	b	cmp_ft_d_done
cmp_ft_d_f10:
	mfc1	t7, $f10
	mfc1	t4, $f11
	b	cmp_ft_d_done
cmp_ft_d_f12:
	mfc1	t7, $f12
	mfc1	t4, $f13
	b	cmp_ft_d_done
cmp_ft_d_f14:
	mfc1	t7, $f14
	mfc1	t4, $f15
	b	cmp_ft_d_done
cmp_ft_d_f16:
	mfc1	t7, $f16
	mfc1	t4, $f17
	b	cmp_ft_d_done
cmp_ft_d_f18:
	mfc1	t7, $f18
	mfc1	t4, $f19
	b	cmp_ft_d_done
cmp_ft_d_f20:
	mfc1	t7, $f20
	mfc1	t4, $f21
	b	cmp_ft_d_done
cmp_ft_d_f22:
	mfc1	t7, $f22
	mfc1	t4, $f23
	b	cmp_ft_d_done
cmp_ft_d_f24:
	mfc1	t7, $f24
	mfc1	t4, $f25
	b	cmp_ft_d_done
cmp_ft_d_f26:
	mfc1	t7, $f26
	mfc1	t4, $f27
	b	cmp_ft_d_done
cmp_ft_d_f28:
	mfc1	t7, $f28
	mfc1	t4, $f29
	b	cmp_ft_d_done
cmp_ft_d_f30:
	mfc1	t7, $f30
	mfc1	t4, $f31
cmp_ft_d_done:
	srl	t5, t4, 20			# get exponent
	and	t5, t5, 0x7FF
	and	t6, t4, 0xFFFFF			# get fraction
	srl	t4, t4, 31			# get sign
	j	ra
#endif
END(get_cmp_d)

/*----------------------------------------------------------------------------
 * set_fd_s --
 *
 *	Write (single precision) the FD register (bits 10-6).
 *	This is an internal routine used by MachEmulateFP only.
 *
 * Arguments:
 *	a0	contains the FP instruction
 *	t0	contains the sign
 *	t1	contains the (biased) exponent
 *	t2	contains the fraction
 *
 * set_fd_word --
 *
 *	Write (integer) the FD register (bits 10-6).
 *	This is an internal routine used by MachEmulateFP only.
 *
 * Arguments:
 *	a0	contains the FP instruction
 *	t2	contains the integer
 *
 *----------------------------------------------------------------------------
 */
LEAF(set_fd_s)
	sll	t0, t0, 31			# position sign
	sll	t1, t1, 23			# position exponent
	or	t2, t2, t0
	or	t2, t2, t1
XLEAF(set_fd_word)
#ifdef SOFTFLOAT
	srl	t1, a0, 6-2
	lw	t0, _C_LABEL(curpcb)		# get pcb of current process
	andi	t1, t1, 0x0078			# Even regs only
	addu	t0, t0, t1

	sw	t2, U_PCB_FPREGS+FRAME_FP0(t0)
	j	ra
#else
	srl	a3, a0, 7 - 2			# get FD field (even regs only)
	and	a3, a3, 0xF << 2		# mask FT field
	lw	a3, set_fd_s_tbl(a3)		# switch on register number
	j	a3

	.rdata
set_fd_s_tbl:
	.word	set_fd_s_f0
	.word	set_fd_s_f2
	.word	set_fd_s_f4
	.word	set_fd_s_f6
	.word	set_fd_s_f8
	.word	set_fd_s_f10
	.word	set_fd_s_f12
	.word	set_fd_s_f14
	.word	set_fd_s_f16
	.word	set_fd_s_f18
	.word	set_fd_s_f20
	.word	set_fd_s_f22
	.word	set_fd_s_f24
	.word	set_fd_s_f26
	.word	set_fd_s_f28
	.word	set_fd_s_f30
	.text

set_fd_s_f0:
	mtc1	t2, $f0
	j	ra
set_fd_s_f2:
	mtc1	t2, $f2
	j	ra
set_fd_s_f4:
	mtc1	t2, $f4
	j	ra
set_fd_s_f6:
	mtc1	t2, $f6
	j	ra
set_fd_s_f8:
	mtc1	t2, $f8
	j	ra
set_fd_s_f10:
	mtc1	t2, $f10
	j	ra
set_fd_s_f12:
	mtc1	t2, $f12
	j	ra
set_fd_s_f14:
	mtc1	t2, $f14
	j	ra
set_fd_s_f16:
	mtc1	t2, $f16
	j	ra
set_fd_s_f18:
	mtc1	t2, $f18
	j	ra
set_fd_s_f20:
	mtc1	t2, $f20
	j	ra
set_fd_s_f22:
	mtc1	t2, $f22
	j	ra
set_fd_s_f24:
	mtc1	t2, $f24
	j	ra
set_fd_s_f26:
	mtc1	t2, $f26
	j	ra
set_fd_s_f28:
	mtc1	t2, $f28
	j	ra
set_fd_s_f30:
	mtc1	t2, $f30
	j	ra
#endif
END(set_fd_s)

/*----------------------------------------------------------------------------
 * set_fd_d --
 *
 *	Write (double precision) the FT register (bits 10-6).
 *	This is an internal routine used by MachEmulateFP only.
 *
 * Arguments:
 *	a0	contains the FP instruction
 *	t0	contains the sign
 *	t1	contains the (biased) exponent
 *	t2	contains the fraction
 *	t3	contains the remaining fraction
 *
 *----------------------------------------------------------------------------
 */
LEAF(set_fd_d)
#ifdef SOFTFLOAT
	sll	t0, t0, 31			# set sign
	sll	t1, t1, 20			# set exponent
	or	t0, t0, t1
	or	t0, t0, t2			# set fraction

	srl	t1, a0, 6-2
	lw	t2, _C_LABEL(curpcb)		# get pcb of current process
	andi	t1, t1, 0x0078			# Even regs only
	addu	t2, t2, t1

	sw	t3, U_PCB_FPREGS+FRAME_FP0(t2)
	sw	t0, U_PCB_FPREGS+FRAME_FP0+4(t2)
	j	ra
#else
	sll	t0, t0, 31			# set sign
	sll	t1, t1, 20			# set exponent
	or	t0, t0, t1
	or	t0, t0, t2			# set fraction
	srl	a3, a0, 7 - 2			# get FD field (even regs only)
	and	a3, a3, 0xF << 2		# mask FD field
	lw	a3, set_fd_d_tbl(a3)		# switch on register number
	j	a3

	.rdata
set_fd_d_tbl:
	.word	set_fd_d_f0
	.word	set_fd_d_f2
	.word	set_fd_d_f4
	.word	set_fd_d_f6
	.word	set_fd_d_f8
	.word	set_fd_d_f10
	.word	set_fd_d_f12
	.word	set_fd_d_f14
	.word	set_fd_d_f16
	.word	set_fd_d_f18
	.word	set_fd_d_f20
	.word	set_fd_d_f22
	.word	set_fd_d_f24
	.word	set_fd_d_f26
	.word	set_fd_d_f28
	.word	set_fd_d_f30
	.text

set_fd_d_f0:
	mtc1	t3, $f0
	mtc1	t0, $f1
	j	ra
set_fd_d_f2:
	mtc1	t3, $f2
	mtc1	t0, $f3
	j	ra
set_fd_d_f4:
	mtc1	t3, $f4
	mtc1	t0, $f5
	j	ra
set_fd_d_f6:
	mtc1	t3, $f6
	mtc1	t0, $f7
	j	ra
set_fd_d_f8:
	mtc1	t3, $f8
	mtc1	t0, $f9
	j	ra
set_fd_d_f10:
	mtc1	t3, $f10
	mtc1	t0, $f11
	j	ra
set_fd_d_f12:
	mtc1	t3, $f12
	mtc1	t0, $f13
	j	ra
set_fd_d_f14:
	mtc1	t3, $f14
	mtc1	t0, $f15
	j	ra
set_fd_d_f16:
	mtc1	t3, $f16
	mtc1	t0, $f17
	j	ra
set_fd_d_f18:
	mtc1	t3, $f18
	mtc1	t0, $f19
	j	ra
set_fd_d_f20:
	mtc1	t3, $f20
	mtc1	t0, $f21
	j	ra
set_fd_d_f22:
	mtc1	t3, $f22
	mtc1	t0, $f23
	j	ra
set_fd_d_f24:
	mtc1	t3, $f24
	mtc1	t0, $f25
	j	ra
set_fd_d_f26:
	mtc1	t3, $f26
	mtc1	t0, $f27
	j	ra
set_fd_d_f28:
	mtc1	t3, $f28
	mtc1	t0, $f29
	j	ra
set_fd_d_f30:
	mtc1	t3, $f30
	mtc1	t0, $f31
	j	ra
#endif
END(set_fd_d)

/*----------------------------------------------------------------------------
 * renorm_fs_s --
 *
 * Results:
 *	t1	unbiased exponent
 *	t2	normalized fraction
 *
 *----------------------------------------------------------------------------
 */
LEAF(renorm_fs_s)
/*
 * Find out how many leading zero bits are in t2 and put in t9.
 */
	move	v0, t2
	move	t9, zero
	srl	v1, v0, 16
	bne	v1, zero, 1f
	addu	t9, 16
	sll	v0, 16
1:
	srl	v1, v0, 24
	bne	v1, zero, 1f
	addu	t9, 8
	sll	v0, 8
1:
	srl	v1, v0, 28
	bne	v1, zero, 1f
	addu	t9, 4
	sll	v0, 4
1:
	srl	v1, v0, 30
	bne	v1, zero, 1f
	addu	t9, 2
	sll	v0, 2
1:
	srl	v1, v0, 31
	bne	v1, zero, 1f
	addu	t9, 1
/*
 * Now shift t2 the correct number of bits.
 */
1:
	subu	t9, t9, SLEAD_ZEROS	# dont count normal leading zeros
	li	t1, SEXP_MIN
	subu	t1, t1, t9		# adjust exponent
	sll	t2, t2, t9
	j	ra
END(renorm_fs_s)

/*----------------------------------------------------------------------------
 * renorm_fs_d --
 *
 * Results:
 *	t1	unbiased exponent
 *	t2,t3	normalized fraction
 *
 *----------------------------------------------------------------------------
 */
LEAF(renorm_fs_d)
/*
 * Find out how many leading zero bits are in t2,t3 and put in t9.
 */
	move	v0, t2
	move	t9, zero
	bne	t2, zero, 1f
	move	v0, t3
	addu	t9, 32
1:
	srl	v1, v0, 16
	bne	v1, zero, 1f
	addu	t9, 16
	sll	v0, 16
1:
	srl	v1, v0, 24
	bne	v1, zero, 1f
	addu	t9, 8
	sll	v0, 8
1:
	srl	v1, v0, 28
	bne	v1, zero, 1f
	addu	t9, 4
	sll	v0, 4
1:
	srl	v1, v0, 30
	bne	v1, zero, 1f
	addu	t9, 2
	sll	v0, 2
1:
	srl	v1, v0, 31
	bne	v1, zero, 1f
	addu	t9, 1
/*
 * Now shift t2,t3 the correct number of bits.
 */
1:
	subu	t9, t9, DLEAD_ZEROS	# dont count normal leading zeros
	li	t1, DEXP_MIN
	subu	t1, t1, t9		# adjust exponent
	li	v0, 32
	blt	t9, v0, 1f
	subu	t9, t9, v0		# shift fraction left >= 32 bits
	sll	t2, t3, t9
	move	t3, zero
	j	ra
1:
	subu	v0, v0, t9		# shift fraction left < 32 bits
	sll	t2, t2, t9
	srl	v1, t3, v0
	or	t2, t2, v1
	sll	t3, t3, t9
	j	ra
END(renorm_fs_d)

/*----------------------------------------------------------------------------
 * renorm_ft_s --
 *
 * Results:
 *	t5	unbiased exponent
 *	t6	normalized fraction
 *
 *----------------------------------------------------------------------------
 */
LEAF(renorm_ft_s)
/*
 * Find out how many leading zero bits are in t6 and put in t9.
 */
	move	v0, t6
	move	t9, zero
	srl	v1, v0, 16
	bne	v1, zero, 1f
	addu	t9, 16
	sll	v0, 16
1:
	srl	v1, v0, 24
	bne	v1, zero, 1f
	addu	t9, 8
	sll	v0, 8
1:
	srl	v1, v0, 28
	bne	v1, zero, 1f
	addu	t9, 4
	sll	v0, 4
1:
	srl	v1, v0, 30
	bne	v1, zero, 1f
	addu	t9, 2
	sll	v0, 2
1:
	srl	v1, v0, 31
	bne	v1, zero, 1f
	addu	t9, 1
/*
 * Now shift t6 the correct number of bits.
 */
1:
	subu	t9, t9, SLEAD_ZEROS	# dont count normal leading zeros
	li	t5, SEXP_MIN
	subu	t5, t5, t9		# adjust exponent
	sll	t6, t6, t9
	j	ra
END(renorm_ft_s)

/*----------------------------------------------------------------------------
 * renorm_ft_d --
 *
 * Results:
 *	t5	unbiased exponent
 *	t6,t7	normalized fraction
 *
 *----------------------------------------------------------------------------
 */
LEAF(renorm_ft_d)
/*
 * Find out how many leading zero bits are in t6,t7 and put in t9.
 */
	move	v0, t6
	move	t9, zero
	bne	t6, zero, 1f
	move	v0, t7
	addu	t9, 32
1:
	srl	v1, v0, 16
	bne	v1, zero, 1f
	addu	t9, 16
	sll	v0, 16
1:
	srl	v1, v0, 24
	bne	v1, zero, 1f
	addu	t9, 8
	sll	v0, 8
1:
	srl	v1, v0, 28
	bne	v1, zero, 1f
	addu	t9, 4
	sll	v0, 4
1:
	srl	v1, v0, 30
	bne	v1, zero, 1f
	addu	t9, 2
	sll	v0, 2
1:
	srl	v1, v0, 31
	bne	v1, zero, 1f
	addu	t9, 1
/*
 * Now shift t6,t7 the correct number of bits.
 */
1:
	subu	t9, t9, DLEAD_ZEROS	# dont count normal leading zeros
	li	t5, DEXP_MIN
	subu	t5, t5, t9		# adjust exponent
	li	v0, 32
	blt	t9, v0, 1f
	subu	t9, t9, v0		# shift fraction left >= 32 bits
	sll	t6, t7, t9
	move	t7, zero
	j	ra
1:
	subu	v0, v0, t9		# shift fraction left < 32 bits
	sll	t6, t6, t9
	srl	v1, t7, v0
	or	t6, t6, v1
	sll	t7, t7, t9
	j	ra
END(renorm_ft_d)

#ifdef SOFTFLOAT
/*
 * Emulate branch delay slot CPU instruction.
 * Enter from BC1x emulation.
 * These instructions are not implemented and causes SIGILL.
 *  jump/branch
 *  COP0
 *  64bit operation
 *  trap/syscall/break
 *
 * Args are same as MachFPInterrupt.
 * It should be used to emulate instruction in branch delay slot.
 */
LEAF(bcemul_delay_slot)
	REG_PROLOGUE
	REG_S	zero, FRAME_ZERO(a1)		# ensure zero has value 0
	REG_EPILOGUE

	srl	t0, a0, 26-2
	andi	t0, t0, 0x00FC
	lw	t0, bcemul_optbl(t0)
	j	t0

bcemul_special:
	sll	t0, a0, 2
	andi	t0, t0, 0x00FC
	lw	t0, bcemul_specialtbl(t0)
	j	t0

	.rdata
bcemul_optbl:
	.word	bcemul_special		# 0
	.word	_C_LABEL(bcemul_sigill)	# 1
	.word	_C_LABEL(bcemul_sigill)	# 2
	.word	_C_LABEL(bcemul_sigill)	# 3
	.word	_C_LABEL(bcemul_sigill)	# 4
	.word	_C_LABEL(bcemul_sigill)	# 5
	.word	_C_LABEL(bcemul_sigill)	# 6
	.word	_C_LABEL(bcemul_sigill)	# 7
	.word	bcemul_addi		# 8
	.word	bcemul_addiu		# 9
	.word	bcemul_slti		# 10
	.word	bcemul_sltiu		# 11
	.word	bcemul_andi		# 12
	.word	bcemul_ori		# 13
	.word	bcemul_xori		# 14
	.word	bcemul_lui		# 15
	.word	_C_LABEL(bcemul_sigill)	# 16
	.word	_C_LABEL(MachEmulateFP)	# 17
	.word	_C_LABEL(bcemul_sigill)	# 18
	.word	_C_LABEL(bcemul_sigill)	# 19
	.word	_C_LABEL(bcemul_sigill)	# 20
	.word	_C_LABEL(bcemul_sigill)	# 21
	.word	_C_LABEL(bcemul_sigill)	# 22
	.word	_C_LABEL(bcemul_sigill)	# 23
	.word	_C_LABEL(bcemul_sigill)	# 24
	.word	_C_LABEL(bcemul_sigill)	# 25
	.word	_C_LABEL(bcemul_sigill)	# 26
	.word	_C_LABEL(bcemul_sigill)	# 27
	.word	_C_LABEL(bcemul_sigill)	# 28
	.word	_C_LABEL(bcemul_sigill)	# 29
	.word	_C_LABEL(bcemul_sigill)	# 30
	.word	_C_LABEL(bcemul_sigill)	# 31
	.word	_C_LABEL(bcemul_lb)	# 32
	.word	_C_LABEL(bcemul_lh)	# 33
	.word	_C_LABEL(bcemul_lwl)	# 34
	.word	_C_LABEL(bcemul_lw)	# 35
	.word	_C_LABEL(bcemul_lbu)	# 36
	.word	_C_LABEL(bcemul_lhu)	# 37
	.word	_C_LABEL(bcemul_lwr)	# 38
	.word	_C_LABEL(bcemul_sigill)	# 39
	.word	_C_LABEL(bcemul_sb)	# 40
	.word	_C_LABEL(bcemul_sh)	# 41
	.word	_C_LABEL(bcemul_swl)	# 42
	.word	_C_LABEL(bcemul_sw)	# 43
	.word	_C_LABEL(bcemul_sigill)	# 44
	.word	_C_LABEL(bcemul_sigill)	# 45
	.word	_C_LABEL(bcemul_swr)	# 46
	.word	_C_LABEL(bcemul_sigill)	# 47
	.word	_C_LABEL(bcemul_sigill)	# 48
	.word	_C_LABEL(MachEmulateLWC1) # 49
	.word	_C_LABEL(bcemul_sigill)	# 50
	.word	_C_LABEL(bcemul_sigill)	# 51
	.word	_C_LABEL(bcemul_sigill)	# 52
	.word	_C_LABEL(MachEmulateLDC1) # 53
	.word	_C_LABEL(bcemul_sigill)	# 54
	.word	_C_LABEL(bcemul_sigill)	# 55
	.word	_C_LABEL(bcemul_sigill)	# 56
	.word	_C_LABEL(MachEmulateSWC1) # 57
	.word	_C_LABEL(bcemul_sigill)	# 58
	.word	_C_LABEL(bcemul_sigill)	# 59
	.word	_C_LABEL(bcemul_sigill)	# 60
	.word	_C_LABEL(MachEmulateSDC1) # 61
	.word	_C_LABEL(bcemul_sigill)	# 62
	.word	_C_LABEL(bcemul_sigill)	# 63

bcemul_specialtbl:
	.word	bcemul_sll		# 0
	.word	_C_LABEL(bcemul_sigill)	# 1
	.word	bcemul_srl		# 2
	.word	bcemul_sra		# 3
	.word	bcemul_sllv		# 4
	.word	_C_LABEL(bcemul_sigill)	# 5
	.word	bcemul_srlv		# 6
	.word	bcemul_srav		# 7
	.word	_C_LABEL(bcemul_sigill)	# 8
	.word	_C_LABEL(bcemul_sigill)	# 9
	.word	_C_LABEL(bcemul_sigill)	# 10
	.word	_C_LABEL(bcemul_sigill)	# 11
	.word	_C_LABEL(bcemul_sigill)	# 12
	.word	_C_LABEL(bcemul_sigill)	# 13
	.word	_C_LABEL(bcemul_sigill)	# 14
	.word	bcemul_sync		# 15
	.word	bcemul_mfhi		# 16
	.word	bcemul_mthi		# 17
	.word	bcemul_mflo		# 18
	.word	bcemul_mtlo		# 19
	.word	_C_LABEL(bcemul_sigill)	# 20
	.word	_C_LABEL(bcemul_sigill)	# 21
	.word	_C_LABEL(bcemul_sigill)	# 22
	.word	_C_LABEL(bcemul_sigill)	# 23
	.word	bcemul_mult		# 24
	.word	bcemul_multu		# 25
	.word	bcemul_div		# 26
	.word	bcemul_divu		# 27
	.word	_C_LABEL(bcemul_sigill)	# 28
	.word	_C_LABEL(bcemul_sigill)	# 29
	.word	_C_LABEL(bcemul_sigill)	# 30
	.word	_C_LABEL(bcemul_sigill)	# 31
	.word	bcemul_add		# 32
	.word	bcemul_addu		# 33
	.word	bcemul_sub		# 34
	.word	bcemul_subu		# 35
	.word	bcemul_and		# 36
	.word	bcemul_or		# 37
	.word	bcemul_xor		# 38
	.word	bcemul_nor		# 39
	.word	_C_LABEL(bcemul_sigill)	# 40
	.word	_C_LABEL(bcemul_sigill)	# 41
	.word	bcemul_slt		# 42
	.word	bcemul_sltu		# 43
	.word	_C_LABEL(bcemul_sigill)	# 44
	.word	_C_LABEL(bcemul_sigill)	# 45
	.word	_C_LABEL(bcemul_sigill)	# 46
	.word	_C_LABEL(bcemul_sigill)	# 47
	.word	_C_LABEL(bcemul_sigill)	# 48
	.word	_C_LABEL(bcemul_sigill)	# 49
	.word	_C_LABEL(bcemul_sigill)	# 50
	.word	_C_LABEL(bcemul_sigill)	# 51
	.word	_C_LABEL(bcemul_sigill)	# 52
	.word	_C_LABEL(bcemul_sigill)	# 53
	.word	_C_LABEL(bcemul_sigill)	# 54
	.word	_C_LABEL(bcemul_sigill)	# 55
	.word	_C_LABEL(bcemul_sigill)	# 56
	.word	_C_LABEL(bcemul_sigill)	# 57
	.word	_C_LABEL(bcemul_sigill)	# 58
	.word	_C_LABEL(bcemul_sigill)	# 59
	.word	_C_LABEL(bcemul_sigill)	# 60
	.word	_C_LABEL(bcemul_sigill)	# 61
	.word	_C_LABEL(bcemul_sigill)	# 62
	.word	_C_LABEL(bcemul_sigill)	# 63

	.text

bcemul_addi:
	srl	t0, a0, 21-SZREG_SHFT	# rs
	srl	t1, a0, 16-SZREG_SHFT	# rt
	andi	t0, t0, SZREG_MASK
	andi	t1, t1, SZREG_MASK
	addu	t0, a1, t0
	addu	t1, a1, t1
	sll	t2, a0, 16
	sra	t2, t2, 16
	REG_PROLOGUE
	REG_L	v0, FRAME_ZERO(t0)
	REG_EPILOGUE
	addu	t0, v0, t2

	/* Overflow check */
	xor	t2, v0, t2
	srl	t2, t2, 31
	bne	t2, zero, addiok

	xor	v0, v0, t0
	srl	v0, v0, 31
	beq	v0, zero, addiok

	j	_C_LABEL(bcemul_sigfpe)

addiok:
	REG_PROLOGUE
	REG_S	t0, FRAME_ZERO(t1)
	REG_EPILOGUE
	b	bcemul_done

bcemul_addiu:
	srl	t0, a0, 21-SZREG_SHFT	# rs
	srl	t1, a0, 16-SZREG_SHFT	# rt
	andi	t0, t0, SZREG_MASK
	andi	t1, t1, SZREG_MASK
	addu	t0, a1, t0
	addu	t1, a1, t1
	sll	t2, a0, 16
	sra	t2, t2, 16
	REG_PROLOGUE
	REG_L	v0, FRAME_ZERO(t0)
	addu	v0, v0, t2
	REG_S	v0, FRAME_ZERO(t1)
	REG_EPILOGUE
	b	bcemul_done

bcemul_slti:
	srl	t0, a0, 21-SZREG_SHFT	# rs
	srl	t1, a0, 16-SZREG_SHFT	# rt
	andi	t0, t0, SZREG_MASK
	andi	t1, t1, SZREG_MASK
	addu	t0, a1, t0
	addu	t1, a1, t1
	sll	t2, a0, 16
	sra	t2, t2, 16
	REG_PROLOGUE
	REG_L	v0, FRAME_ZERO(t0)
	slt	v0, v0, t2
	REG_S	v0, FRAME_ZERO(t1)
	REG_EPILOGUE
	b	bcemul_done

bcemul_sltiu:
	srl	t0, a0, 21-SZREG_SHFT	# rs
	srl	t1, a0, 16-SZREG_SHFT	# rt
	andi	t0, t0, SZREG_MASK
	andi	t1, t1, SZREG_MASK
	addu	t0, a1, t0
	addu	t1, a1, t1
	sll	t2, a0, 16
	sra	t2, t2, 16
	REG_PROLOGUE
	REG_L	v0, FRAME_ZERO(t0)
	sltu	v0, v0, t2
	REG_S	v0, FRAME_ZERO(t1)
	REG_EPILOGUE
	b	bcemul_done

bcemul_andi:
	srl	t0, a0, 21-SZREG_SHFT	# rs
	srl	t1, a0, 16-SZREG_SHFT	# rt
	andi	t0, t0, SZREG_MASK
	andi	t1, t1, SZREG_MASK
	addu	t0, a1, t0
	addu	t1, a1, t1
	andi	t2, a0, 0xFFFF
	REG_PROLOGUE
	REG_L	v0, FRAME_ZERO(t0)
	and	v0, v0, t2
	REG_S	v0, FRAME_ZERO(t1)
	REG_EPILOGUE
	b	bcemul_done

bcemul_ori:
	srl	t0, a0, 21-SZREG_SHFT	# rs
	srl	t1, a0, 16-SZREG_SHFT	# rt
	andi	t0, t0, SZREG_MASK
	andi	t1, t1, SZREG_MASK
	addu	t0, a1, t0
	addu	t1, a1, t1
	andi	t2, a0, 0xFFFF
	REG_PROLOGUE
	REG_L	v0, FRAME_ZERO(t0)
	or	v0, v0, t2
	REG_S	v0, FRAME_ZERO(t1)
	REG_EPILOGUE
	b	bcemul_done

bcemul_xori:
	srl	t0, a0, 21-SZREG_SHFT	# rs
	srl	t1, a0, 16-SZREG_SHFT	# rt
	andi	t0, t0, SZREG_MASK
	andi	t1, t1, SZREG_MASK
	addu	t0, a1, t0
	addu	t1, a1, t1
	andi	t2, a0, 0xFFFF
	REG_PROLOGUE
	REG_L	v0, FRAME_ZERO(t0)
	xor	v0, v0, t2
	REG_S	v0, FRAME_ZERO(t1)
	REG_EPILOGUE
	b	bcemul_done

bcemul_lui:
	srl	t0, a0, 16-SZREG_SHFT	# rt
	andi	t0, t0, SZREG_MASK
	addu	t0, a1, t0
	sll	v0, a0, 16
	REG_PROLOGUE
	REG_S	v0, FRAME_ZERO(t0)
	REG_EPILOGUE
	b	bcemul_done

bcemul_sll:
	srl	t0, a0, 16-SZREG_SHFT	# rt
	srl	t1, a0, 11-SZREG_SHFT	# rd
	srl	t2, a0, 6		# sa
	andi	t0, t0, SZREG_MASK
	andi	t1, t1, SZREG_MASK
	andi	t2, t2, 0x001F
	addu	t0, a1, t0
	addu	t1, a1, t1
	REG_PROLOGUE
	REG_L	v0, FRAME_ZERO(t0)
	sllv	v0, v0, t2
	REG_S	v0, FRAME_ZERO(t1)
	REG_EPILOGUE
	b	bcemul_done

bcemul_srl:
	srl	t0, a0, 16-SZREG_SHFT	# rt
	srl	t1, a0, 11-SZREG_SHFT	# rd
	srl	t2, a0, 6		# sa
	andi	t0, t0, SZREG_MASK
	andi	t1, t1, SZREG_MASK
	andi	t2, t2, 0x001F
	addu	t0, a1, t0
	addu	t1, a1, t1
	REG_PROLOGUE
	REG_L	v0, FRAME_ZERO(t0)
	srlv	v0, v0, t2
	REG_S	v0, FRAME_ZERO(t1)
	REG_EPILOGUE
	b	bcemul_done

bcemul_sra:
	srl	t0, a0, 16-SZREG_SHFT	# rt
	srl	t1, a0, 11-SZREG_SHFT	# rd
	srl	t2, a0, 6		# sa
	andi	t0, t0, SZREG_MASK
	andi	t1, t1, SZREG_MASK
	andi	t2, t2, 0x001F
	addu	t0, a1, t0
	addu	t1, a1, t1
	REG_PROLOGUE
	REG_L	v0, FRAME_ZERO(t0)
	srav	v0, v0, t2
	REG_S	v0, FRAME_ZERO(t1)
	REG_EPILOGUE
	b	bcemul_done

bcemul_sllv:
	srl	t0, a0, 21-SZREG_SHFT	# rs
	srl	t1, a0, 16-SZREG_SHFT	# rt
	srl	t2, a0, 11-SZREG_SHFT	# rd
	andi	t0, t0, SZREG_MASK
	andi	t1, t1, SZREG_MASK
	andi	t2, t2, SZREG_MASK
	addu	t0, a1, t0
	addu	t1, a1, t1
	addu	t2, a1, t2
	REG_PROLOGUE
	REG_L	v0, FRAME_ZERO(t0)
	REG_L	v1, FRAME_ZERO(t1)
	sllv	v0, v1, v0
	REG_S	v0, FRAME_ZERO(t2)
	REG_EPILOGUE
	b	bcemul_done

bcemul_srlv:
	srl	t0, a0, 21-SZREG_SHFT	# rs
	srl	t1, a0, 16-SZREG_SHFT	# rt
	srl	t2, a0, 11-SZREG_SHFT	# rd
	andi	t0, t0, SZREG_MASK
	andi	t1, t1, SZREG_MASK
	andi	t2, t2, SZREG_MASK
	addu	t0, a1, t0
	addu	t1, a1, t1
	addu	t2, a1, t2
	REG_PROLOGUE
	REG_L	v0, FRAME_ZERO(t0)
	REG_L	v1, FRAME_ZERO(t1)
	srlv	v0, v1, v0
	REG_S	v0, FRAME_ZERO(t2)
	REG_EPILOGUE
	b	bcemul_done

bcemul_srav:
	srl	t0, a0, 21-SZREG_SHFT	# rs
	srl	t1, a0, 16-SZREG_SHFT	# rt
	srl	t2, a0, 11-SZREG_SHFT	# rd
	andi	t0, t0, SZREG_MASK
	andi	t1, t1, SZREG_MASK
	andi	t2, t2, SZREG_MASK
	addu	t0, a1, t0
	addu	t1, a1, t1
	addu	t2, a1, t2
	REG_PROLOGUE
	REG_L	v0, FRAME_ZERO(t0)
	REG_L	v1, FRAME_ZERO(t1)
	srav	v0, v1, v0
	REG_S	v0, FRAME_ZERO(t2)
	REG_EPILOGUE
	b	bcemul_done

bcemul_sync:
	b	bcemul_done

bcemul_mfhi:
	srl	t0, a0, 11-SZREG_SHFT	# rd
	andi	t0, t0, SZREG_MASK
	addu	t0, a1, t0
	REG_PROLOGUE
	REG_L	v0, FRAME_MULHI(a1)
	REG_S	v0, FRAME_ZERO(t0)
	REG_EPILOGUE
	b	bcemul_done

bcemul_mthi:
	srl	t0, a0, 21-SZREG_SHFT	# rs
	andi	t0, t0, SZREG_MASK
	addu	t0, a1, t0
	REG_PROLOGUE
	REG_L	v0, FRAME_ZERO(t0)
	REG_S	v0, FRAME_MULHI(a1)
	REG_EPILOGUE
	b	bcemul_done

bcemul_mflo:
	srl	t0, a0, 11-SZREG_SHFT	# rd
	andi	t0, t0, SZREG_MASK
	addu	t0, a1, t0
	REG_PROLOGUE
	REG_L	v0, FRAME_MULLO(a1)
	REG_S	v0, FRAME_ZERO(t0)
	REG_EPILOGUE
	b	bcemul_done

bcemul_mtlo:
	srl	t0, a0, 21-SZREG_SHFT	# rs
	andi	t0, t0, SZREG_MASK
	addu	t0, a1, t0
	REG_PROLOGUE
	REG_L	v0, FRAME_ZERO(t0)
	REG_S	v0, FRAME_MULLO(a1)
	REG_EPILOGUE
	b	bcemul_done

bcemul_mult:
	srl	t0, a0, 21-SZREG_SHFT	# rs
	srl	t1, a0, 16-SZREG_SHFT	# rt
	andi	t0, t0, SZREG_MASK
	andi	t1, t1, SZREG_MASK
	addu	t0, a1, t0
	addu	t1, a1, t1
	REG_PROLOGUE
	REG_L	v0, FRAME_ZERO(t0)
	REG_L	v1, FRAME_ZERO(t1)
	REG_EPILOGUE
	mult	v0, v1
	mflo	v0
	mfhi	v1
	REG_PROLOGUE
	REG_S	v0, FRAME_MULLO(a1)
	REG_S	v1, FRAME_MULHI(a1)
	REG_EPILOGUE
	b	bcemul_done

bcemul_multu:
	srl	t0, a0, 21-SZREG_SHFT	# rs
	srl	t1, a0, 16-SZREG_SHFT	# rt
	andi	t0, t0, SZREG_MASK
	andi	t1, t1, SZREG_MASK
	addu	t0, a1, t0
	addu	t1, a1, t1
	REG_PROLOGUE
	REG_L	v0, FRAME_ZERO(t0)
	REG_L	v1, FRAME_ZERO(t1)
	REG_EPILOGUE
	multu	v0, v1
	mflo	v0
	mfhi	v1
	REG_PROLOGUE
	REG_S	v0, FRAME_MULLO(a1)
	REG_S	v1, FRAME_MULHI(a1)
	REG_EPILOGUE
	b	bcemul_done

bcemul_div:
	srl	t0, a0, 21-SZREG_SHFT	# rs
	srl	t1, a0, 16-SZREG_SHFT	# rt
	andi	t0, t0, SZREG_MASK
	andi	t1, t1, SZREG_MASK
	addu	t0, a1, t0
	addu	t1, a1, t1
	REG_PROLOGUE
	REG_L	v0, FRAME_ZERO(t0)
	REG_L	v1, FRAME_ZERO(t1)
	REG_EPILOGUE
	div	v0, v1
	mflo	v0
	mfhi	v1
	REG_PROLOGUE
	REG_S	v0, FRAME_MULLO(a1)
	REG_S	v1, FRAME_MULHI(a1)
	REG_EPILOGUE
	b	bcemul_done

bcemul_divu:
	srl	t0, a0, 21-SZREG_SHFT	# rs
	srl	t1, a0, 16-SZREG_SHFT	# rt
	andi	t0, t0, SZREG_MASK
	andi	t1, t1, SZREG_MASK
	addu	t0, a1, t0
	addu	t1, a1, t1
	REG_PROLOGUE
	REG_L	v0, FRAME_ZERO(t0)
	REG_L	v1, FRAME_ZERO(t1)
	REG_EPILOGUE
	divu	v0, v1
	mflo	v0
	mfhi	v1
	REG_PROLOGUE
	REG_S	v0, FRAME_MULLO(a1)
	REG_S	v1, FRAME_MULHI(a1)
	REG_EPILOGUE
	b	bcemul_done

bcemul_add:
	srl	t0, a0, 21-SZREG_SHFT	# rs
	srl	t1, a0, 16-SZREG_SHFT	# rt
	srl	t2, a0, 11-SZREG_SHFT	# rd
	andi	t0, t0, SZREG_MASK
	andi	t1, t1, SZREG_MASK
	andi	t2, t2, SZREG_MASK
	addu	t0, a1, t0
	addu	t1, a1, t1
	addu	t2, a1, t2
	REG_PROLOGUE
	REG_L	v0, FRAME_ZERO(t0)
	REG_L	v1, FRAME_ZERO(t1)
	REG_EPILOGUE
	addu	t0, v0, v1

	/* Overflow check */
	xor	v1, v0, v1
	srl	v1, v1, 31
	bne	v1, zero, addok

	xor	v0, v0, t0
	srl	v0, v0, 31
	beq	v0, zero, addok

	j	_C_LABEL(bcemul_sigfpe)

addok:
	REG_PROLOGUE
	REG_S	t0, FRAME_ZERO(t2)
	REG_EPILOGUE
	b	bcemul_done

bcemul_addu:
	srl	t0, a0, 21-SZREG_SHFT	# rs
	srl	t1, a0, 16-SZREG_SHFT	# rt
	srl	t2, a0, 11-SZREG_SHFT	# rd
	andi	t0, t0, SZREG_MASK
	andi	t1, t1, SZREG_MASK
	andi	t2, t2, SZREG_MASK
	addu	t0, a1, t0
	addu	t1, a1, t1
	addu	t2, a1, t2
	REG_PROLOGUE
	REG_L	v0, FRAME_ZERO(t0)
	REG_L	v1, FRAME_ZERO(t1)
	addu	v0, v0, v1
	REG_S	v0, FRAME_ZERO(t2)
	REG_EPILOGUE
	b	bcemul_done

bcemul_sub:
	srl	t0, a0, 21-SZREG_SHFT	# rs
	srl	t1, a0, 16-SZREG_SHFT	# rt
	srl	t2, a0, 11-SZREG_SHFT	# rd
	andi	t0, t0, SZREG_MASK
	andi	t1, t1, SZREG_MASK
	andi	t2, t2, SZREG_MASK
	addu	t0, a1, t0
	addu	t1, a1, t1
	addu	t2, a1, t2
	REG_PROLOGUE
	REG_L	v0, FRAME_ZERO(t0)
	REG_L	v1, FRAME_ZERO(t1)
	REG_EPILOGUE
	subu	t0, v0, v1

	/* Overflow check */
	xor	v1, v0, v1
	srl	v1, v1, 31
	beq	v1, zero, subok

	xor	v0, v0, t0
	srl	v0, v0, 31
	beq	v0, zero, subok

	j	_C_LABEL(bcemul_sigfpe)

subok:
	REG_PROLOGUE
	REG_S	t0, FRAME_ZERO(t2)
	REG_EPILOGUE
	b	bcemul_done

bcemul_subu:
	srl	t0, a0, 21-SZREG_SHFT	# rs
	srl	t1, a0, 16-SZREG_SHFT	# rt
	srl	t2, a0, 11-SZREG_SHFT	# rd
	andi	t0, t0, SZREG_MASK
	andi	t1, t1, SZREG_MASK
	andi	t2, t2, SZREG_MASK
	addu	t0, a1, t0
	addu	t1, a1, t1
	addu	t2, a1, t2
	REG_PROLOGUE
	REG_L	v0, FRAME_ZERO(t0)
	REG_L	v1, FRAME_ZERO(t1)
	subu	v0, v0, v1
	REG_S	v0, FRAME_ZERO(t2)
	REG_EPILOGUE
	b	bcemul_done

bcemul_and:
	srl	t0, a0, 21-SZREG_SHFT	# rs
	srl	t1, a0, 16-SZREG_SHFT	# rt
	srl	t2, a0, 11-SZREG_SHFT	# rd
	andi	t0, t0, SZREG_MASK
	andi	t1, t1, SZREG_MASK
	andi	t2, t2, SZREG_MASK
	addu	t0, a1, t0
	addu	t1, a1, t1
	addu	t2, a1, t2
	REG_PROLOGUE
	REG_L	v0, FRAME_ZERO(t0)
	REG_L	v1, FRAME_ZERO(t1)
	and	v0, v0, v1
	REG_S	v0, FRAME_ZERO(t2)
	REG_EPILOGUE
	b	bcemul_done

bcemul_or:
	srl	t0, a0, 21-SZREG_SHFT	# rs
	srl	t1, a0, 16-SZREG_SHFT	# rt
	srl	t2, a0, 11-SZREG_SHFT	# rd
	andi	t0, t0, SZREG_MASK
	andi	t1, t1, SZREG_MASK
	andi	t2, t2, SZREG_MASK
	addu	t0, a1, t0
	addu	t1, a1, t1
	addu	t2, a1, t2
	REG_PROLOGUE
	REG_L	v0, FRAME_ZERO(t0)
	REG_L	v1, FRAME_ZERO(t1)
	or	v0, v0, v1
	REG_S	v0, FRAME_ZERO(t2)
	REG_EPILOGUE
	b	bcemul_done

bcemul_xor:
	srl	t0, a0, 21-SZREG_SHFT	# rs
	srl	t1, a0, 16-SZREG_SHFT	# rt
	srl	t2, a0, 11-SZREG_SHFT	# rd
	andi	t0, t0, SZREG_MASK
	andi	t1, t1, SZREG_MASK
	andi	t2, t2, SZREG_MASK
	addu	t0, a1, t0
	addu	t1, a1, t1
	addu	t2, a1, t2
	REG_PROLOGUE
	REG_L	v0, FRAME_ZERO(t0)
	REG_L	v1, FRAME_ZERO(t1)
	xor	v0, v0, v1
	REG_S	v0, FRAME_ZERO(t2)
	REG_EPILOGUE
	b	bcemul_done

bcemul_nor:
	srl	t0, a0, 21-SZREG_SHFT	# rs
	srl	t1, a0, 16-SZREG_SHFT	# rt
	srl	t2, a0, 11-SZREG_SHFT	# rd
	andi	t0, t0, SZREG_MASK
	andi	t1, t1, SZREG_MASK
	andi	t2, t2, SZREG_MASK
	addu	t0, a1, t0
	addu	t1, a1, t1
	addu	t2, a1, t2
	REG_PROLOGUE
	REG_L	v0, FRAME_ZERO(t0)
	REG_L	v1, FRAME_ZERO(t1)
	nor	v0, v0, v1
	REG_S	v0, FRAME_ZERO(t2)
	REG_EPILOGUE
	b	bcemul_done

bcemul_slt:
	srl	t0, a0, 21-SZREG_SHFT	# rs
	srl	t1, a0, 16-SZREG_SHFT	# rt
	srl	t2, a0, 11-SZREG_SHFT	# rd
	andi	t0, t0, SZREG_MASK
	andi	t1, t1, SZREG_MASK
	andi	t2, t2, SZREG_MASK
	addu	t0, a1, t0
	addu	t1, a1, t1
	addu	t2, a1, t2
	REG_PROLOGUE
	REG_L	v0, FRAME_ZERO(t0)
	REG_L	v1, FRAME_ZERO(t1)
	slt	v0, v0, v1
	REG_S	v0, FRAME_ZERO(t2)
	REG_EPILOGUE
	b	bcemul_done

bcemul_sltu:
	srl	t0, a0, 21-SZREG_SHFT	# rs
	srl	t1, a0, 16-SZREG_SHFT	# rt
	srl	t2, a0, 11-SZREG_SHFT	# rd
	andi	t0, t0, SZREG_MASK
	andi	t1, t1, SZREG_MASK
	andi	t2, t2, SZREG_MASK
	addu	t0, a1, t0
	addu	t1, a1, t1
	addu	t2, a1, t2
	REG_PROLOGUE
	REG_L	v0, FRAME_ZERO(t0)
	REG_L	v1, FRAME_ZERO(t1)
	sltu	v0, v0, v1
	REG_S	v0, FRAME_ZERO(t2)
	REG_EPILOGUE
	sw	v0, FRAME_ZERO(t2)
#	b	bcemul_done		# fall through to bcemul_done

bcemul_done:
/*
 * Succeeded to emulate instruction with no error
 * so compute the next PC.
 */
	subu	sp, sp, CALLFRAME_SIZ
	sw	ra, CALLFRAME_RA(sp)
	sw	a1, CALLFRAME_SIZ + 4(sp)

	/* Fetch previous branch instruction */
	REG_PROLOGUE
	REG_L	a0, FRAME_EPC(a1)
	REG_EPILOGUE
	jal	_C_LABEL(fuiword)

	lw	a1, CALLFRAME_SIZ + 4(sp)

	/* Calculate branch destination */
	sll	t0, v0, 16
	sra	t0, t0, 16-2
	REG_PROLOGUE
	REG_L	t1, FRAME_EPC(a1)
	addiu	t0, t0, 4
	addu	t1, t1, t0
	REG_S	t1, FRAME_EPC(a1)
	REG_EPILOGUE

	lw	ra, CALLFRAME_RA(sp)
	addu	sp, sp, CALLFRAME_SIZ
	j	ra

END(bcemul_delay_slot)

#endif

/*
 * Send SIGILL, SIGFPE.
 * Args are same as MachFPInterrupt.
 */
LEAF(fpemul_sigill)
#ifdef SOFTFLOAT
XLEAF(bcemul_sigill)
#endif
	li	t0, 0xFFFFFF00
	and	a2, a2, t0
	ori	a2, a2, T_RES_INST << MIPS_CR_EXC_CODE_SHIFT
	REG_PROLOGUE
	REG_S	a2, FRAME_CAUSE(a1)
	REG_EPILOGUE

	move	a2, a0				# code = instruction
	lw	a0, _C_LABEL(curproc)		# get current process
	li	a1, SIGILL
	j	_C_LABEL(trapsignal)
END(fpemul_sigill)

LEAF(fpemul_sigfpe)
	li	t0, 0xFFFFFF00
	and	a2, a2, t0
	ori	a2, a2, T_FPE << MIPS_CR_EXC_CODE_SHIFT
	REG_PROLOGUE
	REG_S	a2, FRAME_CAUSE(a1)
	REG_EPILOGUE

	move	a2, a0				# code = instruction
	lw	a0, _C_LABEL(curproc)		# get current process
	li	a1, SIGFPE
	j	_C_LABEL(trapsignal)
END(fpemul_sigfpe)

#ifdef SOFTFLOAT
LEAF(bcemul_sigfpe)
	li	t0, 0xFFFFFF00
	and	a2, a2, t0
	ori	a2, a2, T_OVFLOW << MIPS_CR_EXC_CODE_SHIFT
	REG_PROLOGUE
	REG_S	a2, FRAME_CAUSE(a1)
	REG_EPILOGUE

	move	a2, a0				# code = instruction
	lw	a0, _C_LABEL(curproc)		# get current process
	li	a1, SIGFPE
	j	_C_LABEL(trapsignal)
END(bcemul_sigfpe)
#endif
