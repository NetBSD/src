/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <machine/asm.h>

RCSID("$Id: e_sqrt.S,v 1.3 1995/04/28 22:43:47 jtc Exp $")

ENTRY(__ieee754_sqrt)
	fldl	4(%esp)
	fsqrt
	ret
