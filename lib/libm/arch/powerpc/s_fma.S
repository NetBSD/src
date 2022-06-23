/* $NetBSD: s_fma.S,v 1.1 2022/06/23 16:42:50 martin Exp $ */

/* The contents of this file are in the public domain. */

#include <machine/asm.h>

#ifdef WEAK_ALIAS
WEAK_ALIAS(fmal, fma)
#endif

ENTRY(fma)
	fmadd	%f1, %f1, %f2, %f3
	blr
END(fma)
