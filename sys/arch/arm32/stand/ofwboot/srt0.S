/*	$NetBSD: srt0.S,v 1.1 2001/11/01 22:50:19 thorpej Exp $	*/

#include <machine/asm.h>

ASENTRY_NP(_start)
#	ldr	sp, L1
	b	_C_LABEL(startup)

	.align 0
L1:
	.word	stack+4096

	.comm	stack, 4096
