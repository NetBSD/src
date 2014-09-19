/* $NetBSD: lrintf.S,v 1.1 2014/09/19 17:36:25 matt Exp $ */

/* The contents of this file are in the public domain. */

#include <machine/asm.h>

#ifdef _LP64_X
STRONG_ALIAS(llrintf,lrintf)
#endif

ENTRY(lrintf)
#ifdef _LP64
	fcvt.l.s	v0, fa0
#else
	fcvt.w.s	v0, fa0
#endif
	ret
END(lrintf)
