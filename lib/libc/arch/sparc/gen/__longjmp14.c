/*	$NetBSD: __longjmp14.c,v 1.1 2004/03/22 12:35:04 pk Exp $	*/

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christian Limpach.
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

#include "namespace.h"
#include <sys/types.h>
#include <ucontext.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#define __LIBC12_SOURCE__
#include <setjmp.h>

/*
 * Use setcontext to reload the stack pointer, program counter <pc,npc>, and
 * set the return value in %o0.  The %i and %l registers will be reloaded
 * from the place to which %sp points.
 */
void
__longjmp14(jmp_buf env, int val)
{
	struct sigcontext *sc = (void *)env;
	ucontext_t uc;

	/* Ensure non-zero SP */
	if (sc->sc_sp == 0)
		goto err;

	/* Initialise the fields we're going to use */
	uc.uc_link = 0;

	/*
	 * Set _UC_SIGMASK, _UC_CPU. No FPU data saved, so we can't restore
	 * that. Set _UC_{SET,CLR}STACK according to SS_ONSTACK
	 */
	uc.uc_flags = _UC_CPU | (sc->sc_onstack ? _UC_SETSTACK : _UC_CLRSTACK);

	/*
	 * Set the signal mask - this is a weak symbol, so don't use
	 * _UC_SIGMASK in the mcontext, libpthread might override sigprocmask.
	 */
	sigprocmask(SIG_SETMASK, &sc->sc_mask, NULL);

	/* Fill other registers */
	uc.uc_mcontext.__gregs[_REG_PSR] = sc->sc_psr;
	uc.uc_mcontext.__gregs[_REG_PC] = sc->sc_pc;
	uc.uc_mcontext.__gregs[_REG_nPC] = sc->sc_npc;
	uc.uc_mcontext.__gregs[_REG_G1] = sc->sc_g1;
	uc.uc_mcontext.__gregs[_REG_G2] = sc->sc_o0;
	uc.uc_mcontext.__gregs[_REG_O6] = sc->sc_sp;

	/* Make return value non-zero */
	if (val == 0)
		val = 1;

	/* Save return value in context */
	uc.uc_mcontext.__gregs[_REG_O0] = val;

	setcontext(&uc);

err:
	longjmperror();
	abort();
	/* NOTREACHED */
}
