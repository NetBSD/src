/*	$NetBSD: signal.h,v 1.5 2003/03/15 23:41:25 fvdl Exp $	*/

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
#include <machine/mcontext.h>

/*
 * Information pushed on stack when a signal is delivered.
 * This is used by the kernel to restore state following
 * execution of the signal handler.  It is also made available
 * to the handler to allow it to restore state properly if
 * a non-standard exit is performed.
 */
struct sigcontext {
	struct fxsave64 *sc_fpstate;
	u_int64_t	sc_onstack;
	sigset_t	sc_mask;
	u_int64_t	sc_pad0;
	mcontext_t	sc_mcontext;
};

#define _MCONTEXT_TO_SIGCONTEXT(uc, sc) 				\
do {									\
	memcpy(&(sc)->sc_mcontext.__gregs, &(uc)->uc_mcontext.__gregs,	\
	    sizeof ((uc)->uc_mcontext.__gregs));			\
	if ((uc)->uc_flags & _UC_FPU) {					\
		memcpy(&(sc)->sc_mcontext.__fpregs,			\
		    &(uc)->uc_mcontext.__fpregs,			\
		    sizeof ((uc)->uc_mcontext.__fpregs));		\
	}								\
} while (/*CONSTCOND*/0)

#define _SIGCONTEXT_TO_MCONTEXT(sc, uc)					\
do {									\
	memcpy(&(uc)->uc_mcontext.__gregs, &(sc)->sc_mcontext.__gregs,	\
	    sizeof ((uc)->uc_mcontext.__gregs));			\
	if ((uc)->uc_flags & _UC_FPU) {					\
		memcpy(&(uc)->uc_mcontext.__fpregs,			\
		    &(sc)->sc_mcontext.__fpregs,			\
		    sizeof ((uc)->uc_mcontext.__fpregs));		\
	}								\
} while (/*CONSTCOND*/0)

#endif	/* !_ANSI_SOURCE && !_POSIX_C_SOURCE && !_XOPEN_SOURCE */
#endif	/* !_X86_64_SIGNAL_H_ */
