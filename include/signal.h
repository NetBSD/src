/*	$NetBSD: signal.h,v 1.31 2003/07/18 15:50:01 thorpej Exp $	*/

/*-
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	@(#)signal.h	8.3 (Berkeley) 3/30/94
 */

#ifndef _SIGNAL_H_
#define _SIGNAL_H_

#include <sys/cdefs.h>
#include <sys/featuretest.h>

#if defined(_POSIX_C_SOURCE) || defined(_XOPEN_SOURCE) || \
    defined(_NETBSD_SOURCE)
#include <sys/types.h>
#endif

#include <sys/signal.h>

#if defined(_NETBSD_SOURCE)
extern __const char *__const *sys_signame __RENAME(__sys_signame14);
#ifndef __SYS_SIGLIST_DECLARED
#define __SYS_SIGLIST_DECLARED
/* also in unistd.h */
extern __const char *__const *sys_siglist __RENAME(__sys_siglist14);
#endif /* __SYS_SIGLIST_DECLARED */
extern __const int sys_nsig __RENAME(__sys_nsig14);
#endif

__BEGIN_DECLS
int	raise __P((int));
#if defined(_POSIX_C_SOURCE) || defined(_XOPEN_SOURCE) || \
    defined(_NETBSD_SOURCE)
int	kill __P((pid_t, int));

int	pthread_sigmask __P((int, const sigset_t *, sigset_t *));
int	pthread_kill __P((pthread_t, int));

#ifdef __LIBC12_SOURCE__
int	sigaction __P((int, const struct sigaction13 *, struct sigaction13 *));
int	__sigaction14 __P((int, const struct sigaction *, struct sigaction *));
int	__libc_sigaction14 __P((int, const struct sigaction *,
	    struct sigaction *));
int	sigaddset __P((sigset13_t *, int));
int	__sigaddset14 __P((sigset_t *, int));
int	sigdelset __P((sigset13_t *, int));
int	__sigdelset14 __P((sigset_t *, int));
int	sigemptyset __P((sigset13_t *));
int	__sigemptyset14 __P((sigset_t *));
int	sigfillset __P((sigset13_t *));
int	__sigfillset14 __P((sigset_t *));
int	sigismember __P((const sigset13_t *, int));
int	__sigismember14 __P((const sigset_t *, int));
int	sigpending __P((sigset13_t *));
int	__sigpending14 __P((sigset_t *));
int	sigprocmask __P((int, const sigset13_t *, sigset13_t *));
int	__sigprocmask14 __P((int, const sigset_t *, sigset_t *));
int	sigsuspend __P((const sigset13_t *));
int	__sigsuspend14 __P((const sigset_t *));
#else /* !__LIBC12_SOURCE__ */
int	sigaction __P((int, const struct sigaction *, struct sigaction *)) __RENAME(__sigaction14);
int	sigaddset __P((sigset_t *, int)) __RENAME(__sigaddset14);
int	sigdelset __P((sigset_t *, int)) __RENAME(__sigdelset14);
int	sigemptyset __P((sigset_t *)) __RENAME(__sigemptyset14);
int	sigfillset __P((sigset_t *)) __RENAME(__sigfillset14);
int	sigismember __P((const sigset_t *, int)) __RENAME(__sigismember14);
int	sigpending __P((sigset_t *)) __RENAME(__sigpending14);
int	sigprocmask __P((int, const sigset_t *, sigset_t *)) __RENAME(__sigprocmask14);
int	sigsuspend __P((const sigset_t *)) __RENAME(__sigsuspend14);

#if defined(__GNUC__) && defined(__STDC__)
#ifndef errno
int *__errno __P((void));
#define errno (*__errno())
#endif
extern __inline int
sigaddset(sigset_t *set, int signo)
{
	if (signo <= 0 || signo >= _NSIG) {
		errno = 22;			/* EINVAL */
		return (-1);
	}
	__sigaddset(set, signo);
	return (0);
}

