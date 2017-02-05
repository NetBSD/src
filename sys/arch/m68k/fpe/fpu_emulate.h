/*	$NetBSD: fpu_emulate.h,v 1.24.12.2 2017/02/05 13:40:14 skrll Exp $	*/

/*
 * Copyright (c) 1995 Gordon Ross
 * Copyright (c) 1995 Ken Nakata
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 4. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Gordon Ross
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _FPU_EMULATE_H_
#define _FPU_EMULATE_H_

#include <sys/types.h>
#include <sys/signal.h>
#include <sys/time.h>
#include <sys/signalvar.h>
#include <sys/siginfo.h>
#include <m68k/fpreg.h>

/*
 * Floating point emulator (tailored for SPARC/modified for m68k, but
 * structurally machine-independent).
 *
 * Floating point numbers are carried around internally in an `expanded'
 * or `unpacked' form consisting of:
 *	- sign
 *	- unbiased exponent
 *	- mantissa (`1.' + 80-bit fraction + guard + round)
 *	- sticky bit
 * Any implied `1' bit is inserted, giving a 81-bit mantissa that is
 * always nonzero.  Additional low-order `guard' and `round' bits are
 * scrunched in, making the entire mantissa 83 bits long.  This is divided
 * into three 32-bit words, with `spare' bits left over in the upper part
 * of the top word (the high bits of fp_mant[0]).  An internal `exploded'
 * number is thus kept within the half-open interval [1.0,2.0) (but see
 * the `number classes' below).  This holds even for denormalized numbers:
 * when we explode an external denorm, we normalize it, introducing low-order
 * zero bits, so that the rest of the code always sees normalized values.
 *
 * Note that a number of our algorithms use the `spare' bits at the top.
 * The most demanding algorithm---the one for sqrt---depends on two such
 * bits, so that it can represent values up to (but not including) 8.0,
 * and then it needs a carry on top of that, so that we need three `spares'.
 *
 * The sticky-word is 32 bits so that we can use `OR' operators to goosh
 * whole words from the mantissa into it.
 *
 * All operations are done in this internal extended precision.  According
 * to Hennesey & Patterson, Appendix A, rounding can be repeated---that is,
 * it is OK to do a+b in extended precision and then round the result to
 * single precision---provided single, double, and extended precisions are
 * `far enough apart' (they always are), but we will try to avoid any such
 * extra work where possible.
 */
struct fpn {
	int	fp_class;		/* see below */
	int	fp_sign;		/* 0 => positive, 1 => negative */
	int	fp_exp;			/* exponent (unbiased) */
	int	fp_sticky;		/* nonzero bits lost at right end */
	uint32_t fp_mant[3];		/* 83-bit mantissa */
};

#define	FP_NMANT	83		/* total bits in mantissa (incl g,r) */
#define	FP_NG		2		/* number of low-order guard bits */
#define	FP_LG		((FP_NMANT - 1) & 31)	/* log2(1.0) for fp_mant[0] */
#define	FP_QUIETBIT	(1 << (FP_LG - 1))	/* Quiet bit in NaNs (0.5) */
#define	FP_1		(1 << FP_LG)		/* 1.0 in fp_mant[0] */
#define	FP_2		(1 << (FP_LG + 1))	/* 2.0 in fp_mant[0] */

static inline void CPYFPN(struct fpn *, const struct fpn *);

static inline void
CPYFPN(struct fpn *dst, const struct fpn *src)
{

	if (dst != src) {
		*dst = *src;
	}
}

/*
 * Number classes.  Since zero, Inf, and NaN cannot be represented using
 * the above layout, we distinguish these from other numbers via a class.
 */
#define	FPC_SNAN	-2		/* signalling NaN (sign irrelevant) */
#define	FPC_QNAN	-1		/* quiet NaN (sign irrelevant) */
#define	FPC_ZERO	0		/* zero (sign matters) */
#define	FPC_NUM		1		/* number (sign matters) */
#define	FPC_INF		2		/* infinity (sign matters) */

#define	ISNAN(fp)	((fp)->fp_class < 0)
#define	ISZERO(fp)	((fp)->fp_class == 0)
#define	ISINF(fp)	((fp)->fp_class == FPC_INF)

/*
 * ORDER(x,y) `sorts' a pair of `fpn *'s so that the right operand (y) points
 * to the `more significant' operand for our purposes.  Appendix N says that
 * the result of a computation involving two numbers are:
 *
 *	If both are SNaN: operand 2, converted to Quiet
 *	If only one is SNaN: the SNaN operand, converted to Quiet
 *	If both are QNaN: operand 2
 *	If only one is QNaN: the QNaN operand
 *
 * In addition, in operations with an Inf operand, the result is usually
 * Inf.  The class numbers are carefully arranged so that if
 *	(unsigned)class(op1) > (unsigned)class(op2)
 * then op1 is the one we want; otherwise op2 is the one we want.
 */
#define	ORDER(x, y) { \
	if ((uint32_t)(x)->fp_class > (uint32_t)(y)->fp_class) \
		SWAP(x, y); \
}
#define	SWAP(x, y) {				\
	struct fpn *swap;			\
	swap = (x), (x) = (y), (y) = swap;	\
}

/*
 * Emulator state.
 */
struct fpemu {
	struct frame *fe_frame; /* integer regs, etc */
	struct fpframe *fe_fpframe; /* FP registers, etc */
	uint32_t fe_fpsr;	/* fpsr copy (modified during op) */
	uint32_t fe_fpcr;	/* fpcr copy */
	struct fpn fe_f1;	/* operand 1 */
	struct fpn fe_f2;	/* operand 2, if required */
	struct fpn fe_f3;	/* available storage for result */
};

