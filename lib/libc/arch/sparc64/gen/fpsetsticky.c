/*	$NetBSD: fpsetsticky.c,v 1.5 2011/03/06 10:32:47 martin Exp $	*/

/*
 * Written by J.T. Conklin, Apr 10, 1995
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: fpsetsticky.c,v 1.5 2011/03/06 10:32:47 martin Exp $");
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"

#include <ieeefp.h>

#ifdef __weak_alias
__weak_alias(fpsetsticky,_fpsetsticky)
#endif

#ifdef EXCEPTIONS_WITH_SOFTFLOAT
extern fp_except _softfloat_float_exception_flags;
#endif

fp_except
fpsetsticky(sticky)
	fp_except sticky;
{
	fp_except old;
	fp_except new;

	__asm("st %%fsr,%0" : "=m" (*&old));

	new = old;
	new &= ~(0x1f << 5); 
	new |= ((sticky & 0x1f) << 5);

	__asm("ld %0,%%fsr" : : "m" (*&new));

	old = (old >> 5) & 0x1f;

#ifdef EXCEPTIONS_WITH_SOFTFLOAT
	old |= _softfloat_float_exception_flags;
	_softfloat_float_exception_flags = sticky;
#endif
	return old;
}
