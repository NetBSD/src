/*	$NetBSD: svr4_ucontext.h,v 1.7.76.1 2008/01/09 01:51:57 matt Exp $	 */

/*-
 * Copyright (c) 1994 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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

#ifndef	_SVR4_UCONTEXT_H_
#define	_SVR4_UCONTEXT_H_

#include <compat/svr4/svr4_types.h>
#include <compat/svr4/svr4_signal.h>
#include <machine/svr4_machdep.h>

/*
 * Machine context
 */

#define SVR4_UC_SIGMASK		0x01
#define	SVR4_UC_STACK		0x02

#define SVR4_UC_CPU		0x04
#define SVR4_UC_FPU		0x08
#define SVR4_UC_WEITEK		0x10

#define SVR4_UC_MCONTEXT	(SVR4_UC_CPU|SVR4_UC_FPU|SVR4_UC_WEITEK)

#define SVR4_UC_ALL		(SVR4_UC_SIGMASK|SVR4_UC_STACK|SVR4_UC_MCONTEXT)

typedef struct svr4_ucontext {
	u_long			 uc_flags;
	struct svr4_ucontext	*uc_link;
	svr4_sigset_t		 uc_sigmask;
	struct svr4_sigaltstack	 uc_stack;
	svr4_mcontext_t		 uc_mcontext;
#ifdef SVR4_UC_MACHINE_PAD
	long			 uc_pad[SVR4_UC_MACHINE_PAD];
#endif
} svr4_ucontext_t;

#define SVR4_UC_GETREGSET	0
#define SVR4_UC_SETREGSET	1

/*
 * Signal frame
 */
struct svr4_sigframe {
	int	sf_signum;
	union	svr4_siginfo  *sf_sip;
	struct	svr4_ucontext *sf_ucp;
	sig_t	sf_handler;
	struct	svr4_ucontext sf_uc;
	union	svr4_siginfo  sf_si;
};


void *svr4_getmcontext(struct lwp *, struct svr4_mcontext *, u_long *);
int svr4_setmcontext(struct lwp *, struct svr4_mcontext *, u_long);

void svr4_getcontext(struct lwp *, struct svr4_ucontext *);
int svr4_setcontext(struct lwp *, struct svr4_ucontext *);

#endif /* !_SVR4_UCONTEXT_H_ */
