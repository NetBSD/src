/* $NetBSD: s_copysign.S,v 1.2 2014/11/15 20:59:02 joerg Exp $ */

/* The contents of this file are in the public domain. */

#include <machine/asm.h>

STRONG_ALIAS(_copysignl, _copysign)
WEAK_ALIAS(copysignl, _copysignl)

ENTRY(copysign)
	fsgnj.d	fv0, fa0, fa1
	ret
END(copysign)
