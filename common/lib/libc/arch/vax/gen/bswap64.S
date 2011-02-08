/* Written by Anders Magnusson. Public Domain */

#include <machine/asm.h>

RCSID("$NetBSD: bswap64.S,v 1.2.48.1 2011/02/08 16:18:28 bouyer Exp $")

ENTRY(bswap64, 0)
        movq 4(%ap),%r3
	rotl $-8,%r3,%r1
	insv %r1,$16,$8,%r1
	rotl $8,%r3,%r2
	movb %r2,%r1
	rotl $-8,%r4,%r0
	insv %r0,$16,$8,%r0
	rotl $8,%r4,%r2
	movb %r2,%r0
	ret
END(bswap64)
