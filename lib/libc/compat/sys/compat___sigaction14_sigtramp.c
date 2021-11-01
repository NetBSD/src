/*	$NetBSD: compat___sigaction14_sigtramp.c,v 1.1 2021/11/01 05:53:45 thorpej Exp $	*/

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

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: compat___sigaction14_sigtramp.c,v 1.1 2021/11/01 05:53:45 thorpej Exp $");
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>
#include <stddef.h>
#include <signal.h>
#include <errno.h>

#include "extern.h"

#define C(a,b) __CONCAT(a,b)
#define __SIGTRAMP_SIGCONTEXT  \
    C(__sigtramp_sigcontext_,__SIGTRAMP_SIGCONTEXT_VERSION)
#define __SIGTRAMP_SIGINFO  \
    C(__sigtramp_siginfo_,__SIGTRAMP_SIGINFO_VERSION)

__weak_alias(__sigaction14, __libc_sigaction14)

#define __LIBC12_SOURCE__

/*
 * The symbol must remain, but we don't want this exposed in a header
 * anywhere, so the prototype goes here.
 */
int	__libc_sigaction14(int, const struct sigaction *, struct sigaction *);

int
__libc_sigaction14(int sig, const struct sigaction *act, struct sigaction *oact)
{
	extern const char __SIGTRAMP_SIGINFO[];

	/*
	 * If no sigaction, use the "default" trampoline since it won't
	 * be used.
	 */
	if (act == NULL)
		return  __sigaction_sigtramp(sig, act, oact, NULL, 0);

#if defined(__HAVE_STRUCT_SIGCONTEXT) &&  defined(__LIBC12_SOURCE__)
	/*
	 * We select the non-SA_SIGINFO trampoline if SA_SIGINFO is not
	 * set in the sigaction.
	 */
	if ((act->sa_flags & SA_SIGINFO) == 0) {
		extern const char __SIGTRAMP_SIGCONTEXT[];
		int sav = errno;
		int rv =  __sigaction_sigtramp(sig, act, oact,
		    __SIGTRAMP_SIGCONTEXT, __SIGTRAMP_SIGCONTEXT_VERSION);
		if (rv >= 0 || errno != EINVAL)
			return rv;
		errno = sav;
	}
#endif

	/*
	 * If SA_SIGINFO was specified or the compatibility trampolines
	 * can't be used, use the siginfo trampoline.
	 */
	return __sigaction_sigtramp(sig, act, oact,
	    __SIGTRAMP_SIGINFO, __SIGTRAMP_SIGINFO_VERSION);
}
