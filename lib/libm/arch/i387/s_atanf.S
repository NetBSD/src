/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <machine/asm.h>

RCSID("$Id: s_atanf.S,v 1.2 1995/04/28 22:43:49 jtc Exp $")

ENTRY(atanf)
	flds	4(%esp)
	fld1
	fpatan
	ret
