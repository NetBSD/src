/*	$NetBSD: locore_machdep.S,v 1.3 1997/06/18 13:16:13 jonathan Exp $	*/


/*
 * ARC-specific mips locore code
 */

#include <mips/asm.h>

/*
 * fillw(pat, addr, count)
 */
LEAF(fillw)
1:
	addiu	a2, a2, -1
	sh	a0, 0(a1)
	bne	a2,zero, 1b
	addiu	a1, a1, 2

	jr	ra
	nop
END(fillw)


/*
 * Interrupt counters for vmstat.
 * XXX These aren't used yet.
 */
	.data
	.globl intrcnt
	.globl eintrcnt
	.globl intrnames
	.globl eintrnames
intrnames:
	.asciiz	"spur"
	.asciiz	"hil"
	.asciiz	"lev2"
	.asciiz	"lev3"
	.asciiz	"lev4"
	.asciiz	"lev5"
	.asciiz	"dma"
	.asciiz	"clock"
	.asciiz	"statclock"
	.asciiz	"nmi"
eintrnames:
	.align	2
intrcnt:
	.word	0,0,0,0,0,0,0,0,0,0
eintrcnt:
