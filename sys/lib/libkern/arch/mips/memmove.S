/*	$NetBSD: memmove.S,v 1.1 1999/03/15 07:04:04 jonathan Exp $	*/
/* XXXX We need to define this in a way which supports multiple architectures */

#include <mips/asm.h>
#include <machine/endian.h>

/*
 * On mips, we fold memcpy and memmove into a single
 * routine, to reduce I-cache footprint in the small
 * direct-mapped Icaches on  mips1 CPUs.
 */
	.sdata
	.globl	__no_memmove
__no_memmove:	.word 0
