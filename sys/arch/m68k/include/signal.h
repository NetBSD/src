/*	$NetBSD: signal.h,v 1.13 2003/08/07 16:28:15 agc Exp $	*/

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

#ifndef _M68K_SIGNAL_H_
#define _M68K_SIGNAL_H_

#include <sys/featuretest.h>

typedef int sig_atomic_t;

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

/*
 * The following macros are used to convert from a ucontext to sigcontext,
 * and vice-versa.  This is for building a sigcontext to deliver to old-style
 * signal handlers, and converting back (in the event the handler modifies
 * the context).
 *
 * On m68k, we also need the sigstate conversion macros below.
 */
#define	_MCONTEXT_TO_SIGCONTEXT(uc, sc)					\
do {									\
	(sc)->sc_sp = (uc)->uc_mcontext.__gregs[_REG_A7];		\
	(sc)->sc_fp = (uc)->uc_mcontext.__gregs[_REG_A6];		\
	/* sc_ap points to sigstate */					\
	(sc)->sc_pc = (uc)->uc_mcontext.__gregs[_REG_PC];		\
	(sc)->sc_ps = (uc)->uc_mcontext.__gregs[_REG_PS];		\
} while (/*CONSTCOND*/0)

#define	_SIGCONTEXT_TO_MCONTEXT(sc, uc)					\
do {									\
	(uc)->uc_mcontext.__gregs[_REG_A7] = (sc)->sc_sp;		\
	(uc)->uc_mcontext.__gregs[_REG_A6] = (sc)->sc_fp;		\
	(uc)->uc_mcontext.__gregs[_REG_PC] = (sc)->sc_pc;		\
	(uc)->uc_mcontext.__gregs[_REG_PS] = (sc)->sc_ps;		\
} while (/*CONSTCOND*/0)

#if defined(__M68K_SIGNAL_PRIVATE)
#include <m68k/frame.h>

/*
 * Register state saved while kernel delivers a signal.
 */
struct sigstate {
	int	ss_flags;		/* which of the following are valid */
	struct frame ss_frame;		/* original exception frame */
	struct fpframe ss_fpstate;	/* 68881/68882 state info */
};

#define	SS_RTEFRAME	0x01
#define	SS_FPSTATE	0x02
#define	SS_USERREGS	0x04

#ifdef _KERNEL
#define	_SIGSTATE_EXFRAMESIZE(fmt)	exframesize[(fmt)]
#endif

#define	_MCONTEXT_TO_SIGSTATE(uc, ss)					\
do {									\
	(ss)->ss_flags = SS_USERREGS;					\
	memcpy(&(ss)->ss_frame.f_regs, &(uc)->uc_mcontext.__gregs,	\
	    16 * sizeof(unsigned int));					\
	(ss)->ss_frame.f_pc = (uc)->uc_mcontext.__gregs[_REG_PC];	\
	(ss)->ss_frame.f_sr = (uc)->uc_mcontext.__gregs[_REG_PS];	\
	(ss)->ss_frame.f_pad = 0;					\
	(ss)->ss_frame.f_stackadj = 0;					\
	(ss)->ss_frame.f_format =					\
	    (uc)->uc_mcontext.__mc_pad.__mc_frame.__mcf_format;		\
	if ((ss)->ss_frame.f_format >= FMT4) {				\
		(ss)->ss_flags |= SS_RTEFRAME;				\
		(ss)->ss_frame.f_vector =				\
		    (uc)->uc_mcontext.__mc_pad.__mc_frame.__mcf_vector;	\
		memcpy(&(ss)->ss_frame.F_u,				\
		    &(uc)->uc_mcontext.__mc_pad.__mc_frame.__mcf_exframe,\
		    _SIGSTATE_EXFRAMESIZE((ss)->ss_frame.f_format));	\
	}								\
									\
	(ss)->ss_fpstate.FPF_u1 =					\
	    (uc)->uc_mcontext.__mc_pad.__mc_frame.__mcf_fpf_u1;		\
	if ((uc)->uc_flags & _UC_FPU) { /* non-null FP frame */		\
		(ss)->ss_fpstate.FPF_u2 =				\
		    (uc)->uc_mcontext.__mc_pad.__mc_frame.__mcf_fpf_u2;	\
		memcpy(&(ss)->ss_fpstate.fpf_regs,			\
		    &(uc)->uc_mcontext.__fpregs.__fp_fpregs,		\
		    sizeof((ss)->ss_fpstate.fpf_regs));			\
		(ss)->ss_fpstate.fpf_fpcr =				\
		    (uc)->uc_mcontext.__fpregs.__fp_pcr;		\
		(ss)->ss_fpstate.fpf_fpsr =				\
		    (uc)->uc_mcontext.__fpregs.__fp_psr;		\
		(ss)->ss_fpstate.fpf_fpiar =				\
		    (uc)->uc_mcontext.__fpregs.__fp_piaddr;		\
		(ss)->ss_flags |= SS_FPSTATE;				\
	}								\
} while (/*CONSTCOND*/0)

