/*	$NetBSD: __sigaction14_sigtramp.c,v 1.6 2003/09/12 20:42:15 christos Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

#include <sys/types.h>
#include <sys/param.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <sys/syscall.h>

#include "extern.h"

__weak_alias(__sigaction14, __libc_sigaction14)

static int have_sigreturn = -1;

static void get_have_sigreturn(void);

extern int __sigtramp_siginfo_2[];
extern int __sigtramp_sigcontext_1[];

static void
get_have_sigreturn(void)
{
	struct sigaction nsa, osa;

	sigemptyset(&nsa.sa_mask);
	nsa.sa_flags = 0;
	nsa.sa_handler = SIG_IGN;

	__sigaction_sigtramp(SIGSYS, &nsa, &osa, __sigtramp_siginfo_2, 2);

	(void)syscall(SYS_compat_16___sigreturn14, NULL);
	have_sigreturn = errno == EFAULT;

	__sigaction_sigtramp(SIGSYS, &osa, NULL, __sigtramp_siginfo_2, 2);
}


int
__libc_sigaction14(int sig, const struct sigaction *act, struct sigaction *oact)
{
	if (have_sigreturn == -1)
		get_have_sigreturn();

	if (have_sigreturn && act && (act->sa_flags & SA_SIGINFO) == 0)
		return __sigaction_sigtramp(sig, act, oact,
		    __sigtramp_sigcontext_1, 1);

	return __sigaction_sigtramp(sig, act, oact, __sigtramp_siginfo_2, 2);
}
