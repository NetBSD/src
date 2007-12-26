/*	$NetBSD: spl.S,v 1.4.82.2 2007/12/26 22:24:35 rjs Exp $	*/

/*
 * Copyright (c) 1996-1998 Mark Brinicombe.
 * Copyright (c) Brini.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Mark Brinicombe
 *	for the NetBSD Project.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * spl routines
 *
 * Created      : 01/03/96
 */

#include "assym.h"
#include <arm/arm32/psl.h>
#include <machine/asm.h>

	.text
	.align	0

.Lcurrent_spl_level:
	.word	_C_LABEL(current_spl_level)

ENTRY(raisespl)
	mov	r3, r0			/* Save the new value */
	ldr	r1, .Lcurrent_spl_level	/* Get the current spl level */
	ldr	r0, [r1]
	cmp	r3, r0
	movle	pc, lr

	str	r3, [r1]		/* Store the new spl level */

					/* stack alignment is 8 bytes */
	stmfd	sp!, {r0, lr}		/* Preserve registers */
	bl	_C_LABEL(irq_setmasks)	/* Update the actual masks */
	ldmfd	sp!, {r0, pc}		/* Exit */

ENTRY(lowerspl)
	mov	r3, r0			/* Save the new value */
	ldr	r1, .Lcurrent_spl_level	/* Get the current spl level */
	ldr	r0, [r1]
	cmp	r3, r0
	movge	pc, lr

	str	r3, [r1]		/* Store the new spl level */

					/* stack alignment is 8 bytes */
	stmfd	sp!, {r0, lr}		/* Preserve registers */
	bl	_C_LABEL(irq_setmasks)	/* Update the actual masks */
#ifdef __HAVE_FAST_SOFTINTS
	bl	_C_LABEL(dosoftints)	/* Process any pending soft ints */
#endif
	ldmfd	sp!, {r0, pc}		/* Exit */

ENTRY(splx)
	ldr	r1, .Lcurrent_spl_level	/* Get the current spl level */
	mov	r3, r0			/* Save the new value */
	ldr	r0, [r1]
	cmp	r3, r0
	moveq	pc, lr

	str	r3, [r1]		/* Store the new spl level */

					/* stack alignment is 8 bytes */
	stmfd	sp!, {r0, lr}		/* Preserve registers */

	bl	_C_LABEL(irq_setmasks)	/* Update the actual masks */
#ifdef __HAVE_FAST_SOFTINTS
	bl	_C_LABEL(dosoftints)	/* Process any pending soft ints */
#endif
	ldmfd	sp!, {r0, pc}		/* Exit */
