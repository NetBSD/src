/*	$NetBSD: rpcc.S,v 1.1 1996/09/18 20:03:14 cgd Exp $	*/

#include <machine/asm.h>

        .text
LEAF(alpha_rpcc,1)
        rpcc    v0
        RET
        END(alpha_rpcc)
