/*	$NetBSD: signal.h,v 1.42 1998/12/21 10:35:00 drochner Exp $	*/

/*
 * Copyright (c) 1982, 1986, 1989, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)signal.h	8.4 (Berkeley) 5/4/95
 */

#ifndef	_SYS_SIGNAL_H_
#define	_SYS_SIGNAL_H_

#include <sys/featuretest.h>

#define _NSIG		33

#if !defined(_ANSI_SOURCE) && !defined(_POSIX_C_SOURCE) && \
    !defined(_XOPEN_SOURCE)
#define NSIG _NSIG

#if defined(__LIBC12_SOURCE__) || defined(_KERNEL)
/* Number of signals supported by old style signal mask. */
#define	NSIG13		32
#endif

#endif /* !_ANSI_SOURCE && !_POSIX_C_SOURCE && !_XOPEN_SOURCE */

#define	SIGHUP		1	/* hangup */
#define	SIGINT		2	/* interrupt */
#define	SIGQUIT		3	/* quit */
#define	SIGILL		4	/* illegal instruction (not reset when caught) */
#ifndef _POSIX_SOURCE
#define	SIGTRAP		5	/* trace trap (not reset when caught) */
#endif
#define	SIGABRT		6	/* abort() */
#ifndef _POSIX_SOURCE
#define	SIGIOT		SIGABRT	/* compatibility */
#define	SIGEMT		7	/* EMT instruction */
#endif
#define	SIGFPE		8	/* floating point exception */
#define	SIGKILL		9	/* kill (cannot be caught or ignored) */
#ifndef _POSIX_SOURCE
#define	SIGBUS		10	/* bus error */
#endif
#define	SIGSEGV		11	/* segmentation violation */
#ifndef _POSIX_SOURCE
#define	SIGSYS		12	/* bad argument to system call */
#endif
#define	SIGPIPE		13	/* write on a pipe with no one to read it */
#define	SIGALRM		14	/* alarm clock */
#define	SIGTERM		15	/* software termination signal from kill */
#ifndef _POSIX_SOURCE
#define	SIGURG		16	/* urgent condition on IO channel */
#endif
#define	SIGSTOP		17	/* sendable stop signal not from tty */
#define	SIGTSTP		18	/* stop signal from tty */
#define	SIGCONT		19	/* continue a stopped process */
#define	SIGCHLD		20	/* to parent on child stop or exit */
#define	SIGTTIN		21	/* to readers pgrp upon background tty read */
#define	SIGTTOU		22	/* like TTIN for output if (tp->t_local&LTOSTOP) */
#ifndef _POSIX_SOURCE
#define	SIGIO		23	/* input/output possible signal */
#define	SIGXCPU		24	/* exceeded CPU time limit */
#define	SIGXFSZ		25	/* exceeded file size limit */
#define	SIGVTALRM	26	/* virtual time alarm */
#define	SIGPROF		27	/* profiling time alarm */
#define SIGWINCH	28	/* window size changes */
#define SIGINFO		29	/* information request */
#endif
#define SIGUSR1		30	/* user defined signal 1 */
#define SIGUSR2		31	/* user defined signal 2 */
#ifndef _POSIX_SOURCE
#define	SIGPWR		32	/* power fail/restart (not reset when caught) */
#endif

#ifndef _KERNEL
#include <sys/cdefs.h>
#endif

#define	SIG_DFL		((void (*) __P((int)))  0)
#define	SIG_IGN		((void (*) __P((int)))  1)
#define	SIG_ERR		((void (*) __P((int))) -1)

#ifndef _ANSI_SOURCE
#if defined(__LIBC12_SOURCE__) || defined(_KERNEL)
typedef unsigned int sigset13_t;

/*
 * Macro for manipulating signal masks.
 */
#define __sigmask13(n)		(1 << ((n) - 1))
#define	__sigaddset13(s, n)	(*(s) |= __sigmask13(n))
#define	__sigdelset13(s, n)	(*(s) &= ~__sigmask13(n))
#define	__sigismember13(s, n)	(*(s) & __sigmask13(n))
#define	__sigemptyset13(s)	(*(s) = 0)
#define	__sigfillset13(s)	(*(s) = ~(sigset13_t)0)

