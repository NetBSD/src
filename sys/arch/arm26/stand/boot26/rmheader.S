#include <riscoscalls.h>

rmbase:
	.word	rmstart - rmbase	/* Start code */
	.word	0			/* Initialisation code */
	.word	0			/* Finalisation code */
	.word	0			/* Service call handler */
	.word	rmtitle - rmbase	/* Title string */
	.word	rmhelp - rmbase		/* Help string */
	.word	0			/* Help and command keyword table */

rmtitle:
	.asciz	"boot26"
	.align
rmstart:
	/*
	 * This entry point is called when we're started up as an application,
	 * so we're allowed to use application workspace.  This is good,
	 * because most of our code is linked to run at 0x8000.
	 */
	mov	r0, #0x8000
	adr	r1, rmbase
	ldr	r2, Lsize
Lcopyloop:
	ldr	r3, [r1], #4
	str	r3, [r0], #4
	subs	r2, r2, #4
	bgt	Lcopyloop
	ldr	r0, Lstart
	mov	pc, r0

Lsize:
	.word	end - rmbase
Lstart:
	.word	_start