/*	$NetBSD: srt0.S,v 1.11.44.1 2017/12/03 11:36:44 jdolecek Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg.
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

#define _LOCORE		/* XXX - fix Makefile */

#include <machine/param.h>
#include <machine/asm.h>
#include <machine/psl.h>

#define	CCFSZ	96

	.file	"str0.s"

	.comm	_C_LABEL(romp), 4
	.comm	_C_LABEL(cputyp), 4
	.comm	_C_LABEL(nbpg), 4
	.comm	_C_LABEL(pgofset), 4
	.comm	_C_LABEL(pgshift), 4

	.text	
	.globl	start

start:
	/*
	 * Set up a stack.
	 */
	set	start, %o1
	save	%o1, -CCFSZ, %sp

	/*
	 * Find which address we are at.
	 */
1:	call	2f
	 sethi	%hi(1b), %l0
2:	or	%l0, %lo(1b), %l0
	cmp	%l0, %o7	! %o7 contains actual address of 1b
	beq	4f		! already there, no need to relocate
	 nop

	/*
	 * Relocate.
	 */
	add	%o7, (start-1b), %l0
	set	start, %l1
	set	_edata, %o0
	sub	%o0, %l1, %l2		! length
3:	ld	[%l0], %o0
	add	%l0, 4, %l0
	st	%o0, [%l1]
	subcc	%l2, 4, %l2
	bg	3b
	 add	%l1, 4, %l1

	/*
	 * Jump to our relocated self.
	 */
	set	4f, %g1
	jmp	%g1
	 nop

4:
#ifdef notyet
	/*
	 * Enable traps
	 */
	wr	%g0, 0, %wim		! make sure we can set psr
	nop; nop; nop
	wr	%g0, PSR_S|PSR_PS|PSR_PIL, %psr	! set initial psr
	nop; nop; nop
	wr	%g0, 2, %wim		! set initial %wim (w1 invalid)

	rd	%psr, %l0
	wr	%l0, PSR_ET, %psr
	nop; nop; nop
#endif

	/*
	 * Clear BSS
	 */
	set     _edata, %o0             ! bzero(edata, end - edata)
1:	! check if %o0 is well aligned
	andcc	%o0, 3, %o2
	bz	2f
	 nop
	stb	%g0, [%o0]
	b	1b
	 add	%o0, 1, %o0

2:	! now we have %o0 4 byte aligned
	set     _end, %o1
	subcc	%o1, %o0, %o1
	bz	2f			! in case there is no BSS

	srl	%o1, 2, %o1		! we do 4 byte writes now
1:
	st	%g0, [%o0]		! while (n--)
	subcc	%o1, 1, %o1		!	*p = 0; etc..
	bnz	1b
	 add	%o0, 4, %o0
2:

	/*
	 * Enable interrupts, but only above level 11. This enables "L1-A",
	 * but avoids spurious interrupt bites from most other devices.
	 */
	rd	%psr, %o0
	andn	%o0, PSR_PIL, %o0
	wr	%o0, 0xb00, %psr	! (11 << 8)
	nop; nop; nop

	/*
	 * Set CPU type that we are running on.
	 */
	sethi	%hi(_C_LABEL(cputyp)), %o0
	set	0x4000, %g7
	cmp	%i0, %g7
	beq	is_sun4
	 nop

	mov	CPU_SUN4C, %g4
	mov	SUN4CM_PGSHIFT, %g5

	/*
	 * OpenProm machines pass PROM vector in %o0 (%i0 after save)
	 * OpenFirm machines pass OF entry in %o3 (%i3 after save)
	 */
	cmp	%i0, 0
	be	is_openfirm
	 nop

	! save address of PROM vector
	sethi	%hi(_C_LABEL(romp)), %o1
	st	%i0, [%o1 + %lo(_C_LABEL(romp))]
	b,a	is_sun4cm

is_openfirm:
	! only javastations have OFW, so we know it is a sun4m
	mov	CPU_SUN4M, %g4
	! save address of OpenFirmware client interface handler
	sethi	%hi(_C_LABEL(romp)), %o1
	st	%i3, [%o1 + %lo(_C_LABEL(romp))]
	b,a	is_sun4cm

is_sun4:
	mov	CPU_SUN4, %g4
	mov	SUN4_PGSHIFT, %g5

is_sun4cm:
	st	%g4, [%o0 + %lo(_C_LABEL(cputyp))]
	sethi	%hi(_C_LABEL(pgshift)), %o0	! pgshift = log2(nbpg)
	st	%g5, [%o0 + %lo(_C_LABEL(pgshift))]

	mov	1, %o0				! nbpg = 1 << pgshift
	sll	%o0, %g5, %g5
	sethi	%hi(_C_LABEL(nbpg)), %o0	! nbpg = bytes in a page
	st	%g5, [%o0 + %lo(_C_LABEL(nbpg))]

	sub	%g5, 1, %g5
	sethi	%hi(_C_LABEL(pgofset)), %o0	! page offset = nbpg - 1
	st	%g5, [%o0 + %lo(_C_LABEL(pgofset))]

	call	_C_LABEL(main)
	 mov	%i0, %o0

	ret
	 restore

/*
 * NO-OP place holder function.
 */
ENTRY(sparc_noop)
	retl
	 nop

/*
 * Openfirmware entry point: openfirmware(void *args)
 */
ENTRY(openfirmware)
	sethi	%hi(_C_LABEL(romp)), %o1
	ld	[%o1 + %lo(_C_LABEL(romp))], %o2
	jmp	%o2
	 nop

#ifdef TIGHT

/*
 * XXX - Space saving .div & .rem routines (small & non-negative numbres only)
 */
	.align	4
	.global .div, .udiv
! int n = 0; while (a >= b) { a -= b; n++; }; return n;
.div:
.udiv:
	cmp	%o0, %o1
	bl	2f
	 mov	0, %o5
1:
	sub	%o0, %o1, %o0
	cmp	%o0, %o1
	bge	1b
	 add	%o5, 1, %o5
2:
	retl
	 mov	%o5, %o0

	.align	4
	.global .rem, .urem
! while (a>=b) a -= b; return a;
.rem:
.urem:
	cmp	%o0, %o1
	bl	2f
	 nop
	sub	%o0, %o1, %o0
1:
	cmp	%o0, %o1
	bge,a	1b
	 sub	%o0, %o1, %o0
2:
	retl
	 nop

#endif /* TIGHT */
