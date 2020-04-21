/* $NetBSD: ffsdi2.S,v 1.1.26.2 2020/04/21 19:37:41 martin Exp $ */

#include <machine/asm.h>

RCSID("$NetBSD: ffsdi2.S,v 1.1.26.2 2020/04/21 19:37:41 martin Exp $")

ENTRY(ffsdi2)
	rbit	x1, x0
	clz	x1, x1
	add	x1, x1, #1
	cmp	x0, #0
	csel	w0, wzr, w1, eq
	ret
END(ffsdi2)
