/*	$NetBSD: signal.h,v 1.1.14.1 2002/05/30 15:36:56 gehenna Exp $	*/

/*
 * Copyright (c) 1982, 1986, 1989, 1991 Regents of the University of California.
 * All rights reserved.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *	@(#)signal.h	7.16 (Berkeley) 3/17/91
 */

#ifndef _X86_64_SIGNAL_H_
#define _X86_64_SIGNAL_H_

typedef int sig_atomic_t;

#if !defined(_ANSI_SOURCE) && !defined(_POSIX_C_SOURCE) && \
    !defined(_XOPEN_SOURCE)
/*
 * Get the "code" values
 */
#include <machine/trap.h>
#include <machine/fpu.h>

/*
 * Information pushed on stack when a signal is delivered.
 * This is used by the kernel to restore state following
 * execution of the signal handler.  It is also made available
 * to the handler to allow it to restore state properly if
 * a non-standard exit is performed.
 */
struct sigcontext {
	int		sc_gs;
	int		sc_fs;	/* unused */
	int		sc_es;	/* unused */
	int		sc_ds;	/* unused */
	u_int64_t	sc_r8;
	u_int64_t	sc_r9;
	u_int64_t	sc_r10;
	u_int64_t	sc_r11;
	u_int64_t	sc_r12;
	u_int64_t	sc_r13;
	u_int64_t	sc_r14;
	u_int64_t	sc_r15;
	u_int64_t	sc_rdi;
	u_int64_t	sc_rsi;
	u_int64_t	sc_rbp;
	u_int64_t	sc_rbx;
	u_int64_t	sc_rdx;
	u_int64_t	sc_rcx;
	u_int64_t	sc_rax;
	u_int64_t	sc_trapno;
	u_int64_t	sc_err;
	u_int64_t	sc_rip;
	int		sc_cs;
	int		sc_pad0;
	u_int64_t	sc_rflags;
	u_int64_t	sc_rsp_onsig;
	struct fxsave64 *sc_fpstate;	/* XXXfvdl compat with Linux, but.. */
	int		sc_ss;
	int		sc_pad1;

	sigset_t	sc_mask;	/* signal mask to restore (new style) */
	u_int64_t	sc_onstack;	/* sigstack state to restore */

	u_int64_t	sc_rsp;
};

#endif	/* !_ANSI_SOURCE && !_POSIX_C_SOURCE && !_XOPEN_SOURCE */
#endif	/* !_X86_64_SIGNAL_H_ */
