/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <machine/asm.h>

RCSID("$NetBSD: e_logf.S,v 1.1 1996/07/03 16:50:23 jtc Exp $")

ENTRY(__ieee754_log)
	fldln2
	flds	4(%esp)
	fyl2x
	ret
