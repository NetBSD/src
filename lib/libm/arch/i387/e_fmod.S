/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <machine/asm.h>

RCSID("$Id: e_fmod.S,v 1.3 1995/04/28 22:43:39 jtc Exp $")

ENTRY(__ieee754_fmod)
	fldl	12(%esp)
	fldl	4(%esp)
1:	fprem
	fstsw	%ax
	sahf
	jp	1b
	fstpl	%st(1)
	ret
