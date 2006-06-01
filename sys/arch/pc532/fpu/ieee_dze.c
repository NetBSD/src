/*	$NetBSD: ieee_dze.c,v 1.5.6.1 2006/06/01 22:35:07 kardel Exp $	*/

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
 *	File:	ieee_dze.c
 *	Author:	Ian Dall
 *	Date:	November 1995
 *
 *	Handle divide by zero, generating +-Infty as required.
 *
 * HISTORY
 * 14-Dec-95  Ian Dall (Ian.Dall@dsto.defence.gov.au)
 *	First release.
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ieee_dze.c,v 1.5.6.1 2006/06/01 22:35:07 kardel Exp $");

#include "ieee_internal.h"

int ieee_dze(struct operand *op1, struct operand *op2, struct operand *f0_op,
    int xopcode, state *mystate)
{
	int user_trap = FPC_TT_NONE;
	unsigned int fsr = mystate->FSR;

	DP(1, "Divide by zero trap: xopcode = 0x%x\n", xopcode);
	fsr |= FPC_DZF;
	if (fsr & FPC_DZE) {
		/* Users trap handler will fix it */
		user_trap = FPC_TT_DIV0;
	} else {
		/* Set dest to + or - infinity ? */
		int sign1 = op2->data.d_bits.sign;
		int sign2 = op1->data.d_bits.sign;
		op2->data = infty;
		op2->data.d_bits.sign = sign1 ^ sign2;
		switch(xopcode) {
		case LOGBF:
			/*
			 * logb(0) gives a divide by zero exception according
			 * to ieee.
			 * No idea if the logbf instruction conforms, but just
			 * in case...
			 */
			op2->data.d_bits.sign = 1;
		}
	}
	mystate->FSR = fsr;
	return user_trap;
}
