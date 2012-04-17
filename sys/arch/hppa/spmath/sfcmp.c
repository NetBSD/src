/*	$NetBSD: sfcmp.c,v 1.3.112.1 2012/04/17 00:06:27 yamt Exp $	*/

/*	$OpenBSD: sfcmp.c,v 1.4 2001/03/29 03:58:19 mickey Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: sfcmp.c,v 1.3.112.1 2012/04/17 00:06:27 yamt Exp $");

#include "../spmath/float.h"
#include "../spmath/sgl_float.h"

/*
 * sgl_cmp: compare two values
 */
int
sgl_fcmp(sgl_floating_point *leftptr, sgl_floating_point *rightptr,
    unsigned int cond, unsigned int *status)
{
    register unsigned int left, right;
    register int xorresult;

    /* Create local copies of the numbers */
    left = *leftptr;
    right = *rightptr;
    /*
     * Test for NaN
     */
    if(    (Sgl_exponent(left) == SGL_INFINITY_EXPONENT)
	|| (Sgl_exponent(right) == SGL_INFINITY_EXPONENT) )
	{
	/* Check if a NaN is involved.  Signal an invalid exception when
	 * comparing a signaling NaN or when comparing quiet NaNs and the
	 * low bit of the condition is set */
	if( (  (Sgl_exponent(left) == SGL_INFINITY_EXPONENT)
	    && Sgl_isnotzero_mantissa(left)
	    && (Exception(cond) || Sgl_isone_signaling(left)))
	   ||
	    (  (Sgl_exponent(right) == SGL_INFINITY_EXPONENT)
	    && Sgl_isnotzero_mantissa(right)
	    && (Exception(cond) || Sgl_isone_signaling(right)) ) )
	    {
	    if( Is_invalidtrap_enabled() ) {
		Set_status_cbit(Unordered(cond));
		return(INVALIDEXCEPTION);
	    }
	    else Set_invalidflag();
	    Set_status_cbit(Unordered(cond));
	    return(NOEXCEPTION);
	    }
	/* All the exceptional conditions are handled, now special case
	   NaN compares */
	else if( ((Sgl_exponent(left) == SGL_INFINITY_EXPONENT)
	    && Sgl_isnotzero_mantissa(left))
	   ||
	    ((Sgl_exponent(right) == SGL_INFINITY_EXPONENT)
	    && Sgl_isnotzero_mantissa(right)) )
	    {
	    /* NaNs always compare unordered. */
	    Set_status_cbit(Unordered(cond));
	    return(NOEXCEPTION);
	    }
	/* infinities will drop down to the normal compare mechanisms */
	}
    /* First compare for unequal signs => less or greater or
     * special equal case */
    Sgl_xortointp1(left,right,xorresult);
    if( xorresult < 0 )
	{
	/* left negative => less, left positive => greater.
	 * equal is possible if both operands are zeros. */
	if( Sgl_iszero_exponentmantissa(left)
	  && Sgl_iszero_exponentmantissa(right) )
	    {
	    Set_status_cbit(Equal(cond));
	    }
	else if( Sgl_isone_sign(left) )
	    {
	    Set_status_cbit(Lessthan(cond));
	    }
	else
	    {
	    Set_status_cbit(Greaterthan(cond));
	    }
	}
    /* Signs are the same.  Treat negative numbers separately
     * from the positives because of the reversed sense.  */
    else if( Sgl_all(left) == Sgl_all(right) )
	{
	Set_status_cbit(Equal(cond));
	}
    else if( Sgl_iszero_sign(left) )
	{
	/* Positive compare */
	if( Sgl_all(left) < Sgl_all(right) )
	    {
	    Set_status_cbit(Lessthan(cond));
	    }
	else
	    {
	    Set_status_cbit(Greaterthan(cond));
	    }
	}
    else
	{
	/* Negative compare.  Signed or unsigned compares
	 * both work the same.  That distinction is only
	 * important when the sign bits differ. */
	if( Sgl_all(left) > Sgl_all(right) )
	    {
	    Set_status_cbit(Lessthan(cond));
	    }
	else
	    {
	    Set_status_cbit(Greaterthan(cond));
	    }
	}
	return(NOEXCEPTION);
    }
