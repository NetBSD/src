/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <machine/asm.h>

RCSID("$Id: e_atan2.S,v 1.3 1995/04/28 22:43:36 jtc Exp $")

ENTRY(__ieee754_atan2)
	fldl	 4(%esp)
	fldl	12(%esp)
	fpatan
	ret
