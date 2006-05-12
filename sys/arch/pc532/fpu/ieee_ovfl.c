/*	$NetBSD: ieee_ovfl.c,v 1.8 2006/05/12 06:05:23 simonb Exp $	*/

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
 *	File:	ieee_ovfl.c
 *	Author:	Ian Dall
 *	Date:	November 1995
 *
 *	Handle operations which generated overflow traps.
 *
 * HISTORY
 * 14-Dec-95  Ian Dall (Ian.Dall@dsto.defence.gov.au)
 *	First release.
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ieee_ovfl.c,v 1.8 2006/05/12 06:05:23 simonb Exp $");

#include "ieee_internal.h"

#if defined(__NetBSD__) && defined(_KERNEL)
#include <machine/limits.h>
#else
#include <limits.h>
#endif

int ieee_ovfl(struct operand *op1, struct operand *op2, struct operand *f0_op,
    int xopcode, state *mystate)
{
	int user_trap = FPC_TT_NONE;
	unsigned int fsr = mystate->FSR;

	fsr |= FPC_OVF;
	if (fsr & FPC_OVE) {
		/* Users trap handler will fix it */
		user_trap = FPC_TT_OVFL;
	} else {
		/* If destination is float or double, set to +- Infty, else
		 * if byte, word or long, set to max/min int.
		 */
		int sign1 = op1->data.d_bits.sign;
		int sign2 = op2->data.d_bits.sign;
		switch(xopcode) {
		case NEGF:
			sign1 ^= 1;
			/* Fall through */
		case MOVF:
		case MOVLF:
		case MOVFL:
			op2->data = infty;
			op2->data.d_bits.sign = sign1;
			break;
		case CMPF:
			/* Can't happen */
			break;
		case SUBF:
			sign1 ^= 1;
			/* Fall through */
		case ADDF:
			/*
			 * Overflow can only happen if both operands are same
			 * sign.
			 */
			op2->data = infty;
			op2->data.d_bits.sign = sign2;
			break;
		case MULF:
		case DIVF:
			op2->data = infty;
			op2->data.d_bits.sign = sign1 ^ sign2;
			break;
		case ROUNDFI:
		case TRUNCFI:
		case FLOORFI:
			op2->data.i = sign1? INT_MIN: INT_MAX;
			break;
		case SCALBF:
			op2->data = infty;
			op2->data.d_bits.sign = sign1;
			break;
		case LOGBF:
			op2->data = infty;
			op2->data.d_bits.sign = ISZERO(op1->data)? 1: 0;
			break;
		case DOTF:
			(void)ieee_dot(op1->data.d, op2->data.d,
			    &f0_op->data.d);
			break;
		case POLYF: {
			union t_conv t;
			t = op2->data;
			(void)ieee_dot(f0_op->data.d, op1->data.d, &t.d);
			f0_op->data = t;
			break;
			}
		}
	}
	mystate->FSR = fsr;
	return user_trap;
}
