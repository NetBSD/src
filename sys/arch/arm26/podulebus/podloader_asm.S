/*	$NetBSD: podloader_asm.S,v 1.1.2.2 2001/01/05 17:34:04 bouyer Exp $	*/

/*-
 * Copyright (c) 2000 Ben Harris
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/* This file is part of NetBSD/arm26 -- a port of NetBSD to ARM2/3 machines. */

#include <machine/asm.h>

RCSID("$NetBSD: podloader_asm.S,v 1.1.2.2 2001/01/05 17:34:04 bouyer Exp $")

#include <sys/errno.h>

/*
 * register_t podloader_call(register_t r0, register_t r1, register_t r11,
 *     void *loader, int entry)
 */
ENTRY(podloader_call)
	mov	ip, sp
	stmfd	sp!, {r4, fp, ip, lr, pc}
	sub	fp, ip, #4
	ldr	r4, [fp, #4]		/* fetch entry */
	bl	_C_LABEL(int_off)
	stmfd	sp!, {fp}		/* Save FP, since that's R11 */
	mov	r11, r2
	mov	lr, pc
	add	pc, r3, r4, lsl #2	/* Call loader */
	ldmfd	sp!, {fp}
	bl	_C_LABEL(int_on)
	adrvs	r0, Lpodloader_panicmsg
	blvs	_C_LABEL(panic)
#ifdef __APCS_26__
	ldmdb	fp, {r4, fp, sp, pc}^
#else
	ldmdb	fp, {r4, fp, sp, pc}
#endif
Lpodloader_panicmsg:
	.asciz	"podloader_call"
