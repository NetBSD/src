/*	$NetBSD: svr4_32_ucontext.h,v 1.7.16.1 2008/01/09 01:52:03 matt Exp $	 */

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

#ifndef	_SVR4_32_UCONTEXT_H_
#define	_SVR4_32_UCONTEXT_H_

#include <compat/svr4_32/svr4_32_types.h>
#include <compat/svr4_32/svr4_32_signal.h>
#include <machine/svr4_32_machdep.h>
#include <compat/svr4/svr4_ucontext.h>

/*
 * Machine context
 */
typedef netbsd32_caddr_t svr4_32_ucontext_tp;
typedef struct svr4_32_ucontext {
	netbsd32_u_long			uc_flags;
	svr4_32_ucontext_tp		uc_link;
	svr4_32_sigset_t		uc_sigmask;
	struct svr4_32_sigaltstack	uc_stack;
	svr4_32_mcontext_t		uc_mcontext;
#ifdef SVR4_32_UC_MACHINE_PAD
	netbsd32_long			uc_pad[SVR4_32_UC_MACHINE_PAD];
#endif
} svr4_32_ucontext_t;

/*
 * Signal frame
 */
struct svr4_32_sigframe {
	int			sf_signum;
	svr4_32_siginfo_tp	sf_sip;
	svr4_32_ucontext_tp	sf_ucp;
	sig_t			sf_handler;
	struct svr4_32_ucontext sf_uc;
	union svr4_32_siginfo	sf_si;
};


void *svr4_32_getmcontext(struct lwp *, struct svr4_32_mcontext *,
			       netbsd32_u_long *);
int svr4_32_setmcontext(struct lwp *, struct svr4_32_mcontext *,
			     netbsd32_u_long);

void svr4_32_getcontext(struct lwp *, struct svr4_32_ucontext *, const sigset_t *);
int svr4_32_setcontext(struct lwp *, struct svr4_32_ucontext *);

#endif /* !_SVR4_32_UCONTEXT_H_ */
