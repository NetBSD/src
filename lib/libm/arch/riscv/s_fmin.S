/* $NetBSD: s_fmin.S,v 1.1 2014/09/19 17:36:25 matt Exp $ */

/* The contents of this file are in the public domain. */

#include <machine/asm.h>
#include <machine/math.h>

#ifndef __HAVE_LONG_DOUBLE
STRONG_ALIAS(fminl, fmin)
#endif

ENTRY(fmin)
	fmin.d	fv0, fa0, fa1
	ret
END(fmin)
