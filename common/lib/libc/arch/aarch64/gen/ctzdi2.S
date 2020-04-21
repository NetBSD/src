/* $NetBSD: ctzdi2.S,v 1.1.26.2 2020/04/21 19:37:41 martin Exp $ */

#include <machine/asm.h>

RCSID("$NetBSD: ctzdi2.S,v 1.1.26.2 2020/04/21 19:37:41 martin Exp $")

ENTRY(ctzdi2)
	rbit	x0, x0
	clz	x0, x0
	ret
END(ctzdi2)
