/*	$NetBSD: __setjmp14.S,v 1.3 2004/03/22 12:35:04 pk Exp $	*/

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
	RCSID("$NetBSD: __setjmp14.S,v 1.3 2004/03/22 12:35:04 pk Exp $")
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
	/*
	 * We use the caller's `arg dump' area (%sp+0x44; there are 6 ints
	 * reserved there for us) to avoid having to allocate stack space
	 * here.
	 */
	add	%o0, 32, %o2	/* build sigcontext sc.sc_mask */
	mov	1, %o0		/* SIG_BLOCK */
	mov	SYS___sigprocmask14, %g1
	clr	%o1		/* sigprocmask(SIG_BLOCK, NULL, &sc.sc_mask) */
	t	ST_SYSCALL
	sub	%o2, 32, %o2
	mov	SYS___sigaltstack14, %g1
	clr	%o0		/* sigstack(NULL, &foo) */
	add	%sp, 0x48, %o1	/* (foo being in arg dump area) */
	t	ST_SYSCALL
	ld	[%sp + 0x50], %o0	/* foo.ss_flags */
	and	%o0, 1, %o1	/* onstack = foo.ss_flags & 1; */
	st	%o0, [%o2 + 0]	/* sc.sc_onstack = current onstack; */
	st	%sp, [%o2 + 8]	/* sc.sc_sp = sp (both ours and caller's) */
	add	%o7, 8, %o0
	st	%o0, [%o2 + 12]	/* sc.sc_pc = return_pc */
	add	%o7, 12, %o0
	st	%o0, [%o2 + 16]	/* sc.sc_npc = return_pc + 4 */
	st	%g0, [%o2 + 20]	/* sc.sc_psr = (clean psr) */
	st	%fp, [%o2 + 24]	/* sc.sc_g1 = %fp (misuse, but what the heck) */
				/* sc.sc_o0 = random(), set in longjmp */
	retl			/* return 0 */
	 clr	%o0

