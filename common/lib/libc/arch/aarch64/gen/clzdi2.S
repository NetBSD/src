/* $NetBSD: clzdi2.S,v 1.1.4.2 2014/08/19 23:45:12 tls Exp $ */

#include <machine/asm.h>

RCSID("$NetBSD: clzdi2.S,v 1.1.4.2 2014/08/19 23:45:12 tls Exp $")

ENTRY(clzdi2)
	clz	x0, x0
	ret
END(clsdi2)