extern __inline int
sigdelset(sigset_t *set, int signo)
{
	if (signo <= 0 || signo >= _NSIG) {
		errno = 22;			/* EINVAL */
		return (-1);
	}
	__sigdelset(set, signo);
	return (0);
}

extern __inline int
sigismember(const sigset_t *set, int signo)
{
	if (signo <= 0 || signo >= _NSIG) {
		errno = 22;			/* EINVAL */
		return (-1);
	}
	return (__sigismember(set, signo));
}
#endif /* __GNUC__ && __STDC__ */

/* List definitions after function declarations, or Reiser cpp gets upset. */
#define	sigemptyset(set)	(__sigemptyset(set), /*LINTED*/0)
#define	sigfillset(set)		(__sigfillset(set), /*LINTED*/ 0)
#endif /* !__LIBC12_SOURCE__ */

/*
 * X/Open CAE Specification Issue 5 Version 2
 */      
#if (defined(_XOPEN_SOURCE) && defined(_XOPEN_SOURCE_EXTENDED)) || \
    (_XOPEN_SOURCE - 0) >= 500 || defined(_NETBSD_SOURCE)
int	killpg __P((pid_t, int));
int	siginterrupt __P((int, int));
int	sigstack __P((const struct sigstack *, struct sigstack *));
#ifdef __LIBC12_SOURCE__
int	sigaltstack __P((const struct sigaltstack13 *, struct sigaltstack13 *));
int	__sigaltstack14 __P((const stack_t *, stack_t *));
#else
int	sigaltstack __P((const stack_t *, stack_t *)) __RENAME(__sigaltstack14);
#endif
#endif /* _XOPEN_SOURCE_EXTENDED || _XOPEN_SOURCE >= 500 || _NETBSD_SOURCE */


/*
 * X/Open CAE Specification Issue 5 Version 2; IEEE Std 1003.1-2001 (POSIX)
 */      
#if (_POSIX_C_SOURCE - 0) >= 200112L || \
    (defined(_XOPEN_SOURCE) && defined(_XOPEN_SOURCE_EXTENDED)) || \
    (_XOPEN_SOURCE - 0) >= 500 || defined(_NETBSD_SOURCE)
int	sighold __P((int));
int	sigignore __P((int));
int	sigpause __P((int));
int	sigrelse __P((int));
void	(*sigset __P((int, void (*)(int)))) __P((int));
int	sigwait	__P((const sigset_t * __restrict, int * __restrict));
int	sigwaitinfo __P((const sigset_t * __restrict, siginfo_t * __restrict));

struct timespec;
int	sigtimedwait __P((const sigset_t * __restrict,
	    siginfo_t * __restrict, const struct timespec * __restrict));
int	__sigtimedwait __P((const sigset_t * __restrict,
	    siginfo_t * __restrict, struct timespec * __restrict));
#endif /* _POSIX_C_SOURCE >= 200112 || _XOPEN_SOURCE_EXTENDED || ... */


#if defined(_NETBSD_SOURCE)
#ifndef __PSIGNAL_DECLARED
#define __PSIGNAL_DECLARED
/* also in unistd.h */
void	psignal __P((unsigned int, const char *));
#endif /* __PSIGNAL_DECLARED */
int	sigblock __P((int));
#ifdef __LIBC12_SOURCE__
int	sigreturn __P((struct sigcontext13 *));
int	__sigreturn14 __P((struct sigcontext *));
#else
int	sigreturn __P((struct sigcontext *)) __RENAME(__sigreturn14);
#endif
int	sigsetmask __P((int));
int	sigvec __P((int, struct sigvec *, struct sigvec *));
#endif /* _NETBSD_SOURCE */

#endif	/* _POSIX_C_SOURCE || _XOPEN_SOURCE || _NETBSD_SOURCE */
__END_DECLS

#endif	/* !_SIGNAL_H_ */
