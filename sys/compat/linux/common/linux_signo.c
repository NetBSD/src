/*	$NetBSD: linux_signo.c,v 1.3.2.2 2002/04/17 00:05:12 nathanw Exp $	*/
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
__KERNEL_RCSID(0, "$NetBSD: linux_signo.c,v 1.3.2.2 2002/04/17 00:05:12 nathanw Exp $");

#include <sys/types.h>
#include <sys/signal.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>

#ifndef LINUX_SIGINFO
#define LINUX_SIGINFO	0
#endif

#ifndef LINUX_SIGEMT
#define LINUX_SIGEMT	0
#endif

/* Note: linux_to_native_signo[] is in <arch>/linux_sigarray.c */
#ifdef LINUX_SIGHUP
const int native_to_linux_signo[] = {
	0,			/* 0 */
	LINUX_SIGHUP,		/* 1 */
	LINUX_SIGINT,		/* 2 */
	LINUX_SIGQUIT,		/* 3 */
	LINUX_SIGILL,		/* 4 */
	LINUX_SIGTRAP,		/* 5 */
	LINUX_SIGABRT,		/* 6 */
	LINUX_SIGEMT,		/* 7 */
	LINUX_SIGFPE,		/* 8 */
	LINUX_SIGKILL,		/* 9 */
	LINUX_SIGBUS,		/* 10 */
	LINUX_SIGSEGV,		/* 11 */
	LINUX_SIGSYS,		/* 12 */
	LINUX_SIGPIPE,		/* 13 */
	LINUX_SIGALRM,		/* 14 */
	LINUX_SIGTERM,		/* 15 */
	LINUX_SIGURG,		/* 16 */
	LINUX_SIGSTOP,		/* 17 */
	LINUX_SIGTSTP,		/* 18 */
	LINUX_SIGCONT,		/* 19 */
	LINUX_SIGCHLD,		/* 20 */
	LINUX_SIGTTIN,		/* 21 */
	LINUX_SIGTTOU,		/* 22 */
	LINUX_SIGIO,		/* 23 */
	LINUX_SIGXCPU,		/* 24 */
	LINUX_SIGXFSZ,		/* 25 */
	LINUX_SIGVTALRM,	/* 26 */
	LINUX_SIGPROF,		/* 27 */
	LINUX_SIGWINCH,		/* 28 */
	LINUX_SIGINFO,		/* 29 */
	LINUX_SIGUSR1,		/* 30 */
	LINUX_SIGUSR2,		/* 31 */
	LINUX_SIGPWR,		/* 32 */
	LINUX_SIGRTMIN + 0,	/* 33 */
	LINUX_SIGRTMIN + 1,	/* 34 */
	LINUX_SIGRTMIN + 2,	/* 35 */
	LINUX_SIGRTMIN + 3,	/* 36 */
	LINUX_SIGRTMIN + 4,	/* 37 */
	LINUX_SIGRTMIN + 5,	/* 38 */
	LINUX_SIGRTMIN + 6,	/* 39 */
	LINUX_SIGRTMIN + 7,	/* 40 */
	LINUX_SIGRTMIN + 8,	/* 41 */
	LINUX_SIGRTMIN + 9,	/* 42 */
	LINUX_SIGRTMIN + 10,	/* 43 */
	LINUX_SIGRTMIN + 11,	/* 44 */
	LINUX_SIGRTMIN + 12,	/* 45 */
	LINUX_SIGRTMIN + 13,	/* 46 */
	LINUX_SIGRTMIN + 14,	/* 47 */
	LINUX_SIGRTMIN + 15,	/* 48 */
	LINUX_SIGRTMIN + 16,	/* 49 */
	LINUX_SIGRTMIN + 17,	/* 50 */
	LINUX_SIGRTMIN + 18,	/* 51 */
	LINUX_SIGRTMIN + 19,	/* 52 */
	LINUX_SIGRTMIN + 20,	/* 53 */
	LINUX_SIGRTMIN + 21,	/* 54 */
	LINUX_SIGRTMIN + 22,	/* 55 */
	LINUX_SIGRTMIN + 23,	/* 56 */
	LINUX_SIGRTMIN + 24,	/* 57 */
	LINUX_SIGRTMIN + 25,	/* 58 */
	LINUX_SIGRTMIN + 26,	/* 59 */
	LINUX_SIGRTMIN + 27,	/* 60 */
	LINUX_SIGRTMIN + 28,	/* 61 */
	LINUX_SIGRTMIN + 29,	/* 62 */
	LINUX_SIGRTMIN + 30,	/* 63 */
};
#else
const int native_to_linux_signo[NSIG];
#endif

#if defined(__i386__)
#include <compat/linux/arch/i386/linux_sigarray.c>
#elif defined(__m68k__)
#include <compat/linux/arch/m68k/linux_sigarray.c>
#elif defined(__alpha__)
#include <compat/linux/arch/alpha/linux_sigarray.c>
#elif defined(__powerpc__)
#include <compat/linux/arch/powerpc/linux_sigarray.c>
#elif defined(__mips__)
#include <compat/linux/arch/mips/linux_sigarray.c>
#elif defined(__arm__)
#include <compat/linux/arch/arm/linux_sigarray.c>
#else
const int linux_to_native_signo[NSIG];
#endif
