/*	$NetBSD: fpu_hyperb.c,v 1.6.12.3 2017/12/03 11:36:23 jdolecek Exp $	*/

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
 *	@(#)fpu_hyperb.c	10/24/95
 */

/*
 * Copyright (c) 2011 Tetsuya Isaki. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: fpu_hyperb.c,v 1.6.12.3 2017/12/03 11:36:23 jdolecek Exp $");

#include <machine/ieee.h>

#include "fpu_emulate.h"

/*
 * fpu_hyperb.c: defines the following functions
 *
 *	fpu_atanh(), fpu_cosh(), fpu_sinh(), and fpu_tanh()
 */

/*
 *             1       1 + x
 * atanh(x) = ---*log(-------)
 *             2       1 - x
 */
struct fpn *
fpu_atanh(struct fpemu *fe)
{
	struct fpn x;
	struct fpn t;
	struct fpn *r;

	if (ISNAN(&fe->fe_f2))
		return &fe->fe_f2;
	if (ISINF(&fe->fe_f2))
		return fpu_newnan(fe);

	/* if x is +0/-0, return +0/-0 */
	if (ISZERO(&fe->fe_f2))
		return &fe->fe_f2;

	/*
	 * -INF if x == -1
	 * +INF if x ==  1
	 */
	r = &fe->fe_f2;
	if (r->fp_exp == 0 && r->fp_mant[0] == FP_1 &&
	    r->fp_mant[1] == 0 && r->fp_mant[2] == 0) {
		r->fp_class = FPC_INF;
		return r;
	}

	/* NAN if |x| > 1 */
	if (fe->fe_f2.fp_exp >= 0)
		return fpu_newnan(fe);

	CPYFPN(&x, &fe->fe_f2);

	/* t := 1 - x */
	fpu_const(&fe->fe_f1, FPU_CONST_1);
	fe->fe_f2.fp_sign = !fe->fe_f2.fp_sign;
	r = fpu_add(fe);
	CPYFPN(&t, r);

	/* r := 1 + x */
	fpu_const(&fe->fe_f1, FPU_CONST_1);
	CPYFPN(&fe->fe_f2, &x);
	r = fpu_add(fe);

	/* (1-x)/(1+x) */
	CPYFPN(&fe->fe_f1, r);
	CPYFPN(&fe->fe_f2, &t);
	r = fpu_div(fe);

	/* log((1-x)/(1+x)) */
	CPYFPN(&fe->fe_f2, r);
	r = fpu_logn(fe);

	/* r /= 2 */
	r->fp_exp--;

	return r;
}

/*
 *            exp(x) + exp(-x)
 * cosh(x) = ------------------
 *                   2
 */
struct fpn *
fpu_cosh(struct fpemu *fe)
{
	struct fpn x, *fp;

	if (ISNAN(&fe->fe_f2))
		return &fe->fe_f2;

	if (ISINF(&fe->fe_f2)) {
		fe->fe_f2.fp_sign = 0;
		return &fe->fe_f2;
	}

	/* if x is +0/-0, return 1 */ /* XXX is this necessary? */
	if (ISZERO(&fe->fe_f2)) {
		fpu_const(&fe->fe_f2, FPU_CONST_1);
		return &fe->fe_f2;
	}

	fp = fpu_etox(fe);
	CPYFPN(&x, fp);

	fpu_const(&fe->fe_f1, FPU_CONST_1);
	CPYFPN(&fe->fe_f2, fp);
	fp = fpu_div(fe);

	CPYFPN(&fe->fe_f1, fp);
	CPYFPN(&fe->fe_f2, &x);
	fp = fpu_add(fe);

	fp->fp_exp--;

	return fp;
}

/*
 *            exp(x) - exp(-x)
 * sinh(x) = ------------------
 *                   2
 */
struct fpn *
fpu_sinh(struct fpemu *fe)
{
	struct fpn x, *fp;

	if (ISNAN(&fe->fe_f2))
		return &fe->fe_f2;
	if (ISINF(&fe->fe_f2))
		return &fe->fe_f2;

	/* if x is +0/-0, return +0/-0 */
	if (ISZERO(&fe->fe_f2))
		return &fe->fe_f2;

	fp = fpu_etox(fe);
	CPYFPN(&x, fp);

	fpu_const(&fe->fe_f1, FPU_CONST_1);
	CPYFPN(&fe->fe_f2, fp);
	fp = fpu_div(fe);

	fp->fp_sign = 1;
	CPYFPN(&fe->fe_f1, fp);
	CPYFPN(&fe->fe_f2, &x);
	fp = fpu_add(fe);

	fp->fp_exp--;

	return fp;
}

/*
 *            sinh(x)
 * tanh(x) = ---------
 *            cosh(x)
 */
struct fpn *
fpu_tanh(struct fpemu *fe)
{
	struct fpn x;
	struct fpn s;
	struct fpn *r;
	int sign;

	if (ISNAN(&fe->fe_f2))
		return &fe->fe_f2;

	/* if x is +0/-0, return +0/-0 */
	if (ISZERO(&fe->fe_f2))
		return &fe->fe_f2;

	if (ISINF(&fe->fe_f2)) {
		sign = fe->fe_f2.fp_sign;
		fpu_const(&fe->fe_f2, FPU_CONST_1);
		fe->fe_f2.fp_sign = sign;
		return &fe->fe_f2;
	}

	CPYFPN(&x, &fe->fe_f2);

	/* sinh(x) */
	CPYFPN(&fe->fe_f2, &x);
	r = fpu_sinh(fe);
	CPYFPN(&s, r);

	/* cosh(x) */
	CPYFPN(&fe->fe_f2, &x);
	r = fpu_cosh(fe);
	CPYFPN(&fe->fe_f2, r);

	CPYFPN(&fe->fe_f1, &s);
	r = fpu_div(fe);

	return r;
}
