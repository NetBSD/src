/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <machine/asm.h>

RCSID("$Id: e_sqrtf.S,v 1.1 1995/05/03 14:47:36 jtc Exp $")

ENTRY(__ieee754_sqrtf)
	flds	4(%esp)
	fsqrt
	ret
