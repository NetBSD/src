/*	$NetBSD: makecontext.c,v 1.3 2003/01/21 11:29:29 scw Exp $	*/

/*
 * Copyright 2003 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>

#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: makecontext.c,v 1.3 2003/01/21 11:29:29 scw Exp $");
#endif

#include <sys/types.h>
#include <inttypes.h>
#include <ucontext.h>
#include "extern.h"

#include <stdarg.h>
#include <sh5/fpu.h>

void
makecontext(ucontext_t *ucp, void (*func)(void), int argc, ...)
{
	__greg_t *gr = ucp->uc_mcontext.__gregs;
	register_t *sp;
	register_t r;
	va_list ap;
	int i;

	sp  = (register_t *)
	    (((intptr_t)ucp->uc_stack.ss_sp + ucp->uc_stack.ss_size) & ~7);
	if (argc > 8)
		sp -= (argc - 8);

	gr[_REG_PC] = (__greg_t)(intptr_t)func;
	gr[_REG_R(18)] = (__greg_t)(intptr_t)_resumecontext;
	gr[_REG_SP] = (__greg_t)(intptr_t)sp;
	gr[_REG_FP] = 0;
	gr[_REG_USR] = 0x000f;	/* r0-r31 are dirty */

	/*
	 * The new context inherits the global variable/constant pointers
	 * in r26 and r27.
	 *
	 * XXXSCW: I've never actually seen these used anywhere, so this
	 * may well be unnecessary...
	 */
	__asm __volatile("or r26, r63, %0" : "=r"(r));
	gr[_REG_R(26)] = r;
	__asm __volatile("or r27, r63, %0" : "=r"(r));
	gr[_REG_R(27)] = r;

	/*
	 * Ensure the branch target registers contain sane values
	 * or else the kernel will refuse to restore the context.
	 */
	gr[_REG_TR(0)] = 0;
	gr[_REG_TR(1)] = 0;
	gr[_REG_TR(2)] = 0;
	gr[_REG_TR(3)] = 0;
	gr[_REG_TR(4)] = 0;
	gr[_REG_TR(5)] = 0;
	gr[_REG_TR(6)] = 0;
	gr[_REG_TR(7)] = 0;

	/*
	 * Ensure the FPSCR is valid
	 */
	ucp->uc_mcontext.__fpregs.__fp_scr = SH5_FPSCR_DN_FLUSH_ZERO;

	va_start(ap, argc);

	/*
	 * First 8 args are passed in r2-r9
	 */
	for (i = 0; i < argc && i < 8; i++)
		gr[_REG_R(i)] = (register_t)va_arg(ap, long);

	/*
	 * Additional args are passed on the stack as 64-bit quantities
	 */
	for (argc -= i; argc > 0; argc--)
		*sp++ = (register_t)va_arg(ap, long);
	va_end(ap);
}
