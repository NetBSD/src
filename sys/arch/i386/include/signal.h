/*	$NetBSD: signal.h,v 1.21 2003/09/11 20:22:30 christos Exp $	*/

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

#ifndef _I386_SIGNAL_H_
#define _I386_SIGNAL_H_

#include <sys/featuretest.h>

typedef int sig_atomic_t;

#define __HAVE_SIGINFO

#if defined(_NETBSD_SOURCE)
/*
 * Get the "code" values
 */
#include <machine/trap.h>

/*
 * Information pushed on stack when a signal is delivered.
 * This is used by the kernel to restore state following
 * execution of the signal handler.  It is also made available
 * to the handler to allow it to restore state properly if
 * a non-standard exit is performed.
 */
#if defined(__LIBC12_SOURCE__) || defined(_KERNEL)
struct sigcontext13 {
	int	sc_gs;
	int	sc_fs;
	int	sc_es;
	int	sc_ds;
	int	sc_edi;
	int	sc_esi;
	int	sc_ebp;
	int	sc_ebx;
	int	sc_edx;
	int	sc_ecx;
	int	sc_eax;
	/* XXX */
	int	sc_eip;
	int	sc_cs;
	int	sc_eflags;
	int	sc_esp;
	int	sc_ss;

	int	sc_onstack;		/* sigstack state to restore */
	int	sc_mask;		/* signal mask to restore (old style) */

	int	sc_trapno;		/* XXX should be above */
	int	sc_err;
};
#endif

#if defined(COMPAT_16) || compat(COMPAT_IBCS2) || !defined(_KERNEL)
/*
 * We expose this to userland for legacy interfaces, but only use
 * it in the kernel for compat code.
 */
struct sigcontext {
	int	sc_gs;
	int	sc_fs;
	int	sc_es;
	int	sc_ds;
	int	sc_edi;
	int	sc_esi;
	int	sc_ebp;
	int	sc_ebx;
	int	sc_edx;
	int	sc_ecx;
	int	sc_eax;
	/* XXX */
	int	sc_eip;
	int	sc_cs;
	int	sc_eflags;
	int	sc_esp;
	int	sc_ss;

	int	sc_onstack;		/* sigstack state to restore */
	int	__sc_mask13;		/* signal mask to restore (old style) */

	int	sc_trapno;		/* XXX should be above */
	int	sc_err;

	sigset_t sc_mask;		/* signal mask to restore (new style) */
};

/*
 * The following macros are used to convert from a ucontext to sigcontext,
 * and vice-versa.  This is for building a sigcontext to deliver to old-style
 * signal handlers, and converting back (in the event the handler modifies
 * the context).
 */
#define	_MCONTEXT_TO_SIGCONTEXT(uc, sc)					\
do {									\
	(sc)->sc_gs  = (uc)->uc_mcontext.__gregs[_REG_GS];		\
	(sc)->sc_fs  = (uc)->uc_mcontext.__gregs[_REG_FS];		\
	(sc)->sc_es  = (uc)->uc_mcontext.__gregs[_REG_ES];		\
	(sc)->sc_ds  = (uc)->uc_mcontext.__gregs[_REG_DS];		\
	(sc)->sc_edi = (uc)->uc_mcontext.__gregs[_REG_EDI];		\
	(sc)->sc_esi = (uc)->uc_mcontext.__gregs[_REG_ESI];		\
	(sc)->sc_ebp = (uc)->uc_mcontext.__gregs[_REG_EBP];		\
	(sc)->sc_ebx = (uc)->uc_mcontext.__gregs[_REG_EBX];		\
	(sc)->sc_edx = (uc)->uc_mcontext.__gregs[_REG_EDX];		\
	(sc)->sc_ecx = (uc)->uc_mcontext.__gregs[_REG_ECX];		\
	(sc)->sc_eax = (uc)->uc_mcontext.__gregs[_REG_EAX];		\
	(sc)->sc_eip = (uc)->uc_mcontext.__gregs[_REG_EIP];		\
	(sc)->sc_cs  = (uc)->uc_mcontext.__gregs[_REG_CS];		\
	(sc)->sc_eflags = (uc)->uc_mcontext.__gregs[_REG_EFL];		\
	(sc)->sc_esp = (uc)->uc_mcontext.__gregs[_REG_UESP];		\
	(sc)->sc_ss  = (uc)->uc_mcontext.__gregs[_REG_SS];		\
	(sc)->sc_trapno = (uc)->uc_mcontext.__gregs[_REG_TRAPNO];	\
	(sc)->sc_err = (uc)->uc_mcontext.__gregs[_REG_ERR];		\
} while (/*CONSTCOND*/0)

#define	_SIGCONTEXT_TO_MCONTEXT(sc, uc)					\
do {									\
	(uc)->uc_mcontext.__gregs[_REG_GS]  = (sc)->sc_gs;		\
	(uc)->uc_mcontext.__gregs[_REG_FS]  = (sc)->sc_fs;		\
	(uc)->uc_mcontext.__gregs[_REG_ES]  = (sc)->sc_es;		\
	(uc)->uc_mcontext.__gregs[_REG_DS]  = (sc)->sc_ds;		\
	(uc)->uc_mcontext.__gregs[_REG_EDI] = (sc)->sc_edi;		\
	(uc)->uc_mcontext.__gregs[_REG_ESI] = (sc)->sc_esi;		\
	(uc)->uc_mcontext.__gregs[_REG_EBP] = (sc)->sc_ebp;		\
	(uc)->uc_mcontext.__gregs[_REG_EBX] = (sc)->sc_ebx;		\
	(uc)->uc_mcontext.__gregs[_REG_EDX] = (sc)->sc_edx;		\
	(uc)->uc_mcontext.__gregs[_REG_ECX] = (sc)->sc_ecx;		\
	(uc)->uc_mcontext.__gregs[_REG_EAX] = (sc)->sc_eax;		\
	(uc)->uc_mcontext.__gregs[_REG_EIP] = (sc)->sc_eip;		\
	(uc)->uc_mcontext.__gregs[_REG_CS]  = (sc)->sc_cs;		\
	(uc)->uc_mcontext.__gregs[_REG_EFL] = (sc)->sc_eflags;		\
	(uc)->uc_mcontext.__gregs[_REG_UESP] = (sc)->sc_esp;		\
	(uc)->uc_mcontext.__gregs[_REG_UESP] = (sc)->sc_esp;		\
	(uc)->uc_mcontext.__gregs[_REG_SS]  = (sc)->sc_ss;		\
	(uc)->uc_mcontext.__gregs[_REG_TRAPNO] = (sc)->sc_trapno;	\
	(uc)->uc_mcontext.__gregs[_REG_ERR] = (sc)->sc_err;		\
} while (/*CONSTCOND*/0)
#endif

#define sc_sp sc_esp
#define sc_fp sc_ebp
#define sc_pc sc_eip
#define sc_ps sc_eflags

#endif	/* _NETBSD_SOURCE */
#endif	/* !_I386_SIGNAL_H_ */
