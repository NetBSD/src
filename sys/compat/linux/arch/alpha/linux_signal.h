/* 	$NetBSD: linux_signal.h,v 1.9.64.1 2015/12/27 12:09:46 skrll Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Eric Haszlakiewicz.
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

/*
 * Whoever decided how linux would handle
 * binary compatibility should be shot.
 */

#ifndef _ALPHA_LINUX_SIGNAL_H
#define _ALPHA_LINUX_SIGNAL_H

#define LINUX_SIGHUP	 1
#define LINUX_SIGINT	 2
#define LINUX_SIGQUIT	 3
#define LINUX_SIGILL	 4
#define LINUX_SIGTRAP	 5
#define LINUX_SIGABRT	 6
#define LINUX_SIGEMT	 7
#define LINUX_SIGFPE	 8
#define LINUX_SIGKILL	 9
#define LINUX_SIGBUS	10
#define LINUX_SIGSEGV	11
#define LINUX_SIGSYS	12
#define LINUX_SIGPIPE	13
#define LINUX_SIGALRM	14
#define LINUX_SIGTERM	15
#define LINUX_SIGURG	16
#define LINUX_SIGSTOP	17
#define LINUX_SIGTSTP	18
#define LINUX_SIGCONT	19
#define LINUX_SIGCHLD	20
#define LINUX_SIGTTIN	21
#define LINUX_SIGTTOU	22
#define LINUX_SIGIO	23
#define LINUX_SIGXCPU	24
#define LINUX_SIGXFSZ	25
#define LINUX_SIGVTALRM	26
#define LINUX_SIGPROF	27
#define LINUX_SIGWINCH	28
#define LINUX_SIGINFO	29
#define LINUX_SIGUSR1	30
#define LINUX_SIGUSR2	31

#define LINUX_SIGIOT	LINUX_SIGABRT
#define LINUX_SIGPWR	LINUX_SIGINFO
#define LINUX_SIGPOLL	LINUX_SIGIO

/* Min/max real-time linux signal */
#define LINUX_SIGRTMIN		32
#define LINUX_SIGRTMAX		(LINUX__NSIG - 1)

#define LINUX__NSIG		64
#define LINUX__NSIG_BPW		64
#define LINUX__NSIG_WORDS	(LINUX__NSIG / LINUX__NSIG_BPW)
#define LINUX_NSIG		32

#define	LINUX_MINSIGSTKSZ	4096

/* sa_flags */
#define LINUX_SA_ONSTACK	0x00000001
#define LINUX_SA_RESTART	0x00000002
#define LINUX_SA_NOCLDSTOP	0x00000004
#define LINUX_SA_NODEFER	0x00000008
#define LINUX_SA_RESETHAND	0x00000010
#define LINUX_SA_NOCLDWAIT	0x00000020
#define LINUX_SA_SIGINFO	0x00000040

#define LINUX_SA_NOMASK		LINUX_SA_NODEFER
#define LINUX_SA_ONESHOT	LINUX_SA_RESETHAND
#define LINUX_SA_INTERRUPT	0x20000000	/* Ignore this */

#define LINUX_SA_ALLBITS	0x2000007f

#define LINUX_SIG_BLOCK		1
#define LINUX_SIG_UNBLOCK	2
#define LINUX_SIG_SETMASK	3

typedef void	(*linux_handler_t)(int);

typedef u_long	linux_old_sigset_t;
typedef struct {
	u_long sig[LINUX__NSIG_WORDS];
} linux_sigset_t;

/* aka osf_sigaction in Linux sources.  Note absence of sa_restorer */
struct linux_old_sigaction {
	linux_handler_t		linux_sa_handler;
	linux_old_sigset_t	linux_sa_mask;
	int			linux_sa_flags;
};

/* Used in rt_* calls */
struct linux_sigaction {
	linux_handler_t		linux_sa_handler;
	u_long			linux_sa_flags;
	linux_sigset_t		linux_sa_mask;
};

struct linux_k_sigaction {
	struct linux_sigaction sa;
	void		(*k_sa_restorer)(void);
};

typedef struct linux_sigaltstack {
	void *ss_sp;
	int ss_flags;
	size_t ss_size;
} linux_stack_t;

#endif /* !_ALPHA_LINUX_SIGNAL_H */
