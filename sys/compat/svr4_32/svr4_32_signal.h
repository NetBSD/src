/*	$NetBSD: svr4_32_signal.h,v 1.3 2002/07/04 23:32:14 thorpej Exp $	 */

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

#ifndef	_SVR4_32_SIGNAL_H_
#define	_SVR4_32_SIGNAL_H_

#include <compat/svr4_32/svr4_32_siginfo.h>
#include <compat/svr4/svr4_signal.h>


typedef netbsd32_caddr_t svr4_32_sig_t;
#define	SVR4_32_SIG_DFL		(svr4_32_sig_t)	 0
#define	SVR4_32_SIG_ERR		(svr4_32_sig_t)	-1
#define	SVR4_32_SIG_IGN		(svr4_32_sig_t)	 1
#define	SVR4_32_SIG_HOLD	(svr4_32_sig_t)	 2

typedef struct {
        netbsd32_u_long bits[4];
} svr4_32_sigset_t;
typedef netbsd32_caddr_t svr4_32_sigset_tp;

struct svr4_32_sigaction {
	int			sa_flags;
	svr4_32_sig_t		sa_handler;
	svr4_32_sigset_t	sa_mask;
	int			sa_reserved[2];
};
typedef netbsd32_caddr_t svr4_32_sigactionp;

struct svr4_32_sigaltstack {
	netbsd32_charp	ss_sp;
	int		ss_size;
	int		ss_flags;
};
typedef netbsd32_caddr_t svr4_32_sigaltstackp;

void native_to_svr4_32_sigset __P((const sigset_t *, svr4_32_sigset_t *));
void svr4_32_to_native_sigset __P((const svr4_32_sigset_t *, sigset_t *));
void native_to_svr4_32_sigaltstack __P((const struct sigaltstack *, struct svr4_32_sigaltstack *));
void svr4_32_to_native_sigaltstack __P((const struct svr4_32_sigaltstack *, struct sigaltstack *));
void svr4_32_sendsig __P((int, sigset_t *, u_long));

#endif /* !_SVR4_32_SIGNAL_H_ */
