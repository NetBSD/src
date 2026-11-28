/*	$NetBSD: mp_intr_stubs.s,v 1.1 2026/11/28 01:35:36 thorpej Exp $	*/

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <machine/asm.h>

#include "assym.h"

	.file	"mp_intr_stubs.s"
	.text

ENTRY_NOPROFILE(ipi_intrstub)
	movew	#PSL_HIGHIPL,%sr	| spl7()
	clrl	-(%sp)			| pad + stackadj
	moveml	#0xFFFF,-(%sp)		| save regs (including KSP)
	pea	(%sp)			| push address of trap frame
	jbsr	_C_LABEL(cpu_ipi_process) | go do the work
	addql	#4,%sp			| pop address of trap frame
	CHECK_ASTPENDING(%a0)		| check for AST
	moveml	(%sp)+,#0x7FFF		| restore all except %sp
	addql	#8,%sp			| pop stack adjust count and %sp
	RTE_AST

ENTRY_NOPROFILE(ipi_poll)
	tstl	CPUINFO_VA+CI_IPIS	| cpuinfo.ci_ipis == 0?
	bne	1f			|   nope, go deal with them
	rts				| yup, go back whence we came
1:
	/*
	 * Create a FMT0 frame on the stack and jump to ipi_intrstub.  It
	 * will proccess the IPIs as needed and then RTE directly to our
	 * caller.
	 *
	 * 0(%sp) has the address to return to.  FMT0 is a total of 4 words
	 * (8 bytes):
	 *
	 *	6	format + vector offset (1 word)
	 *	2	saved %pc (2 words)
	 *	0	saved %sr (1 word)
	 *
	 * We currently have 4 bytes on the stack for our return address.
	 */
	movl	(%sp),%a0		| %a0 = return address
	clrl	(%sp)			| set format + vector offset to 0
	subql	#4,%sp			| more more bytes of stack space
	movew	%sr,(%sp)		| save %sr
	movl	%a0,2(%sp)		| save return address
	bra	_C_LABEL(ipi_intrstub)
