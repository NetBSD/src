/*	$NetBSD: fcnvff.c,v 1.5.80.1 2012/04/17 00:06:27 yamt Exp $	*/

/*	$OpenBSD: fcnvff.c,v 1.5 2001/03/29 03:58:18 mickey Exp $	*/

/*
 * Copyright 1996 1995 by Open Software Foundation, Inc.
 *              All Rights Reserved
 *
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appears in all copies and
 * that both the copyright notice and this permission notice appear in
 * supporting documentation.
 *
 * OSF DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE.
 *
 * IN NO EVENT SHALL OSF BE LIABLE FOR ANY SPECIAL, INDIRECT, OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT,
 * NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */
/*
 * pmk1.1
 */
/*
 * (c) Copyright 1986 HEWLETT-PACKARD COMPANY
 *
 * To anyone who acknowledges that this file is provided "AS IS"
 * without any express or implied warranty:
 *     permission to use, copy, modify, and distribute this file
 * for any purpose is hereby granted without fee, provided that
 * the above copyright notice and this notice appears in all
 * copies, and that the name of Hewlett-Packard Company not be
 * used in advertising or publicity pertaining to distribution
 * of the software without specific, written prior permission.
 * Hewlett-Packard Company makes no representations about the
 * suitability of this software for any purpose.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: fcnvff.c,v 1.5.80.1 2012/04/17 00:06:27 yamt Exp $");

#include "../spmath/float.h"
#include "../spmath/sgl_float.h"
#include "../spmath/dbl_float.h"
#include "../spmath/cnv_float.h"

/*
 *  Single Floating-point to Double Floating-point
 */
/*ARGSUSED*/
int
sgl_to_dbl_fcnvff(sgl_floating_point *srcptr, dbl_floating_point *dstptr,
    unsigned int *status)
{
	register unsigned int src, resultp1, resultp2;
	register int src_exponent;

	src = *srcptr;
	src_exponent = Sgl_exponent(src);
	Dbl_allp1(resultp1) = Sgl_all(src);  /* set sign of result */
	/*
	 * Test for NaN or infinity
	 */
	if (src_exponent == SGL_INFINITY_EXPONENT) {
		/*
		 * determine if NaN or infinity
		 */
		if (Sgl_iszero_mantissa(src)) {
			/*
			 * is infinity; want to return double infinity
			 */
			Dbl_setinfinity_exponentmantissa(resultp1,resultp2);
			Dbl_copytoptr(resultp1,resultp2,dstptr);
			return(NOEXCEPTION);
		}
		else {
			/*
			 * is NaN; signaling or quiet?
			 */
			if (Sgl_isone_signaling(src)) {
				/* trap if INVALIDTRAP enabled */
				if (Is_invalidtrap_enabled())
					return(INVALIDEXCEPTION);
				/* make NaN quiet */
				else {
					Set_invalidflag();
					Sgl_set_quiet(src);
				}
			}
			/*
			 * NaN is quiet, return as double NaN
			 */
			Dbl_setinfinity_exponent(resultp1);
			Sgl_to_dbl_mantissa(src,resultp1,resultp2);
			Dbl_copytoptr(resultp1,resultp2,dstptr);
			return(NOEXCEPTION);
		}
	}
	/*
	 * Test for zero or denormalized
	 */
	if (src_exponent == 0) {
		/*
		 * determine if zero or denormalized
		 */
		if (Sgl_isnotzero_mantissa(src)) {
			/*
			 * is denormalized; want to normalize
			 */
			Sgl_clear_signexponent(src);
			Sgl_leftshiftby1(src);
			Sgl_normalize(src,src_exponent);
			Sgl_to_dbl_exponent(src_exponent,resultp1);
			Sgl_to_dbl_mantissa(src,resultp1,resultp2);
		}
		else {
			Dbl_setzero_exponentmantissa(resultp1,resultp2);
		}
		Dbl_copytoptr(resultp1,resultp2,dstptr);
		return(NOEXCEPTION);
	}
	/*
	 * No special cases, just complete the conversion
	 */
	Sgl_to_dbl_exponent(src_exponent, resultp1);
	Sgl_to_dbl_mantissa(Sgl_mantissa(src), resultp1,resultp2);
	Dbl_copytoptr(resultp1,resultp2,dstptr);
	return(NOEXCEPTION);
}

/*
 *  Double Floating-point to Single Floating-point
 */