#define	_SIGSTATE_TO_MCONTEXT(ss, uc)					\
do {									\
	memcpy(&(uc)->uc_mcontext.__gregs, &(ss)->ss_frame.f_regs,	\
	    16 * sizeof(unsigned int));					\
	(uc)->uc_mcontext.__gregs[_REG_PC] = (ss)->ss_frame.f_pc;	\
	(uc)->uc_mcontext.__gregs[_REG_PS] = (ss)->ss_frame.f_sr;	\
	(uc)->uc_mcontext.__mc_pad.__mc_frame.__mcf_format =		\
	    (ss)->ss_frame.f_format;					\
	if ((ss)->ss_flags & SS_RTEFRAME) {				\
		(uc)->uc_mcontext.__mc_pad.__mc_frame.__mcf_vector =	\
		    (ss)->ss_frame.f_vector;				\
		memcpy(&(uc)->uc_mcontext.__mc_pad.__mc_frame.__mcf_exframe,\
		    &(ss)->ss_frame.F_u,				\
		    _SIGSTATE_EXFRAMESIZE((ss)->ss_frame.f_format));	\
	}								\
									\
	(uc)->uc_mcontext.__mc_pad.__mc_frame.__mcf_fpf_u1 =		\
	    (ss)->ss_fpstate.FPF_u1;					\
	if ((ss)->ss_flags & SS_FPSTATE) {				\
		(uc)->uc_mcontext.__mc_pad.__mc_frame.__mcf_fpf_u2 =	\
		    (ss)->ss_fpstate.FPF_u2;				\
		memcpy(&(uc)->uc_mcontext.__fpregs.__fp_fpregs,		\
		    &(ss)->ss_fpstate.fpf_regs,				\
		    sizeof((ss)->ss_fpstate.fpf_regs));			\
		(uc)->uc_mcontext.__fpregs.__fp_pcr =			\
		    (ss)->ss_fpstate.fpf_fpcr;				\
		(uc)->uc_mcontext.__fpregs.__fp_psr =			\
		    (ss)->ss_fpstate.fpf_fpsr;				\
		(uc)->uc_mcontext.__fpregs.__fp_piaddr =		\
		    (ss)->ss_fpstate.fpf_fpiar;				\
		(uc)->uc_flags |= _UC_FPU;				\
	} else								\
		(uc)->uc_flags &= ~_UC_FPU;				\
} while (/*CONSTCOND*/0)

/*
 * Stack frame layout when delivering a signal.
 */
struct sigframe {
	int	sf_ra;			/* handler return address */
	int	sf_signum;		/* signal number for handler */
	int	sf_code;		/* additional info for handler */
	struct sigcontext *sf_scp;	/* context pointer for handler */
	struct sigcontext sf_sc;	/* actual context */
	struct sigstate sf_state;	/* state of the hardware */
};
#endif /* _KERNEL && __M68K_SIGNAL_PRIVATE */

#endif	/* _NETBSD_SOURCE */
#endif	/* !_M68K_SIGNAL_H_ */
