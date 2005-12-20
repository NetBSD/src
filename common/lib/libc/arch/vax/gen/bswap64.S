/* Written by Anders Magnusson. Public Domain */

#if defined(LIBC_SCCS) && !defined(lint)
	.asciz "$NetBSD: bswap64.S,v 1.1 2005/12/20 19:28:50 christos Exp $"
#endif /* LIBC_SCCS and not lint */

#include "DEFS.h"

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
