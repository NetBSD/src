/*	$NetBSD: _lwp.c,v 1.1 2003/01/19 23:05:01 scw Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nathan J. Williams and Steve C. Woodford.
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

#include <sys/types.h>
#include <ucontext.h>
#include <lwp.h>
#include <stdlib.h>

void
_lwp_makecontext(ucontext_t *u, void (*start)(void *),
    void *arg, void *private, caddr_t stack_base, size_t stack_size)
{
	__greg_t *gr;

	getcontext(u);
	gr = u->uc_mcontext.__gregs;

	u->uc_link = NULL;

	u->uc_stack.ss_sp = stack_base;
	u->uc_stack.ss_size = stack_size;

	gr[_REG_PC] = (register_t)(intptr_t)start;
	gr[_REG_R(18)] = (register_t)(intptr_t)_lwp_exit;
	gr[_REG_R(2)] = (register_t)(intptr_t)arg;
	gr[_REG_SP] = (register_t)((stack_base + stack_size) & ~0x7);
	gr[_REG_FP] = 0;
	gr[_REG_USR] = 0x000f;	/* r0-r31 are dirty */
	gr[_REG_R(24)] = 0x12345678ACEBABE5ULL;	/* magic number */

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
}
