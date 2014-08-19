/* $NetBSD: ctzdi2.S,v 1.1.4.2 2014/08/19 23:45:12 tls Exp $ */

#include <machine/asm.h>

RCSID("$NetBSD: ctzdi2.S,v 1.1.4.2 2014/08/19 23:45:12 tls Exp $")

ENTRY(ctzdi2)
	rbit	x0, x0
	clz	x0, x0
	ret
END(ctzdi2)
