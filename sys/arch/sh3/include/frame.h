/*	$NetBSD: frame.h,v 1.13 2004/03/24 15:38:41 wiz Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc. All rights reserved.
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
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
 *	@(#)frame.h	5.2 (Berkeley) 1/18/91
 */

/*-
 * Copyright (c) 1995 Charles M. Hannum.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
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
 *	@(#)frame.h	5.2 (Berkeley) 1/18/91
 */

#ifndef _SH3_FRAME_H_
#define	_SH3_FRAME_H_

#include <sys/signal.h>
#include <sys/sa.h>

/*
 * Exception Stack Frame
 */
struct trapframe {
	/* software member */
	int	tf_expevt;
	int	tf_ubc;
	/* hardware registers */
	int	tf_spc;
	int	tf_ssr;
	int	tf_macl;
	int	tf_mach;
	int	tf_pr;
	int	tf_r13;
	int	tf_r12;
	int	tf_r11;
	int	tf_r10;
	int	tf_r9;
	int	tf_r8;
	int	tf_r7;
	int	tf_r6;
	int	tf_r5;
	int	tf_r4;
	int	tf_r3;
	int	tf_r2;
	int	tf_r1;
	int	tf_r0;
	int	tf_r15;
	int	tf_r14;
};

/*
 * Stack frame inside cpu_switch()
 */
struct switchframe {
	int	sf_r15;
	int	sf_r14;
	int	sf_r13;
	int	sf_r12;
	int	sf_r11;
	int	sf_r10;
	int	sf_r9;
	int	sf_r8;
	int	sf_pr;
	int	sf_r6_bank;
	int	sf_sr;
	int	sf_r7_bank;
};

/*
 * Signal frame.
 *
 * NB: The order of sf_uc and sf_si is different from what other ports
 * use (siginfo at the top of the stack), because we want to avoid
 * wasting two instructions in __sigtramp_siginfo_2 to skip to the
 * ucontext.  Not that this order really matters, but I think this
 * inconsistency deserves an explanation.
 */
struct sigframe_siginfo {
#if 0 /* in registers on entry to signal trampoline */
	int		sf_signum; /* r4 - "signum" argument for handler */
	siginfo_t	*sf_sip;   /* r5 - "sip" argument for handler */
	ucontext_t	*sf_ucp;   /* r6 - "ucp" argument for handler */
#endif
	ucontext_t	sf_uc;	/* actual saved ucontext */
	siginfo_t	sf_si;	/* actual saved siginfo */
};

#if defined(COMPAT_16) && defined(_KERNEL)
/*
 * Old signal frame format.
 */
struct sigframe_sigcontext {
#if 0 /* in registers on entry to signal trampoline */
	int	sf_signum;	/* r4 - "signum" argument for handler */
	int	sf_code;	/* r5 - "code" argument for handler */
	struct sigcontext *sf_scp; /* r6 - "scp" argument for handler */
#endif
	struct sigcontext sf_sc; /* actual saved context */
};
#endif

/*
 * Scheduler activations upcall frame
 */
struct saframe {
#if 0 /* in registers on entry to upcallcode */
	int		sa_type;	/* r4 */
	struct sa_t **	sa_sas;		/* r5 */
	int		sa_events;	/* r6 */
	int		sa_interrupted;	/* r7 */
#endif
	void *		sa_arg;
};

#endif /* !_SH3_FRAME_H_ */
