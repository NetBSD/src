/*	$NetBSD: signal.h,v 1.23 2004/03/26 21:39:57 drochner Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell.
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
 *	@(#)signal.h	8.1 (Berkeley) 6/10/93
 */

#ifndef	_MIPS_SIGNAL_H_
#define	_MIPS_SIGNAL_H_

#include <sys/featuretest.h>

#include <machine/cdefs.h>	/* for API selection */

#ifdef COMPAT_16 
#define SIGTRAMP_VALID(vers) ((unsigned)(vers) <= 2)
#else
#define SIGTRAMP_VALID(vers) ((vers) == 2)
#endif 

#if !defined(__ASSEMBLER__)


/*
 * Machine-dependent signal definitions
 */

typedef int sig_atomic_t;

#if defined(_NETBSD_SOURCE)
/*
 * Information pushed on stack when a signal is delivered.
 * This is used by the kernel to restore state following
 * execution of the signal handler.  It is also made available
 * to the handler to allow it to restore state properly if
 * a non-standard exit is performed.
 *
 * sizeof(sigcontext) = 45 * sizeof(int) + 35 * sizeof(mips_reg_t)
 */
#if defined(__LIBC12_SOURCE__) || defined(_KERNEL)
struct sigcontext13 {
	int	sc_onstack;	/* sigstack state to restore */
	int	sc_mask;	/* signal mask to restore (old style) */
	mips_reg_t sc_pc;	/* pc at time of signal */
	mips_reg_t sc_regs[32];	/* processor regs 0 to 31 */
	mips_reg_t mullo, mulhi;/* mullo and mulhi registers... */
	int	sc_fpused;	/* fp has been used */
	int	sc_fpregs[33];	/* fp regs 0 to 31 and csr */
	int	sc_fpc_eir;	/* floating point exception instruction reg */
	int	sc_xxx[8];	/* XXX reserved */
};
#endif /* __LIBC12_SOURCE__ || _KERNEL */

struct sigcontext {
	int	sc_onstack;	/* sigstack state to restore */
	int	__sc_mask13;	/* signal mask to restore (old style) */
	mips_reg_t sc_pc;	/* pc at time of signal */
	mips_reg_t sc_regs[32];	/* processor regs 0 to 31 */
	mips_reg_t mullo, mulhi;/* mullo and mulhi registers... */
	int	sc_fpused;	/* fp has been used */
	int	sc_fpregs[33];	/* fp regs 0 to 31 and csr */
	int	sc_fpc_eir;	/* floating point exception instruction reg */
	int	sc_xxx[8];	/* XXX reserved */
	sigset_t sc_mask;	/* signal mask to restore (new style) */
};

#endif	/* _NETBSD_SOURCE */

#endif	/* !_LANGUAGE_ASSEMBLY */
#if !defined(_KERNEL)
/*
 * Hard code these to make people think twice about breaking compatibility.
 * These macros are generated independently for the kernel.
 */
#if !defined(_MIPS_BSD_API) || _MIPS_BSD_API == _MIPS_BSD_API_LP32
#define _OFFSETOF_SC_REGS	12
#define _OFFSETOF_SC_FPREGS	152
#define _OFFSETOF_SC_MASK	320
#else
#define _OFFSETOF_SC_REGS	16
#define _OFFSETOF_SC_FPREGS	292
#define _OFFSETOF_SC_MASK	460
#endif
#endif	/* !_KERNEL */
#endif	/* !_MIPS_SIGNAL_H_ */
