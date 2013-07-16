/*	$NetBSD: swab.S,v 1.9 2013/07/16 21:46:42 matt Exp $	*/

#include <machine/asm.h>

ENTRY(swab)
	movl	4(%sp),%a0		| source
	movl	8(%sp),%a1		| destination
	movl	12(%sp),%d0		| count
	lsrl	#1,%d0			| count is in bytes; we need words
	jeq	swdone

swloop:
	movw	(%a0)+,%d1
	rorw	#8,%d1
	movw	%d1,(%a1)+
	subql	#1,%d0
	jne	swloop

swdone:
	rts
END(swab)
