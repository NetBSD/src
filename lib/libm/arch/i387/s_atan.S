/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <machine/asm.h>

RCSID("$Id: s_atan.S,v 1.3 1995/04/28 22:43:48 jtc Exp $")

ENTRY(atan)
	fldl	4(%esp)
	fld1
	fpatan
	ret
