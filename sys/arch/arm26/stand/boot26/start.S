/*	$NetBSD: start.S,v 1.1 2001/07/27 23:13:50 bjh21 Exp $	*/

#include <machine/asm.h>
#include <riscoscalls.h>

ENTRY(start_kernel)
	swi	OS_EnterOS
	mov	sp, r2		/* Set up initial kernel stack */
	mov	fp, #0		/* of which this is the top. */
	mov	pc, r1		/* GO! */