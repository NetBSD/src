/*	$NetBSD: rumpclient.h,v 1.10.4.1 2012/04/17 00:05:33 yamt Exp $	*/

/*-
 * Copyright (c) 2010 Antti Kantee.  All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _RUMP_RUMPCLIENT_H_
#define _RUMP_RUMPCLIENT_H_

#include <sys/types.h>
#include <sys/null.h>

struct rumpclient_fork;

#define rumpclient_vfork() rumpclient__dofork(vfork)

__BEGIN_DECLS

int rumpclient_syscall(int, const void *, size_t, register_t *);
int rumpclient_init(void);

struct rumpclient_fork *rumpclient_prefork(void);
int			rumpclient_fork_init(struct rumpclient_fork *);
void			rumpclient_fork_cancel(struct rumpclient_fork *);
void			rumpclient_fork_vparent(struct rumpclient_fork *);

pid_t rumpclient_fork(void);
int rumpclient_exec(const char *, char *const [], char *const[]);
int rumpclient_daemon(int, int);

#define RUMPCLIENT_RETRYCONN_INFTIME ((time_t)-1)
#define RUMPCLIENT_RETRYCONN_ONCE ((time_t)-2)
#define RUMPCLIENT_RETRYCONN_DIE ((time_t)-3)
void rumpclient_setconnretry(time_t);

enum rumpclient_closevariant {
	RUMPCLIENT_CLOSE_CLOSE,
	RUMPCLIENT_CLOSE_DUP2,
	RUMPCLIENT_CLOSE_FCLOSEM
};
int rumpclient__closenotify(int *, enum rumpclient_closevariant);

/*
 * vfork needs to be implemented as an inline to make everything
 * run in the caller's stackframe.
 */
static __attribute__((__always_inline__)) __returns_twice inline pid_t
rumpclient__dofork(pid_t (*forkfn)(void))
{
	struct rumpclient_fork *rf;
	pid_t pid;
	int childran = 0;

	if ((rf = rumpclient_prefork()) == NULL)
		return -1;
                
	switch ((pid = forkfn())) {
	case -1:
		rumpclient_fork_cancel(rf);
		break;
	case 0:
		childran = 1;
		if (rumpclient_fork_init(rf) == -1)
			pid = -1;
		break;
	default:
		/* XXX: multithreaded vforker?  do they exist? */
		if (childran)
			rumpclient_fork_vparent(rf);
		break;
	}

	return pid;
}

__END_DECLS

#endif /* _RUMP_RUMPCLIENT_H_ */