/*
 * Signal vector "template" used in sigaction call.
 */
struct	sigaction13 {
	void	(*sa_handler)		/* signal handler */
			    __P((int));
	sigset13_t sa_mask;		/* signal mask to apply */
	int	sa_flags;		/* see signal options below */
};
#endif

typedef struct {
	u_int32_t	__bits[4];
} sigset_t;

/*
 * Macro for manipulating signal masks.
 */
#define __sigmask(n)		(1 << (((unsigned int)(n) - 1) & 31))
#define	__sigword(n)		(((unsigned int)(n) - 1) >> 5)
#define	__sigaddset(s, n)	((s)->__bits[__sigword(n)] |= __sigmask(n))
#define	__sigdelset(s, n)	((s)->__bits[__sigword(n)] &= ~__sigmask(n))
#define	__sigismember(s, n)	(((s)->__bits[__sigword(n)] & __sigmask(n)) != 0)
#define	__sigemptyset(s)	((s)->__bits[0] = 0x00000000, \
				 (s)->__bits[1] = 0x00000000, \
				 (s)->__bits[2] = 0x00000000, \
				 (s)->__bits[3] = 0x00000000)
#define	__sigfillset(s)		((s)->__bits[0] = 0xffffffff, \
				 (s)->__bits[1] = 0xffffffff, \
				 (s)->__bits[2] = 0xffffffff, \
				 (s)->__bits[3] = 0xffffffff)

#ifdef _KERNEL
#define	sigaddset(s, n)		__sigaddset(s, n)
#define	sigdelset(s, n)		__sigdelset(s, n)
#define	sigismember(s, n)	__sigismember(s, n)
#define	sigemptyset(s)		__sigemptyset(s)
#define	sigfillset(s)		__sigfillset(s)
#define	sigplusset(s, t) \
	do {						\
		(t)->__bits[0] |= (s)->__bits[0];	\
		(t)->__bits[1] |= (s)->__bits[1];	\
		(t)->__bits[2] |= (s)->__bits[2];	\
		(t)->__bits[3] |= (s)->__bits[3];	\
	} while (0)
#define	sigminusset(s, t) \
	do {						\
		(t)->__bits[0] &= ~(s)->__bits[0];	\
		(t)->__bits[1] &= ~(s)->__bits[1];	\
		(t)->__bits[2] &= ~(s)->__bits[2];	\
		(t)->__bits[3] &= ~(s)->__bits[3];	\
	} while (0)
#endif /* _KERNEL */

/*
 * Signal vector "template" used in sigaction call.
 */
struct	sigaction {
	void	(*sa_handler)		/* signal handler */
			    __P((int));
	sigset_t sa_mask;		/* signal mask to apply */
	int	sa_flags;		/* see signal options below */
};

#if (!defined(_POSIX_C_SOURCE) && !defined(_XOPEN_SOURCE)) || \
    (defined(_XOPEN_SOURCE) && defined(_XOPEN_SOURCE_EXTENDED)) || \
    (_XOPEN_SOURCE - 0) >= 500
#define SA_ONSTACK	0x0001	/* take signal on signal stack */
#define SA_RESTART	0x0002	/* restart system on signal return */
#define SA_RESETHAND	0x0004	/* reset to SIG_DFL when taking signal */
#define SA_NODEFER	0x0010	/* don't mask the signal we're delivering */
#if defined(_KERNEL) && defined(COMPAT_LINUX)
#define SA_SIGINFO	0x0040
#endif /* (_KERNEL && linux) */
#endif /* (!_POSIX_C_SOURCE && !_XOPEN_SOURCE) || ... */
/* Only valid for SIGCHLD. */
#define SA_NOCLDSTOP	0x0008	/* do not generate SIGCHLD on child stop */
#define SA_NOCLDWAIT	0x0020	/* do not generate zombies on unwaited child */
#ifdef _KERNEL
#define	SA_ALLBITS	0x007f
#endif

/*
 * Flags for sigprocmask():
 */
