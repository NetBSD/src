/* $NetBSD: zeropage.S,v 1.1 2000/05/09 21:55:57 bjh21 Exp $ */
/*-
 * Copyright (c) 1999 Ben Harris
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
/*
 * Data to be copied to zero page at runtime.
 */

#include <machine/asm.h>

RCSID("$NetBSD: zeropage.S,v 1.1 2000/05/09 21:55:57 bjh21 Exp $")

/*
 * Now, here we could do with the ability to change the assembler's
 * idea of where we're assembling to without changing where it puts
 * the output.  Unfortunately, this kind of thing needs the connivance
 * of the linker, which is asking a bit much.  Thus...
 *
 * B and BL can be used normally within zero-page, but not between
 * zero-page and the outside world.
 *
 * LDR can safely be used with a label within zero-page, but not to
 * outside.
 *
 * Absolute references only work to outside.  Relative references only
 * work to inside.
 */

        .text
        .align 0

/*
 * Instructions to copy to the bottom of zero page
 * These are the entry point to the system exception routines
 *
 * They go:
 * Reset
 * Undefined instruction
 * SWI
 * Prefetch abort
 * Data abort
 * Address exception
 * IRQ
 * FIQ
 */

	.global	_C_LABEL(page0), _C_LABEL(page0_end)
_C_LABEL(page0):
	ldr	pc, Lreset_target
	ldr	pc, Lundefined_target
	ldr	pc, Lswi_target
	ldr	pc, Lprefetch_abort_target
	ldr	pc, Ldata_abort_target
	ldr	pc, Laddress_exception_target
	ldr	pc, Lirq_target
Lfiqhandler:
	.global _C_LABEL(fiqhandler)
	.set	_C_LABEL(fiqhandler), . - _C_LABEL(page0)
	subs	pc, lr, #4
	.org	Lfiqhandler + 0x100

Lreset_target:
	.word	reset_entry
Lundefined_target:
	.word	undefined_entry
Lswi_target:
	.word	swi_entry
Lprefetch_abort_target:
	.word	prefetch_abort_entry
Ldata_abort_target:
	.word	data_abort_entry
Laddress_exception_target:
	.word	address_exception_entry
Lirq_target:
	.word	irq_entry
_C_LABEL(page0_end):
