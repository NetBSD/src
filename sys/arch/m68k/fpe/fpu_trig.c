/*	$NetBSD: fpu_trig.c,v 1.10 2013/04/18 13:40:25 isaki Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: fpu_trig.c,v 1.10 2013/04/18 13:40:25 isaki Exp $");

#include "fpu_emulate.h"

static inline struct fpn *fpu_cos_halfpi(struct fpemu *);
static inline struct fpn *fpu_sin_halfpi(struct fpemu *);

struct fpn *
fpu_acos(struct fpemu *fe)
{
	/* stub */
	return &fe->fe_f2;
}

struct fpn *
fpu_asin(struct fpemu *fe)
{
	/* stub */
	return &fe->fe_f2;
}

struct fpn *
fpu_atan(struct fpemu *fe)
{
	/* stub */
	return &fe->fe_f2;
}

/*
 * taylor expansion used by sin(), cos(), sinh(), cosh().
 * hyperb is for sinh(), cosh().
 */
struct fpn *
fpu_sincos_taylor(struct fpemu *fe, struct fpn *s0, uint32_t f, int hyperb)
{
	struct fpn res;
	struct fpn x2;
	struct fpn *s1;
	struct fpn *r;
	int sign;
	uint32_t k;

	/* x2 := x * x */
	CPYFPN(&fe->fe_f1, &fe->fe_f2);
	r = fpu_mul(fe);
	CPYFPN(&x2, r);

	/* res := s0 */
	CPYFPN(&res, s0);

	sign = 1;	/* sign := (-1)^n */

	for (;;) {
		/* (f1 :=) s0 * x^2 */
		CPYFPN(&fe->fe_f1, s0);
		CPYFPN(&fe->fe_f2, &x2);
		r = fpu_mul(fe);
		CPYFPN(&fe->fe_f1, r);

		/*
		 * for sin(),  s1 := s0 * x^2 / (2n+1)2n
		 * for cos(),  s1 := s0 * x^2 / 2n(2n-1)
		 */
		k = f * (f + 1);
		fpu_explode(fe, &fe->fe_f2, FTYPE_LNG, &k);
		s1 = fpu_div(fe);

		/* break if s1 is enough small */
		if (ISZERO(s1))
			break;
		if (res.fp_exp - s1->fp_exp >= FP_NMANT)
			break;

		/* s0 := s1 for next loop */
		CPYFPN(s0, s1);

		CPYFPN(&fe->fe_f2, s1);
		if (!hyperb) {
			/* (-1)^n */
			fe->fe_f2.fp_sign = sign;
		}

		/* res += s1 */
		CPYFPN(&fe->fe_f1, &res);
		r = fpu_add(fe);
		CPYFPN(&res, r);

		f += 2;
		sign ^= 1;
	}

	CPYFPN(&fe->fe_f2, &res);
	return &fe->fe_f2;
}

/*
 *           inf   (-1)^n    2n
 * cos(x) = \sum { ------ * x   }
 *           n=0     2n!
 *
 *              x^2   x^4   x^6
 *        = 1 - --- + --- - --- + ...
 *               2!    4!    6!
 *
 *        = s0 - s1 + s2  - s3  + ...
 *
 *               x*x           x*x           x*x
 * s0 = 1,  s1 = ---*s0,  s2 = ---*s1,  s3 = ---*s2, ...
 *               1*2           3*4           5*6
 *
 * here 0 <= x < pi/2
 */
static inline struct fpn *
fpu_cos_halfpi(struct fpemu *fe)
{
	struct fpn s0;

	/* s0 := 1 */
	fpu_const(&s0, FPU_CONST_1);

	return fpu_sincos_taylor(fe, &s0, 1, 0);
}

/*
 *          inf    (-1)^n     2n+1
 * sin(x) = \sum { ------- * x     }
 *          n=0    (2n+1)!
 *
 *              x^3   x^5   x^7
 *        = x - --- + --- - --- + ...
 *               3!    5!    7!
 *
 *        = s0 - s1 + s2  - s3  + ...
 *
 *               x*x           x*x           x*x
 * s0 = x,  s1 = ---*s0,  s2 = ---*s1,  s3 = ---*s2, ...
 *               2*3           4*5           6*7
 *
 * here 0 <= x < pi/2
 */
static inline struct fpn *
fpu_sin_halfpi(struct fpemu *fe)
{
	struct fpn s0;

	/* s0 := x */
	CPYFPN(&s0, &fe->fe_f2);

	return fpu_sincos_taylor(fe, &s0, 2, 0);
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
		CPYFPN(&fe->fe_f2, r);
		r = fpu_sin_halfpi(fe);
		sign ^= 1;
	} else {
		CPYFPN(&fe->fe_f2, &x);
		r = fpu_cos_halfpi(fe);
	}

	CPYFPN(&fe->fe_f2, r);
	fe->fe_f2.fp_sign = sign;

	return &fe->fe_f2;
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
		CPYFPN(&fe->fe_f2, r);
		r = fpu_cos_halfpi(fe);
	} else {
		CPYFPN(&fe->fe_f2, &x);
		r = fpu_sin_halfpi(fe);
	}

	CPYFPN(&fe->fe_f2, r);
	fe->fe_f2.fp_sign = sign;

	return &fe->fe_f2;
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

	CPYFPN(&fe->fe_f2, r);

	return &fe->fe_f2;
}

struct fpn *
fpu_sincos(struct fpemu *fe, int regc)
{
	struct fpn x;
	struct fpn *r;

	CPYFPN(&x, &fe->fe_f2);

	/* cos(x) */
	r = fpu_cos(fe);
	fpu_implode(fe, r, FTYPE_EXT, &fe->fe_fpframe->fpf_regs[regc]);

	/* sin(x) */
	CPYFPN(&fe->fe_f2, &x);
	r = fpu_sin(fe);
	return r;
}
