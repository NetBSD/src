/*	$NetBSD: fpgetsticky.c,v 1.5 2011/03/06 10:32:47 martin Exp $	*/

/*
 * Written by J.T. Conklin, Apr 10, 1995
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: fpgetsticky.c,v 1.5 2011/03/06 10:32:47 martin Exp $");
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"

#include <ieeefp.h>

#ifdef __weak_alias
__weak_alias(fpgetsticky,_fpgetsticky)
#endif

#ifdef EXCEPTIONS_WITH_SOFTFLOAT
extern fp_except _softfloat_float_exception_flags;
#endif

fp_except
fpgetsticky()
{
	int x;
	fp_except res;

	__asm("st %%fsr,%0" : "=m" (*&x));
	res = (x >> 5) & 0x1f;

#ifdef EXCEPTIONS_WITH_SOFTFLOAT
	res |= _softfloat_float_exception_flags;
#endif

	return res;
}
