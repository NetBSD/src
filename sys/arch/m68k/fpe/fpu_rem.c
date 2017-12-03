/*	$NetBSD: fpu_rem.c,v 1.11.12.2 2017/12/03 11:36:23 jdolecek Exp $	*/

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
 *	@(#)fpu_rem.c	10/24/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: fpu_rem.c,v 1.11.12.2 2017/12/03 11:36:23 jdolecek Exp $");

#include <sys/types.h>
#include <sys/signal.h>
#include <machine/frame.h>

#include "fpu_emulate.h"

/*
 *       ALGORITHM
 *
 *       Step 1.  Save and strip signs of X and Y: signX := sign(X),
 *                signY := sign(Y), X := *X*, Y := *Y*, 
 *                signQ := signX EOR signY. Record whether MOD or REM
 *                is requested.
 *
 *       Step 2.  Set L := expo(X)-expo(Y), k := 0, Q := 0.
 *                If (L < 0) then
 *                   R := X, go to Step 4.
 *                else
 *                   R := 2^(-L)X, j := L.
 *                endif
 *
 *       Step 3.  Perform MOD(X,Y)
 *            3.1 If R = Y, then { Q := Q + 1, R := 0, go to Step 7. }
 *            3.2 If R > Y, then { R := R - Y, Q := Q + 1}
 *            3.3 If j = 0, go to Step 4.
 *            3.4 k := k + 1, j := j - 1, Q := 2Q, R := 2R. Go to
 *                Step 3.1.
 *
 *       Step 4.  R := signX*R.
 *
 *       Step 5.  If MOD is requested, go to Step 7.
 *
 *       Step 6.  Now, R = MOD(X,Y), convert to REM(X,Y) is requested.
 *                Do banker's rounding.
 *                If   abs(R) > Y/2
 *                 || (abs(R) == Y/2 && Q % 2 == 1) then
 *                 { Q := Q + 1, R := R - signX * Y }.
 *
 *       Step 7.  Return signQ, last 7 bits of Q, and R as required.
 */

static struct fpn * __fpu_modrem(struct fpemu *fe, int is_mod);
static int abscmp3(const struct fpn *a, const struct fpn *b);

/* Absolute FORTRAN Compare */
static int
abscmp3(const struct fpn *a, const struct fpn *b)
{
	int i;

	if (a->fp_exp < b->fp_exp) {
		return -1;
	} else if (a->fp_exp > b->fp_exp) {
		return 1;
	} else {
		for (i = 0; i < 3; i++) {
			if (a->fp_mant[i] < b->fp_mant[i])
				return -1;
			else if (a->fp_mant[i] > b->fp_mant[i])
				return 1;
		}
	}
	return 0;
}

static struct fpn *
__fpu_modrem(struct fpemu *fe, int is_mod)
{
	static struct fpn X, Y;
	struct fpn *x, *y, *r;
	uint32_t signX, signY, signQ;
	int j, k, l, q;
	int cmp;

	if (ISNAN(&fe->fe_f1) || ISNAN(&fe->fe_f2))
		return fpu_newnan(fe);
	if (ISINF(&fe->fe_f1) || ISZERO(&fe->fe_f2))
		return fpu_newnan(fe);

	CPYFPN(&X, &fe->fe_f1);
	CPYFPN(&Y, &fe->fe_f2);
	x = &X;
	y = &Y;
	q = 0;
	r = &fe->fe_f2;

	/*
	 * Step 1
	 */
	signX = x->fp_sign;
	signY = y->fp_sign;
	signQ = (signX ^ signY);
	x->fp_sign = y->fp_sign = 0;

	/* Special treatment that just return input value but Q is necessary */
	if (ISZERO(x) || ISINF(y)) {
		r = &fe->fe_f1;
		goto Step7;
	}

	/*
	 * Step 2
	 */
	l = x->fp_exp - y->fp_exp;
	k = 0;
	CPYFPN(r, x);
	if (l >= 0) {
		r->fp_exp -= l;
		j = l;

		/*
		 * Step 3
		 */
		for (;;) {
			cmp = abscmp3(r, y);

			/* Step 3.1 */
			if (cmp == 0)
				break;

			/* Step 3.2 */
			if (cmp > 0) {
				CPYFPN(&fe->fe_f1, r);
				CPYFPN(&fe->fe_f2, y);
				fe->fe_f2.fp_sign = 1;
				r = fpu_add(fe);
				q++;
			}

			/* Step 3.3 */
			if (j == 0)
				goto Step4;

			/* Step 3.4 */
			k++;
			j--;
			q += q;
			r->fp_exp++;
		}
		/* R == Y */
		q++;
		r->fp_class = FPC_ZERO;
		goto Step7;
	}
 Step4:
	r->fp_sign = signX;

	/*
	 * Step 5
	 */
	if (is_mod)
		goto Step7;

	/*
	 * Step 6
	 */
	/* y = y / 2 */
	y->fp_exp--;
	/* abscmp3 ignore sign */
	cmp = abscmp3(r, y);
	/* revert y */
	y->fp_exp++;

	if (cmp > 0 || (cmp == 0 && q % 2)) {
		q++;
		CPYFPN(&fe->fe_f1, r);
		CPYFPN(&fe->fe_f2, y);
		fe->fe_f2.fp_sign = !signX;
		r = fpu_add(fe);
	}

	/*
	 * Step 7
	 */
 Step7:
	q &= 0x7f;
	q |= (signQ << 7);
	fe->fe_fpframe->fpf_fpsr =
	fe->fe_fpsr =
	    (fe->fe_fpsr & ~FPSR_QTT) | (q << 16);
	return r;
}

struct fpn *
fpu_rem(struct fpemu *fe)
{
	return __fpu_modrem(fe, 0);
}

struct fpn *
fpu_mod(struct fpemu *fe)
{
	return __fpu_modrem(fe, 1);
}
