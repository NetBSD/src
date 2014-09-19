/* $NetBSD: s_fabsf.S,v 1.1 2014/09/19 17:36:25 matt Exp $ */

/* The contents of this file are in the public domain. */

#include <machine/asm.h>

ENTRY(fabsf)
	fabs.s	fv0, fa0
	ret
END(fabsf)
