/*	$NetBSD: ieee_subnormal.c,v 1.6 2006/05/12 06:05:23 simonb Exp $	*/

/*
 * IEEE floating point support for NS32081 and NS32381 fpus.
 * Copyright (c) 1995 Ian Dall
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * IAN DALL ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS" CONDITION.
 * IAN DALL DISCLAIMS ANY LIABILITY OF ANY KIND FOR ANY DAMAGES
 * WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 */
/*
 *	File:	ieee_subnormal.c
 *	Author:	Ian Dall
 *	Date:	November 1995
 *
 *	Handle operations which generated underflow traps. Subnormal
 *	(denormalized numbers) are generated as required.
 *
 * HISTORY
 * 14-Dec-95  Ian Dall (Ian.Dall@dsto.defence.gov.au)
 *	First release.
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ieee_subnormal.c,v 1.6 2006/05/12 06:05:23 simonb Exp $");

#include "ieee_internal.h"

#include <machine/psl.h>

/* Return bit pos numbered from lsb 0 to 31. Returns 31 if no bit is set */
static int
find_msb(unsigned int t)
{
	static const int b_mask[] = {
		0xffff0000,
		0xff00ff00,
		0xf0f0f0f0,
		0xcccccccc,
		0xaaaaaaaa
	};
	int i;
	int pos = 31;
	int bit_incr = 16;		/* Half no of bits in int */

	for (i = 0; i < 5; i++, bit_incr /= 2) {
		if (t & b_mask[i]) {
			t &= b_mask[i];
		} else {
			pos -= bit_incr;
			t &= ~b_mask[i];
		}
	}
	return pos;
}

static int
leading_zeros(union t_conv *data)
{
	unsigned int t;

	if ((t = data->d_bits.mantissa)) {
		return 19 - find_msb(t);
	} else if ((t = data->d_bits.mantissa2)) {
		return 51 - find_msb(t);
	} else
		return 52;
}

static void
lshift_mantissa(union t_conv *data, int n)
{
	unsigned long t[2];

	t[1] = data->d_bits.mantissa;
	t[0] = data->d_bits.mantissa2;
	*(unsigned long long *) t <<= n;
	data->d_bits.mantissa = t[1];
	data->d_bits.mantissa2 = t[0];
}


static void
rshift_mantissa(union t_conv *data, int n)
{
	unsigned long t[2];

	t[1] = data->d_bits.mantissa | 0x100000;
	t[0] = data->d_bits.mantissa2;
	*(unsigned long long *) t >>= n;
	data->d_bits.mantissa = t[1];
	data->d_bits.mantissa2 = t[0];
}

/*
 * After this, the data is a normal double and the returned value is
 * such that:
 *   union t_conv t;
 *   t = *data;
 *   norm = normalize(&t);
 *   2**norm * t.d == data->d;
 *
 * Assume data is not already normalized.
 */
int
ieee_normalize(union t_conv *data)
{
	int norm;

	if (data->d_bits.exp != 0)
		return 0;
	norm = leading_zeros(data) + 1; /* plus one for the implied bit */
	data->d_bits.exp = 1;
	lshift_mantissa(data, norm);
	return -norm;
}

/* Divide by 2**n producing a denormalized no if necessary */
static void
denormalize(union t_conv *data, int n)
{
	int exp = data->d_bits.exp;

	if (exp > n)
		exp -= n;
	else if (exp <= n) {
		rshift_mantissa(data, n - exp + 1);
			/* plus 1 for the implied bit */
		exp = 0;
	}
	data->d_bits.exp = exp;
}

static int
scale_and_check(union t_conv *d, int scale)
{
	int exp;

	exp = d->d_bits.exp - scale;
	if (exp >= 0x7ff) {
		/* Overflow */
		d->d_bits.exp = 0x7ff;
		d->d_bits.mantissa = 0;
		d->d_bits.mantissa2 = 0;		/* XXX sig */
		return FPC_TT_OVFL;
	}
	if (exp <= 0) {
		/* Underflow */
		denormalize(d, scale);	/* XXX sig */
		return FPC_TT_UNDFL;
	}
	d->d_bits.exp = exp;
	return FPC_TT_NONE;
}

/*
 * Add two doubles, not caring if one or both is a de-norm.
 * Strategy: First scale and normalize operands so the addition
 * can't overflow or underflow, then do a normal floating point
 * addition, then scale back and possibly denormalize.
 */
