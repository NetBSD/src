/*	$NetBSD: locore_machdep.S,v 1.1 1996/09/30 01:51:17 jonathan Exp $	*/

/*
 * ARC-specific mips locore code
 */


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
