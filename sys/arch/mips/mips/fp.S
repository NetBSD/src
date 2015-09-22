/*	$NetBSD: fp.S,v 1.44.30.1 2015/09/22 12:05:47 skrll Exp $	*/

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
 *	@(#)fp.s	8.1 (Berkeley) 6/10/93
 */

#include <sys/cdefs.h>

#include <mips/asm.h>
#include <mips/trap.h>
#include <mips/cpuregs.h>

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

#if defined(FPEMUL)
#if defined(__mips_o32) || defined(__mips_o64)
#define FPX_L			INT_L
#define FPX_S			INT_S
#define	FPX_SCALESHIFT		INT_SCALESHIFT
#else
#define FPX_L			LONG_L
#define FPX_S			LONG_S
#define	FPX_SCALESHIFT		LONG_SCALESHIFT
#define DFPX_L			REG_L
#define DFPX_S			REG_S
#define	DFPX_SCALESHIFT		REG_SCALESHIFT
#define	SZDFREG			SZREG
#define	DFPX_REGMASK		(0x1F << DFPX_SCALESHIFT)
#define	DFPX_REGEVENMASK	(0x1E << DFPX_SCALESHIFT)
#endif
#define	SZFREG			(1 << FPX_SCALESHIFT)
#define	FPX_REGMASK		(0x1F << FPX_SCALESHIFT)
#define	FPX_REGEVENMASK		(0x1E << FPX_SCALESHIFT)
#define	REG_REGMASK		(0x1F << REG_SCALESHIFT)
#endif

/* insns are reordered in the way as MIPS architecture imposes */
	.set	reorder

/*----------------------------------------------------------------------------
 *
 * mips_emul_fp --
 *
 *	Emulate unimplemented floating point operations.
 *	This routine should only be called by mips_fpu_intr().
 *
 *	mips_emul_fp(uint32_t instr, struct trapframe *tf, uint32_t cause)
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Floating point registers are modified according to instruction.
 *
 *----------------------------------------------------------------------------
 */
#if defined(__mips_o32) || defined(__mips_o64)
#define	CALLFRAME_FRAME		(CALLFRAME_SIZ + 1*SZREG)	/* a1 slot */
#define	CALLFRAME_CAUSE		(CALLFRAME_SIZ + 2*SZREG)	/* a2 slot */
#endif
#if defined(__mips_n32) || defined(__mips_n64)
#define	CALLFRAME_FRAME		(1*SZREG)
#define	CALLFRAME_CAUSE		(2*SZREG)
#if CALLFRAME_RA == CALLFRAME_FRAME || CALLFRAME_RA == CALLFRAME_CAUSE
#error N32/N64 ABI callframe error
#endif
#endif
NESTED(mips_emul_fp, CALLFRAME_SIZ, ra)
	PTR_SUBU sp, CALLFRAME_SIZ
	REG_S	ra, CALLFRAME_RA(sp)
	REG_S	a1, CALLFRAME_FRAME(sp)
	REG_S	a2, CALLFRAME_CAUSE(sp)
/*
 * Decode the FMT field (bits 25-21) and FUNCTION field (bits 5-0).
 */
	srl	v0, a0, 21 - PTR_SCALESHIFT	# get FMT field
	andi	v0, v0, 0x1F << PTR_SCALESHIFT	# mask FMT field
#ifdef FPEMUL
	PTR_L	t0, L_PCB(MIPS_CURLWP)		# get pcb of current lwp
	PTR_L	a3, fmt_tbl(v0)			# switch on FUNC & FMT
	INT_L	a2, PCB_FPREGS+FRAME_FSR(t0)
#else
	cfc1	a2, MIPS_FPU_CSR		# get exception register
	PTR_L	a3, fmt_tbl(v0)			# switch on FUNC & FMT
	and	a2, a2, ~MIPS_FPU_EXCEPTION_UNIMPL	# clear exception
	ctc1	a2, MIPS_FPU_CSR
#endif
	j	a3

single_op:
	andi	v0, a0, 0x3F		# get FUNC field
	sll	v0, v0, PTR_SCALESHIFT
	PTR_L	t9, func_single_tbl(v0)
	j	t9
double_op:
	andi	v0, a0, 0x3F		# get FUNC field
	sll	v0, v0, PTR_SCALESHIFT
	PTR_L	t9, func_double_tbl(v0)
	j	t9
single_fixed_op:
	andi	v0, a0, 0x3F		# get FUNC field
	sll	v0, v0, PTR_SCALESHIFT
	PTR_L	t9, func_single_fixed_tbl(v0)
	j	t9
long_fixed_op:
	andi	v0, a0, 0x3F		# get FUNC field
	sll	v0, v0, PTR_SCALESHIFT
	PTR_L	t9, func_long_fixed_tbl(v0)
	j	t9
#if (defined(__mips_n32) || defined(__mips_n64)) && 0
paired_single_op:
	andi	v0, a0, 0x3F		# get FUNC field
	sll	v0, v0, PTR_SCALESHIFT
	PTR_L	t9, func_paired_single_tbl(v0)
	j	t9
#else
#define	paired_single_op	ill
#endif

#ifndef FPEMUL
#define	mfromc1		ill
#define	dmfromc1	ill
#define	cfromc1		ill
#define	mtoc1		ill
#define	dmtoc1		ill
#define	ctoc1		ill
#define	branchc1	ill
#elif !(defined(__mips_n32) || defined(__mips_n64))
#define	dmfromc1	ill
#define	dmtoc1		ill
#endif
#define	branchc1any2	ill
#define	branchc1any4	ill
#define	pairedsingle_op	ill

/*
 * Single Precisions functions
 */
#define	recip_s		ill
#define	recip1_s	ill
#define	recip2_s	ill
#define round_l_s	ill
#define trunc_l_s	ill
#define ceil_l_s	ill
#define floor_l_s	ill
#define cvt_l_s		ill
#define cvt_ps_s	ill
#define	movcf_s		ill
#define	movn_s		ill
#define	movz_s		ill
#define	rsqrt_s		ill
#define	rsqrt1_s	ill
#define	rsqrt2_s	ill
#ifndef MIPS3_PLUS
#define sqrt_s		ill
#define round_w_s	ill
#define trunc_w_s	ill
#define ceil_w_s	ill
#define floor_w_s	ill
#endif

/*
 * Double Precisions functions
 */
#ifndef MIPS3_PLUS
#define sqrt_d		ill
#define round_w_d	ill
#define trunc_w_d	ill
#define ceil_w_d	ill
#define floor_w_d	ill
#endif
#define round_l_d	ill
#define	ceil_l_d	ill
#define trunc_l_d	ill
#define	floor_l_d	ill
#define	recip_d		ill
#define	recip1_d	ill
#define	recip2_d	ill
#define	cvt_l_d		ill
#define	movcf_d		ill
#define	movz_d		ill
#define	movn_d		ill
#define	recip_d		ill
#define	rsqrt_d		ill
#define	rsqrt1_d	ill
#define	rsqrt2_d	ill

/*
 * Long Fixed functions
 */
#define	cvt_s_l		ill
#define	cvt_d_l		ill
#define	cvt_s_pu	ill

/*
 * Paired Single funtions
 */
#define	addr_ps		ill
#define	abs_ps		ill
#define	add_ps		ill
#define	cmp_ps		ill
#define	cvt_ps_pl	ill
#define	cvt_ps_pw	ill
#define	movcf_ps	ill
#define	movz_ps		ill
#define	movn_ps		ill
#define	mov_ps		ill
#define	mul_ps		ill
#define	mulr_ps		ill
#define	neg_ps		ill
#define	recip1_ps	ill
#define	recip2_ps	ill
#define	rsqrt1_ps	ill
#define	rsqrt2_ps	ill
#define	sub_ps		ill

	.rdata
fmt_tbl:
	PTR_WORD mfromc1	# sub 0		mfc1
	PTR_WORD dmfromc1	# sub 1		dmfc1
	PTR_WORD cfromc1	# sub 2		cfc1
	PTR_WORD ill		# sub 3		mfhc1
	PTR_WORD mtoc1		# sub 4		mtc1
	PTR_WORD dmtoc1		# sub 5		dmtc1
	PTR_WORD ctoc1		# sub 6		ctc1
	PTR_WORD ill		# sub 7		mthc1
	PTR_WORD branchc1	# sub 8		bc1
	PTR_WORD branchc1any2	# sub 9		bc1any2
	PTR_WORD branchc1any4	# sub 10	bc1any4
	PTR_WORD ill		# sub 11
	PTR_WORD ill		# sub 12
	PTR_WORD ill		# sub 13
	PTR_WORD ill		# sub 14
	PTR_WORD ill		# sub 15
	PTR_WORD single_op	# sub 16	S
	PTR_WORD double_op	# sub 17	D
	PTR_WORD ill		# sub 18
	PTR_WORD ill		# sub 19
	PTR_WORD single_fixed_op # sub 20	W
	PTR_WORD long_fixed_op	# sub 21	L
	PTR_WORD paired_single_op # sub 22	PS
	PTR_WORD ill		# sub 23
	PTR_WORD ill		# sub 24
	PTR_WORD ill		# sub 25
	PTR_WORD ill		# sub 26
	PTR_WORD ill		# sub 27
	PTR_WORD ill		# sub 28
	PTR_WORD ill		# sub 29
	PTR_WORD ill		# sub 30
	PTR_WORD ill		# sub 31

func_single_tbl:
	PTR_WORD add_s		# func  0 00	ADD.S
	PTR_WORD sub_s		# func  1 01	SUB.S
	PTR_WORD mul_s		# func  2 02	MUL.S
	PTR_WORD div_s		# func  3 03	DIV.S
	PTR_WORD sqrt_s		# func  4 04	SQRT.S
	PTR_WORD abs_s		# func  5 05	ABS.S
	PTR_WORD mov_s		# func  6 06	MOV.S
	PTR_WORD neg_s		# func  7 07	NEG.S 
	PTR_WORD round_l_s	# func  8 10	ROUND.L.S
	PTR_WORD trunc_l_s	# func  9 11	TRUNC.L.S
	PTR_WORD ceil_l_s	# func 10 12	CEIL.L.S
	PTR_WORD floor_l_s	# func 11 13	FLOOR.L.S
	PTR_WORD round_w_s	# func 12 14	ROUND.W.S
	PTR_WORD trunc_w_s	# func 13 15	TRUNC.W.S
	PTR_WORD ceil_w_s	# func 14 16	CEIL.W.S
	PTR_WORD floor_w_s	# func 15 17	FLOOR.W.S
	PTR_WORD ill		# func 16 20
	PTR_WORD movcf_s	# func 17 21	MOVCF.S
	PTR_WORD movz_s		# func 18 22	MOVZ.S
	PTR_WORD movn_s		# func 19 23	MOVN.S
	PTR_WORD ill		# func 20 24
	PTR_WORD recip_s	# func 21 25	RECIP.S
	PTR_WORD rsqrt_s	# func 22 26	RSQRT.S
	PTR_WORD ill		# func 23 27
	PTR_WORD ill		# func 24 30
	PTR_WORD ill		# func 25 31
	PTR_WORD ill		# func 26 32
	PTR_WORD ill		# func 27 33
	PTR_WORD recip2_s	# func 28 34	RECIP2.S
	PTR_WORD recip1_s	# func 29 35	RECIP1.S
	PTR_WORD rsqrt1_s	# func 30 36	RSQRT1.S
	PTR_WORD rsqrt2_s	# func 31 37	RSQRT2.S
	PTR_WORD ill		# func 32 40
	PTR_WORD cvt_d_s	# func 33 41	CVT.D.S
	PTR_WORD ill		# func 34 42
	PTR_WORD ill		# func 35 43
	PTR_WORD cvt_w_s	# func 36 44	CVT.W.S
	PTR_WORD cvt_l_s	# func 37 45	CVT.L.S
	PTR_WORD cvt_ps_s	# func 38 46	CVT.PS.S
	PTR_WORD ill		# func 39 47
	PTR_WORD ill		# func 40 50
	PTR_WORD ill		# func 41 51
	PTR_WORD ill		# func 42 52
	PTR_WORD ill		# func 43 53
	PTR_WORD ill		# func 44 54
	PTR_WORD ill		# func 45 55
	PTR_WORD ill		# func 46 56
	PTR_WORD ill		# func 47 57
	PTR_WORD cmp_s		# func 48 60	C.F.S
	PTR_WORD cmp_s		# func 49 61	C.UN.S
	PTR_WORD cmp_s		# func 50 62	C.EQ.S
	PTR_WORD cmp_s		# func 51 63	C.UEQ.S
	PTR_WORD cmp_s		# func 52 64	C.OLT.S
	PTR_WORD cmp_s		# func 53 65	C.ULT.S
	PTR_WORD cmp_s		# func 54 66	C.OLE.S
	PTR_WORD cmp_s		# func 55 67	C.ULE.S
	PTR_WORD cmp_s		# func 56 70	C.SF.S
	PTR_WORD cmp_s		# func 57 71	C.NGLE.S
	PTR_WORD cmp_s		# func 58 72	C.SEQ.S
	PTR_WORD cmp_s		# func 59 73	C.NGL.S
	PTR_WORD cmp_s		# func 60 74	C.LT.S
	PTR_WORD cmp_s		# func 61 75	C.NGE.S
	PTR_WORD cmp_s		# func 62 76	C.LE.S
	PTR_WORD cmp_s		# func 63 77	C.NGT.S

func_double_tbl:
	PTR_WORD add_d		# func  0 00	ADD.D
	PTR_WORD sub_d		# func  1 01	SUB.D
	PTR_WORD mul_d		# func  2 02	MUL.D
	PTR_WORD div_d		# func  3 03	DIV.D
	PTR_WORD sqrt_d		# func  4 04	SQRT.D
	PTR_WORD abs_d		# func  5 05	ABS.D
	PTR_WORD mov_d		# func  6 06	MOV.D
	PTR_WORD neg_d		# func  7 07	NEG.D 
	PTR_WORD round_l_d	# func  8 10	ROUND.L.D
	PTR_WORD trunc_l_d	# func  9 11	TRUNC.L.D
	PTR_WORD ceil_l_d	# func 10 12	CEIL.L.D
	PTR_WORD floor_l_d	# func 11 13	FLOOR.L.D
	PTR_WORD round_w_d	# func 12 14	ROUND.W.D
	PTR_WORD trunc_w_d	# func 13 15	TRUNC.W.D
	PTR_WORD ceil_w_d	# func 14 16	CEIL.W.D
	PTR_WORD floor_w_d	# func 15 17	FLOOR.W.D
	PTR_WORD ill		# func 16 20
	PTR_WORD movcf_d	# func 17 21	MOVCF.D
	PTR_WORD movz_d		# func 18 22	MOVZ.D
	PTR_WORD movn_d		# func 19 23	MOVN.D
	PTR_WORD ill		# func 20 24
	PTR_WORD recip_d	# func 21 25	RECIP.D
	PTR_WORD rsqrt_d	# func 22 26	RSQRT.D
	PTR_WORD ill		# func 23 27
	PTR_WORD ill		# func 24 30
	PTR_WORD ill		# func 25 31
	PTR_WORD ill		# func 26 32
	PTR_WORD ill		# func 27 33
	PTR_WORD recip2_d	# func 28 34	RECIP2.D
	PTR_WORD recip2_d	# func 29 35	RECIP1.D
	PTR_WORD rsqrt1_d	# func 30 36	RSQRT1.D
	PTR_WORD rsqrt2_d	# func 31 37	RSQRT2.D
	PTR_WORD cvt_s_d	# func 32 40	CVT.S.D
	PTR_WORD ill		# func 33 41
	PTR_WORD ill		# func 34 42
	PTR_WORD ill		# func 35 43
	PTR_WORD cvt_w_d	# func 36 44	CVT.W.D
	PTR_WORD cvt_l_d	# func 37 45	CVT.L.D
	PTR_WORD ill		# func 38 46
	PTR_WORD ill		# func 39 47
	PTR_WORD ill		# func 40 50
	PTR_WORD ill		# func 41 51
	PTR_WORD ill		# func 42 52
	PTR_WORD ill		# func 43 53
	PTR_WORD ill		# func 44 54
	PTR_WORD ill		# func 45 55
	PTR_WORD ill		# func 46 56
	PTR_WORD ill		# func 47 57
	PTR_WORD cmp_d		# func 48 60	C.F.D
	PTR_WORD cmp_d		# func 49 61	C.UN.D
	PTR_WORD cmp_d		# func 50 62	C.EQ.D
	PTR_WORD cmp_d		# func 51 63	C.UEQ.D
	PTR_WORD cmp_d		# func 52 64	C.OLT.D
	PTR_WORD cmp_d		# func 53 65	C.ULT.D
	PTR_WORD cmp_d		# func 54 66	C.OLE.D
	PTR_WORD cmp_d		# func 55 67	C.ULE.D
	PTR_WORD cmp_d		# func 56 70	C.SF.D
	PTR_WORD cmp_d		# func 57 71	C.NGLE.D
	PTR_WORD cmp_d		# func 58 72	C.SEQ.D
	PTR_WORD cmp_d		# func 59 73	C.NGL.D
	PTR_WORD cmp_d		# func 60 74	C.LT.D
	PTR_WORD cmp_d		# func 61 75	C.NGE.D
	PTR_WORD cmp_d		# func 62 76	C.LE.D
	PTR_WORD cmp_d		# func 63 77	C.NGT.D

func_single_fixed_tbl:
	PTR_WORD ill		# func  0 00
	PTR_WORD ill		# func  1 01
	PTR_WORD ill		# func  2 02
	PTR_WORD ill		# func  3 03
	PTR_WORD ill		# func  4 04
	PTR_WORD ill		# func  5 05
	PTR_WORD ill		# func  6 06
	PTR_WORD ill		# func  7 07
	PTR_WORD ill		# func  8 10
	PTR_WORD ill		# func  9 11
	PTR_WORD ill		# func 10 12
	PTR_WORD ill		# func 11 13
	PTR_WORD ill		# func 12 14
	PTR_WORD ill		# func 13 15
	PTR_WORD ill		# func 14 16
	PTR_WORD ill		# func 15 17
	PTR_WORD ill		# func 16 20
	PTR_WORD ill		# func 17 21
	PTR_WORD ill		# func 18 22
	PTR_WORD ill		# func 19 23
	PTR_WORD ill		# func 20 24
	PTR_WORD ill		# func 21 25
	PTR_WORD ill		# func 22 26
	PTR_WORD ill		# func 23 27
	PTR_WORD ill		# func 24 30
	PTR_WORD ill		# func 25 31
	PTR_WORD ill		# func 26 32
	PTR_WORD ill		# func 27 33
	PTR_WORD ill		# func 28 34
	PTR_WORD ill		# func 29 35
	PTR_WORD ill		# func 30 36
	PTR_WORD ill		# func 31 37
	PTR_WORD cvt_s_w	# func 32 40	CVT.S.W
	PTR_WORD cvt_d_w	# func 33 41	CVT.D.W
	PTR_WORD ill		# func 34 42
	PTR_WORD ill		# func 35 43
	PTR_WORD ill		# func 36 44
	PTR_WORD ill		# func 37 45
	PTR_WORD cvt_ps_pw	# func 38 46	CVT.PS.PW
	PTR_WORD ill		# func 39 47
	PTR_WORD ill		# func 40 50
	PTR_WORD ill		# func 41 51
	PTR_WORD ill		# func 42 52
	PTR_WORD ill		# func 43 53
	PTR_WORD ill		# func 44 54
	PTR_WORD ill		# func 45 55
	PTR_WORD ill		# func 46 56
	PTR_WORD ill		# func 47 57
	PTR_WORD ill		# func 48 60
	PTR_WORD ill		# func 49 61
	PTR_WORD ill		# func 50 62
	PTR_WORD ill		# func 51 63
	PTR_WORD ill		# func 52 64
	PTR_WORD ill		# func 53 65
	PTR_WORD ill		# func 54 66
	PTR_WORD ill		# func 55 67
	PTR_WORD ill		# func 56 70
	PTR_WORD ill		# func 57 71
	PTR_WORD ill		# func 58 72
	PTR_WORD ill		# func 59 73
	PTR_WORD ill		# func 60 74
	PTR_WORD ill		# func 61 75
	PTR_WORD ill		# func 62 76
	PTR_WORD ill		# func 63 77

func_long_fixed_tbl:
	PTR_WORD ill		# func  0 00
	PTR_WORD ill		# func  1 01
	PTR_WORD ill		# func  2 02
	PTR_WORD ill		# func  3 03
	PTR_WORD ill		# func  4 04
	PTR_WORD ill		# func  5 05
	PTR_WORD ill		# func  6 06
	PTR_WORD ill		# func  7 07
	PTR_WORD ill		# func  8 10
	PTR_WORD ill		# func  9 11
	PTR_WORD ill		# func 10 12
	PTR_WORD ill		# func 11 13
	PTR_WORD ill		# func 12 14
	PTR_WORD ill		# func 13 15
	PTR_WORD ill		# func 14 16
	PTR_WORD ill		# func 15 17
	PTR_WORD ill		# func 16 20
	PTR_WORD ill		# func 17 21
	PTR_WORD ill		# func 18 22
	PTR_WORD ill		# func 19 23
	PTR_WORD ill		# func 20 24
	PTR_WORD ill		# func 21 25
	PTR_WORD ill		# func 22 26
	PTR_WORD ill		# func 23 27
	PTR_WORD ill		# func 24 30
	PTR_WORD ill		# func 25 31
	PTR_WORD ill		# func 26 32
	PTR_WORD ill		# func 27 33
	PTR_WORD ill		# func 28 34
	PTR_WORD ill		# func 29 35
	PTR_WORD ill		# func 30 36
	PTR_WORD ill		# func 31 37
	PTR_WORD cvt_s_l	# func 32 40	CVT.S.L
	PTR_WORD cvt_d_l	# func 33 41	CVT.D.L
	PTR_WORD ill		# func 34 42
	PTR_WORD ill		# func 35 43
	PTR_WORD ill		# func 36 44
	PTR_WORD ill		# func 37 45
	PTR_WORD cvt_ps_pl	# func 38 46	CVT.PS.PW
	PTR_WORD ill		# func 39 47
	PTR_WORD ill		# func 40 50
	PTR_WORD ill		# func 41 51
	PTR_WORD ill		# func 42 52
	PTR_WORD ill		# func 43 53
	PTR_WORD ill		# func 44 54
	PTR_WORD ill		# func 45 55
	PTR_WORD ill		# func 46 56
	PTR_WORD ill		# func 47 57
	PTR_WORD ill		# func 48 60
	PTR_WORD ill		# func 49 61
	PTR_WORD ill		# func 50 62
	PTR_WORD ill		# func 51 63
	PTR_WORD ill		# func 52 64
	PTR_WORD ill		# func 53 65
	PTR_WORD ill		# func 54 66
	PTR_WORD ill		# func 55 67
	PTR_WORD ill		# func 56 70
	PTR_WORD ill		# func 57 71
	PTR_WORD ill		# func 58 72
	PTR_WORD ill		# func 59 73
	PTR_WORD ill		# func 60 74
	PTR_WORD ill		# func 61 75
	PTR_WORD ill		# func 62 76
	PTR_WORD ill		# func 63 77

#if defined(MIPS3_PLUS) && 0
func_paired_single_tbl:
	PTR_WORD add_ps		# func  0 00	ADD.PS
	PTR_WORD sub_ps		# func  1 01	SUB.PS
	PTR_WORD mul_ps		# func  2 02	MUL.PS
	PTR_WORD ill		# func  3 03
	PTR_WORD ill		# func  4 04
	PTR_WORD abs_ps		# func  5 05	ABS.PS
	PTR_WORD mov_ps		# func  6 06	MOV.PS
	PTR_WORD neg_ps		# func  7 07	NEG.PS 
	PTR_WORD ill		# func  8 10
	PTR_WORD ill		# func  9 11
	PTR_WORD ill		# func 10 12
	PTR_WORD ill		# func 11 13
	PTR_WORD ill		# func 12 14
	PTR_WORD ill		# func 13 15
	PTR_WORD ill		# func 14 16
	PTR_WORD ill		# func 15 17
	PTR_WORD ill		# func 16 20
	PTR_WORD movcf_ps	# func 17 21	MOVCF.PS
	PTR_WORD movz_ps	# func 18 22	MOVZ.PS
	PTR_WORD movn_ps	# func 19 23	MOVN.PS
	PTR_WORD ill		# func 20 24
	PTR_WORD ill		# func 21 25
	PTR_WORD ill		# func 22 26
	PTR_WORD ill		# func 23 27
	PTR_WORD addr_ps	# func 24 30	ADDR.PS
	PTR_WORD ill		# func 25 31
	PTR_WORD mulr_ps	# func 26 32	MULR.PS
	PTR_WORD ill		# func 27 33
	PTR_WORD recip2_ps	# func 28 34	RECIP2.PS
	PTR_WORD recip1_ps	# func 29 35	RECIP1.PS
	PTR_WORD rsqrt1_ps	# func 30 36	RSQRT1.PS
	PTR_WORD rsqrt2_ps	# func 31 37	RSQRT2.PS
	PTR_WORD cvt_s_pu	# func 32 40	CVT.S.PU
	PTR_WORD ill		# func 33 41
	PTR_WORD ill		# func 34 42
	PTR_WORD ill		# func 35 43
	PTR_WORD ill		# func 36 44	CVT.PW.PS
	PTR_WORD ill		# func 37 45
	PTR_WORD ill		# func 38 46
	PTR_WORD ill		# func 39 47
	PTR_WORD ill		# func 40 50	CVT.S.PL
	PTR_WORD ill		# func 41 51
	PTR_WORD ill		# func 42 52
	PTR_WORD ill		# func 43 53
	PTR_WORD ill		# func 44 54	PLL.PS
	PTR_WORD ill		# func 45 55	PLU.PS
	PTR_WORD ill		# func 46 56	PUL.PS
	PTR_WORD ill		# func 47 57	PUU.PS
	PTR_WORD cmp_ps		# func 48 60	C.F
	PTR_WORD cmp_ps		# func 49 61	C.UN
	PTR_WORD cmp_ps		# func 50 62	C.EQ
	PTR_WORD cmp_ps		# func 51 63	C.UEQ
	PTR_WORD cmp_ps		# func 52 64	C.OLT
	PTR_WORD cmp_ps		# func 53 65	C.ULT
	PTR_WORD cmp_ps		# func 54 66	C.OLE
	PTR_WORD cmp_ps		# func 55 67	C.ULE
	PTR_WORD cmp_ps		# func 56 70	C.SF
	PTR_WORD cmp_ps		# func 57 71	C.NGLE
	PTR_WORD cmp_ps		# func 58 72	C.SEQ
	PTR_WORD cmp_ps		# func 59 73	C.NGL
	PTR_WORD cmp_ps		# func 60 74	C.LT
	PTR_WORD cmp_ps		# func 61 75	C.NGE
	PTR_WORD cmp_ps		# func 62 76	C.LE
	PTR_WORD cmp_ps		# func 63 77	C.NGT
#endif

	.text

#ifdef FPEMUL
mfromc1:
	srl	t1, a0, 11-FPX_SCALESHIFT	# fs is in bits 15:11
	PTR_L	t0, L_PCB(MIPS_CURLWP)		# get pcb of current lwp
	andi	t1, t1, FPX_REGMASK
	PTR_ADDU t0, t0, t1

	FPX_L	v0, PCB_FPREGS+FRAME_FP0(t0)

	srl	t0, a0, 16-REG_SCALESHIFT
	andi	t0, t0, REG_REGMASK
	PTR_ADDU t0, t0, a1

	REG_PROLOGUE
	REG_S	v0, TF_REG_ZERO(t0)
	REG_EPILOGUE

	b	done

mtoc1:
	REG_PROLOGUE
	REG_S	zero, TF_REG_ZERO(a1)		# ensure zero has value 0
	srl	t0, a0, 16-REG_SCALESHIFT
	andi	t0, t0, REG_REGMASK
	PTR_ADDU v0, a1, t0
	REG_L	v0, TF_REG_ZERO(v0)
	REG_EPILOGUE

	srl	t1, a0, 11-FPX_SCALESHIFT
	PTR_L	t0, L_PCB(MIPS_CURLWP)		# get pcb of current lwp
	andi	t1, t1, FPX_REGMASK
	PTR_ADDU t0, t0, t1

	FPX_S	v0, PCB_FPREGS+FRAME_FP0(t0)

	b	done

#if defined(FPEMUL) && (defined(__mips_n32) || defined(__mips_n64))
dmfromc1:
	srl	t1, a0, 11-DFPX_SCALESHIFT	# fs is in bits 15:11
	PTR_L	t0, L_PCB(MIPS_CURLWP)		# get pcb of current lwp
	andi	t1, t1,  DFPX_REGMASK
	PTR_ADDU t0, t0, t1

	DFPX_L	v0, PCB_FPREGS+FRAME_FP0(t0)

	srl	t0, a0, 16-REG_SCALESHIFT
	andi	t0, t0, REG_REGMASK
	PTR_ADDU t0, t0, a1

	REG_PROLOGUE
	REG_S	v0, TF_REG_ZERO(t0)
	REG_EPILOGUE

	b	done

dmtoc1:
	REG_PROLOGUE
	REG_S	zero, TF_REG_ZERO(a1)		# ensure zero has value 0
	srl	t0, a0, 16-REG_SCALESHIFT
	andi	t0, t0, REG_REGMASK
	PTR_ADDU v0, a1, t0
	REG_L	v0, TF_REG_ZERO(v0)
	REG_EPILOGUE

	srl	t1, a0, 11-DFPX_SCALESHIFT
	PTR_L	t0, L_PCB(MIPS_CURLWP)		# get pcb of current lwp
	andi	t1, t1, DFPX_REGMASK
	PTR_ADDU t0, t0, t1

	DFPX_S	v0, PCB_FPREGS+FRAME_FP0(t0)

	b	done
#endif /* FPEMUL && (__mips_n32 || __mips_n64) */

cfromc1:
	srl	t1, a0, 11
	PTR_L	t0, L_PCB(MIPS_CURLWP)		# get pcb of current lwp
	andi	t1, t1, 0x001F
	li	t2, 0x1F
	move	v0, zero
	bne	t1, t2, cfinvalid

	INT_L	v0, PCB_FPREGS+FRAME_FSR(t0)

cfinvalid:

	srl	t0, a0, 16-REG_SCALESHIFT
	andi	t0, t0, REG_REGMASK
	PTR_ADDU t0, t0, a1

	REG_PROLOGUE
	REG_S	v0, TF_REG_ZERO(t0)
	REG_EPILOGUE

	b	done

ctoc1:
	REG_PROLOGUE
	REG_S	zero, TF_REG_ZERO(a1)		# ensure zero has value 0
	REG_EPILOGUE

	srl	t0, a0, 11
	andi	t0, t0, 0x001F
	li	t1, 0x1F
	bne	t0, t1, done

	srl	t0, a0, 16-REG_SCALESHIFT
	andi	t0, t0, REG_REGMASK
	PTR_ADDU v0, a1, t0
	REG_PROLOGUE
	REG_L	v0, TF_REG_ZERO(v0)
	REG_EPILOGUE
	PTR_L	t0, L_PCB(MIPS_CURLWP)		# get pcb of current lwp
	#nop
	INT_S	v0, PCB_FPREGS+FRAME_FSR(t0)

	b	done

branchc1:
	srl	v0, a0, 16 - PTR_SCALESHIFT
	andi	v0, v0, 0x1f << PTR_SCALESHIFT
	PTR_L	t9, branchc1_tbl(v0)
	j	t9

	.rdata
branchc1_tbl:
	PTR_WORD bcfalse		# br 0
	PTR_WORD bctrue		# br 1
	PTR_WORD bcfalse_l	# br 2
	PTR_WORD bctrue_l	# br 3
	PTR_WORD ill		# br 4
	PTR_WORD ill		# br 5
	PTR_WORD ill		# br 6
	PTR_WORD ill		# br 7
	PTR_WORD ill		# br 8
	PTR_WORD ill		# br 9
	PTR_WORD ill		# br 10
	PTR_WORD ill		# br 11
	PTR_WORD ill		# br 12
	PTR_WORD ill		# br 13
	PTR_WORD ill		# br 14
	PTR_WORD ill		# br 15
	PTR_WORD ill		# br 16
	PTR_WORD ill		# br 17
	PTR_WORD ill		# br 18
	PTR_WORD ill		# br 19
	PTR_WORD ill		# br 20
	PTR_WORD ill		# br 21
	PTR_WORD ill		# br 22
	PTR_WORD ill		# br 23
	PTR_WORD ill		# br 24
	PTR_WORD ill		# br 25
	PTR_WORD ill		# br 26
	PTR_WORD ill		# br 27
	PTR_WORD ill		# br 28
	PTR_WORD ill		# br 29
	PTR_WORD ill		# br 30
	PTR_WORD ill		# br 31

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
	REG_L	v0, TF_REG_EPC(a1)
	addiu	v0, v0, 4
	REG_S	v0, TF_REG_EPC(a1)
	REG_EPILOGUE
	b	done
bctrue_l:
	li	v0, MIPS_FPU_COND_BIT
	and	v0, v0, a2
	bne	v0, zero, bcemul_branch
	REG_PROLOGUE
	REG_L	v0, TF_REG_EPC(a1)
	addiu	v0, v0, 4
	REG_S	v0, TF_REG_EPC(a1)
	REG_EPILOGUE
	b	done

bcemul_branch:
	/* Fetch delay slot instruction */
	REG_L	a1, CALLFRAME_FRAME(sp)
	REG_PROLOGUE
	REG_L	a0, TF_REG_EPC(a1)
	REG_EPILOGUE
	PTR_ADDU a0, 4
	jal	_C_LABEL(ufetch_uint32)

	move	a0, v0
	REG_L	a1, CALLFRAME_FRAME(sp)
	REG_L	a2, CALLFRAME_CAUSE(sp)

	/* Update cause */
	li	t0, MIPS_CR_BR_DELAY
	or	a2, a2, t0

	/* Free mips_emul_fp call frame */
	REG_L	ra, CALLFRAME_RA(sp)
	PTR_ADDU sp, CALLFRAME_SIZ

	j	_C_LABEL(mips_emul_branchdelayslot)
#endif

/*
 * Single precision subtract.
 */
sub_s:
	jal	_C_LABEL(get_ft_fs_s)
	xor	ta0, ta0, 1			# negate FT sign bit
	b	add_sub_s
/*
 * Single precision add.
 */
add_s:
	jal	_C_LABEL(get_ft_fs_s)
add_sub_s:
	bne	t1, SEXP_INF, 1f		# is FS an infinity?
	bne	ta1, SEXP_INF, result_fs_s	# if FT is not inf, result=FS
	bne	t2, zero, result_fs_s		# if FS is NAN, result is FS
	bne	ta2, zero, result_ft_s		# if FT is NAN, result is FT
	bne	t0, ta0, invalid_s		# both infinities same sign?
	b	result_fs_s			# result is in FS
1:
	beq	ta1, SEXP_INF, result_ft_s	# if FT is inf, result=FT
	bne	t1, zero, 4f			# is FS a denormalized num?
	beq	t2, zero, 3f			# is FS zero?
	bne	ta1, zero, 2f			# is FT a denormalized num?
	beq	ta2, zero, result_fs_s		# FT is zero, result=FS
	jal	_C_LABEL(renorm_fs_s)
	jal	_C_LABEL(renorm_ft_s)
	b	5f
2:
	jal	_C_LABEL(renorm_fs_s)
	subu	ta1, ta1, SEXP_BIAS		# unbias FT exponent
	or	ta2, ta2, SIMPL_ONE		# set implied one bit
	b	5f
3:
	bne	ta1, zero, result_ft_s		# if FT != 0, result=FT
	bne	ta2, zero, result_ft_s
	and	v0, a2, MIPS_FPU_ROUNDING_BITS	# get rounding mode
	bne	v0, MIPS_FPU_ROUND_RM, 1f	# round to -infinity?
	or	t0, t0, ta0			# compute result sign
	b	result_fs_s
1:
	and	t0, t0, ta0			# compute result sign
	b	result_fs_s
4:
	bne	ta1, zero, 2f			# is FT a denormalized num?
	beq	ta2, zero, result_fs_s		# FT is zero, result=FS
	subu	t1, t1, SEXP_BIAS		# unbias FS exponent
	or	t2, t2, SIMPL_ONE		# set implied one bit
	jal	_C_LABEL(renorm_ft_s)
	b	5f
2:
	subu	t1, t1, SEXP_BIAS		# unbias FS exponent
	or	t2, t2, SIMPL_ONE		# set implied one bit
	subu	ta1, ta1, SEXP_BIAS		# unbias FT exponent
	or	ta2, ta2, SIMPL_ONE		# set implied one bit
/*
 * Perform the addition.
 */
5:
	move	t9, zero			# no shifted bits (sticky reg)
	beq	t1, ta1, 4f			# no shift needed
	subu	v0, t1, ta1			# v0 = difference of exponents
	move	v1, v0				# v1 = abs(difference)
	bge	v0, zero, 1f
	negu	v1
1:
	ble	v1, SFRAC_BITS+2, 2f		# is difference too great?
	li	t9, STICKYBIT			# set the sticky bit
	bge	v0, zero, 1f			# check which exp is larger
	move	t1, ta1				# result exp is FTs
	move	t2, zero			# FSs fraction shifted is zero
	b	4f
1:
	move	ta2, zero			# FTs fraction shifted is zero
	b	4f
2:
	li	t9, 32				# compute 32 - abs(exp diff)
	subu	t9, t9, v1
	bgt	v0, zero, 3f			# if FS > FT, shift FTs frac
	move	t1, ta1				# FT > FS, result exp is FTs
	sll	t9, t2, t9			# save bits shifted out
	srl	t2, t2, v1			# shift FSs fraction
	b	4f
3:
	sll	t9, ta2, t9			# save bits shifted out
	srl	ta2, ta2, v1			# shift FTs fraction
4:
	bne	t0, ta0, 1f			# if signs differ, subtract
	addu	t2, t2, ta2			# add fractions
	b	norm_s
1:
	blt	t2, ta2, 3f			# subtract larger from smaller
	bne	t2, ta2, 2f			# if same, result=0
	move	t1, zero			# result=0
	move	t2, zero
	and	v0, a2, MIPS_FPU_ROUNDING_BITS	# get rounding mode
	bne	v0, MIPS_FPU_ROUND_RM, 1f	# round to -infinity?
	or	t0, t0, ta0			# compute result sign
	b	result_fs_s
1:
	and	t0, t0, ta0			# compute result sign
	b	result_fs_s
2:
	sltu	v0, zero, t9			# compute t2:zero - ta2:t9
	subu	t9, zero, t9
	subu	t2, t2, ta2			# subtract fractions
	subu	t2, t2, v0			# subtract barrow
	b	norm_s
3:
	move	t0, ta0				# sign of result = FTs
	sltu	v0, zero, t9			# compute ta2:zero - t2:t9
	subu	t9, zero, t9
	subu	t2, ta2, t2			# subtract fractions
	subu	t2, t2, v0			# subtract barrow
	b	norm_s

/*
 * Double precision subtract.
 */
sub_d:
	jal	_C_LABEL(get_ft_fs_d)
	xor	ta0, ta0, 1			# negate sign bit
	b	add_sub_d
/*
 * Double precision add.
 */
add_d:
	jal	_C_LABEL(get_ft_fs_d)
add_sub_d:
	bne	t1, DEXP_INF, 1f		# is FS an infinity?
	bne	ta1, DEXP_INF, result_fs_d	# if FT is not inf, result=FS
	bne	t2, zero, result_fs_d		# if FS is NAN, result is FS
	bne	t3, zero, result_fs_d
	bne	ta2, zero, result_ft_d		# if FT is NAN, result is FT
	bne	ta3, zero, result_ft_d
	bne	t0, ta0, invalid_d		# both infinities same sign?
	b	result_fs_d			# result is in FS
1:
	beq	ta1, DEXP_INF, result_ft_d	# if FT is inf, result=FT
	bne	t1, zero, 4f			# is FS a denormalized num?
	bne	t2, zero, 1f			# is FS zero?
	beq	t3, zero, 3f
1:
	bne	ta1, zero, 2f			# is FT a denormalized num?
	bne	ta2, zero, 1f
	beq	ta3, zero, result_fs_d		# FT is zero, result=FS
1:
	jal	_C_LABEL(renorm_fs_d)
	jal	_C_LABEL(renorm_ft_d)
	b	5f
2:
	jal	_C_LABEL(renorm_fs_d)
	subu	ta1, ta1, DEXP_BIAS		# unbias FT exponent
	or	ta2, ta2, DIMPL_ONE		# set implied one bit
	b	5f
3:
	bne	ta1, zero, result_ft_d		# if FT != 0, result=FT
	bne	ta2, zero, result_ft_d
	bne	ta3, zero, result_ft_d
	and	v0, a2, MIPS_FPU_ROUNDING_BITS	# get rounding mode
	bne	v0, MIPS_FPU_ROUND_RM, 1f	# round to -infinity?
	or	t0, t0, ta0			# compute result sign
	b	result_fs_d
1:
	and	t0, t0, ta0			# compute result sign
	b	result_fs_d
4:
	bne	ta1, zero, 2f			# is FT a denormalized num?
	bne	ta2, zero, 1f
	beq	ta3, zero, result_fs_d		# FT is zero, result=FS
1:
	subu	t1, t1, DEXP_BIAS		# unbias FS exponent
	or	t2, t2, DIMPL_ONE		# set implied one bit
	jal	_C_LABEL(renorm_ft_d)
	b	5f
2:
	subu	t1, t1, DEXP_BIAS		# unbias FS exponent
	or	t2, t2, DIMPL_ONE		# set implied one bit
	subu	ta1, ta1, DEXP_BIAS		# unbias FT exponent
	or	ta2, ta2, DIMPL_ONE		# set implied one bit
/*
 * Perform the addition.
 */
5:
	move	t9, zero			# no shifted bits (sticky reg)
	beq	t1, ta1, 4f			# no shift needed
	subu	v0, t1, ta1			# v0 = difference of exponents
	move	v1, v0				# v1 = abs(difference)
	bge	v0, zero, 1f
	negu	v1
1:
	ble	v1, DFRAC_BITS+2, 2f		# is difference too great?
	li	t9, STICKYBIT			# set the sticky bit
	bge	v0, zero, 1f			# check which exp is larger
	move	t1, ta1				# result exp is FTs
	move	t2, zero			# FSs fraction shifted is zero
	move	t3, zero
	b	4f
1:
	move	ta2, zero			# FTs fraction shifted is zero
	move	ta3, zero
	b	4f
2:
	li	t9, 32
	bge	v0, zero, 3f			# if FS > FT, shift FTs frac
	move	t1, ta1				# FT > FS, result exp is FTs
	blt	v1, t9, 1f			# shift right by < 32?
	subu	v1, v1, t9
	subu	t9, t9, v1
	sll	v0, t2, t9			# save bits shifted out
	sltu	t9, zero, t3			# dont lose any one bits
	or	t9, t9, v0			# save sticky bit
	srl	t3, t2, v1			# shift FSs fraction
	move	t2, zero
	b	4f
1:
	subu	v0, t9, v1
	sll	t9, t3, v0			# save bits shifted out
	srl	t3, t3, v1			# shift FSs fraction
	sll	v0, t2, v0			# save bits shifted out of t2
	or	t3, t3, v0			# and put into t3
	srl	t2, t2, v1
	b	4f
3:
	blt	v1, t9, 1f			# shift right by < 32?
	subu	v1, v1, t9
	subu	v0, t9, v1
	sll	t9, ta2, v0			# save bits shifted out
	srl	ta3, ta2, v1			# shift FTs fraction
	move	ta2, zero
	b	4f
1:
	subu	v0, t9, v1
	sll	t9, ta3, v0			# save bits shifted out
	srl	ta3, ta3, v1			# shift FTs fraction
	sll	v0, ta2, v0			# save bits shifted out of t2
	or	ta3, ta3, v0			# and put into t3
	srl	ta2, ta2, v1
4:
	bne	t0, ta0, 1f			# if signs differ, subtract
	addu	t3, t3, ta3			# add fractions
	sltu	v0, t3, ta3			# compute carry
	addu	t2, t2, ta2			# add fractions
	addu	t2, t2, v0			# add carry
	b	norm_d
1:
	blt	t2, ta2, 3f			# subtract larger from smaller
	bne	t2, ta2, 2f
	bltu	t3, ta3, 3f
	bne	t3, ta3, 2f			# if same, result=0
	move	t1, zero			# result=0
	move	t2, zero
	move	t3, zero
	and	v0, a2, MIPS_FPU_ROUNDING_BITS	# get rounding mode
	bne	v0, MIPS_FPU_ROUND_RM, 1f	# round to -infinity?
	or	t0, t0, ta0			# compute result sign
	b	result_fs_d
1:
	and	t0, t0, ta0			# compute result sign
	b	result_fs_d
2:
	beq	t9, zero, 1f			# compute t2:t3:zero - ta2:ta3:t9
	subu	t9, zero, t9
	sltu	v0, t3, 1			# compute barrow out
	subu	t3, t3, 1			# subtract barrow
	subu	t2, t2, v0
1:
	sltu	v0, t3, ta3
	subu	t3, t3, ta3			# subtract fractions
	subu	t2, t2, ta2			# subtract fractions
	subu	t2, t2, v0			# subtract barrow
	b	norm_d
3:
	move	t0, ta0				# sign of result = FTs
	beq	t9, zero, 1f			# compute ta2:ta3:zero - t2:t3:t9
	subu	t9, zero, t9
	sltu	v0, ta3, 1			# compute barrow out
	subu	ta3, ta3, 1			# subtract barrow
	subu	ta2, ta2, v0
1:
	sltu	v0, ta3, t3
	subu	t3, ta3, t3			# subtract fractions
	subu	t2, ta2, t2			# subtract fractions
	subu	t2, t2, v0			# subtract barrow
	b	norm_d

/*
 * Single precision multiply.
 */
mul_s:
	jal	_C_LABEL(get_ft_fs_s)
	xor	t0, t0, ta0			# compute sign of result
	move	ta0, t0
	bne	t1, SEXP_INF, 2f		# is FS an infinity?
	bne	t2, zero, result_fs_s		# if FS is a NAN, result=FS
	bne	ta1, SEXP_INF, 1f		# FS is inf, is FT an infinity?
	bne	ta2, zero, result_ft_s		# if FT is a NAN, result=FT
	b	result_fs_s			# result is infinity
1:
	bne	ta1, zero, result_fs_s		# inf * zero? if no, result=FS
	bne	ta2, zero, result_fs_s
	b	invalid_s			# infinity * zero is invalid
2:
	bne	ta1, SEXP_INF, 1f		# FS != inf, is FT an infinity?
	bne	t1, zero, result_ft_s		# zero * inf? if no, result=FT
	bne	t2, zero, result_ft_s
	bne	ta2, zero, result_ft_s		# if FT is a NAN, result=FT
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
	bne	ta1, zero, 1f			# is FT zero?
	beq	ta2, zero, result_ft_s		# result is zero
	jal	_C_LABEL(renorm_ft_s)
	b	2f
1:
	subu	ta1, ta1, SEXP_BIAS		# unbias FT exponent
	or	ta2, ta2, SIMPL_ONE		# set implied one bit
2:
	addu	t1, t1, ta1			# compute result exponent
	addu	t1, t1, 9			# account for binary point
	multu	t2, ta2				# multiply fractions
	mflo	t9
	mfhi	t2
	b	norm_s

/*
 * Double precision multiply.
 */
mul_d:
	jal	_C_LABEL(get_ft_fs_d)
	xor	t0, t0, ta0			# compute sign of result
	move	ta0, t0
	bne	t1, DEXP_INF, 2f		# is FS an infinity?
	bne	t2, zero, result_fs_d		# if FS is a NAN, result=FS
	bne	t3, zero, result_fs_d
	bne	ta1, DEXP_INF, 1f		# FS is inf, is FT an infinity?
	bne	ta2, zero, result_ft_d		# if FT is a NAN, result=FT
	bne	ta3, zero, result_ft_d
	b	result_fs_d			# result is infinity
1:
	bne	ta1, zero, result_fs_d		# inf * zero? if no, result=FS
	bne	ta2, zero, result_fs_d
	bne	ta3, zero, result_fs_d
	b	invalid_d			# infinity * zero is invalid
2:
	bne	ta1, DEXP_INF, 1f		# FS != inf, is FT an infinity?
	bne	t1, zero, result_ft_d		# zero * inf? if no, result=FT
	bne	t2, zero, result_ft_d		# if FS is a NAN, result=FS
	bne	t3, zero, result_ft_d
	bne	ta2, zero, result_ft_d		# if FT is a NAN, result=FT
	bne	ta3, zero, result_ft_d
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
	bne	ta1, zero, 2f			# is FT zero?
	bne	ta2, zero, 1f
	beq	ta3, zero, result_ft_d		# result is zero
1:
	jal	_C_LABEL(renorm_ft_d)
	b	3f
2:
	subu	ta1, ta1, DEXP_BIAS		# unbias FT exponent
	or	ta2, ta2, DIMPL_ONE		# set implied one bit
3:
	addu	t1, t1, ta1			# compute result exponent
	addu	t1, t1, 12			# ???
	multu	t3, ta3				# multiply fractions (low * low)
	move	ta0, t2				# free up t2,t3 for result
	move	ta1, t3
	mflo	a3				# save low order bits
	mfhi	t9
	not	v0, t9
	multu	ta0, ta3				# multiply FS(high) * FT(low)
	mflo	v1
	mfhi	t3				# init low result
	sltu	v0, v0, v1			# compute carry
	addu	t9, v1
	multu	ta1, ta2				# multiply FS(low) * FT(high)
	addu	t3, t3, v0			# add carry
	not	v0, t9
	mflo	v1
	mfhi	t2
	sltu	v0, v0, v1
	addu	t9, v1
	multu	ta0, ta2				# multiply FS(high) * FT(high)
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
	sltu	a3, zero, a3			# reduce t9,a3 to just t9
	or	t9, a3
	b	norm_d

/*
 * Single precision divide.
 */
div_s:
	jal	_C_LABEL(get_ft_fs_s)
	xor	t0, t0, ta0			# compute sign of result
	move	ta0, t0
	bne	t1, SEXP_INF, 1f		# is FS an infinity?
	bne	t2, zero, result_fs_s		# if FS is NAN, result is FS
	bne	ta1, SEXP_INF, result_fs_s	# is FT an infinity?
	bne	ta2, zero, result_ft_s		# if FT is NAN, result is FT
	b	invalid_s			# infinity/infinity is invalid
1:
	bne	ta1, SEXP_INF, 1f		# is FT an infinity?
	bne	ta2, zero, result_ft_s		# if FT is NAN, result is FT
	move	t1, zero			# x / infinity is zero
	move	t2, zero
	b	result_fs_s
1:
	bne	t1, zero, 2f			# is FS zero?
	bne	t2, zero, 1f
	bne	ta1, zero, result_fs_s		# FS=zero, is FT zero?
	beq	ta2, zero, invalid_s		# 0 / 0
	b	result_fs_s			# result = zero
1:
	jal	_C_LABEL(renorm_fs_s)
	b	3f
2:
	subu	t1, t1, SEXP_BIAS		# unbias FS exponent
	or	t2, t2, SIMPL_ONE		# set implied one bit
3:
	bne	ta1, zero, 2f			# is FT zero?
	bne	ta2, zero, 1f
	or	a2, a2, MIPS_FPU_EXCEPTION_DIV0 | MIPS_FPU_STICKY_DIV0
	and	v0, a2, MIPS_FPU_ENABLE_DIV0	# trap enabled?
	bne	v0, zero, fpe_trap
#ifdef FPEMUL
	PTR_L	t1, L_PCB(MIPS_CURLWP)		# get pcb of current lwp
	#nop
	INT_S	a2, PCB_FPREGS+FRAME_FSR(t1)
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
	subu	ta1, ta1, SEXP_BIAS		# unbias FT exponent
	or	ta2, ta2, SIMPL_ONE		# set implied one bit
3:
	subu	t1, t1, ta1			# compute exponent
	subu	t1, t1, 3			# compensate for result position
	li	v0, SFRAC_BITS+3		# number of bits to divide
	move	t9, t2				# init dividend
	move	t2, zero			# init result
1:
	bltu	t9, ta2, 3f			# is dividend >= divisor?
2:
	subu	t9, t9, ta2			# subtract divisor from dividend
	or	t2, t2, 1			# remember that we did
	bne	t9, zero, 3f			# if not done, continue
	sll	t2, t2, v0			# shift result to final position
	b	norm_s
3:
	sll	t9, t9, 1			# shift dividend
	sll	t2, t2, 1			# shift result
	subu	v0, v0, 1			# are we done?
	bne	v0, zero, 1b			# no, continue
	b	norm_s

/*
 * Double precision divide.
 */
div_d:
	jal	_C_LABEL(get_ft_fs_d)
	xor	t0, t0, ta0			# compute sign of result
	move	ta0, t0
	bne	t1, DEXP_INF, 1f		# is FS an infinity?
	bne	t2, zero, result_fs_d		# if FS is NAN, result is FS
	bne	t3, zero, result_fs_d
	bne	ta1, DEXP_INF, result_fs_d	# is FT an infinity?
	bne	ta2, zero, result_ft_d		# if FT is NAN, result is FT
	bne	ta3, zero, result_ft_d
	b	invalid_d			# infinity/infinity is invalid
1:
	bne	ta1, DEXP_INF, 1f		# is FT an infinity?
	bne	ta2, zero, result_ft_d		# if FT is NAN, result is FT
	bne	ta3, zero, result_ft_d
	move	t1, zero			# x / infinity is zero
	move	t2, zero
	move	t3, zero
	b	result_fs_d
1:
	bne	t1, zero, 2f			# is FS zero?
	bne	t2, zero, 1f
	bne	t3, zero, 1f
	bne	ta1, zero, result_fs_d		# FS=zero, is FT zero?
	bne	ta2, zero, result_fs_d
	beq	ta3, zero, invalid_d		# 0 / 0
	b	result_fs_d			# result = zero
1:
	jal	_C_LABEL(renorm_fs_d)
	b	3f
2:
	subu	t1, t1, DEXP_BIAS		# unbias FS exponent
	or	t2, t2, DIMPL_ONE		# set implied one bit
3:
	bne	ta1, zero, 2f			# is FT zero?
	bne	ta2, zero, 1f
	bne	ta3, zero, 1f
	or	a2, a2, MIPS_FPU_EXCEPTION_DIV0 | MIPS_FPU_STICKY_DIV0
	and	v0, a2, MIPS_FPU_ENABLE_DIV0	# trap enabled?
	bne	v0, zero, fpe_trap
#ifdef FPEMUL
	PTR_L	t1, L_PCB(MIPS_CURLWP)		# get pcb of current lwp
	#nop
	INT_S	a2, PCB_FPREGS+FRAME_FSR(t1)
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
	subu	ta1, ta1, DEXP_BIAS		# unbias FT exponent
	or	ta2, ta2, DIMPL_ONE		# set implied one bit
3:
	subu	t1, t1, ta1			# compute exponent
	subu	t1, t1, 3			# compensate for result position
	li	v0, DFRAC_BITS+3		# number of bits to divide
	move	t9, t2				# init dividend
	move	v1, t3
	move	t2, zero			# init result
	move	t3, zero
1:
	bltu	t9, ta2, 3f			# is dividend >= divisor?
	bne	t9, ta2, 2f
	bltu	v1, ta3, 3f
2:
	.set	noat
	sltu	AT, v1, ta3			# subtract divisor from dividend
	subu	v1, v1, ta3
	subu	t9, t9, ta2
	subu	t9, t9, AT
	.set	at
	or	t3, t3, 1			# remember that we did
	bne	t9, zero, 3f			# if not done, continue
	bne	v1, zero, 3f
	li	v1, 32				# shift result to final position
	blt	v0, v1, 2f			# shift < 32 bits?
	subu	v0, v0, v1			# shift by > 32 bits
	sll	t2, t3, v0			# shift upper part
	move	t3, zero
	b	norm_d
2:
	.set	noat
	subu	v1, v1, v0			# shift by < 32 bits
	sll	t2, t2, v0			# shift upper part
	srl	AT, t3, v1			# save bits shifted out
	or	t2, t2, AT			# and put into upper part
	sll	t3, t3, v0
	b	norm_d
	.set	at
3:
	.set	noat
	sll	t9, t9, 1			# shift dividend
	srl	AT, v1, 31			# save bit shifted out
	or	t9, t9, AT			# and put into upper part
	sll	v1, v1, 1
	sll	t2, t2, 1			# shift result
	srl	AT, t3, 31			# save bit shifted out
	or	t2, t2, AT			# and put into upper part
	sll	t3, t3, 1
	subu	v0, v0, 1			# are we done?
	bne	v0, zero, 1b			# no, continue
	sltu	v0, zero, v1			# be sure to save any one bits
	or	t9, t9, v0			# from the lower remainder
	b	norm_d
	.set	at

#ifdef MIPS3_PLUS
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
	move	ta0, zero			# q = 0; (result)
	li	t9, SIMPL_ONE<<1		# r = 0x01000000;
	move	ta2, zero			# s = 0;
1:	beq	t9, zero, 3f			# while (r != 0) {
	add	v0, ta2, t9			#	t = s + r;
	bgt	v0, t2, 2f			#	if (t <= ix)
	add	ta2, v0, t9			#		s = t + r;
	sub	t2, t2, v0			#		ix -= t;
	add	ta0, ta0, t9			#		q += r;
2:	add	t2, t2, t2			#	ix += ix;
	srl	t9, t9, 1			# 	r >>= 1;
	b	1b				# }
3:
	/* rounding -- all mips rounding modes use the same rounding here */
	beq	t2, zero, 1f			# if (ix != 0)
	and	v0, ta0, 1			# q += q&1;
	add	ta0, ta0, v0			# ...

	/* calculate result */
1:	srl	t2, ta0, 1			# ix = (q >> 1);
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

	move	ta0, zero			# q = 0;	(result)
	move	ta1, zero			# q1 = 0;	(result)
	move	ta2, zero			# s0 = 0;
	move	ta3, zero			# s1 = 0;
	li	t9, DIMPL_ONE<<1		# t = 0x00200000;
1:	beq	t9, zero, 3f			# while (r != 0) {
	add	v0, ta2, t9			#	t = s0+r;
	bgt	v0, t2, 2f			#	if (t <= ix0)
	add	ta2, v0, t9			#		s0 = t + r;
	sub	t2, t2, v0			#		ix0 -= t;
	add	ta0, ta0, t9			#		q += r;
2:	add	t2, t2, t2			#	ix0 += ix0;
	srl	v0, t3, 31			# 	ix0 += (ix1&sign)>>31)
	and	v0, v0, 1			# 	...
	add	t2, t2, v0			# 	...
	addu	t3, t3, t3			#	ix1 += ix1;
	srl	t9, t9, 1			#	r >>= 1;
	b	1b				# }
3:
	/* then lower bits */
	li	t9, 1<<31			# r = sign;
1:	beq	t9, zero, 4f			# while (r != 0) {
	addu	v1, ta3, t9			#    t1 = s1 + r;
	move	v0, ta2				#    t = s0;
	blt	v0, t2, 2f			#    if ( (t<ix0) ||
	bne	v0, t2, 3f			#         ((t == ix0) &&
	bgtu	v1, t3, 3f			#          (t1 <= ix1)))
2:	addu	ta3, v1, t9			#	s1 = t1 + r;
	.set	noat
	srl	AT, v1, 31			#	if (((t1&sign)==sign) &&
	and	AT, AT, 1			#	...
	beq	AT, zero, 2f			#	...
	srl	AT, ta3, 31			#	    (s1&sign) == 0)
	and	AT, AT, 1			#	    ...
	bne	AT, zero, 2f			#	    ...
	add	ta2, ta2, 1			#	    s0 += 1;
	.set	at
2:	sub	t2, t2, v0			#	ix0 -= t;
	bgeu	t3, v1, 2f			#	if (ix1 < t1)
	sub	t2, t2, 1			#	    ix0 -= 1;
2:	subu	t3, t3, v1			#	ix1 -= t1;
	addu	ta1, ta1, t9			#	q1 += r;
3:	add	t2, t2, t2			#    ix0 += ix0;
	srl	v0, t3, 31			#    ix0 += (ix1&sign)>>31)
	and	v0, v0, 1			#    ...
	add	t2, t2, v0			#    ...
	addu	t3, t3, t3			#    ix1 += ix1;
	srl	t9, t9, 1			#    r >>= 1;
	b	1b				# }
4:

	/* rounding -- all mips rounding modes use the same rounding here */
	or	v0, t2, t3			# if (ix0 | ix1)
	beq	v0, zero, 2f			# ...
	li	v0, 0xffffffff			#    if (q1 == 0xffffffff)
	and	v1, t2, v0			#    ...
	bne	v1, v0, 1f			#    ...
	move	ta1, zero			#	q1 = 0;
	add	ta0, ta0, 1			#	q += 1;
	b	2f				#    else
1:	and	v0, ta1, 1			#       q1 += q1 & 1;
	addu	ta1, ta1, v0			#       ...

	/* calculate result */
2:	srl	t2, ta0, 1			# ix0 = q >> 1;
	srl	t3, ta1, 1			# ix1 = q1 >> 1;
	and	v0, ta0, 1			# if ((q & 1) == 1)
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
#endif	/* MIPS3_PLUS */

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

#ifdef MIPS3_PLUS
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
#endif /* MIPS3_PLUS */

/*
 * Convert double to single.
 */
cvt_s_d:
	jal	_C_LABEL(get_fs_d)
	bne	t1, DEXP_INF, 1f		# is FS an infinity?
	li	t1, SEXP_INF			# convert to single
	sll	t2, t2, 3			# convert D fraction to S
	srl	t9, t3, 32 - 3
	or	t2, t2, t9
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
	srl	t9, t3, 32 - 3
	or	t2, t2, t9
	sll	t9, t3, 3
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
 * Find out how many leading zero bits are in t2 and put in v1.
 */
#if __mips == 32 || __mips == 64
	clz	v1, t2
#else
	.set	noat
1:
	move	v0, t2
	move	v1, zero
	srl	AT, v0, 16
	bne	AT, zero, 1f
	addu	v1, 16
	sll	v0, 16
1:
	srl	AT, v0, 24
	bne	AT, zero, 1f
	addu	v1, 8
	sll	v0, 8
1:
	srl	AT, v0, 28
	bne	AT, zero, 1f
	addu	v1, 4
	sll	v0, 4
1:
	srl	AT, v0, 30
	bne	AT, zero, 1f
	addu	v1, 2
	sll	v0, 2
1:
	srl	AT, v0, 31
	bne	AT, zero, 1f
	addu	v1, 1
	.set	at
#endif /* __mips == 32 || __mips == 64 */
/*
 * Now shift t2 the correct number of bits.
 */
1:
	subu	v1, v1, SLEAD_ZEROS		# dont count leading zeros
	li	t1, 23				# init exponent
	subu	t1, t1, v1			# compute exponent
	beq	v1, zero, 1f
	li	v0, 32
	blt	v1, zero, 2f			# if shift < 0, shift right
	subu	v0, v0, v1
	sll	t2, t2, v1			# shift left
1:
	add	t1, t1, SEXP_BIAS		# bias exponent
	and	t2, t2, ~SIMPL_ONE		# clear implied one bit
	b	result_fs_s
2:
	negu	v1				# shift right by v1
	subu	v0, v0, v1
	sll	t9, t2, v0			# save bits shifted out
	srl	t2, t2, v1
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
	move	t9, zero
	sll	t3, t2, 32 - 3			# convert S fraction to D
	srl	t2, t2, 3
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
 * Find out how many leading zero bits are in t2 and put in at.
 */
#if __mips == 32 || __mips == 64
	clz	v1, t2
#else /* __mips == 32 || __mips == 64 */
	.set	noat
1:
	move	v0, t2
	move	v1, zero
	srl	AT, v0, 16
	bne	AT, zero, 1f
	addu	v1, 16
	sll	v0, 16
1:
	srl	AT, v0, 24
	bne	AT, zero, 1f
	addu	v1, 8
	sll	v0, 8
1:
	srl	AT, v0, 28
	bne	AT, zero, 1f
	addu	v1, 4
	sll	v0, 4
1:
	srl	AT, v0, 30
	bne	AT, zero, 1f
	addu	v1, 2
	sll	v0, 2
1:
	srl	AT, v0, 31
	bne	AT, zero, 1f
	addu	v1, 1
1:
	.set	at
#endif /* __mips == 32 || __mips == 64 */
/*
 * Now shift t2 the correct number of bits.
 */
	subu	v1, v1, DLEAD_ZEROS		# dont count leading zeros
	li	t1, DEXP_BIAS + 20		# init exponent
	subu	t1, t1, v1			# compute exponent
	beq	v1, zero, 1f
	li	v0, 32
	blt	v1, zero, 2f			# if shift < 0, shift right
	subu	v0, v0, v1
	sll	t2, t2, v1			# shift left
1:
	and	t2, t2, ~DIMPL_ONE		# clear implied one bit
	move	t3, zero
	b	result_fs_d
2:
	negu	v1				# shift right by v1
	subu	v0, v0, v1
	sll	t3, t2, v0
	srl	t2, t2, v1
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
	srl	v1, t3, v1			# save bits shifted out of t3
	or	t2, t2, v1			# and put into t2
	sll	t3, t3, v0			# shift FSs fraction
	b	2f
1:
	negu	v0				# shift right by v0
	subu	v1, v1, v0
	sll	t9, t3, v1			# save bits shifted out
	sltu	t9, zero, t9			# dont lose any ones
	srl	t3, t3, v0			# shift FSs fraction
	or	t3, t3, t9
	sll	v1, t2, v1			# save bits shifted out of t2
	or	t3, t3, v1			# and put into t3
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
#ifdef FPEMUL
	PTR_L	v0, L_PCB(MIPS_CURLWP)		# get pcb of current lwp
	#nop
	INT_S	a2, PCB_FPREGS+FRAME_FSR(v0)
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
	bne	ta1, SEXP_INF, 2f		# is FT an infinity?
	bne	ta2, zero, unordered		# FT is a NAN
2:
	sll	t1, t1, 23			# reassemble exp & frac
	or	t1, t1, t2
	sll	ta1, ta1, 23			# reassemble exp & frac
	or	ta1, ta1, ta2
	beq	t0, zero, 1f			# is FS positive?
	negu	t1
1:
	beq	ta0, zero, 1f			# is FT positive?
	negu	ta1
1:
	li	v0, COND_LESS
	blt	t1, ta1, test_cond		# is FS < FT?
	li	v0, COND_EQUAL
	beq	t1, ta1, test_cond		# is FS == FT?
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
	bne	ta1, DEXP_INF, 2f		# is FT an infinity?
	bne	ta2, zero, unordered
	bne	ta3, zero, unordered		# FT is a NAN
2:
	sll	t1, t1, 20			# reassemble exp & frac
	or	t1, t1, t2
	sll	ta1, ta1, 20			# reassemble exp & frac
	or	ta1, ta1, ta2
	beq	t0, zero, 1f			# is FS positive?
	not	t3				# negate t1,t3
	not	t1
	addu	t3, t3, 1
	seq	v0, t3, zero			# compute carry
	addu	t1, t1, v0
1:
	beq	ta0, zero, 1f			# is FT positive?
	not	ta3				# negate ta1,ta3
	not	ta1
	addu	ta3, ta3, 1
	seq	v0, ta3, zero			# compute carry
	addu	ta1, ta1, v0
1:
	li	v0, COND_LESS
	blt	t1, ta1, test_cond		# is FS(MSW) < FT(MSW)?
	move	v0, zero
	bne	t1, ta1, test_cond		# is FS(MSW) > FT(MSW)?
	li	v0, COND_LESS
	bltu	t3, ta3, test_cond		# is FS(LSW) < FT(LSW)?
	li	v0, COND_EQUAL
	beq	t3, ta3, test_cond		# is FS(LSW) == FT(LSW)?
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
#ifdef FPEMUL
	PTR_L	v0, L_PCB(MIPS_CURLWP)		# get pcb of current lwp
	#nop
	INT_S	a2, PCB_FPREGS+FRAME_FSR(v0)
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
#ifdef FPEMUL
	PTR_L	v0, L_PCB(MIPS_CURLWP)		# get pcb of current lwp
	#nop
	INT_S	a2, PCB_FPREGS+FRAME_FSR(v0)
#else
	ctc1	a2, MIPS_FPU_CSR		# save condition bit
#endif
	b	done

/*
 * Determine the amount to shift the fraction in order to restore the
 * normalized position. After that, round and handle exceptions.
 */
norm_s:
#if __mips == 32 || __mips == 64
#ifdef __mips_o32
	bne	t2, zero, 1f
	clz	v1, t9
	addu	v1, 32
	b	2f
1:
	clz	v1, t2
2:
#elif __mips_isa_rev == 2
	move	v0, t9
	dins	v0, t2, 32, 32
	dclz	v1, v0
#else
	dsll	v0, t9, 32
	dsrl	v0, v0, 32
	dsll	v1, t2, 32
	or	v0, v1
	dclz	v1, v0
#endif
#else
	.set	noat
	move	v0, t2				# MSW
	move	v1, zero			# v1 = num of leading zeros
	bne	t2, zero, 1f
	move	v0, t9				# LSW
	addu	v1, 32
1:
	srl	AT, v0, 16
	bne	AT, zero, 1f
	addu	v1, 16
	sll	v0, 16
1:
	srl	AT, v0, 24
	bne	AT, zero, 1f
	addu	v1, 8
	sll	v0, 8
1:
	srl	AT, v0, 28
	bne	AT, zero, 1f
	addu	v1, 4
	sll	v0, 4
1:
	srl	AT, v0, 30
	bne	AT, zero, 1f
	addu	v1, 2
	sll	v0, 2
1:
	srl	AT, v0, 31
	bne	AT, zero, 1f
	addu	v1, 1
2:
	.set	at
#endif	/* __mips == 32 || __mips == 64 */
/*
 * Now shift t2,t9 the correct number of bits.
 */
	subu	v1, v1, SLEAD_ZEROS		# dont count leading zeros
	subu	t1, t1, v1			# adjust the exponent
	beq	v1, zero, norm_noshift_s
	li	ta1, 32
	blt	v1, zero, 1f			# if shift < 0, shift right
	subu	ta1, ta1, v1
	sll	t2, t2, v1			# shift t2,t9 left
	srl	v0, t9, ta1			# save bits shifted out
	or	t2, t2, v0
	sll	t9, t9, v1
	b	norm_noshift_s
1:
	negu	v1				# shift t2,t9 right by at
	subu	ta1, ta1, v1
	sll	v0, t9, ta1			# save bits shifted out
	sltu	v0, zero, v0			# be sure to save any one bits
	srl	t9, t9, v1
	or	t9, t9, v0
	sll	v0, t2, ta1			# save bits shifted out
	or	t9, t9, v0
	srl	t2, t2, v1
norm_noshift_s:
	move	ta1, t1				# save unrounded exponent
	move	ta2, t2				# save unrounded fraction
	and	v0, a2, MIPS_FPU_ROUNDING_BITS	# get rounding mode
	beq	v0, MIPS_FPU_ROUND_RN, 3f	# round to nearest
	beq	v0, MIPS_FPU_ROUND_RZ, 5f	# round to zero (truncate)
	beq	v0, MIPS_FPU_ROUND_RP, 1f	# round to +infinity
	beq	t0, zero, 5f			# if sign is positive, truncate
	b	2f
1:
	bne	t0, zero, 5f			# if sign is negative, truncate
2:
	beq	t9, zero, 5f			# if exact, continue
	addu	t2, t2, 1			# add rounding bit
	bne	t2, SIMPL_ONE<<1, 5f		# need to adjust exponent?
	addu	t1, t1, 1			# adjust exponent
	srl	t2, t2, 1			# renormalize fraction
	b	5f
3:
	li	v0, GUARDBIT			# load guard bit for rounding
	addu	v0, v0, t9			# add remainder
	sltu	v1, v0, t9			# compute carry out
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
	bne	t9, zero, inexact_s		# is result inexact?
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
#ifdef FPEMUL
	PTR_L	v0, L_PCB(MIPS_CURLWP)		# get pcb of current lwp
	#nop
	INT_S	a2, PCB_FPREGS+FRAME_FSR(v0)
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
	move	t1, ta1				# get unrounded exponent
	move	t2, ta2				# get unrounded fraction
	li	v0, SEXP_MIN			# compute shift amount
	subu	v0, v0, t1			# shift t2,t9 right by at
	blt	v0, SFRAC_BITS+2, 3f		# shift all the bits out?
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
	.set	noat
	li	v1, 32
	subu	v1, v1, v0
	sltu	AT, zero, t9			# be sure to save any one bits
	sll	t9, t2, v1			# save bits shifted out
	or	t9, t9, AT			# include sticky bits
	srl	t2, t2, v0
	.set	at
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
	beq	t9, zero, 5f			# if exact, continue
	addu	t2, t2, 1			# add rounding bit
	b	5f
3:
	li	v0, GUARDBIT			# load guard bit for rounding
	addu	v0, v0, t9			# add remainder
	sltu	v1, v0, t9			# compute carry out
	beq	v1, zero, 4f			# if no carry, continue
	addu	t2, t2, 1			# add carry to result
4:
	bne	v0, zero, 5f			# if rounded remainder is zero
	and	t2, t2, ~1			#  clear LSB (round to nearest)
5:
	move	t1, zero			# denorm or zero exponent
	jal	_C_LABEL(set_fd_s)		# save result
	beq	t9, zero, done			# check for exact result
	or	a2, a2, MIPS_FPU_EXCEPTION_UNDERFLOW | MIPS_FPU_STICKY_UNDERFLOW
	or	a2, a2, MIPS_FPU_EXCEPTION_INEXACT | MIPS_FPU_STICKY_INEXACT
	and	v0, a2, MIPS_FPU_ENABLE_INEXACT
	bne	v0, zero, fpe_trap
#ifdef FPEMUL
	PTR_L	v0, L_PCB(MIPS_CURLWP)		# get pcb of current lwp
	#nop
	INT_S	a2, PCB_FPREGS+FRAME_FSR(v0)
#else
	ctc1	a2, MIPS_FPU_CSR		# save exceptions
#endif
	b	done

/*
 * Determine the amount to shift the fraction in order to restore the
 * normalized position. After that, round and handle exceptions.
 */
norm_d:
#if __mips == 32 || __mips == 64
	bne	t2, zero, 2f
	bne	t3, zero, 1f
	clz	v1, t9
	addu	v1, 64
	b	3f
1:
	clz	v1, t3
	addu	v1, 32
	b	3f
2:
	clz	v1, t2
3:
#else
	.set	noat
	move	v0, t2
	move	v1, zero			# v1 = num of leading zeros
	bne	t2, zero, 1f
	move	v0, t3
	addu	v1, 32
	bne	t3, zero, 1f
	move	v0, t9
	addu	v1, 32
1:
	srl	AT, v0, 16
	bne	AT, zero, 1f
	addu	v1, 16
	sll	v0, 16
1:
	srl	AT, v0, 24
	bne	AT, zero, 1f
	addu	v1, 8
	sll	v0, 8
1:
	srl	AT, v0, 28
	bne	AT, zero, 1f
	addu	v1, 4
	sll	v0, 4
1:
	srl	AT, v0, 30
	bne	AT, zero, 1f
	addu	v1, 2
	sll	v0, 2
1:
	srl	AT, v0, 31
	bne	AT, zero, 1f
	addu	v1, 1
1:
	.set	at
#endif /* __mips_isa == 32 || __mips_isa == 64 */
/*
 * Now shift t2,t3,t9 the correct number of bits.
 */
	subu	v1, v1, DLEAD_ZEROS		# dont count leading zeros
	subu	t1, t1, v1			# adjust the exponent
	beq	v1, zero, norm_noshift_d

	li	ta1, 32
	blt	v1, zero, 2f			# if shift < 0, shift right
	blt	v1, ta1, 1f			# shift by < 32?
	subu	v1, v1, ta1			# shift by >= 32
	subu	ta1, ta1, v1
	sll	t2, t3, v1			# shift left by v1
	srl	v0, t9, ta1			# save bits shifted out
	or	t2, t2, v0
	sll	t3, t9, v1
	move	t9, zero
	b	norm_noshift_d
1:
	subu	ta1, ta1, v1
	sll	t2, t2, v1			# shift left by v1
	srl	v0, t3, ta1			# save bits shifted out
	or	t2, t2, v0
	sll	t3, t3, v1
	srl	v0, t9, ta1			# save bits shifted out
	or	t3, t3, v0
	sll	t9, t9, v1
	b	norm_noshift_d
2:
	negu	v1				# shift right by at
	subu	ta1, ta1, v1			#  (known to be < 32 bits)
	sll	v0, t9, ta1			# save bits shifted out
	sltu	v0, zero, v0			# be sure to save any one bits
	srl	t9, t9, v1
	or	t9, t9, v0
	sll	v0, t3, ta1			# save bits shifted out
	or	t9, t9, v0
	srl	t3, t3, v1
	sll	v0, t2, ta1			# save bits shifted out
	or	t3, t3, v0
	srl	t2, t2, v1
norm_noshift_d:
	move	ta1, t1				# save unrounded exponent
	move	ta2, t2				# save unrounded fraction (MS)
	move	ta3, t3				# save unrounded fraction (LS)
	and	v0, a2, MIPS_FPU_ROUNDING_BITS	# get rounding mode
	beq	v0, MIPS_FPU_ROUND_RN, 3f	# round to nearest
	beq	v0, MIPS_FPU_ROUND_RZ, 5f	# round to zero (truncate)
	beq	v0, MIPS_FPU_ROUND_RP, 1f	# round to +infinity
	beq	t0, zero, 5f			# if sign is positive, truncate
	b	2f
1:
	bne	t0, zero, 5f			# if sign is negative, truncate
2:
	beq	t9, zero, 5f			# if exact, continue
	addu	t3, t3, 1			# add rounding bit
	bne	t3, zero, 5f			# branch if no carry
	addu	t2, t2, 1			# add carry
	bne	t2, DIMPL_ONE<<1, 5f		# need to adjust exponent?
	addu	t1, t1, 1			# adjust exponent
	srl	t2, t2, 1			# renormalize fraction
	b	5f
3:
	li	v0, GUARDBIT			# load guard bit for rounding
	addu	v0, v0, t9			# add remainder
	sltu	v1, v0, t9			# compute carry out
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
	bne	t9, zero, inexact_d		# is result inexact?
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
#ifdef FPEMUL
	PTR_L	v0, L_PCB(MIPS_CURLWP)		# get pcb of current lwp
	#nop
	INT_S	a2, PCB_FPREGS+FRAME_FSR(v0)
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
	move	t1, ta1				# get unrounded exponent
	move	t2, ta2				# get unrounded fraction (MS)
	move	t3, ta3				# get unrounded fraction (LS)
	li	v0, DEXP_MIN			# compute shift amount
	subu	v0, v0, t1			# shift t2,t9 right by at
	blt	v0, DFRAC_BITS+2, 3f		# shift all the bits out?
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
	blt	v0, v1, 1f			# shift by < 32?
	subu	v0, v0, v1			# shift right by >= 32
	subu	v1, v1, v0
	.set	noat
	sltu	AT, zero, t9			# be sure to save any one bits
	sll	t9, t2, v1			# save bits shifted out
	or	t9, t9, AT			# include sticky bits
	srl	t3, t2, v0
	move	t2, zero
	.set	at
	b	2f
1:
	.set	noat
	subu	v1, v1, v0			# shift right by at
	sltu	AT, zero, t9			# be sure to save any one bits
	sll	t9, t3, v1			# save bits shifted out
	or	t9, t9, AT			# include sticky bits
	srl	t3, t3, v0
	sll	AT, t2, v1			# save bits shifted out
	or	t3, t3, AT
	srl	t2, t2, v0
	.set	at
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
	beq	t9, zero, 5f			# if exact, continue
	addu	t3, t3, 1			# add rounding bit
	bne	t3, zero, 5f			# if no carry, continue
	addu	t2, t2, 1			# add carry
	b	5f
3:
	li	v0, GUARDBIT			# load guard bit for rounding
	addu	v0, v0, t9			# add remainder
	sltu	v1, v0, t9			# compute carry out
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
	beq	t9, zero, done			# check for exact result
	or	a2, a2, MIPS_FPU_EXCEPTION_UNDERFLOW | MIPS_FPU_STICKY_UNDERFLOW
	or	a2, a2, MIPS_FPU_EXCEPTION_INEXACT | MIPS_FPU_STICKY_INEXACT
	and	v0, a2, MIPS_FPU_ENABLE_INEXACT
	bne	v0, zero, fpe_trap
#ifdef FPEMUL
	PTR_L	v0, L_PCB(MIPS_CURLWP)		# get pcb of current lwp
	#nop
	INT_S	a2, PCB_FPREGS+FRAME_FSR(v0)
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
#ifdef FPEMUL
	PTR_L	v0, L_PCB(MIPS_CURLWP)		# get pcb of current lwp
	#nop
	INT_S	a2, PCB_FPREGS+FRAME_FSR(v0)
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
#ifdef FPEMUL
	PTR_L	v0, L_PCB(MIPS_CURLWP)		# get pcb of current lwp
	#nop
	INT_S	a2, PCB_FPREGS+FRAME_FSR(v0)
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
#ifdef FPEMUL
	PTR_L	v0, L_PCB(MIPS_CURLWP)		# get pcb of current lwp
	#nop
	INT_S	a2, PCB_FPREGS+FRAME_FSR(v0)
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
#ifdef FPEMUL
	PTR_L	v0, L_PCB(MIPS_CURLWP)		# get pcb of current lwp
	#nop
	INT_S	a2, PCB_FPREGS+FRAME_FSR(v0)
#else
	/*
	 * ctc1 with fpe bits set causes FPE in kernel mode panic on 5231.
	 */
	REG_S	a2, CALLFRAME_SIZ + 3*SZREG(sp)
	jal	_C_LABEL(fpu_save)		# on RM5231

	REG_L	a2, CALLFRAME_SIZ + 3*SZREG(sp)

	PTR_L	v0, L_PCB(MIPS_CURLWP)		# get pcb of current lwp
	#nop
	INT_S	a2, PCB_FPREGS+FRAME_FSR(v0)
#endif
	move	a3, a2				# fpustat
	REG_L	a1, CALLFRAME_FRAME(sp)		# frame
	REG_L	a2, CALLFRAME_CAUSE(sp)		# cause
	REG_L	ra, CALLFRAME_RA(sp)
	PTR_ADDU sp, CALLFRAME_SIZ
	j	_C_LABEL(fpemul_sigfpe)

/*
 * Send an illegal instruction signal to the current lwp.
 */
ill:
#ifdef FPEMUL
	PTR_L	v0, L_PCB(MIPS_CURLWP)		# get pcb of current lwp
	#nop
	INT_S	a2, PCB_FPREGS+FRAME_FSR(v0)
#else
	ctc1	a2, MIPS_FPU_CSR		# save exceptions
#endif
	REG_L	a1, CALLFRAME_FRAME(sp)		# frame
	REG_L	a2, CALLFRAME_CAUSE(sp)		# cause
	REG_L	ra, CALLFRAME_RA(sp)
	PTR_ADDU sp, CALLFRAME_SIZ
	j	_C_LABEL(fpemul_sigill)

result_ft_s:
	move	t0, ta0				# result is FT
	move	t1, ta1
	move	t2, ta2
result_fs_s:					# result is FS
	jal	_C_LABEL(set_fd_s)		# save result (in t0,t1,t2)
	b	done

result_fs_w:
	jal	_C_LABEL(set_fd_word)		# save result (in t2)
	b	done

result_ft_d:
	move	t0, ta0				# result is FT
	move	t1, ta1
	move	t2, ta2
	move	t3, ta3
result_fs_d:					# result is FS
	jal	_C_LABEL(set_fd_d)		# save result (in t0,t1,t2,t3)

done:
/*
 * Succeeded to emulate instruction with no error
 * so compute the next PC.
 */
	REG_L	t0, CALLFRAME_CAUSE(sp)
	REG_PROLOGUE
	REG_L	v0, TF_REG_EPC(a1)
	REG_EPILOGUE
	bgez	t0, 1f				# Check the branch delay bit.
/*
 * The instruction is in the branch delay slot so the branch will have to
 * be emulated to get the resulting PC.
 */
	REG_S	a1, CALLFRAME_FRAME(sp)
	move	a0, a1				# 1st arg is p. to trapframe
	move	a1, v0				# 2nd arg is instruction PC
						# 3rd arg is FP CSR
	move	a3, zero			# 4th arg is FALSE
	jal	_C_LABEL(mips_emul_branch)	# compute PC after branch

	REG_L	a1, CALLFRAME_FRAME(sp)
	b	2f
/*
 * This is not in the branch delay slot so calculate the resulting
 * PC (epc + 4) into v0.
 */
1:
	addiu	v0, v0, 4			# v0 = next pc
2:
	REG_PROLOGUE
	REG_S	v0, TF_REG_EPC(a1)		# save new pc
	REG_EPILOGUE

	REG_L	ra, CALLFRAME_RA(sp)
	PTR_ADDU sp, CALLFRAME_SIZ
	j	ra
END(mips_emul_fp)

/*----------------------------------------------------------------------------
 * get_fs_int --
 *
 *	Read (integer) the FS register (bits 15-11).
 *	This is an internal routine used by mips_emul_fp only.
 *
 * Results:
 *	t0	contains the sign
 *	t2	contains the fraction
 *
 *----------------------------------------------------------------------------
 */
STATIC_LEAF(get_fs_int)
#ifdef FPEMUL
	srl	t2, a0, 11 - FPX_SCALESHIFT
	PTR_L	t0, L_PCB(MIPS_CURLWP)		# get pcb of current lwp
	andi	t2, t2, FPX_REGEVENMASK		# Even regs only
	PTR_ADDU t0, t0, t2

	lw	t2, PCB_FPREGS+FRAME_FP0(t0)

	srl	t0, t2, 31		# init the sign bit
	bge	t2, zero, 1f
	negu	t2
1:
	j	ra
#else
	srl	a3, a0, 11 - (PTR_SCALESHIFT-1)	# get FS field (even regs only)
	and	a3, a3, 0xf << PTR_SCALESHIFT	# mask FS field
	PTR_L	a3, get_fs_int_tbl(a3)		# switch on register number
	j	a3

	.rdata
get_fs_int_tbl:
	PTR_WORD get_fs_int_f0
	PTR_WORD get_fs_int_f2
	PTR_WORD get_fs_int_f4
	PTR_WORD get_fs_int_f6
	PTR_WORD get_fs_int_f8
	PTR_WORD get_fs_int_f10
	PTR_WORD get_fs_int_f12
	PTR_WORD get_fs_int_f14
	PTR_WORD get_fs_int_f16
	PTR_WORD get_fs_int_f18
	PTR_WORD get_fs_int_f20
	PTR_WORD get_fs_int_f22
	PTR_WORD get_fs_int_f24
	PTR_WORD get_fs_int_f26
	PTR_WORD get_fs_int_f28
	PTR_WORD get_fs_int_f30
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
 *	This is an internal routine used by mips_emul_fp only.
 *
 * Results:
 *	t0	contains the FS sign
 *	t1	contains the FS (biased) exponent
 *	t2	contains the FS fraction
 *	ta0	contains the FT sign
 *	ta1	contains the FT (biased) exponent
 *	ta2	contains the FT fraction
 *
 *----------------------------------------------------------------------------
 */
STATIC_LEAF(get_ft_fs_s)
#ifdef FPEMUL
	srl	ta0, a0, 16 - FPX_SCALESHIFT
	PTR_L	ta1, L_PCB(MIPS_CURLWP)		# get pcb of current lwp
	andi	ta0, ta0, FPX_REGEVENMASK	# Even regs only
	PTR_ADDU ta1, ta1, ta0

	lw	ta0, PCB_FPREGS+FRAME_FP0(ta1)

	srl	ta1, ta0, 23			# get exponent
	and	ta1, ta1, 0xFF
	and	ta2, ta0, 0x7FFFFF		# get fraction
	srl	ta0, ta0, 31			# get sign
	bne	ta1, SEXP_INF, 1f		# is it a signaling NAN?
	and	v0, ta2, SSIGNAL_NAN
	bne	v0, zero, invalid_s
1:
	/* fall through to get FS */
#else
	srl	a3, a0, 16 - (PTR_SCALESHIFT - 1)# get FT field (even regs only)
	and	a3, a3, 0xF << PTR_SCALESHIFT	# mask FT field
	PTR_L	a3, get_ft_s_tbl(a3)		# switch on register number
	j	a3

	.rdata
get_ft_s_tbl:
	PTR_WORD get_ft_s_f0
	PTR_WORD get_ft_s_f2
	PTR_WORD get_ft_s_f4
	PTR_WORD get_ft_s_f6
	PTR_WORD get_ft_s_f8
	PTR_WORD get_ft_s_f10
	PTR_WORD get_ft_s_f12
	PTR_WORD get_ft_s_f14
	PTR_WORD get_ft_s_f16
	PTR_WORD get_ft_s_f18
	PTR_WORD get_ft_s_f20
	PTR_WORD get_ft_s_f22
	PTR_WORD get_ft_s_f24
	PTR_WORD get_ft_s_f26
	PTR_WORD get_ft_s_f28
	PTR_WORD get_ft_s_f30
	.text

get_ft_s_f0:
	mfc1	ta0, $f0
	b	get_ft_s_done
get_ft_s_f2:
	mfc1	ta0, $f2
	b	get_ft_s_done
get_ft_s_f4:
	mfc1	ta0, $f4
	b	get_ft_s_done
get_ft_s_f6:
	mfc1	ta0, $f6
	b	get_ft_s_done
get_ft_s_f8:
	mfc1	ta0, $f8
	b	get_ft_s_done
get_ft_s_f10:
	mfc1	ta0, $f10
	b	get_ft_s_done
get_ft_s_f12:
	mfc1	ta0, $f12
	b	get_ft_s_done
get_ft_s_f14:
	mfc1	ta0, $f14
	b	get_ft_s_done
get_ft_s_f16:
	mfc1	ta0, $f16
	b	get_ft_s_done
get_ft_s_f18:
	mfc1	ta0, $f18
	b	get_ft_s_done
get_ft_s_f20:
	mfc1	ta0, $f20
	b	get_ft_s_done
get_ft_s_f22:
	mfc1	ta0, $f22
	b	get_ft_s_done
get_ft_s_f24:
	mfc1	ta0, $f24
	b	get_ft_s_done
get_ft_s_f26:
	mfc1	ta0, $f26
	b	get_ft_s_done
get_ft_s_f28:
	mfc1	ta0, $f28
	b	get_ft_s_done
get_ft_s_f30:
	mfc1	ta0, $f30
get_ft_s_done:
	srl	ta1, ta0, 23			# get exponent
	and	ta1, ta1, 0xFF
	and	ta2, ta0, 0x7FFFFF		# get fraction
	srl	ta0, ta0, 31			# get sign
	bne	ta1, SEXP_INF, 1f		# is it a signaling NAN?
	and	v0, ta2, SSIGNAL_NAN
	bne	v0, zero, invalid_s
1:
	/* fall through to get FS */
#endif

/*----------------------------------------------------------------------------
 * get_fs_s --
 *
 *	Read (single precision) the FS register (bits 15-11) and
 *	break up into fields.
 *	This is an internal routine used by mips_emul_fp only.
 *
 * Results:
 *	t0	contains the sign
 *	t1	contains the (biased) exponent
 *	t2	contains the fraction
 *
 *----------------------------------------------------------------------------
 */
STATIC_XLEAF(get_fs_s)
#ifdef FPEMUL
	srl	t0, a0, 11 - FPX_SCALESHIFT
	PTR_L	t1, L_PCB(MIPS_CURLWP)		# get pcb of current lwp
	andi	t0, t0, FPX_REGEVENMASK		# Even regs only
	PTR_ADDU t1, t1, t0

	lw	t0, PCB_FPREGS+FRAME_FP0(t1)

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
	srl	a3, a0, 11 - (PTR_SCALESHIFT-1)	# get FS field (even regs only)
	and	a3, a3, 0xF << PTR_SCALESHIFT	# mask FS field
	PTR_L	a3, get_fs_s_tbl(a3)		# switch on register number
	j	a3

	.rdata
get_fs_s_tbl:
	PTR_WORD get_fs_s_f0
	PTR_WORD get_fs_s_f2
	PTR_WORD get_fs_s_f4
	PTR_WORD get_fs_s_f6
	PTR_WORD get_fs_s_f8
	PTR_WORD get_fs_s_f10
	PTR_WORD get_fs_s_f12
	PTR_WORD get_fs_s_f14
	PTR_WORD get_fs_s_f16
	PTR_WORD get_fs_s_f18
	PTR_WORD get_fs_s_f20
	PTR_WORD get_fs_s_f22
	PTR_WORD get_fs_s_f24
	PTR_WORD get_fs_s_f26
	PTR_WORD get_fs_s_f28
	PTR_WORD get_fs_s_f30
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
 *	This is an internal routine used by mips_emul_fp only.
 *
 * Results:
 *	t0	contains the FS sign
 *	t1	contains the FS (biased) exponent
 *	t2	contains the FS fraction
 *	t3	contains the FS remaining fraction
 *	ta0	contains the FT sign
 *	ta1	contains the FT (biased) exponent
 *	ta2	contains the FT fraction
 *	ta3	contains the FT remaining fraction
 *
 *----------------------------------------------------------------------------
 */
STATIC_LEAF(get_ft_fs_d)
#ifdef FPEMUL
	srl	ta3, a0, 16 - FPX_SCALESHIFT
	PTR_L	ta0, L_PCB(MIPS_CURLWP)		# get pcb of current lwp
	andi	ta3, ta3, FPX_REGEVENMASK	# Even regs only
	PTR_ADDU ta0, ta3

#if defined(__mips_n32) || defined(__mips_n64)
	FPX_L	ta3, PCB_FPREGS+FRAME_FP0(ta0)
	dsrl	ta0, ta3, 32
	srl	ta3, ta3, 0
#else
	lw	ta3, PCB_FPREGS+FRAME_FP0(ta0)
	lw	ta0, PCB_FPREGS+FRAME_FP0+SZFPREG(ta0)
#endif

	srl	ta1, ta0, 20			# get exponent
	and	ta1, ta1, 0x7FF
	and	ta2, ta0, 0xFFFFF		# get fraction
	srl	ta0, ta0, 31			# get sign
	bne	ta1, DEXP_INF, 1f		# is it a signaling NAN?
	and	v0, ta2, DSIGNAL_NAN
	bne	v0, zero, invalid_d
1:
	/* fall through to get FS */
#else
	srl	a3, a0, 16 - (PTR_SCALESHIFT-1)	# get FT field (even regs only)
	and	a3, a3, 0xF << PTR_SCALESHIFT	# mask FT field
	PTR_L	a3, get_ft_d_tbl(a3)		# switch on register number
	j	a3

	.rdata
get_ft_d_tbl:
	PTR_WORD get_ft_d_f0
	PTR_WORD get_ft_d_f2
	PTR_WORD get_ft_d_f4
	PTR_WORD get_ft_d_f6
	PTR_WORD get_ft_d_f8
	PTR_WORD get_ft_d_f10
	PTR_WORD get_ft_d_f12
	PTR_WORD get_ft_d_f14
	PTR_WORD get_ft_d_f16
	PTR_WORD get_ft_d_f18
	PTR_WORD get_ft_d_f20
	PTR_WORD get_ft_d_f22
	PTR_WORD get_ft_d_f24
	PTR_WORD get_ft_d_f26
	PTR_WORD get_ft_d_f28
	PTR_WORD get_ft_d_f30
	.text

get_ft_d_f0:
	mfc1	ta3, $f0
	mfc1	ta0, $f1
	b	get_ft_d_done
get_ft_d_f2:
	mfc1	ta3, $f2
	mfc1	ta0, $f3
	b	get_ft_d_done
get_ft_d_f4:
	mfc1	ta3, $f4
	mfc1	ta0, $f5
	b	get_ft_d_done
get_ft_d_f6:
	mfc1	ta3, $f6
	mfc1	ta0, $f7
	b	get_ft_d_done
get_ft_d_f8:
	mfc1	ta3, $f8
	mfc1	ta0, $f9
	b	get_ft_d_done
get_ft_d_f10:
	mfc1	ta3, $f10
	mfc1	ta0, $f11
	b	get_ft_d_done
get_ft_d_f12:
	mfc1	ta3, $f12
	mfc1	ta0, $f13
	b	get_ft_d_done
get_ft_d_f14:
	mfc1	ta3, $f14
	mfc1	ta0, $f15
	b	get_ft_d_done
get_ft_d_f16:
	mfc1	ta3, $f16
	mfc1	ta0, $f17
	b	get_ft_d_done
get_ft_d_f18:
	mfc1	ta3, $f18
	mfc1	ta0, $f19
	b	get_ft_d_done
get_ft_d_f20:
	mfc1	ta3, $f20
	mfc1	ta0, $f21
	b	get_ft_d_done
get_ft_d_f22:
	mfc1	ta3, $f22
	mfc1	ta0, $f23
	b	get_ft_d_done
get_ft_d_f24:
	mfc1	ta3, $f24
	mfc1	ta0, $f25
	b	get_ft_d_done
get_ft_d_f26:
	mfc1	ta3, $f26
	mfc1	ta0, $f27
	b	get_ft_d_done
get_ft_d_f28:
	mfc1	ta3, $f28
	mfc1	ta0, $f29
	b	get_ft_d_done
get_ft_d_f30:
	mfc1	ta3, $f30
	mfc1	ta0, $f31
get_ft_d_done:
	srl	ta1, ta0, 20			# get exponent
	and	ta1, ta1, 0x7FF
	and	ta2, ta0, 0xFFFFF			# get fraction
	srl	ta0, ta0, 31			# get sign
	bne	ta1, DEXP_INF, 1f		# is it a signaling NAN?
	and	v0, ta2, DSIGNAL_NAN
	bne	v0, zero, invalid_d
1:
	/* fall through to get FS */
#endif

/*----------------------------------------------------------------------------
 * get_fs_d --
 *
 *	Read (double precision) the FS register (bits 15-11) and
 *	break up into fields.
 *	This is an internal routine used by mips_emul_fp only.
 *
 * Results:
 *	t0	contains the sign
 *	t1	contains the (biased) exponent
 *	t2	contains the fraction
 *	t3	contains the remaining fraction
 *
 *----------------------------------------------------------------------------
 */
STATIC_XLEAF(get_fs_d)
#ifdef FPEMUL
	srl	t3, a0, 11 - FPX_SCALESHIFT
	PTR_L	t0, L_PCB(MIPS_CURLWP)		# get pcb of current lwp
	andi	t3, t3, FPX_REGEVENMASK		# Even regs only
	PTR_ADDU t0, t3

#if defined(__mips_n32) || defined(__mips_n64)
	FPX_L	t3, PCB_FPREGS+FRAME_FP0(t0)
	dsrl	t0, t3, 32
	srl	t3, t3, 0
#else
	lw	t3, PCB_FPREGS+FRAME_FP0(t0)
	lw	t0, PCB_FPREGS+FRAME_FP0+SZFPREG(t0)
#endif

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
	srl	a3, a0, 11 - (PTR_SCALESHIFT-1)	# get FS field (even regs only)
	and	a3, a3, 0xF << PTR_SCALESHIFT	# mask FS field
	PTR_L	a3, get_fs_d_tbl(a3)		# switch on register number
	j	a3

	.rdata
get_fs_d_tbl:
	PTR_WORD get_fs_d_f0
	PTR_WORD get_fs_d_f2
	PTR_WORD get_fs_d_f4
	PTR_WORD get_fs_d_f6
	PTR_WORD get_fs_d_f8
	PTR_WORD get_fs_d_f10
	PTR_WORD get_fs_d_f12
	PTR_WORD get_fs_d_f14
	PTR_WORD get_fs_d_f16
	PTR_WORD get_fs_d_f18
	PTR_WORD get_fs_d_f20
	PTR_WORD get_fs_d_f22
	PTR_WORD get_fs_d_f24
	PTR_WORD get_fs_d_f26
	PTR_WORD get_fs_d_f28
	PTR_WORD get_fs_d_f30
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
 *	This is an internal routine used by mips_emul_fp only.
 *
 * Results:
 *	t0	contains the sign
 *	t1	contains the (biased) exponent
 *	t2	contains the fraction
 *	ta0	contains the sign
 *	ta1	contains the (biased) exponent
 *	ta2	contains the fraction
 *
 *----------------------------------------------------------------------------
 */
STATIC_LEAF(get_cmp_s)
#ifdef FPEMUL
	srl	t1, a0, 11 - FPX_SCALESHIFT
	PTR_L	ta2, L_PCB(MIPS_CURLWP)		# get pcb of current lwp
	andi	t1, t1, FPX_REGEVENMASK			# Even regs only
	PTR_ADDU t0, ta2, t1

	lw	t0, PCB_FPREGS+FRAME_FP0(t0)

	srl	t1, t0, 23			# get exponent
	and	t1, t1, 0xFF
	and	t2, t0, 0x7FFFFF		# get fraction
	srl	t0, t0, 31			# get sign

	srl	ta0, a0, 16 - FPX_SCALESHIFT
	andi	ta0, ta0, FPX_REGEVENMASK			# Even regs only
	PTR_ADDU ta2, ta0

	lw	ta0, PCB_FPREGS+FRAME_FP0(ta2)

	srl	ta1, ta0, 23			# get exponent
	and	ta1, ta1, 0xFF
	and	ta2, ta0, 0x7FFFFF		# get fraction
	srl	ta0, ta0, 31			# get sign
	j	ra
#else
	srl	a3, a0, 11 - (PTR_SCALESHIFT-1)	# get FS field (even regs only)
	and	a3, a3, 0xF << PTR_SCALESHIFT	# mask FS field
	PTR_L	a3, cmp_fs_s_tbl(a3)		# switch on register number
	j	a3

	.rdata
cmp_fs_s_tbl:
	PTR_WORD cmp_fs_s_f0
	PTR_WORD cmp_fs_s_f2
	PTR_WORD cmp_fs_s_f4
	PTR_WORD cmp_fs_s_f6
	PTR_WORD cmp_fs_s_f8
	PTR_WORD cmp_fs_s_f10
	PTR_WORD cmp_fs_s_f12
	PTR_WORD cmp_fs_s_f14
	PTR_WORD cmp_fs_s_f16
	PTR_WORD cmp_fs_s_f18
	PTR_WORD cmp_fs_s_f20
	PTR_WORD cmp_fs_s_f22
	PTR_WORD cmp_fs_s_f24
	PTR_WORD cmp_fs_s_f26
	PTR_WORD cmp_fs_s_f28
	PTR_WORD cmp_fs_s_f30
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

	srl	a3, a0, 17 - PTR_SCALESHIFT			# get FT field (even regs only)
	and	a3, a3, 0xF << PTR_SCALESHIFT		# mask FT field
	PTR_L	a3, cmp_ft_s_tbl(a3)		# switch on register number
	j	a3

	.rdata
cmp_ft_s_tbl:
	PTR_WORD cmp_ft_s_f0
	PTR_WORD cmp_ft_s_f2
	PTR_WORD cmp_ft_s_f4
	PTR_WORD cmp_ft_s_f6
	PTR_WORD cmp_ft_s_f8
	PTR_WORD cmp_ft_s_f10
	PTR_WORD cmp_ft_s_f12
	PTR_WORD cmp_ft_s_f14
	PTR_WORD cmp_ft_s_f16
	PTR_WORD cmp_ft_s_f18
	PTR_WORD cmp_ft_s_f20
	PTR_WORD cmp_ft_s_f22
	PTR_WORD cmp_ft_s_f24
	PTR_WORD cmp_ft_s_f26
	PTR_WORD cmp_ft_s_f28
	PTR_WORD cmp_ft_s_f30
	.text

cmp_ft_s_f0:
	mfc1	ta0, $f0
	b	cmp_ft_s_done
cmp_ft_s_f2:
	mfc1	ta0, $f2
	b	cmp_ft_s_done
cmp_ft_s_f4:
	mfc1	ta0, $f4
	b	cmp_ft_s_done
cmp_ft_s_f6:
	mfc1	ta0, $f6
	b	cmp_ft_s_done
cmp_ft_s_f8:
	mfc1	ta0, $f8
	b	cmp_ft_s_done
cmp_ft_s_f10:
	mfc1	ta0, $f10
	b	cmp_ft_s_done
cmp_ft_s_f12:
	mfc1	ta0, $f12
	b	cmp_ft_s_done
cmp_ft_s_f14:
	mfc1	ta0, $f14
	b	cmp_ft_s_done
cmp_ft_s_f16:
	mfc1	ta0, $f16
	b	cmp_ft_s_done
cmp_ft_s_f18:
	mfc1	ta0, $f18
	b	cmp_ft_s_done
cmp_ft_s_f20:
	mfc1	ta0, $f20
	b	cmp_ft_s_done
cmp_ft_s_f22:
	mfc1	ta0, $f22
	b	cmp_ft_s_done
cmp_ft_s_f24:
	mfc1	ta0, $f24
	b	cmp_ft_s_done
cmp_ft_s_f26:
	mfc1	ta0, $f26
	b	cmp_ft_s_done
cmp_ft_s_f28:
	mfc1	ta0, $f28
	b	cmp_ft_s_done
cmp_ft_s_f30:
	mfc1	ta0, $f30
cmp_ft_s_done:
	srl	ta1, ta0, 23			# get exponent
	and	ta1, ta1, 0xFF
	and	ta2, ta0, 0x7FFFFF		# get fraction
	srl	ta0, ta0, 31			# get sign
	j	ra
#endif
END(get_cmp_s)

/*----------------------------------------------------------------------------
 * get_cmp_d --
 *
 *	Read (double precision) the FS register (bits 15-11) and
 *	the FT register (bits 20-16) and break up into fields.
 *	This is an internal routine used by mips_emul_fp only.
 *
 * Results:
 *	t0	contains the sign
 *	t1	contains the (biased) exponent
 *	t2	contains the fraction
 *	t3	contains the remaining fraction
 *	ta0	contains the sign
 *	ta1	contains the (biased) exponent
 *	ta2	contains the fraction
 *	ta3	contains the remaining fraction
 *
 *----------------------------------------------------------------------------
 */
STATIC_LEAF(get_cmp_d)
#ifdef FPEMUL
	srl	t1, a0, 11-FPX_SCALESHIFT
	PTR_L	ta2, L_PCB(MIPS_CURLWP)		# get pcb of current lwp
	andi	t1, t1, FPX_REGEVENMASK			# Even regs only
	PTR_ADDU t0, ta2, t1

#if defined(__mips_n32) || defined(__mips_n64)
	FPX_L	t3, PCB_FPREGS+FRAME_FP0(t0)
	dsrl	t0, t3, 32
	srl	t3, t3, 0
#else
	FPX_L	t3, PCB_FPREGS+FRAME_FP0(t0)
	FPX_L	t0, PCB_FPREGS+FRAME_FP0+SZFPREG(t0)
#endif

	srl	t1, t0, 20			# get exponent
	and	t1, t1, 0x7FF
	and	t2, t0, 0xFFFFF			# get fraction
	srl	t0, t0, 31			# get sign

	srl	ta0, a0, 16 - FPX_SCALESHIFT
	andi	ta0, ta0, FPX_REGEVENMASK		# Even regs only
	PTR_ADDU ta2, ta2, ta0

#if defined(__mips_n32) || defined(__mips_n64)
	FPX_L	ta3, PCB_FPREGS+FRAME_FP0(ta2)
	dsrl	ta0, ta3, 32
	srl	ta3, ta3, 0
#else
	lw	ta3, PCB_FPREGS+FRAME_FP0(ta2)
	lw	ta0, PCB_FPREGS+FRAME_FP0+SZFPREG(ta2)
#endif

	srl	ta1, ta0, 20			# get exponent
	and	ta1, ta1, 0x7FF
	and	ta2, ta0, 0xFFFFF			# get fraction
	srl	ta0, ta0, 31			# get sign
	j	ra
#else
	srl	a3, a0, 12 - PTR_SCALESHIFT	# get FS field
	and	a3, a3, 0xF << PTR_SCALESHIFT	# mask FS field (even regs only)
	PTR_L	a3, cmp_fs_d_tbl(a3)		# switch on register number
	j	a3

	.rdata
cmp_fs_d_tbl:
	PTR_WORD cmp_fs_d_f0
	PTR_WORD cmp_fs_d_f2
	PTR_WORD cmp_fs_d_f4
	PTR_WORD cmp_fs_d_f6
	PTR_WORD cmp_fs_d_f8
	PTR_WORD cmp_fs_d_f10
	PTR_WORD cmp_fs_d_f12
	PTR_WORD cmp_fs_d_f14
	PTR_WORD cmp_fs_d_f16
	PTR_WORD cmp_fs_d_f18
	PTR_WORD cmp_fs_d_f20
	PTR_WORD cmp_fs_d_f22
	PTR_WORD cmp_fs_d_f24
	PTR_WORD cmp_fs_d_f26
	PTR_WORD cmp_fs_d_f28
	PTR_WORD cmp_fs_d_f30
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

	srl	a3, a0, 17 - PTR_SCALESHIFT	# get FT field (even regs only)
	and	a3, a3, 0xF << PTR_SCALESHIFT	# mask FT field
	PTR_L	a3, cmp_ft_d_tbl(a3)		# switch on register number
	j	a3

	.rdata
cmp_ft_d_tbl:
	PTR_WORD cmp_ft_d_f0
	PTR_WORD cmp_ft_d_f2
	PTR_WORD cmp_ft_d_f4
	PTR_WORD cmp_ft_d_f6
	PTR_WORD cmp_ft_d_f8
	PTR_WORD cmp_ft_d_f10
	PTR_WORD cmp_ft_d_f12
	PTR_WORD cmp_ft_d_f14
	PTR_WORD cmp_ft_d_f16
	PTR_WORD cmp_ft_d_f18
	PTR_WORD cmp_ft_d_f20
	PTR_WORD cmp_ft_d_f22
	PTR_WORD cmp_ft_d_f24
	PTR_WORD cmp_ft_d_f26
	PTR_WORD cmp_ft_d_f28
	PTR_WORD cmp_ft_d_f30
	.text

cmp_ft_d_f0:
	mfc1	ta3, $f0
	mfc1	ta0, $f1
	b	cmp_ft_d_done
cmp_ft_d_f2:
	mfc1	ta3, $f2
	mfc1	ta0, $f3
	b	cmp_ft_d_done
cmp_ft_d_f4:
	mfc1	ta3, $f4
	mfc1	ta0, $f5
	b	cmp_ft_d_done
cmp_ft_d_f6:
	mfc1	ta3, $f6
	mfc1	ta0, $f7
	b	cmp_ft_d_done
cmp_ft_d_f8:
	mfc1	ta3, $f8
	mfc1	ta0, $f9
	b	cmp_ft_d_done
cmp_ft_d_f10:
	mfc1	ta3, $f10
	mfc1	ta0, $f11
	b	cmp_ft_d_done
cmp_ft_d_f12:
	mfc1	ta3, $f12
	mfc1	ta0, $f13
	b	cmp_ft_d_done
cmp_ft_d_f14:
	mfc1	ta3, $f14
	mfc1	ta0, $f15
	b	cmp_ft_d_done
cmp_ft_d_f16:
	mfc1	ta3, $f16
	mfc1	ta0, $f17
	b	cmp_ft_d_done
cmp_ft_d_f18:
	mfc1	ta3, $f18
	mfc1	ta0, $f19
	b	cmp_ft_d_done
cmp_ft_d_f20:
	mfc1	ta3, $f20
	mfc1	ta0, $f21
	b	cmp_ft_d_done
cmp_ft_d_f22:
	mfc1	ta3, $f22
	mfc1	ta0, $f23
	b	cmp_ft_d_done
cmp_ft_d_f24:
	mfc1	ta3, $f24
	mfc1	ta0, $f25
	b	cmp_ft_d_done
cmp_ft_d_f26:
	mfc1	ta3, $f26
	mfc1	ta0, $f27
	b	cmp_ft_d_done
cmp_ft_d_f28:
	mfc1	ta3, $f28
	mfc1	ta0, $f29
	b	cmp_ft_d_done
cmp_ft_d_f30:
	mfc1	ta3, $f30
	mfc1	ta0, $f31
cmp_ft_d_done:
	srl	ta1, ta0, 20			# get exponent
	and	ta1, ta1, 0x7FF
	and	ta2, ta0, 0xFFFFF			# get fraction
	srl	ta0, ta0, 31			# get sign
	j	ra
#endif
END(get_cmp_d)

/*----------------------------------------------------------------------------
 * set_fd_s --
 *
 *	Write (single precision) the FD register (bits 10-6).
 *	This is an internal routine used by mips_emul_fp only.
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
 *	This is an internal routine used by mips_emul_fp only.
 *
 * Arguments:
 *	a0	contains the FP instruction
 *	t2	contains the integer
 *
 *----------------------------------------------------------------------------
 */
STATIC_LEAF(set_fd_s)
	sll	t0, t0, 31			# position sign
	sll	t1, t1, 23			# position exponent
	or	t2, t2, t0
	or	t2, t2, t1
STATIC_XLEAF(set_fd_word)
#ifdef FPEMUL
	srl	t1, a0, 6 - FPX_SCALESHIFT
	PTR_L	t0, L_PCB(MIPS_CURLWP)		# get pcb of current lwp
	andi	t1, t1, FPX_REGEVENMASK		# Even regs only
	PTR_ADDU t0, t0, t1

	FPX_S	t2, PCB_FPREGS+FRAME_FP0(t0)
	j	ra
#else
	srl	a3, a0, 6 + 1 - PTR_SCALESHIFT	# get FD field (even regs only)
	and	a3, a3, 0xF << PTR_SCALESHIFT	# mask FT field
	PTR_L	a3, set_fd_s_tbl(a3)		# switch on register number
	j	a3

	.rdata
set_fd_s_tbl:
	PTR_WORD set_fd_s_f0
	PTR_WORD set_fd_s_f2
	PTR_WORD set_fd_s_f4
	PTR_WORD set_fd_s_f6
	PTR_WORD set_fd_s_f8
	PTR_WORD set_fd_s_f10
	PTR_WORD set_fd_s_f12
	PTR_WORD set_fd_s_f14
	PTR_WORD set_fd_s_f16
	PTR_WORD set_fd_s_f18
	PTR_WORD set_fd_s_f20
	PTR_WORD set_fd_s_f22
	PTR_WORD set_fd_s_f24
	PTR_WORD set_fd_s_f26
	PTR_WORD set_fd_s_f28
	PTR_WORD set_fd_s_f30
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
 *	This is an internal routine used by mips_emul_fp only.
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
STATIC_LEAF(set_fd_d)
#ifdef FPEMUL
	sll	t0, t0, 31			# set sign
	sll	t1, t1, 20			# set exponent
	or	t0, t0, t1
	or	t0, t0, t2			# set fraction

	srl	t1, a0, 6-FPX_SCALESHIFT
	PTR_L	t2, L_PCB(MIPS_CURLWP)		# get pcb of current lwp
	andi	t1, t1, FPX_REGEVENMASK
	PTR_ADDU t2, t2, t1

#if defined(__mips_n32) || defined(__mips_n64)
	dsll	t0, t0, 32
	or	t0, t0, t3
	FPX_S	t0, PCB_FPREGS+FRAME_FP0(t2)
#else
	FPX_S	t3, PCB_FPREGS+FRAME_FP0(t2)
	FPX_S	t0, PCB_FPREGS+FRAME_FP0+4(t2)
#endif
	j	ra
#else
	sll	t0, t0, 31			# set sign
	sll	t1, t1, 20			# set exponent
	or	t0, t0, t1
	or	t0, t0, t2			# set fraction
	srl	a3, a0, 7 - PTR_SCALESHIFT	# get FD field (even regs only)
	and	a3, a3, 0xF << PTR_SCALESHIFT	# mask FD field
	PTR_L	a3, set_fd_d_tbl(a3)		# switch on register number
	j	a3

	.rdata
set_fd_d_tbl:
	PTR_WORD set_fd_d_f0
	PTR_WORD set_fd_d_f2
	PTR_WORD set_fd_d_f4
	PTR_WORD set_fd_d_f6
	PTR_WORD set_fd_d_f8
	PTR_WORD set_fd_d_f10
	PTR_WORD set_fd_d_f12
	PTR_WORD set_fd_d_f14
	PTR_WORD set_fd_d_f16
	PTR_WORD set_fd_d_f18
	PTR_WORD set_fd_d_f20
	PTR_WORD set_fd_d_f22
	PTR_WORD set_fd_d_f24
	PTR_WORD set_fd_d_f26
	PTR_WORD set_fd_d_f28
	PTR_WORD set_fd_d_f30
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
STATIC_LEAF(renorm_fs_s)
/*
 * Find out how many leading zero bits are in t2 and put in at.
 */
#if __mips == 32 || __mips == 64
	clz	v1, t2
#else
	.set	noat
	move	v0, t2
	move	v1, zero
	srl	AT, v0, 16
	bne	AT, zero, 1f
	addu	v1, 16
	sll	v0, 16
1:
	srl	AT, v0, 24
	bne	AT, zero, 1f
	addu	v1, 8
	sll	v0, 8
1:
	srl	AT, v0, 28
	bne	AT, zero, 1f
	addu	v1, 4
	sll	v0, 4
1:
	srl	AT, v0, 30
	bne	AT, zero, 1f
	addu	v1, 2
	sll	v0, 2
1:
	srl	AT, v0, 31
	bne	AT, zero, 1f
	addu	v1, 1
1:
	.set	at
#endif /* __mips == 32 || __mips == 64 */
/*
 * Now shift t2 the correct number of bits.
 */
	subu	v1, v1, SLEAD_ZEROS	# dont count normal leading zeros
	li	t1, SEXP_MIN
	subu	t1, t1, v1		# adjust exponent
	sll	t2, t2, v1
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
STATIC_LEAF(renorm_fs_d)
/*
 * Find out how many leading zero bits are in t2,t3 and put in v1.
 */
#if __mips == 32 || __mips == 64
#ifdef __mips_o32
	bne	ta2, zero, 1f
	clz	v1, ta3
	addu	v1, 32
	b	2f
1:
	clz	v1, ta2
2:
#elif __mips_isa_rev == 2
	move	v0, ta3
	dins	v0, ta2, 32, 32
	dclz	v1, v0
#else
	dsll	v0, ta3, 32
	dsrl	v0, v0, 32
	dsll	v1, ta2, 32
	or	v0, v1
	dclz	v1, v0
#endif /* __mips_o32 */
#else
	.set	noat
	move	v0, t2
	move	v1, zero
	bne	t2, zero, 1f
	move	v0, t3
	addu	v1, 32
1:
	srl	AT, v0, 16
	bne	AT, zero, 1f
	addu	v1, 16
	sll	v0, 16
1:
	srl	AT, v0, 24
	bne	AT, zero, 1f
	addu	v1, 8
	sll	v0, 8
1:
	srl	AT, v0, 28
	bne	AT, zero, 1f
	addu	v1, 4
	sll	v0, 4
1:
	srl	AT, v0, 30
	bne	AT, zero, 1f
	addu	v1, 2
	sll	v0, 2
1:
	srl	AT, v0, 31
	bne	AT, zero, 1f
	addu	v1, 1
1:
	.set	at
#endif /* __mips == 32 || __mips == 64 */
/*
 * Now shift t2,t3 the correct number of bits.
 */
	subu	v1, v1, DLEAD_ZEROS	# dont count normal leading zeros
	li	t1, DEXP_MIN
	subu	t1, t1, v1		# adjust exponent
#ifdef __mips_o32
	li	v0, 32
	blt	v1, v0, 1f
	subu	v1, v1, v0		# shift fraction left >= 32 bits
	sll	t2, t3, v1
	move	t3, zero
	j	ra
1:
	subu	v0, v0, v1		# shift fraction left < 32 bits
	sll	t2, t2, v1
	srl	v0, t3, v0
	or	t2, t2, v0
	sll	t3, t3, v1
	j	ra
#else
	dsll	v0, v0, t1
	dsrl	t2, v0, 32		# MSW
	sll	t3, v0, 0		# LSW
	j	ra
#endif
END(renorm_fs_d)

/*----------------------------------------------------------------------------
 * renorm_ft_s --
 *
 * Results:
 *	ta1	unbiased exponent
 *	ta2	normalized fraction
 *
 *----------------------------------------------------------------------------
 */
STATIC_LEAF(renorm_ft_s)
#if __mips == 32 || __mips == 64
	clz	v1, ta2
#else
	.set	noat
/*
 * Find out how many leading zero bits are in ta2 and put in v1.
 */
	move	v0, ta2
	move	v1, zero
	srl	AT, v0, 16
	bne	AT, zero, 1f
	addu	v1, 16
	sll	v0, 16
1:
	srl	AT, v0, 24
	bne	AT, zero, 1f
	addu	v1, 8
	sll	v0, 8
1:
	srl	AT, v0, 28
	bne	AT, zero, 1f
	addu	v1, 4
	sll	v0, 4
1:
	srl	AT, v0, 30
	bne	AT, zero, 1f
	addu	v1, 2
	sll	v0, 2
1:
	srl	AT, v0, 31
	bne	AT, zero, 1f
	addu	v1, 1
1:
	.set	at
#endif /* __mips == 32 || __mips == 64 */
/*
 * Now shift ta2 the correct number of bits.
 */
	subu	v1, v1, SLEAD_ZEROS	# dont count normal leading zeros
	li	ta1, SEXP_MIN
	subu	ta1, ta1, v1		# adjust exponent
	sll	ta2, ta2, v1
	j	ra
END(renorm_ft_s)

/*----------------------------------------------------------------------------
 * renorm_ft_d --
 *
 * Results:
 *	ta1	unbiased exponent
 *	ta2,ta3	normalized fraction
 *
 *----------------------------------------------------------------------------
 */
STATIC_LEAF(renorm_ft_d)
/*
 * Find out how many leading zero bits are in ta2,ta3 and put in at.
 */
#if __mips == 32 || __mips == 64
#ifdef __mips_o32
	bne	ta2, zero, 1f
	clz	v1, ta3
	addu	v1, 32
	b	2f
1:
	clz	v1, ta2
2:
#elif __mips_isa_rev == 2
	move	v0, ta3
	dins	v0, ta2, 32, 32
	dclz	v1, v0
#else
	dsll	v0, ta3, 32
	dsrl	v0, v0, 32
	dsll	v1, ta2, 32
	or	v0, v1
	dclz	v1, v0
#endif /* __mips_o32 */
#else
	.set	noat
	move	v0, ta2
	move	v1, zero
	bne	ta2, zero, 1f
	move	v0, ta3
	addu	v1, 32
1:
	srl	AT, v0, 16
	bne	AT, zero, 1f
	addu	v1, 16
	sll	v0, 16
1:
	srl	AT, v0, 24
	bne	AT, zero, 1f
	addu	v1, 8
	sll	v0, 8
1:
	srl	AT, v0, 28
	bne	AT, zero, 1f
	addu	v1, 4
	sll	v0, 4
1:
	srl	AT, v0, 30
	bne	AT, zero, 1f
	addu	v1, 2
	sll	v0, 2
1:
	srl	AT, v0, 31
	bne	AT, zero, 1f
	addu	v1, 1
1:
	.set	at
#endif /* __mips == 32 || __mips == 64 */
/*
 * Now shift ta2,ta3 the correct number of bits.
 */
	subu	v1, v1, DLEAD_ZEROS	# dont count normal leading zeros
	li	ta1, DEXP_MIN
	subu	ta1, ta1, v1		# adjust exponent
#ifdef __mips_o32
	li	v0, 32
	blt	v1, v0, 1f
	subu	v1, v1, v0		# shift fraction left >= 32 bits
	sll	ta2, ta3, v1
	move	ta3, zero
	j	ra
1:
	subu	v0, v0, v1		# shift fraction left < 32 bits
	sll	ta2, ta2, v1
	srl	v0, ta3, v0
	or	ta2, ta2, v0
	sll	ta3, ta3, v1
	j	ra
#else
	dsll	v0, v0, t1
	dsrl	ta2, v0, 32
	sll	ta3, v0, 0
	j	ra
#endif	/* __mips_o32 */
END(renorm_ft_d)

/*
 * Send SIGILL, SIGFPE.
 * Args are same as mips_emul_fp.
 */
STATIC_LEAF(fpemul_sigill)
	li	t0, 0xFFFFFF00
	and	a2, a2, t0
	ori	a2, a2, T_RES_INST << MIPS_CR_EXC_CODE_SHIFT
	REG_PROLOGUE
	REG_S	a2, TF_REG_CAUSE(a1)
	REG_EPILOGUE

	move	a1, a0				# code = instruction
	move	a0, MIPS_CURLWP			# get current lwp
	j	_C_LABEL(mips_fpuillinst)
END(fpemul_sigill)

STATIC_LEAF(fpemul_sigfpe)
	li	t0, 0xFFFFFF00
	and	a2, a2, t0
	ori	a2, a2, T_FPE << MIPS_CR_EXC_CODE_SHIFT
	REG_PROLOGUE
	REG_S	a2, TF_REG_CAUSE(a1)
	REG_EPILOGUE

	move	a1, a3				# fpustat
	move	a0, MIPS_CURLWP			# get current lwp
	j	_C_LABEL(mips_fpuexcept)
END(fpemul_sigfpe)

#ifdef FPEMUL
STATIC_LEAF(mips_emul_sigfpe)
	li	t0, 0xFFFFFF00
	and	a2, a2, t0
	ori	a2, a2, T_OVFLOW << MIPS_CR_EXC_CODE_SHIFT
	REG_PROLOGUE
	REG_S	a2, TF_REG_CAUSE(a1)
	REG_EPILOGUE

	move	a1, a3				# fpustat
	move	a0, MIPS_CURLWP			# get current lwp
	j	_C_LABEL(mips_fpuexcept)
END(mips_emul_sigfpe)
#endif
