/*	$NetBSD: ieee_invop.c,v 1.9.12.1 2006/05/24 15:48:14 tron Exp $	*/

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
 *	File:	ieee_invop.c
 *	Author:	Ian Dall
 *	Date:	November 1995
 *
 *	Handle operations which generated reserved operand traps.
 *
 * HISTORY
 * 23-Apr-96  Ian Dall (Ian.Dall@dsto.defence.gov.au)
 *	Don't change sign of NaN operands of NEGF and SUBF.
 *
 * 23-Apr-96  Ian Dall (Ian.Dall@dsto.defence.gov.au)
 *	Operations on signaling NaN's always produce a quiet NaN.
 *
 * 08-Apr-96  Ian Dall (Ian.Dall@dsto.defence.gov.au)
 *	Generate correct return value so flags get set properly.
 *
 * 05-Apr-96  Ian Dall (Ian.Dall@dsto.defence.gov.au)
 *	Get subnormal number divided by subnormal number right.
 *	Get infinty multiplied by zero right.
 *
 * 02-Apr-96  Ian Dall (Ian.Dall@dsto.defence.gov.au)
 *	Make 0/0 produce NaN.
 *
 * 14-Dec-95  Ian Dall (Ian.Dall@dsto.defence.gov.au)
 *	First release.
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ieee_invop.c,v 1.9.12.1 2006/05/24 15:48:14 tron Exp $");

#include "ieee_internal.h"

#include <machine/psl.h>
#if defined(__NetBSD__) && defined(_KERNEL)
#include <machine/limits.h>
#else
#include <limits.h>
#endif

static int
nan_2(union t_conv data1, union t_conv *data2)
{
	int nans = (ISNAN(data1)? 1: 0) + (ISNAN(*data2)? 2: 0);

	switch (nans) {
	case 0:
		return 0;
	case 1:
		*data2 = data1;
		break;
	case 2:
		break;
	case 3:
		if ((data1.d_bits.mantissa > data2->d_bits.mantissa) ||
		    (data1.d_bits.mantissa == data1.d_bits.mantissa &&
		    data1.d_bits.mantissa2 > data2->d_bits.mantissa2)) {
			*data2 = data1;
		}
		break;
	}
	data2->d_bits.mantissa |= 0x80000; /* Make sure it is a quiet NaN */
	return 1;
}

static int
add_iv(struct operand *op1, struct operand *op2)
{
	int inftys;
	int ret = FPC_TT_NONE;

	if (nan_2(op1->data, &op2->data)) {
		return ret;
	}
	inftys = (ISINFTY(op1->data)? 1: 0) + (ISINFTY(op2->data)? 2:0);
	switch (inftys) {
	case 0:
		break;
	case 1:
		op2->data = op1->data;
		return ret;
	case 2:
		return ret;
	case 3:
		if (op1->data.d_bits.sign != op2->data.d_bits.sign) {
			op2->data = qnan;
			return FPC_TT_INVOP;
		}
		return ret;
	}
	/* Must be subnormals */
	return ieee_add(op1->data.d, &op2->data.d);
}

static int
div_iv(struct operand *op1, struct operand *op2)
{
	int inftys, sign1, sign2;
	int ret = FPC_TT_NONE;

	if (nan_2(op1->data, &op2->data)) {
		return ret;
	}
	inftys = (ISINFTY(op1->data)? 1: 0) + (ISINFTY(op2->data)? 2:0);
	sign1 = op1->data.d_bits.sign;
	sign2 = op2->data.d_bits.sign;
	switch (inftys) {
	case 0:
		break;
	case 1:			/* n/oo */
		op2->data.d = 0.0;
		op2->data.d_bits.sign = sign1 ^ sign2;
		return ret;
	case 2:			/* oo/n */
		op2->data.d_bits.sign = sign1 ^ sign2;
		return ret;
	case 3:
		op2->data = qnan;
		return FPC_TT_INVOP;
	}
	if (ISZERO(op1->data)) {
		if (ISZERO(op2->data)) {
			/* Must be 0/0 */
			op2->data = qnan;
			return FPC_TT_INVOP;
		} else {
			/* Must be subnorm/0 */
			op2->data = infty;
			op2->data.d_bits.sign = sign1 ^ sign2;
			return FPC_TT_DIV0;
		}
	}
	/* Must be subnorm/subnorm */
	return ieee_div(op1->data.d, &op2->data.d);
}

static int
mul_iv(struct operand *op1, struct operand *op2)
{
	int inftys, sign1, sign2;
	int ret = FPC_TT_NONE;

	if (nan_2(op1->data, &op2->data)) {
		return ret;
	}
	inftys = (ISINFTY(op1->data)? 1: 0) + (ISINFTY(op2->data)? 2:0);
	sign1 = op1->data.d_bits.sign;
	sign2 = op2->data.d_bits.sign;
	switch (inftys) {
	case 0:
		break;
	case 1:			/* oo * n */
		if (ISZERO(op2->data)) {
			op2->data = qnan;
			return FPC_TT_INVOP;
		} else {
			op2->data = op1->data;
			op2->data.d_bits.sign = sign1 ^ sign2;
			return ret;
		}
	case 2:			/* n * oo */
		if (ISZERO(op1->data)) {
			op2->data = qnan;
			return FPC_TT_INVOP;
		}
		/* Fall through */
	case 3:			/* oo * oo */
		op2->data.d_bits.sign = sign1 ^ sign2;
		return ret;
	}
	/* Must be subnormals */
	return ieee_mul(op1->data.d, &op2->data.d);
}

