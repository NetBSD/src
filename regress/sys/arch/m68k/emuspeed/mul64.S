/*	$NetBSD: mul64.S,v 1.1.2.1 1999/12/27 18:30:20 wrstuden Exp $	*/

#include <m68k/asm.h>

/*
 * stack:
 * + 8: count
 * + 4: retads
 * + 0: d2
 */

ENTRY_NOPROFILE(mul64ureg)
	movl	%d2,%sp@-
	movl	%sp@(8),%d2
L1:
	mulul	%d2,%d1:%d0
	subql	#1,%d2
	jne	L1
	movl	%sp@+,%d2
	rts

ENTRY_NOPROFILE(mul64sreg)
	movl	%d2,%sp@-
	movl	%sp@(8),%d2
L2:
	mulsl	%d2,%d1:%d0
	subql	#1,%d2
	jne	L2
	movl	%sp@+,%d2
	rts

ENTRY_NOPROFILE(mul64umem)
	movl	%d2,%sp@-
	movl	%sp@(8),%d2
L3:
	mulul	%sp@(8),%d1:%d0
	subql	#1,%d2
	jne	L3
	movl	%sp@+,%d2
	rts

ENTRY_NOPROFILE(mul64smem)
	movl	%d2,%sp@-
	movl	%sp@(8),%d2
L4:
	mulsl	%sp@(8),%d1:%d0
	subql	#1,%d2
	jne	L4
	movl	%sp@+,%d2
	rts
