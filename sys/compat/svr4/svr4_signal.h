/* $NetBSD: svr4_signal.h,v 1.1 1994/10/24 17:37:45 deraadt Exp $	*/
/*
 * Copyright (c) 1994 Christos Zoulas
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef	_SVR4_SIGNAL_H_
#define	_SVR4_SIGNAL_H_

#define	SVR4_SIGHUP	 1
#define	SVR4_SIGINT	 2
#define	SVR4_SIGQUIT	 3
#define	SVR4_SIGILL	 4
#define	SVR4_SIGTRAP	 5
#define	SVR4_SIGIOT	 6
#define	SVR4_SIGABRT	 6
#define	SVR4_SIGEMT	 7
#define	SVR4_SIGFPE	 8
#define	SVR4_SIGKILL	 9
#define	SVR4_SIGBUS	10
#define	SVR4_SIGSEGV	11
#define	SVR4_SIGSYS	12
#define	SVR4_SIGPIPE	13
#define	SVR4_SIGALRM	14
#define	SVR4_SIGTERM	15
#define	SVR4_SIGUSR1	16
#define	SVR4_SIGUSR2	17
#define	SVR4_SIGCLD	18
#define	SVR4_SIGCHLD	18
#define	SVR4_SIGPWR	19
#define	SVR4_SIGWINCH	20
#define	SVR4_SIGURG	21
#define	SVR4_SIGPOLL	22
#define	SVR4_SIGIO	22
#define	SVR4_SIGSTOP	23
#define	SVR4_SIGTSTP	24
#define	SVR4_SIGCONT	25
#define	SVR4_SIGTTIN	26
#define	SVR4_SIGTTOU	27
#define	SVR4_SIGVTALRM	28
#define	SVR4_SIGPROF	29
#define	SVR4_SIGXCPU	30
#define	SVR4_SIGXFSZ	31

#define	SVR4_SIGNO_MASK		0x00FF
#define	SVR4_SIGNAL_MASK	0x0000
#define	SVR4_SIGDEFER_MASK	0x0100
#define	SVR4_SIGHOLD_MASK	0x0200
#define	SVR4_SIGRELSE_MASK	0x0400
#define	SVR4_SIGIGNORE_MASK	0x0800
#define	SVR4_SIGPAUSE_MASK	0x1000

#define	SVR4_SIG_DFL	(void(*)())	 0
#define	SVR4_SIG_ERR	(void(*)())	-1
#define	SVR4_SIG_IGN	(void (*)())	 1
#define	SVR4_SIG_HOLD	(void(*)())	 2


#define SVR4_SIGNO(a)	((a) & SVR4_SIGNO_MASK)
#define SVR4_SIGCALL(a) ((a) & ~SVR4_SIGNO_MASK)

#endif /* !_SVR4_SIGNAL_H_ */
