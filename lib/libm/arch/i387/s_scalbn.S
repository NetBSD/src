/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <machine/asm.h>

RCSID("$Id: s_scalbn.S,v 1.3 1995/04/28 22:44:18 jtc Exp $")

ENTRY(scalbn)
	fildl	12(%esp)
	fldl	4(%esp)
	fscale
	ret
