/* $NetBSD: s_copysign.S,v 1.5 2019/04/13 17:18:26 maya Exp $ */

/* The contents of this file are in the public domain. */

#include <machine/asm.h>

ENTRY(copysign)
	fsgnj.d	fa0, fa0, fa1
	ret
END(copysign)