static int
dot_iv(struct operand *op1, struct operand *op2, struct operand *op3)
{
	int inftys;
	int ret = FPC_TT_NONE;

	union t_conv t = op2->data;
	if (nan_2(op1->data, &t)) {
		if (nan_2(t, &op3->data))
			return ret;
	}
	inftys = ((ISINFTY(op1->data)? 1: 0) + (ISINFTY(op2->data)? 2:0) +
	    (ISINFTY(op3->data)? 4: 0));
	switch (inftys) {
	case 0:
		break;
	case 1: case 2: case 3:	/* oo * n + m or n * oo + m or oo * oo + m */
		op3->data = op2->data;
		return mul_iv(op1, op3);
	case 4:			/* n * m + oo */
		return ret;
	case 5: case 6: case 7: {
		struct operand top = *op2;
		mul_iv(op1, &top);
		add_iv(&top, op3);
		return ret;
		}
	}
	/* Must be subnormals */
	return ieee_dot(op1->data.d, op2->data.d, &op3->data.d);
}

static int
poly_iv(struct operand *op1, struct operand *op2, struct operand *op3)
{
	int ret;
	struct operand t = *op2;

	ret = dot_iv(op3, op1, &t);
	*op3 = t;
	return ret;
}

static int
round_iv(struct operand *op1, struct operand *op2, int xopcode)
{
	int ret = FPC_TT_NONE;

	if (ISNAN(op1->data)) {
		/* XXX */
		op2->data.i = 0;
		return FPC_TT_INVOP;	/* Need a special code */
	}
	else if (ISINFTY(op1->data)) {
		op2->data.i = op1->data.d_bits.sign? INT_MIN: INT_MAX;
	} else {
		/* Must be a denorm */
		op2->data.i = 0;
	}
	return ret;
}

static int
scalb_iv(struct operand *op1, struct operand *op2)
{
	int ret = FPC_TT_NONE;

	if (nan_2(op1->data, &op2->data)) {
		return ret;
	}
	return ieee_scalb(op1->data.d, &op2->data.d);
}

static int
logb_iv(struct operand *op1, struct operand *op2)
{
	int ret = FPC_TT_NONE;

	if (ISNAN(op1->data))
		op2->data = op1->data;
	else if (ISINFTY(op1->data))
		op2->data = infty;
	else {
		/* Is a denormal */
		union t_conv t = op1->data;
		int norm = ieee_normalize(&t);
		op2->data.d = t.d_bits.exp + norm;
	}
	return ret;
}

static void
cmp_iv(struct operand *op1, struct operand *op2, state *mystate)
{

	mystate->PSR &= ~(PSR_N | PSR_Z | PSR_L);

	if (ISNAN(op1->data) || ISNAN(op2->data)) {
		return;
	}
	ieee_cmp(op1->data.d, op2->data.d, mystate);
	return;
}

int
ieee_invop(struct operand *op1, struct operand *op2, struct operand *f0_op,
    int xopcode, state *mystate)
{
	int user_trap = FPC_TT_NONE;

	/* Don't fiddle with fsr. The FPC_TT_INVOP bit should only be set
	 * if we attempt to *generate* a NaN. This is achieved by returning
	 * FPC_TT_INVOP when we generate a NaN.
	 */
	/*
	 * Figure out right thing to do.
	 */
	switch(xopcode) {
	case NEGF:
		if (! ISNAN(op1->data))
			op1->data.d_bits.sign ^= 1;
		/* Fall through */
	case MOVF:
	case MOVLF:
	case MOVFL:
		op2->data = op1->data;
		break;
	case CMPF:
		cmp_iv(op1, op2, mystate);
		break;
	case SUBF:
		if (! ISNAN(op1->data))
			op1->data.d_bits.sign ^= 1;
		/* Fall through */
	case ADDF:
		user_trap = add_iv(op1, op2);
		break;
	case MULF:
		user_trap = mul_iv(op1, op2);
		break;
	case DIVF:
		user_trap = div_iv(op1, op2);
		break;
	case ROUNDFI:
	case TRUNCFI:
	case FLOORFI:
		user_trap = round_iv(op1, op2, xopcode);
		break;
	case SCALBF:
		user_trap = scalb_iv(op1, op2);
		break;
	case LOGBF:
		user_trap = logb_iv(op1, op2);
		break;
	case DOTF:
		user_trap = dot_iv(op1, op2, f0_op);
		break;
	case POLYF:
		user_trap = poly_iv(op1, op2, f0_op);
		break;
	}
	return user_trap;
}
