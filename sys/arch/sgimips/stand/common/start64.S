/*	$NetBSD: start64.S,v 1.2.74.1 2008/06/02 13:22:38 mjf Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jonathan Stone, Michael Hitch and Simon Burge.
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

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)start.s	8.2 (Berkeley) 1/21/94
 */

/*
 * start.s -
 *
 *	Contains code that is the first executed at boot time.
 *
 *	Copyright (C) 1989 Digital Equipment Corporation.
 *	Permission to use, copy, modify, and distribute this software and
 *	its documentation for any purpose and without fee is hereby granted,
 *	provided that the above copyright notice appears in all copies.
 *	Digital Equipment Corporation makes no representations about the
 *	suitability of this software for any purpose.  It is provided "as is"
 *	without express or implied warranty.
 *
 * from: Header: /sprite/src/boot/decprom/ds3100.md/RCS/start.s,
 *	v 1.1 90/02/16 16:19:39 shirriff Exp  SPRITE (DECWRL)
 */

#include <mips/asm.h>
#include <mips/cpuregs.h>

#	.extern ARCBIOS, 4

	.globl	start
start:
	.set	noreorder
#ifdef __GP_SUPPORT__
	dla	gp, _C_LABEL (_gp)
#endif
	daddi	sp, - (CALLFRAME_SIZ * 2)
	sd	ra, CALLFRAME_RA * 2(sp)	# Save RA
	sd	a0, 0(sp)			# save argc
	sd	a1, 8(sp)			# save argv
	sd	a2, 16(sp)			# save envp
	dla	v0, 0xa800000000001000		# ARCBIOS_SPB
	dla	a0, _C_LABEL (edata)		# clear BSS
	ld	v0, 64(v0)			# FirmwareVector (64-bit offset)
	move	a1, zero
	sd	v0, _C_LABEL(ARCBIOS)		# save ARCBIOS
	dla	a2, _C_LABEL (end)
	jal	_C_LABEL(memset)		# memset(edata, 0, end - edata)
	dsubu	a2, a2, a0
	ld	a0, 0(sp)			# restore argc
	ld	a1, 8(sp)			# restore argv
	jal	_C_LABEL(main)			# main(argc, argv)
	ld	a2, 16(sp)			# restore envp
	ld	ra, CALLFRAME_RA * 2(sp)	# restore RA
	j	ra				# return to PROM
	daddi	sp, CALLFRAME_SIZ * 2		# BDslot: adjust stack

	.globl	_rtt
_rtt:
	j	ra				# XXXX return to PROM?

	.data
	.global	ARCBIOS
ARCBIOS: .data	0
