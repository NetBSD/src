/*	$NetBSD: __setjmp14.S,v 1.5 2004/01/17 22:01:15 martin Exp $	*/

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

#define _LOCORE
#include <machine/asm.h>
#include <machine/frame.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
	.asciz "@(#)setjmp.s	8.1 (Berkeley) 6/4/93"
#else
	RCSID("$NetBSD: __setjmp14.S,v 1.5 2004/01/17 22:01:15 martin Exp $")
#endif
#endif /* LIBC_SCCS and not lint */

/*
 * C library -- setjmp, __longjmp14
 *
 *	__longjmp14(a,v)
 * will generate a "return(v)" from
 * the last call to
 *	setjmp(a)
 * by restoring registers from the stack,
 * and a struct sigcontext, see <signal.h>
 */

#include "SYS.h"

	.register %g2,#ignore
	.register %g3,#ignore
	.register %g6,#ignore

ENTRY(__setjmp14)
	/*
	 * We use the part of the sigcontext structure, the sp, pc, and npc fields,
	 * for the sigstack call so we don't need to get our own stackframe.  It 
	 * won't be filled out till later anyway.
	 */
	mov	%o0, %o3		/* Save our jmp_buf in %o3 */
	add	%o0, 0x38, %o2		/* build sigcontext in [%o2]->sc.sc_mask */
	mov	1, %o0			/* SIG_BLOCK */
	mov	SYS___sigprocmask14, %g1
	clr	%o1			/* sigprocmask(SIG_BLOCK, (sigset_t *)NULL, (sigset_t *)a) */
	t	ST_SYSCALL
	
	mov	SYS___sigaltstack14, %g1
	clr	%o0			/* sigstack(NULL, &foo) */
	add	%o3, 8, %o1		/* (foo being part of the sigcontext we're overwriting) */
	t	ST_SYSCALL
	
	lduw	[%o3 + 8 + 0x10], %o0	/* foo.ss_flags */
	and	%o0, 1, %o1		/* onstack = foo.ss_flags & 1; */
	st	%o0, [%o3 + 0x00]	/* sc.sc_onstack = current onstack; */
	stx	%sp, [%o3 + 0x08]	/* sc.sc_sp = sp (both ours and caller's) */
	add	%o7, 8, %o0
	stx	%o0, [%o3 + 0x10]	/* sc.sc_pc = return_pc */
	add	%o7, 12, %o0
	stx	%o0, [%o3 + 0x18]	/* sc.sc_npc = return_pc + 4 */
	stx	%g0, [%o3 + 0x20]	/* sc.sc_tstate = (clean ccr) */
	stx	%g1, [%o3 + 0x28]	/* sc.sc_g1 */
	stx	%g2, [%o3 + 0x30]	/* sc.sc_o0, set in longjmp, use as %o1 */
	/* 0x38 */			/* sc.sc_mask set above already */

	/* save additional registers needed to fill a complete mcontext */
	stx	%g3, [%o3 + 0x40]
	stx	%g4, [%o3 + 0x48]
	stx	%g5, [%o3 + 0x50]
	stx	%g6, [%o3 + 0x58]
	stx	%g7, [%o3 + 0x60]

	retl				/* return 0 */
	 clr	%o0
