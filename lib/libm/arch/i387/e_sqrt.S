/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <machine/asm.h>

RCSID("$NetBSD: e_sqrt.S,v 1.5 2001/06/19 00:26:30 fvdl Exp $")

ENTRY(__ieee754_sqrt)
#ifdef __i386__
	fldl	4(%esp)
	fsqrt
#else
	sqrtsd	%xmm0,%xmm0
#endif
	ret
