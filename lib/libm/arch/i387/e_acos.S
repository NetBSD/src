/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <machine/asm.h>

RCSID("$Id: e_acos.S,v 1.3 1995/04/28 22:43:31 jtc Exp $")

/* acos = atan (sqrt(1 - x^2) / x) */
ENTRY(__ieee754_acos)
	fldl	4(%esp)			/* x */
	fst	%st(1)
	fmul	%st(0)			/* x^2 */
	fld1				
	fsubp				/* 1 - x^2 */
	fsqrt				/* sqrt (1 - x^2) */
	fxch	%st(1)
	fpatan
	ret
