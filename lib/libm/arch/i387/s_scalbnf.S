/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <machine/asm.h>

RCSID("$Id: s_scalbnf.S,v 1.2 1995/04/28 22:44:19 jtc Exp $")

ENTRY(scalbnf)
	fildl	8(%esp)
	flds	4(%esp)
	fscale
	ret
