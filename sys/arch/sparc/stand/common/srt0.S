/*	$NetBSD: srt0.S,v 1.3 1999/02/15 19:01:38 pk Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
#include <machine/psl.h>

#define	CCFSZ	96

	.file	"str0.s"

	.comm	_romp, 4
	.comm	_opf_romp, 4
	.comm	_cputyp, 4
	.comm	_nbpg, 4
	.comm	_pgofset, 4
	.comm	_pgshift, 4

	.text	
	.globl	start

start:
	/*
	 * Set up a stack.
	 */
	set	start, %o1
	save	%o1, -CCFSZ, %sp

	/*
	 * Relocate.
	 */
1:	call	2f
	 nop
2:	add	%o7, (start-1b), %l0
	set	start, %l1
	set	_end, %o0
	sub	%o0, %l1, %l2		! length
3:	ld	[%l0], %o0
	add	%l0, 4, %l0
	st	%o0, [%l1]
	subcc	%l2, 4, %l2
	bg	3b
	 add	%l1, 4, %l1

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
	set     _end, %o1
	/* note: bzero() expanded inline for compactness */
	subcc	%o1, %o0, %o1
	bz	2f			! in case there is no BSS

	srl	%o1, 2, %o1		! assume _edata & _end are aligned
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
	sethi	%hi(_cputyp), %o0
	set	0x4000, %g7
	cmp	%i0, %g7
	beq	is_sun4
	 nop

	mov	CPU_SUN4C, %g4
	mov	SUN4CM_PGSHIFT, %g5

	/*
	 * Save address of PROM vector (passed in %i0).
	 */
	sethi	%hi(_romp), %o1
	st	%i0, [%o1 + %lo(_romp)]

	/* Also save %i3, which is the Openfirmware entry, if any */
	sethi	%hi(_opf_romp), %o1
	st	%i3, [%o1 + %lo(_opf_romp)]
	b,a	is_sun4cm

is_sun4:
	mov	CPU_SUN4, %g4
	mov	SUN4_PGSHIFT, %g5

is_sun4cm:
	st	%g4, [%o0 + %lo(_cputyp)]
	sethi	%hi(_pgshift), %o0	! pgshift = log2(nbpg)
	st	%g5, [%o0 + %lo(_pgshift)]

	mov	1, %o0			! nbpg = 1 << pgshift
	sll	%o0, %g5, %g5
	sethi	%hi(_nbpg), %o0		! nbpg = bytes in a page
	st	%g5, [%o0 + %lo(_nbpg)]

	sub	%g5, 1, %g5
	sethi	%hi(_pgofset), %o0	! page offset = bytes in a page - 1
	st	%g5, [%o0 + %lo(_pgofset)]

	call	_main
	 mov	%i0, %o0

	ret
	 restore

/*
 * NO-OP place holder function.
 */
	.globl	_sparc_noop
_sparc_noop:
	retl
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
