/*	$NetBSD: linux_sigarray.c,v 1.18.2.2 2002/04/01 07:44:10 nathanw Exp $	*/

/*-
 * Copyright (c) 1995, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Frank van der Linden and Eric Haszlakiewicz.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: linux_sigarray.c,v 1.18.2.2 2002/04/01 07:44:10 nathanw Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/signal.h>

#include <compat/linux/common/linux_signal.h>

int const linux_to_native_sig[LINUX__NSIG] = {
	0,		/* 0 */
	SIGHUP,		/* 1 */
	SIGINT,		/* 2 */
	SIGQUIT,	/* 3 */
	SIGILL,		/* 4 */
	SIGTRAP,	/* 5 */
	SIGABRT,	/* 6 */
	SIGBUS,		/* 7 */
	SIGFPE,		/* 8 */
	SIGKILL,	/* 9 */
	SIGUSR1,	/* 10 */
	SIGSEGV,	/* 11 */
	SIGUSR2,	/* 12 */
	SIGPIPE,	/* 13 */
	SIGALRM,	/* 14 */
	SIGTERM,	/* 15 */
	0,		/* 16 SIGSTKFLT */
	SIGCHLD,	/* 17 */
	SIGCONT,	/* 18 */
	SIGSTOP,	/* 19 */
	SIGTSTP,	/* 20 */
	SIGTTIN,	/* 21 */
	SIGTTOU,	/* 22 */
	SIGURG,		/* 23 */
	SIGXCPU,	/* 24 */
	SIGXFSZ,	/* 25 */
	SIGVTALRM,	/* 26 */
	SIGPROF,	/* 27 */
	SIGWINCH,	/* 28 */
	SIGIO,		/* 29 */
	SIGPWR,		/* 30 */
	SIGSYS,		/* 31 */
	SIGRTMIN + 0,	/* 32 */
	SIGRTMIN + 1,	/* 33 */
	SIGRTMIN + 2,	/* 34 */
	SIGRTMIN + 3,	/* 35 */
	SIGRTMIN + 4,	/* 36 */
	SIGRTMIN + 5,	/* 37 */
	SIGRTMIN + 6,	/* 38 */
	SIGRTMIN + 7,	/* 39 */
	SIGRTMIN + 8,	/* 40 */
	SIGRTMIN + 9,	/* 41 */
	SIGRTMIN + 10,	/* 42 */
	SIGRTMIN + 11,	/* 43 */
	SIGRTMIN + 12,	/* 44 */
	SIGRTMIN + 13,	/* 45 */
	SIGRTMIN + 14,	/* 46 */
	SIGRTMIN + 15,	/* 47 */
	SIGRTMIN + 16,	/* 48 */
	SIGRTMIN + 17,	/* 49 */
	SIGRTMIN + 18,	/* 50 */
	SIGRTMIN + 19,	/* 51 */
	SIGRTMIN + 20,	/* 52 */
	SIGRTMIN + 21,	/* 53 */
	SIGRTMIN + 22,	/* 54 */
	SIGRTMIN + 23,	/* 55 */
	SIGRTMIN + 24,	/* 56 */
	SIGRTMIN + 25,	/* 57 */
	SIGRTMIN + 26,	/* 58 */
	SIGRTMIN + 27,	/* 59 */
	SIGRTMIN + 28,	/* 60 */
	SIGRTMIN + 29,	/* 61 */
	SIGRTMIN + 30,	/* 62 */
	0		/* 63 */
};