/*ARGSUSED*/
int
dbl_to_sgl_fcnvff(dbl_floating_point *srcptr, sgl_floating_point *dstptr,
    unsigned int *status)
{
	register unsigned int srcp1, srcp2, result;
	register int src_exponent, dest_exponent, dest_mantissa;
	register int inexact = false, guardbit = false, stickybit = false;
	register int lsb_odd = false;
	int is_tiny = false;

	Dbl_copyfromptr(srcptr,srcp1,srcp2);
	src_exponent = Dbl_exponent(srcp1);
	Sgl_all(result) = Dbl_allp1(srcp1);  /* set sign of result */
	/*
	 * Test for NaN or infinity
	 */
	if (src_exponent == DBL_INFINITY_EXPONENT) {
		/*
		 * determine if NaN or infinity
		 */
		if (Dbl_iszero_mantissa(srcp1,srcp2)) {
			/*
			 * is infinity; want to return single infinity
			 */
			Sgl_setinfinity_exponentmantissa(result);
			*dstptr = result;
			return(NOEXCEPTION);
		}
		/*
		 * is NaN; signaling or quiet?
		 */
		if (Dbl_isone_signaling(srcp1)) {
			/* trap if INVALIDTRAP enabled */
			if (Is_invalidtrap_enabled()) return(INVALIDEXCEPTION);
			else {
				Set_invalidflag();
				/* make NaN quiet */
				Dbl_set_quiet(srcp1);
			}
		}
		/*
		 * NaN is quiet, return as single NaN
		 */
		Sgl_setinfinity_exponent(result);
		Sgl_set_mantissa(result,Dallp1(srcp1)<<3 | Dallp2(srcp2)>>29);
		if (Sgl_iszero_mantissa(result)) Sgl_set_quiet(result);
		*dstptr = result;
		return(NOEXCEPTION);
	}
	/*
	 * Generate result
	 */
	Dbl_to_sgl_exponent(src_exponent,dest_exponent);
	if (dest_exponent > 0) {
		Dbl_to_sgl_mantissa(srcp1,srcp2,dest_mantissa,inexact,guardbit,
		stickybit,lsb_odd);
	}
	else {
		if (Dbl_iszero_exponentmantissa(srcp1,srcp2)){
			Sgl_setzero_exponentmantissa(result);
			*dstptr = result;
			return(NOEXCEPTION);
		}
		if (Is_underflowtrap_enabled()) {
			Dbl_to_sgl_mantissa(srcp1,srcp2,dest_mantissa,inexact,
			guardbit,stickybit,lsb_odd);
		}
		else {
			/* compute result, determine inexact info,
			 * and set Underflowflag if appropriate
			 */
			Dbl_to_sgl_denormalized(srcp1,srcp2,dest_exponent,
			dest_mantissa,inexact,guardbit,stickybit,lsb_odd,
			is_tiny);
		}
	}
	/*
	 * Now round result if not exact
	 */
	if (inexact) {
		switch (Rounding_mode()) {
			case ROUNDPLUS:
				if (Sgl_iszero_sign(result)) dest_mantissa++;
				break;
			case ROUNDMINUS:
				if (Sgl_isone_sign(result)) dest_mantissa++;
				break;
			case ROUNDNEAREST:
				if (guardbit) {
				   if (stickybit || lsb_odd) dest_mantissa++;
				   }
		}
	}
	Sgl_set_exponentmantissa(result,dest_mantissa);

	/*
	 * check for mantissa overflow after rounding
	 */
	if ((dest_exponent>0 || Is_underflowtrap_enabled()) &&
	    Sgl_isone_hidden(result)) dest_exponent++;

	/*
	 * Test for overflow
	 */
	if (dest_exponent >= SGL_INFINITY_EXPONENT) {
		/* trap if OVERFLOWTRAP enabled */
		if (Is_overflowtrap_enabled()) {
			/*
			 * Check for gross overflow
			 */
			if (dest_exponent >= SGL_INFINITY_EXPONENT+SGL_WRAP)
				return(UNIMPLEMENTEDEXCEPTION);

			/*
			 * Adjust bias of result
			 */
			Sgl_setwrapped_exponent(result,dest_exponent,ovfl);
			*dstptr = result;
			if (inexact) {
			    if (Is_inexacttrap_enabled())
				return(OVERFLOWEXCEPTION|INEXACTEXCEPTION);
			    else
				Set_inexactflag();
			}
			return(OVERFLOWEXCEPTION);
		}
		Set_overflowflag();
		inexact = true;
		/* set result to infinity or largest number */
		Sgl_setoverflow(result);
	}
	/*
	 * Test for underflow
	 */
	else if (dest_exponent <= 0) {
		/* trap if UNDERFLOWTRAP enabled */
		if (Is_underflowtrap_enabled()) {
			/*
			 * Check for gross underflow
			 */
			if (dest_exponent <= -(SGL_WRAP))
				return(UNIMPLEMENTEDEXCEPTION);
			/*
			 * Adjust bias of result
			 */
			Sgl_setwrapped_exponent(result,dest_exponent,unfl);
			*dstptr = result;
			if (inexact) {
			    if (Is_inexacttrap_enabled())
				return(UNDERFLOWEXCEPTION|INEXACTEXCEPTION);
			    else
				Set_inexactflag();
			}
			return(UNDERFLOWEXCEPTION);
		}
		 /*
		  * result is denormalized or signed zero
		  */
	       if (inexact && is_tiny) Set_underflowflag();

	}
	else Sgl_set_exponent(result,dest_exponent);
	*dstptr = result;
	/*
	 * Trap if inexact trap is enabled
	 */
	if (inexact) {
		if (Is_inexacttrap_enabled())
			return(INEXACTEXCEPTION);
		else
			Set_inexactflag();
	}
	return(NOEXCEPTION);
}
