/*	$NetBSD: fpu_exp.c,v 1.5.12.2 2017/12/03 11:36:23 jdolecek Exp $	*/

/*
 * Copyright (c) 1995  Ken Nakata
 *	All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)fpu_exp.c	10/24/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: fpu_exp.c,v 1.5.12.2 2017/12/03 11:36:23 jdolecek Exp $");

#include <machine/ieee.h>

#include "fpu_emulate.h"

/* The number of items to terminate the Taylor expansion */
#define MAX_ITEMS	(2000)

/*
 * fpu_exp.c: defines fpu_etox(), fpu_etoxm1(), fpu_tentox(), and fpu_twotox();
 */

/*
 *                  x^2   x^3   x^4
 * exp(x) = 1 + x + --- + --- + --- + ...
 *                   2!    3!    4!
 */
static struct fpn *
fpu_etox_taylor(struct fpemu *fe)
{
	struct fpn res;
	struct fpn x;
	struct fpn s0;
	struct fpn *s1;
	struct fpn *r;
	uint32_t k;

	CPYFPN(&x, &fe->fe_f2);
	CPYFPN(&s0, &fe->fe_f2);

	/* res := 1 + x */
	fpu_const(&fe->fe_f1, FPU_CONST_1);
	r = fpu_add(fe);
	CPYFPN(&res, r);

	k = 2;
	for (; k < MAX_ITEMS; k++) {
		/* s1 = s0 * x / k */
		CPYFPN(&fe->fe_f1, &s0);
		CPYFPN(&fe->fe_f2, &x);
		r = fpu_mul(fe);

		CPYFPN(&fe->fe_f1, r);
		fpu_explode(fe, &fe->fe_f2, FTYPE_LNG, &k);
		s1 = fpu_div(fe);

		/* break if s1 is enough small */
		if (ISZERO(s1))
			break;
		if (res.fp_exp - s1->fp_exp >= EXT_FRACBITS)
			break;

		/* s0 := s1 for next loop */
		CPYFPN(&s0, s1);

		/* res += s1 */
		CPYFPN(&fe->fe_f2, s1);
		CPYFPN(&fe->fe_f1, &res);
		r = fpu_add(fe);
		CPYFPN(&res, r);
	}

	CPYFPN(&fe->fe_f2, &res);
	return &fe->fe_f2;
}

/*
 * exp(x) = 2^k * exp(r) with k = round(x / ln2) and r = x - k * ln2
 *
 * Algorithm partially taken from libm, where exp(r) is approximated by a
 * rational function of r. We use the Taylor expansion instead.
 */
struct fpn *
fpu_etox(struct fpemu *fe)
{
	struct fpn x, *fp;
	int k;

	if (ISNAN(&fe->fe_f2))
		return &fe->fe_f2;
	if (ISINF(&fe->fe_f2)) {
		if (fe->fe_f2.fp_sign)
			fpu_const(&fe->fe_f2, FPU_CONST_0);
		return &fe->fe_f2;
	}

	/*
	 * return inf if x >=  2^14
	 * return +0  if x <= -2^14
	 */
	if (fe->fe_f2.fp_exp >= 14) {
		if (fe->fe_f2.fp_sign) {
			fe->fe_f2.fp_class = FPC_ZERO;
			fe->fe_f2.fp_sign = 0;
		} else {
			fe->fe_f2.fp_class = FPC_INF;
		}
		return &fe->fe_f2;
	}

	CPYFPN(&x, &fe->fe_f2);

	/* k = round(x / ln2) */
	CPYFPN(&fe->fe_f1, &fe->fe_f2);
	fpu_const(&fe->fe_f2, FPU_CONST_LN_2);
	fp = fpu_div(fe);
	CPYFPN(&fe->fe_f2, fp);
	fp = fpu_int(fe);
	if (ISZERO(fp)) {
		/* k = 0 */
		CPYFPN(&fe->fe_f2, &x);
		fp = fpu_etox_taylor(fe);
		return fp;
	}
	/* extract k as integer format from fpn format */
	k = fp->fp_mant[0] >> (FP_LG - fp->fp_exp);
	if (fp->fp_sign)
		k *= -1;

	/* exp(r) = exp(x - k * ln2) */
	CPYFPN(&fe->fe_f1, fp);
	fpu_const(&fe->fe_f2, FPU_CONST_LN_2);
	fp = fpu_mul(fe);
	fp->fp_sign = !fp->fp_sign;
	CPYFPN(&fe->fe_f1, fp);
	CPYFPN(&fe->fe_f2, &x);
	fp = fpu_add(fe);
	CPYFPN(&fe->fe_f2, fp);
	fp = fpu_etox_taylor(fe);

	/* 2^k */
	fp->fp_exp += k;

	return fp;
}

/*
 * exp(x) - 1
 */
struct fpn *
fpu_etoxm1(struct fpemu *fe)
{
	struct fpn *fp;

	fp = fpu_etox(fe);

	CPYFPN(&fe->fe_f1, fp);
	/* build a 1.0 */
	fp = fpu_const(&fe->fe_f2, FPU_CONST_1);
	fe->fe_f2.fp_sign = !fe->fe_f2.fp_sign;
	/* fp = f2 - 1.0 */
	fp = fpu_add(fe);

	return fp;
}

/*
 * 10^x = exp(x * ln10)
 */
struct fpn *
fpu_tentox(struct fpemu *fe)
{
	struct fpn *fp;

	/* build a ln10 */
	fp = fpu_const(&fe->fe_f1, FPU_CONST_LN_10);
	/* fp = ln10 * f2 */
	fp = fpu_mul(fe);

	/* copy the result to the src opr */
	CPYFPN(&fe->fe_f2, fp);

	return fpu_etox(fe);
}

/*
 * 2^x = exp(x * ln2)
 */
struct fpn *
fpu_twotox(struct fpemu *fe)
{
	struct fpn *fp;

	/* build a ln2 */
	fp = fpu_const(&fe->fe_f1, FPU_CONST_LN_2);
	/* fp = ln2 * f2 */
	fp = fpu_mul(fe);

	/* copy the result to the src opr */
	CPYFPN(&fe->fe_f2, fp);

	return fpu_etox(fe);
}
