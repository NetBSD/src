/*	$NetBSD: fpu_int.c,v 1.10.8.1 2012/07/04 20:30:00 jdc Exp $	*/

/*
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
 *	@(#)fpu_int.c
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: fpu_int.c,v 1.10.8.1 2012/07/04 20:30:00 jdc Exp $");

#include <sys/types.h>

#include <machine/reg.h>

#include "fpu_arith.h"
#include "fpu_emulate.h"

/* FINTRZ - always round to zero */
struct fpn *
fpu_intrz(struct fpemu *fe)
{
	register struct fpn *x = &fe->fe_f2;
	register int sh, clr, mask, i;

	/* special cases first */
	if (x->fp_class != FPC_NUM) {
		return x;
	}
	/* when |x| < 1.0 */
	if (x->fp_exp < 0) {
		x->fp_class = FPC_ZERO;
		x->fp_mant[0] = x->fp_mant[1] = x->fp_mant[2] = 0;
		return x;
	}

	/* real work */
	sh = FP_NMANT - 1 - x->fp_exp;
	if (sh <= 0) {
		return x;
	}

	clr = 2 - sh / 32;
	mask = (0xffffffff << (sh % 32));

	for (i = 2; i > clr; i--) {
		x->fp_mant[i] = 0;
	}
	x->fp_mant[i] &= mask;

	return x;
}

/* FINT */
struct fpn *
fpu_int(struct fpemu *fe)
{
	register struct fpn *x = &fe->fe_f2;
	register int rsh;

	/* special cases first */
	if (x->fp_class != FPC_NUM) {
		return x;
	}

	rsh = FP_NMANT - 1 - x->fp_exp;
	if (rsh <= FP_NG) {
		return x;
	}

	/* shift to the right */
	x->fp_exp = 0;
	fpu_shr(x, rsh - FP_NG);

	/* round according to FPCR round mode */
	fpu_round(fe, x);

	/* shift it back to the left */
	x->fp_exp = FP_NMANT - 1;
	fpu_norm(x);

	return x;
}
