/*	$NetBSD: signal.h,v 1.7 2003/08/07 16:29:29 agc Exp $	*/

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
 *	@(#)signal.h	7.16 (Berkeley) 3/17/91
 */

#ifndef _SH3_SIGNAL_H_
#define	_SH3_SIGNAL_H_

#include <sys/featuretest.h>

typedef int sig_atomic_t;

#if defined(_NETBSD_SOURCE)

/*
 * Information pushed on stack when a signal is delivered.
 * This is used by the kernel to restore state following
 * execution of the signal handler.  It is also made available
 * to the handler to allow it to restore state properly if
 * a non-standard exit is performed.
 */
#if defined(__LIBC12_SOURCE__) || defined(_KERNEL)
struct sigcontext13 {
	int	sc_spc;
	int	sc_ssr;
	int	sc_pr;
	int	sc_r14;
	int	sc_r13;
	int	sc_r12;
	int	sc_r11;
	int	sc_r10;
	int	sc_r9;
	int	sc_r8;
	int	sc_r7;
	int	sc_r6;
	int	sc_r5;
	int	sc_r4;
	int	sc_r3;
	int	sc_r2;
	int	sc_r1;
	int	sc_r0;
	int	sc_r15;

	int	sc_onstack;		/* sigstack state to restore */
	int	sc_mask;		/* signal mask to restore (old style) */

	int	sc_expevt;		/* XXX should be above */
	int	sc_err;
};
#endif

struct sigcontext {
	int	sc_spc;
	int	sc_ssr;
	int	sc_pr;
	int	sc_r14;
	int	sc_r13;
	int	sc_r12;
	int	sc_r11;
	int	sc_r10;
	int	sc_r9;
	int	sc_r8;
	int	sc_r7;
	int	sc_r6;
	int	sc_r5;
	int	sc_r4;
	int	sc_r3;
	int	sc_r2;
	int	sc_r1;
	int	sc_r0;
	int	sc_r15;

	int	sc_onstack;	/* sigstack state to restore */

	int	sc_expevt;	/* XXX should be above */
	int	sc_err;

	sigset_t sc_mask;	/* signal mask to restore (new style) */
};

/*
 * The following macros are used to convert from a ucontext to sigcontext,
 * and vice-versa.  This is for building a sigcontext to deliver to old-style
 * signal handlers, and converting back (in the event the handler modifies
 * the context).
 */
#define	_MCONTEXT_TO_SIGCONTEXT(uc, sc)					\
do {									\
	memcpy(&(sc)->sc_pr, &(uc)->uc_mcontext.__gregs[_REG_PR],	\
	    17 * sizeof(unsigned int));					\
	(sc)->sc_spc    = (uc)->uc_mcontext.__gregs[_REG_PC];		\
	(sc)->sc_ssr    = (uc)->uc_mcontext.__gregs[_REG_SR];		\
	(sc)->sc_expevt = (uc)->uc_mcontext.__gregs[_REG_EXPEVT];	\
	(sc)->sc_err    = 0;	/* XXX */				\
} while (/*CONSTCOND*/0)

#define	_SIGCONTEXT_TO_MCONTEXT(sc, uc)					\
do {									\
	memcpy(&(uc)->uc_mcontext.__gregs[_REG_PR], &(sc)->sc_pr,	\
	    17 * sizeof(unsigned int));					\
	(uc)->uc_mcontext.__gregs[_REG_PC]     = (sc)->sc_spc;		\
	(uc)->uc_mcontext.__gregs[_REG_SR]     = (sc)->sc_ssr;		\
	(uc)->uc_mcontext.__gregs[_REG_EXPEVT] = (sc)->sc_expevt;	\
} while (/*CONSTCOND*/0)

#endif	/* _NETBSD_SOURCE */
#endif	/* !_SH3_SIGNAL_H_ */
