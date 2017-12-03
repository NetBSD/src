/*      $NetBSD: signal.h,v 1.15.24.1 2017/12/03 11:36:48 jdolecek Exp $   */

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

 /* All bugs are subject to removal without further notice */

#ifndef _VAX_SIGNAL_H_
#define _VAX_SIGNAL_H_

#include <sys/featuretest.h>
#include <sys/siginfo.h>
#include <machine/trap.h>

typedef int sig_atomic_t;

#if defined(_NETBSD_SOURCE)
#include <sys/sigtypes.h>
/*
 * Information pushed on stack when a signal is delivered.
 * This is used by the kernel to restore state following
 * execution of the signal handler.  It is also made available
 * to the handler to allow it to restore state properly if
 * a non-standard exit is performed.
 */
#if defined(__LIBC12_SOURCE__) || defined(_KERNEL)
struct sigcontext13 {
	int	sc_onstack;		/* sigstack state to restore */
	int	sc_mask;		/* signal mask to restore (old style) */
	int	sc_sp;			/* sp to restore */
	int	sc_fp;			/* fp to restore */
	int	sc_ap;			/* ap to restore */
	int	sc_pc;			/* pc to restore */
	int	sc_ps;			/* psl to restore */
};
#endif /* __LIBC12_SOURCE__ || _KERNEL */

struct sigcontext {
	int	sc_onstack;		/* sigstack state to restore */
	int	__sc_mask13;		/* signal mask to restore (old style) */
	int	sc_sp;			/* sp to restore */
	int	sc_fp;			/* fp to restore */
	int	sc_ap;			/* ap to restore */
	int	sc_pc;			/* pc to restore */
	int	sc_ps;			/* psl to restore */
	sigset_t sc_mask;		/* signal mask to restore (new style) */
};

#ifdef _KERNEL
#define sendsig_sigcontext	sendsig_sighelper
#define sendsig_siginfo		sendsig_sighelper

void sendsig_context(int, const sigset_t *, u_long);

/* Avoid a cyclic dependency and don't use ksiginfo_t here. */
struct ksiginfo;

#if defined(COMPAT_13) || defined(COMPAT_ULTRIX) || defined(COMPAT_IBCS2)
struct otrampframe {
	unsigned sig;	/* Signal number */
	unsigned code;	/* Info code */
	vaddr_t	scp;	/* Pointer to struct sigcontext */
	unsigned r0, r1, r2, r3, r4, r5; /* Registers saved when interrupt */
	register_t pc;	/* Address of signal handler */
	vaddr_t	arg;	/* Pointer to first (and only) sigreturn argument */
};

vaddr_t setupstack_oldsigcontext(const struct ksiginfo *, const sigset_t *,
	int, struct lwp *, struct trapframe *, vaddr_t, int, vaddr_t);
#endif /* COMPAT_13 || COMPAT_ULTRIX || COMPAT_IBCS2 */

#if defined(COMPAT_16) || defined(COMPAT_ULTRIX)
struct trampoline2 {
	unsigned int narg;	/* Argument count (== 3) */
	unsigned int sig;	/* Signal number */
	unsigned int code;	/* Info code */
	vaddr_t scp;		/* Pointer to struct sigcontext */
};

vaddr_t setupstack_sigcontext2(const struct ksiginfo *, const sigset_t *,
	int, struct lwp *, struct trapframe *, vaddr_t, int, vaddr_t);
#endif /* COMPAT_16 || COMPAT_ULTRIX */

#endif	/* _KERNEL */

#endif	/* _NETBSD_SOURCE */
#endif	/* !_VAX_SIGNAL_H_ */
