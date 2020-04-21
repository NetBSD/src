/* $NetBSD: clzdi2.S,v 1.1.26.2 2020/04/21 19:37:41 martin Exp $ */

#include <machine/asm.h>

RCSID("$NetBSD: clzdi2.S,v 1.1.26.2 2020/04/21 19:37:41 martin Exp $")

ENTRY(clzdi2)
	clz	x0, x0
	ret
END(clsdi2)
