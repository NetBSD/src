/*	$NetBSD: signal.h,v 1.1 2002/07/05 13:32:02 scw Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
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

/*
 * SH-5 Signal Data Structures.
 *
 * XXX: SH-5 Signals are still very much in a state of flux.
 */

#ifndef _SH5_SIGNAL_H_
#define	_SH5_SIGNAL_H_

typedef int	sig_atomic_t;

#if !defined(_ANSI_SOURCE) && !defined(_POSIX_C_SOURCE) && \
    !defined(_XOPEN_SOURCE)
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
	int	sc_ownedfp;		/* fp has been used */
	long	sc_mask;		/* signal mask to restore (old style) */
	register_t sc_pc;		/* pc to restore */
	register_t sc_sr;		/* sr to restore */
	register_t sc_usr;		/* usr to restore */
	register_t sc_regs[64];		/* integer register set */
#define	sc_sp	sc_regs[15]
	register_t sc_fpscr;		/* FP statuus/control register */
	register_t sc_fpregs[32];	/* FP register set */
	long	sc_reserved[2];		/* XXX */
	long	sc_xxx[8];		/* XXX */
};
#endif /* __LIBC12_SOURCE__ || _KERNEL */

struct sigcontext {
	int	sc_onstack;		/* sigstack state to restore */
	int	sc_ownedfp;		/* fp has been used */
	long	__sc_mask13;		/* signal mask to restore (old style) */
	register_t sc_pc;		/* pc to restore */
	register_t sc_sr;		/* sr to restore */
	register_t sc_usr;		/* usr to restore */
	register_t sc_regs[64];		/* integer register set */
#define	sc_sp	sc_regs[15]
	register_t sc_fpregs[32];	/* FP register set */
	register_t sc_fpcr;		/* FP status/control register */
	long	sc_reserved[2];		/* XXX */
	long	sc_xxx[8];		/* XXX */
	sigset_t sc_mask;		/* signal mask to restore (new style) */
};

#endif /* !_ANSI_SOURCE && !_POSIX_C_SOURCE && !_XOPEN_SOURCE */
#endif /* !_SH5_SIGNAL_H_*/
