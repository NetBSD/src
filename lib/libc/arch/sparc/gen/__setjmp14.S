/*	$NetBSD: __setjmp14.S,v 1.4 2004/03/23 18:46:47 pk Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
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
 * from: Header: setjmp.s,v 1.2 92/06/25 03:18:43 torek Exp
 */

#include <machine/asm.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
	.asciz "@(#)setjmp.s	8.1 (Berkeley) 6/4/93"
#else
	RCSID("$NetBSD: __setjmp14.S,v 1.4 2004/03/23 18:46:47 pk Exp $")
#endif
#endif /* LIBC_SCCS and not lint */

/*
 * C library -- setjmp, longjmp
 *
 *	longjmp(a,v)
 * will generate a "return(v)" from
 * the last call to
 *	setjmp(a)
 * by restoring registers from the stack,
 * and a struct sigcontext, see <signal.h>
 */

#include "SYS.h"

ENTRY(__setjmp14)
	save	%sp, -96, %sp
#ifdef PIC
	PIC_PROLOGUE(%l0,%o0)	! %l0 = _GLOBAL_OFFSET_TABLE
#endif
	/*
	 * local vars on stack: struct sigstack foo @ offset 0x48
	 * Note: sc_g1 is not used; sc_o0 is set in longjmp()
	 */
#ifdef PIC
	ld	[%l0 + _C_LABEL(__sigprocmask14)], %g2
#endif
	mov	1, %o0		/* SIG_BLOCK */
	add	%i0, 32, %o2	/* build sigcontext sc.sc_mask */
#ifdef PIC
	call	%g2
#else
	call	_C_LABEL(__sigprocmask14)
#endif
	 clr	%o1		/* sigprocmask(SIG_BLOCK, NULL, &sc.sc_mask) */

#ifdef PIC
	ld	[%l0 + _C_LABEL(__sigaltstack14)], %g2
#endif
	add	%sp, 0x48, %o1	/* (foo being in arg dump area) */
#ifdef PIC
	call	%g2
#else
	call	_C_LABEL(__sigaltstack14)
#endif
	 clr	%o0		/* sigstack(NULL, &foo) */

	ld	[%sp + 0x48 + 8], %o0	/* foo.ss_flags */
	and	%o0, 1, %o1	/* onstack = foo.ss_flags & 1; */
	st	%o1, [%i0 + 0]	/* sc.sc_onstack = current onstack; */
	st	%fp, [%i0 + 8]	/* sc.sc_sp = (caller's) sp */
	add	%i7, 8, %o0
	st	%o0, [%i0 + 12]	/* sc.sc_pc = return_pc */
	add	%i7, 12, %o0
	st	%o0, [%i0 + 16]	/* sc.sc_npc = return_pc + 4 */
	st	%g0, [%i0 + 20]	/* sc.sc_psr = (clean psr) */
	ret			/* return 0 */
	 restore	%g0, %g0, %o0

