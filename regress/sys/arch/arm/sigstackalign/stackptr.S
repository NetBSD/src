/*	$NetBSD: stackptr.S,v 1.1 2001/08/06 22:29:59 bjh21 Exp $	*/

#include <machine/asm.h>

RCSID("$NetBSD: stackptr.S,v 1.1 2001/08/06 22:29:59 bjh21 Exp $")

ENTRY(getstackptr)
	ldr	r0, Lstackptr
	str	r13, [r0]
	mov	pc, r14

Lstackptr:
	.word	_C_LABEL(stackptr)