/*	$NetBSD: memcpy.S,v 1.1 1998/09/11 16:46:34 jonathan Exp $	*/

#include <mips/asm.h>
#include <machine/endian.h>

	.data
	.globl	__no_memcpy
__no_memcpy:	.word 0
