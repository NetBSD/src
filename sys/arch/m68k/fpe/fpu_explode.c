/*	$NetBSD: fpu_explode.c,v 1.15 2015/02/05 12:23:27 isaki Exp $ */

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
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
 *	@(#)fpu_explode.c	8.1 (Berkeley) 6/11/93
 */

/*
 * FPU subroutines: `explode' the machine's `packed binary' format numbers
 * into our internal format.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: fpu_explode.c,v 1.15 2015/02/05 12:23:27 isaki Exp $");

#include <sys/types.h>
#include <sys/systm.h>

#include <machine/ieee.h>
#include <machine/reg.h>

#include "fpu_arith.h"
#include "fpu_emulate.h"


/* Conversion to internal format -- note asymmetry. */
static int	fpu_itof(struct fpn *fp, uint32_t i);
static int	fpu_stof(struct fpn *fp, uint32_t i);
static int	fpu_dtof(struct fpn *fp, uint32_t i, uint32_t j);
static int	fpu_xtof(struct fpn *fp, uint32_t i, uint32_t j, uint32_t k);

/*
 * N.B.: in all of the following, we assume the FP format is
 *
 *	---------------------------
 *	| s | exponent | fraction |
 *	---------------------------
 *
 * (which represents -1**s * 1.fraction * 2**exponent), so that the
 * sign bit is way at the top (bit 31), the exponent is next, and
 * then the remaining bits mark the fraction.  A zero exponent means
 * zero or denormalized (0.fraction rather than 1.fraction), and the
 * maximum possible exponent, 2bias+1, signals inf (fraction==0) or NaN.
 *
 * Since the sign bit is always the topmost bit---this holds even for
 * integers---we set that outside all the *tof functions.  Each function
 * returns the class code for the new number (but note that we use
 * FPC_QNAN for all NaNs; fpu_explode will fix this if appropriate).
 */

/*
 * int -> fpn.
 */
static int
fpu_itof(struct fpn *fp, uint32_t i)
{

	if (i == 0)
		return (FPC_ZERO);
	/*
	 * The value FP_1 represents 2^FP_LG, so set the exponent
	 * there and let normalization fix it up.  Convert negative
	 * numbers to sign-and-magnitude.  Note that this relies on
	 * fpu_norm()'s handling of `supernormals'; see fpu_subr.c.
	 */
	fp->fp_exp = FP_LG;
	fp->fp_mant[0] = (int)i < 0 ? -i : i;
	fp->fp_mant[1] = 0;
	fp->fp_mant[2] = 0;
	fpu_norm(fp);
	return (FPC_NUM);
}

#define	mask(nbits) ((1 << (nbits)) - 1)

/*
 * All external floating formats convert to internal in the same manner,
 * as defined here.  Note that only normals get an implied 1.0 inserted.
 */
#define	FP_TOF(exp, expbias, allfrac, f0, f1, f2, f3) \
	if (exp == 0) { \
		if (allfrac == 0) \
			return (FPC_ZERO); \
		fp->fp_exp = 1 - expbias; \
		fp->fp_mant[0] = f0; \
		fp->fp_mant[1] = f1; \
		fp->fp_mant[2] = f2; \
		fpu_norm(fp); \
		return (FPC_NUM); \
	} \
	if (exp == (2 * expbias + 1)) { \
		if (allfrac == 0) \
			return (FPC_INF); \
		fp->fp_mant[0] = f0; \
		fp->fp_mant[1] = f1; \
		fp->fp_mant[2] = f2; \
		return (FPC_QNAN); \
	} \
	fp->fp_exp = exp - expbias; \
	fp->fp_mant[0] = FP_1 | f0; \
	fp->fp_mant[1] = f1; \
	fp->fp_mant[2] = f2; \
	return (FPC_NUM)

/*
 * 32-bit single precision -> fpn.
 * We assume a single occupies at most (64-FP_LG) bits in the internal
 * format: i.e., needs at most fp_mant[0] and fp_mant[1].
 */
static int
fpu_stof(struct fpn *fp, uint32_t i)
{
	int exp;
	uint32_t frac, f0, f1;
#define SNG_SHIFT (SNG_FRACBITS - FP_LG)

	exp = (i >> (32 - 1 - SNG_EXPBITS)) & mask(SNG_EXPBITS);
	frac = i & mask(SNG_FRACBITS);
	f0 = frac >> SNG_SHIFT;
	f1 = frac << (32 - SNG_SHIFT);
	FP_TOF(exp, SNG_EXP_BIAS, frac, f0, f1, 0, 0);
}

/*
 * 64-bit double -> fpn.
 * We assume this uses at most (96-FP_LG) bits.
 */
static int
fpu_dtof(struct fpn *fp, uint32_t i, uint32_t j)
{
	int exp;
	uint32_t frac, f0, f1, f2;
#define DBL_SHIFT (DBL_FRACBITS - 32 - FP_LG)

	exp = (i >> (32 - 1 - DBL_EXPBITS)) & mask(DBL_EXPBITS);
	frac = i & mask(DBL_FRACBITS - 32);
	f0 = frac >> DBL_SHIFT;
	f1 = (frac << (32 - DBL_SHIFT)) | (j >> DBL_SHIFT);
	f2 = j << (32 - DBL_SHIFT);
	frac |= j;
	FP_TOF(exp, DBL_EXP_BIAS, frac, f0, f1, f2, 0);
}

/*
 * 96-bit extended -> fpn.
 */
static int
fpu_xtof(struct fpn *fp, uint32_t i, uint32_t j, uint32_t k)
{
	int exp;
	uint32_t f0, f1, f2;
#define EXT_SHIFT (EXT_FRACBITS - 1 - 32 - FP_LG)

	exp = (i >> (32 - 1 - EXT_EXPBITS)) & mask(EXT_EXPBITS);
	f0 = j >> EXT_SHIFT;
	f1 = (j << (32 - EXT_SHIFT)) | (k >> EXT_SHIFT);
	f2 = k << (32 - EXT_SHIFT);

	/* m68k extended does not imply denormal by exp==0 */
	if (exp == 0) {
		if ((j | k) == 0)
			return (FPC_ZERO);
		fp->fp_exp = - EXT_EXP_BIAS;
		fp->fp_mant[0] = f0;
		fp->fp_mant[1] = f1;
		fp->fp_mant[2] = f2;
		fpu_norm(fp);
		return (FPC_NUM);
	}
	if (exp == (2 * EXT_EXP_BIAS + 1)) {
		/* MSB is an integer part and don't care */
		if ((j & 0x7fffffff) == 0 && k == 0)
			return (FPC_INF);
		fp->fp_mant[0] = f0;
		fp->fp_mant[1] = f1;
		fp->fp_mant[2] = f2;
		return (FPC_QNAN);
	}
	fp->fp_exp = exp - EXT_EXP_BIAS;
	fp->fp_mant[0] = FP_1 | f0;
	fp->fp_mant[1] = f1;
	fp->fp_mant[2] = f2;
	return (FPC_NUM);
}

/*
 * Explode the contents of a memory operand.
 */
void
fpu_explode(struct fpemu *fe, struct fpn *fp, int type, const uint32_t *space)
{
	uint32_t s;

	s = space[0];
	fp->fp_sign = s >> 31;
	fp->fp_sticky = 0;
	switch (type) {

	case FTYPE_BYT:
		s >>= 8;
	case FTYPE_WRD:
		s >>= 16;
	case FTYPE_LNG:
		s = fpu_itof(fp, s);
		break;

	case FTYPE_SNG:
		s = fpu_stof(fp, s);
		break;

	case FTYPE_DBL:
		s = fpu_dtof(fp, s, space[1]);
		break;

	case FTYPE_EXT:
		s = fpu_xtof(fp, s, space[1], space[2]);
		break;

	default:
		panic("fpu_explode");
	}
	if (s == FPC_QNAN && (fp->fp_mant[0] & FP_QUIETBIT) == 0) {
		/*
		 * Input is a signalling NaN.  All operations that return
		 * an input NaN operand put it through a ``NaN conversion'',
		 * which basically just means ``turn on the quiet bit''.
		 * We do this here so that all NaNs internally look quiet
		 * (we can tell signalling ones by their class).
		 */
		fp->fp_mant[0] |= FP_QUIETBIT;
		fe->fe_fpsr |= FPSR_SNAN;	/* assert SNAN exception */
		s = FPC_SNAN;
	}
	fp->fp_class = s;
}