int
ieee_add(double data1, double *data2)
{
	union t_conv d1 = (union t_conv) data1;
	union t_conv *d2 = (union t_conv *) data2;
	int scale;
	int norm1 = ieee_normalize(&d1);
	int norm2 = ieee_normalize(d2);
	int exp1 = d1.d_bits.exp + norm1;
	int exp2 = d2->d_bits.exp + norm2;

	if (exp1 > exp2) {
		scale = EXP_DBIAS - exp1;
		exp1 = EXP_DBIAS;
		exp2 += scale;
	} else {
		scale = EXP_DBIAS - exp2;
		exp2 = EXP_DBIAS;
		exp1 += scale;
	}
	if (exp1 > 0) {
		d1.d_bits.exp = exp1;
		if (exp2 > 0) {
			d2->d_bits.exp = exp2;
			d2->d += d1.d;
		} else {
			d2->d = d1.d;
		}
	} else {
		d2->d_bits.exp = exp2;
	}
	return scale_and_check(d2, scale);
}

/*
 * Multiply two doubles, not caring if one or both is a de-norm.
 * Strategy: First scale and normalize operands so the multiplication
 * can't overflow or underflow, then do a normal floating point
 * addition, then scale back and possibly denormalize.
 */
int
ieee_mul(double data1, double *data2)
{
	union t_conv d1 = (union t_conv) data1;
	union t_conv *d2 = (union t_conv *) data2;
	int scale;
	int norm1 = ieee_normalize(&d1);
	int norm2 = ieee_normalize(d2);
	int exp1 = d1.d_bits.exp + norm1;
	int exp2 = d2->d_bits.exp + norm2;

	d1.d_bits.exp = EXP_DBIAS;	/* Add EXP_DBIAS - exp1 */
	d2->d_bits.exp = EXP_DBIAS;
	d2->d *= d1.d;
	scale = EXP_DBIAS - exp1 + EXP_DBIAS - exp2;
	return scale_and_check(d2, scale);
}

/*
 * Divide d2 by d1, not caring if one or both is a de-norm.
 * Strategy: First scale and normalize operands so the division
 * can't overflow or underflow, then do a normal floating point
 * division, then scale back and possibly denormalize.
 */
int
ieee_div(double data1, double *data2)
{
	union t_conv d1 = (union t_conv) data1;
	union t_conv *d2 = (union t_conv *) data2;
	int scale;
	int norm1 = ieee_normalize(&d1);
	int norm2 = ieee_normalize(d2);
	int exp1 = d1.d_bits.exp + norm1;
	int exp2 = d2->d_bits.exp + norm2;

	d1.d_bits.exp = EXP_DBIAS;	/* Add EXP_DBIAS - exp1 */
	d2->d_bits.exp = EXP_DBIAS;
	d2->d /= d1.d;
	scale = exp1 - exp2;
	return scale_and_check(d2, scale);
}

/*
 * Add mul-add three doubles d1 * d2 + d3 -> d3, not caring if any a de-norm.
 * Strategy: First scale and normalize operands so the operations
 * can't overflow or underflow, then do a normal floating point operation
 * addition, then scale back and possibly denormalize.
 */
int
ieee_dot(double data1, double data2, double *data3)
{
	union t_conv d1 = (union t_conv) data1;
	union t_conv d2 = (union t_conv) data2;
	union t_conv *d3 = (union t_conv *) data3;
	int scale;
	int norm1 = ieee_normalize(&d1);
	int norm2 = ieee_normalize(&d2);
	int norm3 = ieee_normalize(d3);
	int exp1 = d1.d_bits.exp + norm1;
	int exp2 = d2.d_bits.exp + norm2;
	int exp3 = d3->d_bits.exp + norm3;
	int exp_prod = exp1 + exp2;

	if (exp_prod > exp3) {
		scale = EXP_DBIAS + EXP_DBIAS - exp_prod;
		exp1 = EXP_DBIAS;		/* Add EXP_DBIAS - exp1 */
		exp2 = EXP_DBIAS;
		exp3 += scale;
	} else {
		scale = EXP_DBIAS - exp3;
		exp3 = EXP_DBIAS;
		exp1 = (exp_prod + scale)/2;
		exp2 = exp_prod + scale - exp1;
	}

	if (exp1 > 0 && exp2 > 0) {
		d1.d_bits.exp = exp1;
		d2.d_bits.exp = exp2;
		if (exp3 > 0) {
			d3->d_bits.exp = exp3;
			d3->d += d1.d * d2.d;
		} else {
			d3->d = d1.d * d2.d;
		}
	} else {
		d3->d_bits.exp = exp3;
	}
	return scale_and_check(d3, scale);
}

/*
 * Compare the magnitude of two ops.
 * return:	1  |op1| > |op2|
 *	       -1  |op1| < |op2|
 *		0  |op1| == |op2|
 */
