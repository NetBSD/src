/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <machine/asm.h>

RCSID("$NetBSD: s_scalbn.S,v 1.5 1999/01/02 05:15:40 kristerw Exp $")

ENTRY(scalbn)
	fildl	12(%esp)
	fldl	4(%esp)
	fscale
	fstp	%st(1)		/* bug fix for fp stack overflow */
	ret
