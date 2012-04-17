/*	$NetBSD: fpgetsticky.c,v 1.5.4.1 2012/04/17 00:05:15 yamt Exp $	*/

/*
 * Written by J.T. Conklin, Apr 10, 1995
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: fpgetsticky.c,v 1.5.4.1 2012/04/17 00:05:15 yamt Exp $");
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"

#include <sys/types.h>
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
	uint32_t x;
	fp_except res;

	__asm("st %%fsr,%0" : "=m" (*&x));
	res = (x >> 5) & 0x1f;

#ifdef EXCEPTIONS_WITH_SOFTFLOAT
	res |= _softfloat_float_exception_flags;
#endif

	return res;
}