static int
u_cmp(double data1, double data2)
{
	union t_conv d1 = (union t_conv) data1;
	union t_conv d2 = (union t_conv) data2;
	int exp1 = d1.d_bits.exp;
	int exp2 = d2.d_bits.exp;

	if (exp1 > exp2)
		return 1;
	else if (exp1 < exp2)
		return -1;
	else if (d1.d_bits.mantissa > d2.d_bits.mantissa)
		return 1;
	else if (d1.d_bits.mantissa < d2.d_bits.mantissa)
		return -1;
	else if (d1.d_bits.mantissa2 > d2.d_bits.mantissa2)
		return 1;
	else if (d1.d_bits.mantissa2 < d2.d_bits.mantissa2)
		return -1;
	else
		return 0;
}

void
ieee_cmp(double data1, double data2, state *mystate)
{
	union t_conv d1 = (union t_conv) data1;
	union t_conv d2 = (union t_conv) data2;
	int sign1 = d1.d_bits.sign;
	int sign2 = d2.d_bits.sign;
	int cmp;

	mystate->PSR &= ~(PSR_N | PSR_Z | PSR_L);
	switch(sign2 * 2 + sign1) {
	case 2:			/* op2 is negative op1 is positive */
		mystate->PSR |= PSR_N;
		break;
	case 1:			/* op2 is positive op1 is negative */
		break;
	case 0:			/* Both ops same sign */
	case 3:
		cmp = u_cmp(d1.d, d2.d);

		if (sign1)
			cmp *= -1;
		if (cmp > 0)
			mystate->PSR |= PSR_N;
		else if (cmp == 0)
			mystate->PSR |= PSR_Z;
		break;
	}
	return;
}


int
ieee_scalb(double data1, double *data2)
{
	union t_conv d1 = (union t_conv) data1;
	union t_conv *d2 = (union t_conv *) data2;
	int exp2 = d2->d_bits.exp - EXP_DBIAS;
	int n;

	if (exp2 > 16) {
		*d2 = infty;
		d2->d_bits.sign = d1.d_bits.sign;
		return FPC_TT_OVFL;
	} else if (exp2 < -16) {
		d2->d = 0.0;
		d2->d_bits.sign = d1.d_bits.sign;
		return FPC_TT_OVFL;
	}
	n = d2->d;
	*d2 = d1;
	return scale_and_check(d2, n);
}

/*
 * With no trap, hardware produces zero, which is fast but not
 * strictly correct. We should always have the hardware trap bit set
 * and generate denormalized numbers by simulation unless the user
 * indicates via the FPC_UNDE flag they want to handle it.
 */

int ieee_undfl(struct operand *op1, struct operand *op2, struct operand *f0_op,
    int xopcode, state *mystate)
{
	unsigned int fsr = mystate->FSR;
	int user_trap = FPC_TT_NONE;

	DP(1, "Underflow trap: xopcode = 0x%x\n", xopcode);
	if (fsr & FPC_UNDE) {
		user_trap = FPC_TT_UNDFL;
	} else {
		user_trap = FPC_TT_NONE;
		/*
		 * Calculate correct denormal output. The easiest way is to
		 * prescale the operands so they won't underflow, then use the
		 * hardware operation, then post scale.
		 */

		/*
		 * The exact sematics are a bit tricky. Apparently, we should
		 * only set flag if we underflowed *and* there was loss of
		 * precision.
		 * For now, just set the flag always  XXX
		 */
		fsr |= FPC_UF;

		switch(xopcode) {
		case NEGF:
			op1->data.d_bits.sign ^= 1;
			/* Fall through */
		case MOVF:
		case MOVLF:
		case MOVFL:
			op2->data = op1->data;
			break;
		case CMPF:
			ieee_cmp(op1->data.d, op2->data.d, mystate);
			break;
		case SUBF:
			op1->data.d_bits.sign ^= 1;
			/* Fall through */
		case ADDF:
			user_trap = ieee_add(op1->data.d, &op2->data.d);
			break;
		case MULF:
			user_trap = ieee_mul(op1->data.d, &op2->data.d);
			break;
		case DIVF:
			user_trap = ieee_div(op1->data.d, &op2->data.d);
			break;
		case ROUNDFI:
		case TRUNCFI:
		case FLOORFI:
			op2->data.i = 0;
			break;
		case SCALBF:
			user_trap = ieee_scalb(op1->data.d, &op2->data.d);
			break;
		case LOGBF:
			op2->data.d = 0.0;
			break;
		case DOTF:
			user_trap =
			    ieee_dot(op1->data.d, op2->data.d, &f0_op->data.d);
			break;
		case POLYF: {
			union t_conv t = op2->data;
			user_trap = ieee_dot(f0_op->data.d, op1->data.d, &t.d);
			f0_op->data = t;
			}
			break;
		}
	}
	mystate->FSR = fsr;
	return user_trap;
}
