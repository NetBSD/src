/*	$NetBSD: linux_sigarray.c,v 1.2.6.4 2002/04/17 00:05:03 nathanw Exp $ */

/*-
 * Copyright (c) 1995, 1998, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Frank van der Linden, Eric Haszlakiewicz and Emmanuel Dreyfus.
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

__KERNEL_RCSID(1, "$NetBSD: linux_sigarray.c,v 1.2.6.4 2002/04/17 00:05:03 nathanw Exp $");

/* 
 * From Linux's include/asm-mips/signal.h 
 */
const int linux_to_native_signo[LINUX__NSIG] = {
	0,				/* 0 */
	SIGHUP,
	SIGINT,
	SIGQUIT,
	SIGILL,
	SIGTRAP,		/* 5 */
	SIGABRT,
	SIGEMT, 
	SIGFPE,
	SIGKILL,
	SIGBUS,		/* 10 */
	SIGSEGV,
	SIGSYS,
	SIGPIPE,
	SIGALRM,
	SIGTERM,		/* 15 */
	SIGUSR1,
	SIGUSR2,
	SIGCHLD,
	SIGPWR,
	SIGWINCH,	/* 20 */
	SIGURG,
	SIGIO,
	SIGSTOP,
	SIGTSTP,
	SIGCONT,		/* 25 */
	SIGTTIN,
	SIGTTOU,
	SIGVTALRM,
	SIGPROF,
	SIGXCPU,		/* 30 */
	SIGXFSZ,
	0,
	0,
	0,
	0,				/* 35 */
	0,
	0,
	0,
	0,
	0,				/* 40 */
	0,
	0,
	0,
	0,
	0,				/* 45 */
	0,
	0,
	0,
	0,
	0,				/* 50 */
	0,
	0,
	0,
	0,
	0,				/* 55 */
	0,
	0,
	0,
	0,
	0,				/* 60 */
	0,
	0,
	0,
};