/*****************************************************************************
 * End of definitions derived from Sparc FPE
 *****************************************************************************/

/*
 * Internal info about a decoded effective address.
 */
struct insn_ea {
	int	ea_regnum;
	int	ea_ext[3];	/* extension words if any */
	int	ea_flags;	/* flags == 0 means mode 2: An@ */
#define	EA_DIRECT	0x001	/* mode [01]: Dn or An */
#define EA_PREDECR	0x002	/* mode 4: An@- */
#define	EA_POSTINCR	0x004	/* mode 3: An@+ */
#define EA_OFFSET	0x008	/* mode 5 or (7,2): APC@(d16) */
#define	EA_INDEXED	0x010	/* mode 6 or (7,3): APC@(Xn:*:*,d8) etc */
#define EA_ABS  	0x020	/* mode (7,[01]): abs */
#define EA_PC_REL	0x040	/* mode (7,[23]): PC@(d16) etc */
#define	EA_IMMED	0x080	/* mode (7,4): #immed */
#define EA_MEM_INDIR	0x100	/* mode 6 or (7,3): APC@(Xn:*:*,*)@(*) etc */
#define EA_BASE_SUPPRSS	0x200	/* mode 6 or (7,3): base register suppressed */
#define EA_FRAME_EA	0x400	/* MC68LC040 only: precalculated EA from
				   format 4 stack frame */
	int	ea_moffs;	/* offset used for fmoveMulti */
};

#define ea_offset	ea_ext[0]	/* mode 5: offset word */
#define ea_absaddr	ea_ext[0]	/* mode (7,[01]): absolute address */
#define ea_immed	ea_ext		/* mode (7,4): immediate value */
#define ea_basedisp	ea_ext[0]	/* mode 6: base displacement */
#define ea_outerdisp	ea_ext[1]	/* mode 6: outer displacement */
#define	ea_idxreg	ea_ext[2]	/* mode 6: index register number */
#define ea_fea		ea_ext[0]	/* MC68LC040 only: frame EA */

struct instruction {
	uint32_t is_pc;		/* insn's address */
	uint32_t is_nextpc;	/* next PC */
	int	is_advance;	/* length of instruction */
	int	is_datasize;	/* size of memory operand */
	int	is_opcode;	/* opcode word */
	int	is_word1;	/* second word */
	struct insn_ea	is_ea;	/* decoded effective address mode */
};

/*
 * FP data types
 */
#define FTYPE_LNG 0 /* Long Word Integer */
#define FTYPE_SNG 1 /* Single Prec */
#define FTYPE_EXT 2 /* Extended Prec */
#define FTYPE_BCD 3 /* Packed BCD */
#define FTYPE_WRD 4 /* Word Integer */
#define FTYPE_DBL 5 /* Double Prec */
#define FTYPE_BYT 6 /* Byte Integer */

/*
 * Other functions.
 */

/* Build a new Quiet NaN (sign=0, frac=all 1's). */
struct	fpn *fpu_newnan(struct fpemu *);

/*
 * Shift a number right some number of bits, taking care of round/sticky.
 * Note that the result is probably not a well-formed number (it will lack
 * the normal 1-bit mant[0]&FP_1).
 */
int	fpu_shr(struct fpn *, int);
/*
 * Round a number according to the round mode in FPCR
 */
int	fpu_round(struct fpemu *, struct fpn *);

/* type conversion */
void	fpu_explode(struct fpemu *, struct fpn *, int t, const uint32_t *);
void	fpu_implode(struct fpemu *, struct fpn *, int t, uint32_t *);

/*
 * non-static emulation functions
 */
/* type 0 */
int fpu_emul_fmovecr(struct fpemu *, struct instruction *);
int fpu_emul_fstore(struct fpemu *, struct instruction *);
int fpu_emul_fscale(struct fpemu *, struct instruction *);

/*
 * include function declarations of those which are called by fpu_emul_arith()
 */
#include "fpu_arith_proto.h"

int fpu_emulate(struct frame *, struct fpframe *, ksiginfo_t *);
struct fpn *fpu_cmp(struct fpemu *);

/* fpu_cordic.c */
extern const struct fpn fpu_cordic_inv_gain1;
void fpu_cordit1(struct fpemu *,
	struct fpn *, struct fpn *, struct fpn *, const struct fpn *);

/*
 * "helper" functions
 */
/* return values from constant rom */
struct fpn *fpu_const(struct fpn *, uint32_t);
#define FPU_CONST_PI	(0x00)	/* pi */
#define FPU_CONST_0 	(0x0f)	/* 0.0 */
#define FPU_CONST_LN_2	(0x30)	/* ln(2) */
#define FPU_CONST_LN_10	(0x31)	/* ln(10) */
#define FPU_CONST_1 	(0x32)	/* 1.0 */

/* update exceptions and FPSR */
int fpu_upd_excp(struct fpemu *);
uint32_t fpu_upd_fpsr(struct fpemu *, struct fpn *);

/* address mode decoder, and load/store */
int fpu_decode_ea(struct frame *, struct instruction *,
		   struct insn_ea *, int);
int fpu_load_ea(struct frame *, struct instruction *,
		 struct insn_ea *, char *);
int fpu_store_ea(struct frame *, struct instruction *,
		  struct insn_ea *, char *);

/* fpu_subr.c */
void fpu_norm(struct fpn *);

#if !defined(FPE_DEBUG)
#  define FPE_DEBUG 0
#endif

#endif /* _FPU_EMULATE_H_ */
