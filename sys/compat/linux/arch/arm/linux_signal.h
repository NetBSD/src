/* 	$NetBSD: linux_signal.h,v 1.2.2.3 2002/04/01 07:44:07 nathanw Exp $	*/

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

#ifndef _ARM_LINUX_SIGNAL_H
#define _ARM_LINUX_SIGNAL_H

#define LINUX_SIGHUP	 1
#define LINUX_SIGINT	 2
#define LINUX_SIGQUIT	 3
#define LINUX_SIGILL	 4
#define LINUX_SIGTRAP	 5
#define LINUX_SIGABRT	 6
#define LINUX_SIGIOT	 6
#define LINUX_SIGBUS	 7
#define LINUX_SIGFPE	 8
#define LINUX_SIGKILL	 9
#define LINUX_SIGUSR1	10
#define LINUX_SIGSEGV	11
#define LINUX_SIGUSR2	12
#define LINUX_SIGPIPE	13
#define LINUX_SIGALRM	14
#define LINUX_SIGTERM	15
#define LINUX_SIGSTKFLT	16
#define LINUX_SIGCHLD	17
#define LINUX_SIGCONT	18
#define LINUX_SIGSTOP	19
#define LINUX_SIGTSTP	20
#define LINUX_SIGTTIN	21
#define LINUX_SIGTTOU	22
#define LINUX_SIGURG	23
#define LINUX_SIGXCPU	24
#define LINUX_SIGXFSZ	25
#define LINUX_SIGVTALRM	26
#define LINUX_SIGPROF	27
#define LINUX_SIGWINCH	28
#define LINUX_SIGIO	29
#define LINUX_SIGPWR	30
#define LINUX_SIGSYS	31
#define LINUX_SIGUNUSED	31
#define LINUX_NSIG	32

/* Min/max real-time linux signal */
#define LINUX_SIGRTMIN		32
#define LINUX_SIGRTMAX		(LINUX__NSIG - 1)

#define LINUX__NSIG 		64
#define LINUX__NSIG_BPW		32
#define LINUX__NSIG_WORDS	(LINUX__NSIG / LINUX__NSIG_BPW)

#define LINUX_SIG_BLOCK		0
#define LINUX_SIG_UNBLOCK	1
#define LINUX_SIG_SETMASK	2

/* sa_flags */
#define LINUX_SA_NOCLDSTOP	0x00000001
#define LINUX_SA_NOCLDWAIT	0x00000002
#define LINUX_SA_SIGINFO	0x00000004
#define LINUX_SA_ONSTACK	0x08000000
#define LINUX_SA_RESTART	0x10000000
#define LINUX_SA_INTERRUPT	0x20000000
#define LINUX_SA_NOMASK		0x40000000
#define LINUX_SA_ONESHOT	0x80000000
#define LINUX_SA_ALLBITS	0xf8000001

typedef void	(*linux_handler_t) __P((int));

typedef u_long	linux_old_sigset_t;
typedef struct {
	u_long sig[LINUX__NSIG_WORDS];
} linux_sigset_t;

struct linux_old_sigaction {
	linux_handler_t		sa_handler;
	linux_old_sigset_t	sa_mask;
	u_long			sa_flags;
	void			(*sa_restorer) __P((void));
};

/* Used in rt_* calls */
struct linux_sigaction {
	linux_handler_t		sa_handler;
	u_long			sa_flags;
	void			(*sa_restorer) __P((void));
	linux_sigset_t		sa_mask;
};

struct linux_k_sigaction {
	struct linux_sigaction sa;
#define k_sa_restorer	sa.sa_restorer
};

#define	LINUX_SS_ONSTACK	1
#define	LINUX_SS_DISABLE	2

#define	LINUX_MINSIGSTKSZ	2048
#define	LINUX_SIGSTKSZ		8192

struct linux_sigaltstack {
	void *ss_sp;
	int ss_flags;
	size_t ss_size;
};

#endif /* !_ARM_LINUX_SIGNAL_H */
