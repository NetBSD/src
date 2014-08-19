/* $NetBSD: ffsdi2.S,v 1.1.4.2 2014/08/19 23:45:12 tls Exp $ */

#include <machine/asm.h>

RCSID("$NetBSD: ffsdi2.S,v 1.1.4.2 2014/08/19 23:45:12 tls Exp $")

ENTRY(ffsdi2)
	rbit	x1, x0
	clz	x1, x1
	add	x1, x1, #1
	cmp	x0, #0
	csel	w0, wzr, w1, eq
	ret
END(ffsdi2)
