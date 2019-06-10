/* $NetBSD: s_copysign.S,v 1.4.16.1 2019/06/10 22:05:24 christos Exp $ */

/* The contents of this file are in the public domain. */

#include <machine/asm.h>

ENTRY(copysign)
	fsgnj.d	fa0, fa0, fa1
	ret
END(copysign)
