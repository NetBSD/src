/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <machine/asm.h>

RCSID("$Id: e_log.S,v 1.3 1995/04/28 22:43:40 jtc Exp $")

ENTRY(__ieee754_log)
	fldln2
	fldl	4(%esp)
	fyl2x
	ret
