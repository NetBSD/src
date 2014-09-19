/* $NetBSD: lrint.S,v 1.1 2014/09/19 17:36:25 matt Exp $ */

/* The contents of this file are in the public domain. */

#include <machine/asm.h>

#ifdef _LP64_X
STRONG_ALIAS(llrint,lrint)
#endif

ENTRY(lrint)
#ifdef _LP64
	fcvt.l.d	v0, fa0
#else
	fcvt.w.d	v0, fa0
#endif
	ret
END(lrint)
