/* $NetBSD: clzdi2.S,v 1.2 2020/09/02 15:43:06 jakllsch Exp $ */

#include <machine/asm.h>

RCSID("$NetBSD: clzdi2.S,v 1.2 2020/09/02 15:43:06 jakllsch Exp $")

ENTRY(clzdi2)
	clz	x0, x0
	ret
END(clzdi2)
