/*	$NetBSD: iomd_fiq.S,v 1.2.172.1 2014/08/20 00:02:46 tls Exp $	*/

/*
 * Copyright (c) 1994-1996 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
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
 *	This product includes software developed by Mark Brinicombe.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL HTE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * fiq.S
 *
 * Low level fiq handlers
 *
 * Created      : 27/09/94
 */

#include <arm/asm.h>
#include <arm/locore.h>


/*
 * if NOFDFIQLOOPS is defined only 1 byte is transferred per
 * FIQ. The new default behaviour is to keep looping
 * until there are no FD FIQ's pending.
 */

/*
 * These registers needs to be consistent with fd
 * driver's assignment.
 *
 *	r8  - scratch
 *	r9  - IOMD_FIQRQ
 *	r10 - transfer size
 *	r11 - data address
 *	r12 - floppy controller DACK address (+ 0x18000 for TC)
 *	r13 - scratch (loop counter)
 */

ENTRY_NP(floppy_read_fiq)
	subs	r10, r10, #0x00000001
	addeq	r12, r12, #0x00018000
	ldrb	r8, [r12]
	strb	r8, [r11], #0x0001
#ifndef NOFDFIQLOOPS
	subseq	pc, lr, #0x00000004
	ldrb	r8, [r9]
	tst	r8, #0x01
	addne	r13, r13, #0x00000001
	bne	_C_LABEL(floppy_read_fiq)
#endif
	subs	pc, lr, #0x00000004
END(floppy_read_fiq)

	.global	_C_LABEL(floppy_read_fiq_end)
_C_LABEL(floppy_read_fiq_end):


/*
 * These registers needs to be consistent with fd
 * driver's assignment.
 *
 *	r8  - scratch
 *	r9  - IOMD_FIQRQ
 *	r10 - transfer size
 *	r11 - data address
 *	r12 - floppy controller DACK address (+ 0x18000 for TC)
 *	r13 - scratch (loop counter)
 */

ENTRY_NP(floppy_write_fiq)
	subs	r10, r10, #0x00000001
	addeq	r12, r12, #0x00018000
	ldrb	r8, [r11], #0x0001
	strb	r8, [r12]
#ifndef NOFDFIQLOOPS
	subseq	pc, lr, #0x00000004
	ldrb	r8, [r9]
	tst	r8, #0x01
	addne	r13, r13, #0x00000001
	bne	_C_LABEL(floppy_write_fiq)
#endif
	subs	pc, lr, #0x00000004
END(floppy_write_fiq)

	.global	_C_LABEL(floppy_write_fiq_end)
_C_LABEL(floppy_write_fiq_end):
