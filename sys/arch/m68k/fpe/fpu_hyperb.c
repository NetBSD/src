/*	$NetBSD: fpu_hyperb.c,v 1.7 2013/03/23 12:06:24 isaki Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: fpu_hyperb.c,v 1.7 2013/03/23 12:06:24 isaki Exp $");

#include "fpu_emulate.h"

/*
 * fpu_hyperb.c: defines the following functions
 *
 *	fpu_atanh(), fpu_cosh(), fpu_sinh(), and fpu_tanh()
 */

struct fpn *
fpu_atanh(struct fpemu *fe)
{
	/* stub */
	return &fe->fe_f2;
}

struct fpn *
fpu_cosh(struct fpemu *fe)
{
	struct fpn s0;
	struct fpn *r;
	int hyperb = 1;

	if (ISNAN(&fe->fe_f2))
		return &fe->fe_f2;

	if (ISINF(&fe->fe_f2)) {
		fe->fe_f2.fp_sign = 0;
		return &fe->fe_f2;
	}

	fpu_const(&s0, 0x32);	/* 1.0 */
	r = fpu_sincos_taylor(fe, &s0, 1, hyperb);
	CPYFPN(&fe->fe_f2, r);

	return &fe->fe_f2;
}

struct fpn *
fpu_sinh(struct fpemu *fe)
{
	struct fpn s0;
	struct fpn *r;
	int hyperb = 1;

	if (ISNAN(&fe->fe_f2))
		return &fe->fe_f2;
	if (ISINF(&fe->fe_f2))
		return &fe->fe_f2;

	CPYFPN(&s0, &fe->fe_f2);
	r = fpu_sincos_taylor(fe, &s0, 2, hyperb);
	CPYFPN(&fe->fe_f2, r);

	return &fe->fe_f2;
}

struct fpn *
fpu_tanh(struct fpemu *fe)
{
	struct fpn x;
	struct fpn s;
	struct fpn *r;
	int sign;

	if (ISNAN(&fe->fe_f2))
		return &fe->fe_f2;

	if (ISINF(&fe->fe_f2)) {
		sign = fe->fe_f2.fp_sign;
		fpu_const(&fe->fe_f2, 0x32);
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

	CPYFPN(&fe->fe_f2, r);

	return &fe->fe_f2;
}
