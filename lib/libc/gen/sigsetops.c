/*-
 * Copyright (c) 1989 The Regents of the University of California.
 * All rights reserved.
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
 */

#if defined(LIBC_SCCS) && !defined(lint)
/*static char *sccsid = "from: @(#)sigsetops.c	5.3 (Berkeley) 2/23/91";*/
static char *rcsid = "$Id: sigsetops.c,v 1.7.4.2 1995/05/02 19:35:13 jtc Exp $";
#endif /* LIBC_SCCS and not lint */

#include <signal.h>
#include <errno.h>

#undef sigemptyset
#undef sigfillset
#undef sigaddset
#undef sigdelset
#undef sigismember

int
sigemptyset(set)
	sigset_t *set;
{
	*set = 0;
	return (0);
}
#pragma weak sigemptyset=_sigemptyset

int
sigfillset(set)
	sigset_t *set;
{
	*set = ~(sigset_t)0;
	return (0);
}
#pragma weak sigfillset=_sigfillset

int
sigaddset(set, signo)
	sigset_t *set;
	int signo;
{
	if (signo <= 0 || signo >= NSIG) {
		errno = EINVAL;
		return -1;
	}
	*set |= sigmask(signo);
	return (0);
}
#pragma weak sigaddset=_sigaddset

int
sigdelset(set, signo)
	sigset_t *set;
	int signo;
{
	if (signo <= 0 || signo >= NSIG) {
		errno = EINVAL;
		return -1;
	}
	*set &= ~sigmask(signo);
	return (0);
}
#pragma weak sigdelset=_sigdelset

int
sigismember(set, signo)
	const sigset_t *set;
	int signo;
{
	if (signo <= 0 || signo >= NSIG) {
		errno = EINVAL;
		return -1;
	}
	return ((*set & sigmask(signo)) != 0);
}
#pragma weak sigismember=_sigismember
