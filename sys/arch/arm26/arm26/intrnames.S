/* $NetBSD: intrnames.S,v 1.1 2000/05/09 21:55:56 bjh21 Exp $ */
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
/*
 * intrnames.S -- interrupt names for systat etc.
 */

#include <machine/asm.h>

RCSID("$NetBSD: intrnames.S,v 1.1 2000/05/09 21:55:56 bjh21 Exp $")

	.text
	.global _C_LABEL(intrnames)
_C_LABEL(intrnames):
.macro	irqname irq
Lirq\irq:
	.asciz	"IRQ \irq"
.endm
	irqname	0
	irqname	1
	irqname	2
	irqname	3
	irqname 4
	irqname 5
	irqname 6
	irqname 7
	irqname 8
	irqname 9
	irqname 10
	irqname 11
	irqname 12
	irqname 13
	irqname 14
	irqname 15
	.global	_C_LABEL(eintrnames)
_C_LABEL(eintrnames):
	.global	_C_LABEL(irqnames)
_C_LABEL(irqnames):
	.word	Lirq0
	.word	Lirq1
	.word	Lirq2
	.word	Lirq3
	.word	Lirq4
	.word	Lirq5
	.word	Lirq6
	.word	Lirq7
	.word	Lirq8
	.word	Lirq9
	.word	Lirq10
	.word	Lirq11
	.word	Lirq12
	.word	Lirq13
	.word	Lirq14
	.word	Lirq15

	.data
	.global	_C_LABEL(intrcnt)
_C_LABEL(intrcnt):
	.fill	16, 4
	.global	_C_LABEL(eintrcnt)
_C_LABEL(eintrcnt):