#define	SIG_BLOCK	1	/* block specified signal set */
#define	SIG_UNBLOCK	2	/* unblock specified signal set */
#define	SIG_SETMASK	3	/* set specified signal set */

#if !defined(_POSIX_C_SOURCE) && !defined(_XOPEN_SOURCE)
typedef	void (*sig_t) __P((int));	/* type of signal function */
#endif

/*
 * Structure used in sigaltstack call.
 */
#if defined(__LIBC12_SOURCE__) || defined(_KERNEL)
struct sigaltstack13 {
	char	*ss_sp;			/* signal stack base */
	int	ss_size;		/* signal stack length */
	int	ss_flags;		/* SS_DISABLE and/or SS_ONSTACK */
};
#endif /* defined(__LIBC12_SOURCE__) || defined(_KERNEL) */

#if (!defined(_POSIX_C_SOURCE) && !defined(_XOPEN_SOURCE)) || \
    (defined(_XOPEN_SOURCE) && defined(_XOPEN_SOURCE_EXTENDED)) || \
    (_XOPEN_SOURCE - 0) >= 500
typedef struct
#ifndef _XOPEN_SOURCE
               sigaltstack
#endif /* !_XOPEN_SOURCE */
			   {
	void	*ss_sp;			/* signal stack base */
	size_t	ss_size;		/* signal stack length */
	int	ss_flags;		/* SS_DISABLE and/or SS_ONSTACK */
} stack_t;

#define SS_ONSTACK	0x0001	/* take signals on alternate stack */
#define SS_DISABLE	0x0004	/* disable taking signals on alternate stack */
#ifdef _KERNEL
#define	SS_ALLBITS	0x0005
#endif
#define	MINSIGSTKSZ	8192			/* minimum allowable stack */
#define	SIGSTKSZ	(MINSIGSTKSZ + 32768)	/* recommended stack size */
#endif /* (!_POSIX_C_SOURCE && !_XOPEN_SOURCE) || ... */

#if !defined(_POSIX_C_SOURCE) && !defined(_XOPEN_SOURCE)
/*
 * 4.3 compatibility:
 * Signal vector "template" used in sigvec call.
 */
struct	sigvec {
	void	(*sv_handler)		/* signal handler */
			    __P((int));
	int	sv_mask;		/* signal mask to apply */
	int	sv_flags;		/* see signal options below */
};
#define SV_ONSTACK	SA_ONSTACK
#define SV_INTERRUPT	SA_RESTART	/* same bit, opposite sense */
#define SV_RESETHAND	SA_RESETHAND
#define sv_onstack sv_flags	/* isn't compatibility wonderful! */
#endif /* !_POSIX_C_SOURCE && !_XOPEN_SOURCE */

#if (!defined(_POSIX_C_SOURCE) && !defined(_XOPEN_SOURCE)) || \
    (defined(_XOPEN_SOURCE) && defined(_XOPEN_SOURCE_EXTENDED)) || \
    (_XOPEN_SOURCE - 0) >= 500
/*
 * Structure used in sigstack call.
 */
struct	sigstack {
	void	*ss_sp;			/* signal stack pointer */
	int	ss_onstack;		/* current status */
};
#endif /* (!_POSIX_C_SOURCE && !_XOPEN_SOURCE) || ... */

#include <machine/signal.h>	/* sigcontext; codes for SIGILL, SIGFPE */

#if !defined(_POSIX_C_SOURCE) && !defined(_XOPEN_SOURCE) && !defined(_KERNEL)
/*
 * Macro for converting signal number to a mask suitable for
 * sigblock().
 */
#define sigmask(n)	__sigmask(n)

#define	BADSIG		SIG_ERR
#endif /* !_POSIX_C_SOURCE && !_XOPEN_SOURCE */

#endif	/* !_ANSI_SOURCE */

/*
 * For historical reasons; programs expect signal's return value to be
 * defined by <sys/signal.h>.
 */
__BEGIN_DECLS
void	(*signal __P((int, void (*) __P((int))))) __P((int));
__END_DECLS
#endif	/* !_SYS_SIGNAL_H_ */
