/* $NetBSD: rpcc.S,v 1.3 1997/04/06 08:41:27 cgd Exp $ */

#include <machine/asm.h>

	.text
LEAF(alpha_rpcc,1)
	rpcc	v0
	RET
	END(alpha_rpcc)
