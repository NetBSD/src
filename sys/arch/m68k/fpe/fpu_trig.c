/*	$NetBSD: fpu_trig.c,v 1.15 2013/04/20 07:32:45 isaki Exp $	*/

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
 *	@(#)fpu_trig.c	10/24/95
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
__KERNEL_RCSID(0, "$NetBSD: fpu_trig.c,v 1.15 2013/04/20 07:32:45 isaki Exp $");

#include "fpu_emulate.h"

/*
 * arccos(x) = pi/2 - arcsin(x)
 */
struct fpn *
fpu_acos(struct fpemu *fe)
{
	struct fpn *r;

	if (ISNAN(&fe->fe_f2))
		return &fe->fe_f2;
	if (ISINF(&fe->fe_f2))
		return fpu_newnan(fe);

	r = fpu_asin(fe);
	CPYFPN(&fe->fe_f2, r);

	/* pi/2 - asin(x) */
	fpu_const(&fe->fe_f1, FPU_CONST_PI);
	fe->fe_f1.fp_exp--;
	fe->fe_f2.fp_sign = !fe->fe_f2.fp_sign;
	r = fpu_add(fe);

	return r;
}

/*
 *                          x
 * arcsin(x) = arctan(---------------)
 *                     sqrt(1 - x^2) 
 */
struct fpn *
fpu_asin(struct fpemu *fe)
{
	struct fpn x;
	struct fpn *r;

	if (ISNAN(&fe->fe_f2))
		return &fe->fe_f2;
	if (ISZERO(&fe->fe_f2))
		return &fe->fe_f2;

	if (ISINF(&fe->fe_f2))
		return fpu_newnan(fe);

	CPYFPN(&x, &fe->fe_f2);

	/* x^2 */
	CPYFPN(&fe->fe_f1, &fe->fe_f2);
	r = fpu_mul(fe);

	/* 1 - x^2 */
	CPYFPN(&fe->fe_f2, r);
	fe->fe_f2.fp_sign = 1;
	fpu_const(&fe->fe_f1, FPU_CONST_1);
	r = fpu_add(fe);

	/* sqrt(1-x^2) */
	CPYFPN(&fe->fe_f2, r);
	r = fpu_sqrt(fe);

	/* x/sqrt */
	CPYFPN(&fe->fe_f2, r);
	CPYFPN(&fe->fe_f1, &x);
	r = fpu_div(fe);

	/* arctan */
	CPYFPN(&fe->fe_f2, r);
	return fpu_atan(fe);
}

/*
 * arctan(x):
 *
 *	if (x < 0) {
 *		x = abs(x);
 *		sign = 1;
 *	}
 *	y = arctan(x);
 *	if (sign) {
 *		y = -y;
 *	}
 */
struct fpn *
fpu_atan(struct fpemu *fe)
{
	struct fpn a;
	struct fpn x;
	struct fpn v;

	if (ISNAN(&fe->fe_f2))
		return &fe->fe_f2;
	if (ISZERO(&fe->fe_f2))
		return &fe->fe_f2;

	CPYFPN(&a, &fe->fe_f2);

	if (ISINF(&fe->fe_f2)) {
		/* f2 <- pi/2 */
		fpu_const(&fe->fe_f2, FPU_CONST_PI);
		fe->fe_f2.fp_exp--;

		fe->fe_f2.fp_sign = a.fp_sign;
		return &fe->fe_f2;
	}

	fpu_const(&x, FPU_CONST_1);
	fpu_const(&fe->fe_f2, FPU_CONST_0);
	CPYFPN(&v, &fe->fe_f2);
	fpu_cordit1(fe, &x, &a, &fe->fe_f2, &v);

	return &fe->fe_f2;
}


/*
 * fe_f1 := sin(in)
 * fe_f2 := cos(in)
 */
static void
__fpu_sincos_cordic(struct fpemu *fe, const struct fpn *in)
{
	struct fpn a;
	struct fpn v;

	CPYFPN(&a, in);
	fpu_const(&fe->fe_f1, FPU_CONST_0);
	CPYFPN(&fe->fe_f2, &fpu_cordic_inv_gain1);
	fpu_const(&v, FPU_CONST_1);
	v.fp_sign = 1;
	fpu_cordit1(fe, &fe->fe_f2, &fe->fe_f1, &a, &v);
}

/*
 * cos(x):
 *
 *	if (x < 0) {
 *		x = abs(x);
 *	}
 *	if (x > 2*pi) {
 *		x %= 2*pi;
 *	}
 *	if (x > pi) {
 *		x -= pi;
 *		sign inverse;
 *	}
 *	if (x > pi/2) {
 *		y = sin(x - pi/2);
 *		sign inverse;
 *	} else {
 *		y = cos(x);
 *	}
 *	if (sign) {
 *		y = -y;
 *	}
 */
struct fpn *
fpu_cos(struct fpemu *fe)
{
	struct fpn x;
	struct fpn p;
	struct fpn *r;
	int sign;

	if (ISNAN(&fe->fe_f2))
		return &fe->fe_f2;
	if (ISINF(&fe->fe_f2))
		return fpu_newnan(fe);

	CPYFPN(&x, &fe->fe_f2);

	/* x = abs(input) */
	x.fp_sign = 0;
	sign = 0;

	/* p <- 2*pi */
	fpu_const(&p, FPU_CONST_PI);
	p.fp_exp++;

	/*
	 * if (x > 2*pi*N)
	 *  cos(x) is cos(x - 2*pi*N)
	 */
	CPYFPN(&fe->fe_f1, &x);
	CPYFPN(&fe->fe_f2, &p);
	r = fpu_cmp(fe);
	if (r->fp_sign == 0) {
		CPYFPN(&fe->fe_f1, &x);
		CPYFPN(&fe->fe_f2, &p);
		r = fpu_mod(fe);
		CPYFPN(&x, r);
	}

	/* p <- pi */
	p.fp_exp--;

	/*
	 * if (x > pi)
	 *  cos(x) is -cos(x - pi)
	 */
	CPYFPN(&fe->fe_f1, &x);
	CPYFPN(&fe->fe_f2, &p);
	fe->fe_f2.fp_sign = 1;
	r = fpu_add(fe);
	if (r->fp_sign == 0) {
		CPYFPN(&x, r);
		sign ^= 1;
	}

	/* p <- pi/2 */
	p.fp_exp--;

	/*
	 * if (x > pi/2)
	 *  cos(x) is -sin(x - pi/2)
	 * else
	 *  cos(x)
	 */
	CPYFPN(&fe->fe_f1, &x);
	CPYFPN(&fe->fe_f2, &p);
	fe->fe_f2.fp_sign = 1;
	r = fpu_add(fe);
	if (r->fp_sign == 0) {
		__fpu_sincos_cordic(fe, r);
		r = &fe->fe_f1;
		sign ^= 1;
	} else {
		__fpu_sincos_cordic(fe, &x);
		r = &fe->fe_f2;
	}
	r->fp_sign = sign;
	return r;
}

/*
 * sin(x):
 *
 *	if (x < 0) {
 *		x = abs(x);
 *		sign = 1;
 *	}
 *	if (x > 2*pi) {
 *		x %= 2*pi;
 *	}
 *	if (x > pi) {
 *		x -= pi;
 *		sign inverse;
 *	}
 *	if (x > pi/2) {
 *		y = cos(x - pi/2);
 *	} else {
 *		y = sin(x);
 *	}
 *	if (sign) {
 *		y = -y;
 *	}
 */
struct fpn *
fpu_sin(struct fpemu *fe)
{
	struct fpn x;
	struct fpn p;
	struct fpn *r;
	int sign;

	if (ISNAN(&fe->fe_f2))
		return &fe->fe_f2;
	if (ISINF(&fe->fe_f2))
		return fpu_newnan(fe);

	/* if x is +0/-0, return +0/-0 */
	if (ISZERO(&fe->fe_f2))
		return &fe->fe_f2;

	CPYFPN(&x, &fe->fe_f2);

	/* x = abs(input) */
	sign = x.fp_sign;
	x.fp_sign = 0;

	/* p <- 2*pi */
	fpu_const(&p, FPU_CONST_PI);
	p.fp_exp++;

	/*
	 * if (x > 2*pi*N)
	 *  sin(x) is sin(x - 2*pi*N)
	 */
	CPYFPN(&fe->fe_f1, &x);
	CPYFPN(&fe->fe_f2, &p);
	r = fpu_cmp(fe);
	if (r->fp_sign == 0) {
		CPYFPN(&fe->fe_f1, &x);
		CPYFPN(&fe->fe_f2, &p);
		r = fpu_mod(fe);
		CPYFPN(&x, r);
	}

	/* p <- pi */
	p.fp_exp--;

	/*
	 * if (x > pi)
	 *  sin(x) is -sin(x - pi)
	 */
	CPYFPN(&fe->fe_f1, &x);
	CPYFPN(&fe->fe_f2, &p);
	fe->fe_f2.fp_sign = 1;
	r = fpu_add(fe);
	if (r->fp_sign == 0) {
		CPYFPN(&x, r);
		sign ^= 1;
	}

	/* p <- pi/2 */
	p.fp_exp--;

	/*
	 * if (x > pi/2)
	 *  sin(x) is cos(x - pi/2)
	 * else
	 *  sin(x)
	 */
	CPYFPN(&fe->fe_f1, &x);
	CPYFPN(&fe->fe_f2, &p);
	fe->fe_f2.fp_sign = 1;
	r = fpu_add(fe);
	if (r->fp_sign == 0) {
		__fpu_sincos_cordic(fe, r);
		r = &fe->fe_f2;
	} else {
		__fpu_sincos_cordic(fe, &x);
		r = &fe->fe_f1;
	}
	r->fp_sign = sign;
	return r;
}

/*
 * tan(x) = sin(x) / cos(x)
 */
struct fpn *
fpu_tan(struct fpemu *fe)
{
	struct fpn x;
	struct fpn s;
	struct fpn *r;

	if (ISNAN(&fe->fe_f2))
		return &fe->fe_f2;
	if (ISINF(&fe->fe_f2))
		return fpu_newnan(fe);

	/* if x is +0/-0, return +0/-0 */
	if (ISZERO(&fe->fe_f2))
		return &fe->fe_f2;

	CPYFPN(&x, &fe->fe_f2);

	/* sin(x) */
	CPYFPN(&fe->fe_f2, &x);
	r = fpu_sin(fe);
	CPYFPN(&s, r);

	/* cos(x) */
	CPYFPN(&fe->fe_f2, &x);
	r = fpu_cos(fe);
	CPYFPN(&fe->fe_f2, r);

	CPYFPN(&fe->fe_f1, &s);
	r = fpu_div(fe);
	return r;
}

struct fpn *
fpu_sincos(struct fpemu *fe, int regc)
{
	__fpu_sincos_cordic(fe, &fe->fe_f2);

	/* cos(x) */
	fpu_implode(fe, &fe->fe_f2, FTYPE_EXT, &fe->fe_fpframe->fpf_regs[regc]);

	/* sin(x) */
	return &fe->fe_f1;
}
