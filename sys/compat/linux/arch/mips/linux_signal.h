/* $NetBSD: linux_signal.h,v 1.3.6.3 2001/09/26 19:54:47 nathanw Exp $ */

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

#ifndef _MIPS_LINUX_SIGNAL_H
#define _MIPS_LINUX_SIGNAL_H

/*
 * From Linux's include/asm-mips/ptrace.h 
 */
struct linux_pt_regs {
	unsigned long lpad0[6];
	unsigned long lregs[32];
	unsigned long llo;
	unsigned long lhi;
	unsigned long lcp0_spc;
	unsigned long lcp0_badvaddr;
	unsigned long lcp0_status;
	unsigned long lcp0_cause;
};

/* 
 * Everything is from Linux's include/asm-mips/signal.h 
 */
#define LINUX_SIGHUP	 	1
#define LINUX_SIGINT		2
#define LINUX_SIGQUIT		3
#define LINUX_SIGILL		4
#define LINUX_SIGTRAP		5
#define LINUX_SIGABRT		6
#define LINUX_SIGIOT		6
#define LINUX_SIGEMT		7
#define LINUX_SIGFPE		8
#define LINUX_SIGKILL		9
#define LINUX_SIGBUS		10
#define LINUX_SIGSEGV		11
#define LINUX_SIGSYS		12
#define LINUX_SIGPIPE		13
#define LINUX_SIGALRM		14
#define LINUX_SIGTERM		15
#define LINUX_SIGUSR1		16
#define LINUX_SIGUSR2		17
#define LINUX_SIGCHLD		18
#define LINUX_SIGCLD		18
#define LINUX_SIGPWR		19
#define LINUX_SIGWINCH		20
#define LINUX_SIGURG		21
#define LINUX_SIGIO		22
#define LINUX_SIGPOLL		22
#define LINUX_SIGSTOP		23
#define LINUX_SIGTSTP		24
#define LINUX_SIGCONT		25
#define LINUX_SIGTTIN		26
#define LINUX_SIGTTOU		27
#define LINUX_SIGVTALRM		28
#define LINUX_SIGPROF		29
#define LINUX_SIGXCPU		30
#define LINUX_SIGXFSZ		31

#define LINUX_SIGRTMIN		32

#define LINUX__NSIG		128
#if defined(ELFSIZE) && (ELFSIZE == 64)
#define LINUX__NSIG_BPW		64
#else
#define LINUX__NSIG_BPW		32
#endif
#define LINUX__NSIG_WORDS (LINUX__NSIG / LINUX__NSIG_BPW)

#define LINUX_SIG_BLOCK		1
#define LINUX_SIG_UNBLOCK	2
#define LINUX_SIG_SETMASK	3

/* sa_flags */
#define LINUX_SA_NOCLDSTOP	0x00000001
#define LINUX_SA_SIGINFO	0x00000008
#define LINUX_SA_ONSTACK	0x08000000
#define LINUX_SA_RESTART	0x10000000
#define LINUX_SA_INTERRUPT	0x20000000
#define LINUX_SA_NODEFER	0x40000000
#define LINUX_SA_RESETHAND	0x80000000
#define LINUX_SA_NOMASK		LINUX_SA_NODEFER
#define LINUX_SA_ONESHOT	LINUX_SA_RESETHAND
#define LINUX_SA_ALLBITS	0xf8000001 /* XXX from i386, not in mips. */

typedef void (*linux___sighandler_t) __P((int));

typedef unsigned long linux_old_sigset_t;
typedef struct {
	unsigned long sig[LINUX__NSIG_WORDS];
} linux_sigset_t;

/* Used in rt_* calls. No old_sigaction is defined for MIPS */
struct linux_sigaction {
	unsigned int		sa_flags;
	linux___sighandler_t	sa_handler;
	linux_sigset_t		sa_mask;
	void			(*sa_restorer) __P((void));
	int			sa_resv[1];
};

struct linux_k_sigaction {
	struct linux_sigaction sa;
};

struct linux_old_sigaction {
	unsigned int		sa_flags;
	linux___sighandler_t	sa_handler;
	linux_old_sigset_t	sa_mask;
	void			(*sa_restorer) __P((void));
	int			sa_resv[1];
};

#define	LINUX_SS_ONSTACK	1
#define	LINUX_SS_DISABLE	2

#define	LINUX_MINSIGSTKSZ	2048
#define	LINUX_SIGSTKSZ		8192

struct linux_sigaltstack {
	void *ss_sp;
	size_t ss_size;
	int ss_flags;
};
typedef struct linux_sigaltstack linux_stack_t; /* XXX really needed ? */

#endif /* !_MIPS_LINUX_SIGNAL_H */
