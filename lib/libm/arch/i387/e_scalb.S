/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <machine/asm.h>

RCSID("$Id: e_scalb.S,v 1.3 1995/04/28 22:43:46 jtc Exp $")

ENTRY(__ieee754_scalb)
	fldl	12(%esp)
	fldl	4(%esp)
	fscale
	ret
