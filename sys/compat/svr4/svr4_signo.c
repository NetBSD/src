/*	$NetBSD: svr4_signo.c,v 1.1.2.2 2002/04/17 00:05:21 nathanw Exp $	 */

/*-
 * Copyright (c) 2002. The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: svr4_signo.c,v 1.1.2.2 2002/04/17 00:05:21 nathanw Exp $");

#include <sys/types.h>
#include <sys/signal.h>

#include <compat/svr4/svr4_types.h>
#include <compat/svr4/svr4_signal.h>

const int native_to_svr4_signo[NSIG] = {
	0,			/* 0 */
	SVR4_SIGHUP,		/* 1 */
	SVR4_SIGINT,		/* 2 */
	SVR4_SIGQUIT,		/* 3 */
	SVR4_SIGILL,		/* 4 */
	SVR4_SIGTRAP,		/* 5 */
	SVR4_SIGABRT,		/* 6 */
	SVR4_SIGEMT,		/* 7 */
	SVR4_SIGFPE,		/* 8 */
	SVR4_SIGKILL,		/* 9 */
	SVR4_SIGBUS,		/* 10 */
	SVR4_SIGSEGV,		/* 11 */
	SVR4_SIGSYS,		/* 12 */
	SVR4_SIGPIPE,		/* 13 */
	SVR4_SIGALRM,		/* 14 */
	SVR4_SIGTERM,		/* 15 */
	SVR4_SIGURG,		/* 16 */
	SVR4_SIGSTOP,		/* 17 */
	SVR4_SIGTSTP,		/* 18 */
	SVR4_SIGCONT,		/* 19 */
	SVR4_SIGCHLD,		/* 20 */
	SVR4_SIGTTIN,		/* 21 */
	SVR4_SIGTTOU,		/* 22 */
	SVR4_SIGIO,		/* 23 */
	SVR4_SIGXCPU,		/* 24 */
	SVR4_SIGXFSZ,		/* 25 */
	SVR4_SIGVTALRM,		/* 26 */
	SVR4_SIGPROF,		/* 27 */
	SVR4_SIGWINCH,		/* 28 */
	0,			/* 29 SIGINFO */
	SVR4_SIGUSR1,		/* 30 */
	SVR4_SIGUSR2,		/* 31 */
	SVR4_SIGPWR,		/* 32 */
	SVR4_SIGRTMIN + 0,	/* 33 */
	SVR4_SIGRTMIN + 1,	/* 34 */
	SVR4_SIGRTMIN + 2,	/* 35 */
	SVR4_SIGRTMIN + 3,	/* 36 */
	SVR4_SIGRTMIN + 4,	/* 37 */
	SVR4_SIGRTMIN + 5,	/* 38 */
	SVR4_SIGRTMIN + 6,	/* 39 */
	SVR4_SIGRTMIN + 7,	/* 40 */
	SVR4_SIGRTMIN + 8,	/* 41 */
	SVR4_SIGRTMIN + 9,	/* 42 */
	SVR4_SIGRTMIN + 10,	/* 43 */
	SVR4_SIGRTMIN + 11,	/* 44 */
	SVR4_SIGRTMIN + 12,	/* 45 */
	SVR4_SIGRTMIN + 13,	/* 46 */
	SVR4_SIGRTMIN + 14,	/* 47 */
	SVR4_SIGRTMIN + 15,	/* 48 */
	SVR4_SIGRTMIN + 16,	/* 49 */
	SVR4_SIGRTMIN + 17,	/* 50 */
	SVR4_SIGRTMIN + 18,	/* 51 */
	SVR4_SIGRTMIN + 19,	/* 52 */
	SVR4_SIGRTMIN + 20,	/* 53 */
	SVR4_SIGRTMIN + 21,	/* 54 */
	SVR4_SIGRTMIN + 22,	/* 55 */
	SVR4_SIGRTMIN + 23,	/* 56 */
	SVR4_SIGRTMIN + 24,	/* 57 */
	SVR4_SIGRTMIN + 25,	/* 58 */
	SVR4_SIGRTMIN + 26,	/* 59 */
	SVR4_SIGRTMIN + 27,	/* 60 */
	SVR4_SIGRTMIN + 28,	/* 61 */
	SVR4_SIGRTMIN + 29,	/* 62 */
	SVR4_SIGRTMIN + 30,	/* 63 */
};

const int svr4_to_native_signo[SVR4_NSIG] = {
	0,			/* 0 */
	SIGHUP,			/* 1 */
	SIGINT,			/* 2 */
	SIGQUIT,		/* 3 */
	SIGILL,			/* 4 */
	SIGTRAP,		/* 5 */
	SIGABRT,		/* 6 */
	SIGEMT,			/* 7 */
	SIGFPE,			/* 8 */
	SIGKILL,		/* 9 */
	SIGBUS,			/* 10 */
	SIGSEGV,		/* 11 */
	SIGSYS,			/* 12 */
	SIGPIPE,		/* 13 */
	SIGALRM,		/* 14 */
	SIGTERM,		/* 15 */
	SIGUSR1,		/* 16 */
	SIGUSR2,		/* 17 */
	SIGCHLD,		/* 18 */
	SIGPWR,			/* 19 */
	SIGWINCH,		/* 20 */
	SIGURG,			/* 21 */
	SIGIO,			/* 22 */
	SIGSTOP,		/* 23 */
	SIGTSTP,		/* 24 */
	SIGCONT,		/* 25 */
	SIGTTIN,		/* 26 */
	SIGTTOU,		/* 27 */
	SIGVTALRM,		/* 28 */
	SIGPROF,		/* 29 */
	SIGXCPU,		/* 30 */
	SIGXFSZ,		/* 31 */
	SIGRTMIN + 0,		/* 32 */
	SIGRTMIN + 1,		/* 33 */
	SIGRTMIN + 2,		/* 34 */
	SIGRTMIN + 3,		/* 35 */
	SIGRTMIN + 4,		/* 36 */
	SIGRTMIN + 5,		/* 37 */
	SIGRTMIN + 6,		/* 38 */
	SIGRTMIN + 7,		/* 39 */
	SIGRTMIN + 8,		/* 40 */
	SIGRTMIN + 9,		/* 41 */
	SIGRTMIN + 10,		/* 42 */
	SIGRTMIN + 11,		/* 43 */
	SIGRTMIN + 12,		/* 44 */
	SIGRTMIN + 13,		/* 45 */
	SIGRTMIN + 14,		/* 46 */
	SIGRTMIN + 15,		/* 47 */
	SIGRTMIN + 16,		/* 48 */
	SIGRTMIN + 17,		/* 49 */
	SIGRTMIN + 18,		/* 50 */
	SIGRTMIN + 19,		/* 51 */
	SIGRTMIN + 20,		/* 52 */
	SIGRTMIN + 21,		/* 53 */
	SIGRTMIN + 22,		/* 54 */
	SIGRTMIN + 23,		/* 55 */
	SIGRTMIN + 24,		/* 56 */
	SIGRTMIN + 25,		/* 57 */
	SIGRTMIN + 26,		/* 58 */
	SIGRTMIN + 27,		/* 59 */
	SIGRTMIN + 28,		/* 60 */
	SIGRTMIN + 29,		/* 61 */
	SIGRTMIN + 30,		/* 62 */
	0,			/* 63 */
};
