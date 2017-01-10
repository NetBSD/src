/*	$NetBSD: svc_run.c,v 1.28 2017/01/10 17:45:27 christos Exp $	*/

/*
 * Copyright (c) 2010, Oracle America, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials
 *       provided with the distribution.
 *     * Neither the name of the "Oracle America, Inc." nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *   FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *   COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 *   INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *   DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 *   GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char *sccsid = "@(#)svc_run.c 1.1 87/10/13 Copyr 1984 Sun Micro";
static char *sccsid = "@(#)svc_run.c	2.1 88/07/29 4.0 RPCSRC";
#else
__RCSID("$NetBSD: svc_run.c,v 1.28 2017/01/10 17:45:27 christos Exp $");
#endif
#endif

/*
 * This is the rpc server side idle loop
 * Wait for input, call server program.
 */
#include "namespace.h"
#include "reentrant.h"
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>

#include <rpc/rpc.h>

#include "svc_fdset.h"
#include "rpc_internal.h"

#ifdef __weak_alias
__weak_alias(svc_run,_svc_run)
__weak_alias(svc_exit,_svc_exit)
#endif

static void
svc_run_select(void)
{
	fd_set *readfds;
	struct timeval timeout;
	int *maxfd, fdsize;
#ifndef RUMP_RPC		
	int probs = 0;
#endif
#ifdef _REENTRANT
	extern rwlock_t svc_fd_lock;
#endif

	readfds = NULL;
	fdsize = 0;
	timeout.tv_sec = 30;
	timeout.tv_usec = 0;

	for (;;) {
		rwlock_rdlock(&svc_fd_lock);

		maxfd = svc_fdset_getmax();
		if (maxfd == NULL) {
			warn("%s: can't get maxfd", __func__);
			goto out;
		}

		if (fdsize != svc_fdset_getsize(0)) {
			fdsize = svc_fdset_getsize(0);
			free(readfds);
			readfds = svc_fdset_copy(svc_fdset_get());
			if (readfds == NULL) {
				warn("%s: can't copy fdset", __func__);
				goto out;
			}
		} else
			memcpy(readfds, svc_fdset_get(), __NFD_BYTES(fdsize));

		rwlock_unlock(&svc_fd_lock);

		switch (select(*maxfd + 1, readfds, NULL, NULL, &timeout)) {
		case -1:
#ifndef RUMP_RPC		
			if ((errno == EINTR || errno == EBADF) && probs < 100) {
				probs++;
				continue;
			}
#endif
			if (errno == EINTR) {
				continue;
			}
			warn("%s: select failed", __func__);
			goto out;
		case 0:
			__svc_clean_idle(NULL, 30, FALSE);
			continue;
		default:
			svc_getreqset2(readfds, fdsize);
#ifndef RUMP_RPC
			probs = 0;
#endif
		}
	}
out:
	free(readfds);
}

static void
svc_run_poll(void)
{
	struct pollfd *pfd;
	int *maxfd, fdsize, i;
#ifndef RUMP_RPC		
	int probs = 0;
#endif
#ifdef _REENTRANT
	extern rwlock_t svc_fd_lock;
#endif

	fdsize = 0;
	pfd = NULL;

	for (;;) {
		rwlock_rdlock(&svc_fd_lock);

		maxfd = svc_pollfd_getmax();
		if (maxfd == NULL) {
			warn("can't get maxfd");
			goto out;
		}

		if (pfd == NULL || fdsize != svc_pollfd_getsize(0)) {
			fdsize = svc_fdset_getsize(0);
			free(pfd);
			pfd = svc_pollfd_copy(svc_pollfd_get());
			if (pfd == NULL) {
				warn("can't get pollfd");
				goto out;
			}
		} else
			memcpy(pfd, svc_pollfd_get(), *maxfd * sizeof(*pfd));

		rwlock_unlock(&svc_fd_lock);

		switch ((i = poll(pfd, (nfds_t)*maxfd, 30 * 1000))) {
		case -1:
#ifndef RUMP_RPC		
			if ((errno == EINTR || errno == EBADF) && probs < 100) {
				probs++;
				continue;
			}
#endif
			if (errno == EINTR) {
				continue;
			}
			warn("%s: poll failed", __func__);
			goto out;
		case 0:
			__svc_clean_idle(NULL, 30, FALSE);
			continue;
		default:
			svc_getreq_poll(pfd, i);
#ifndef RUMP_RPC
			probs = 0;
#endif
		}
	}
out:
	free(pfd);
}

void
svc_run(void)
{
	(__svc_flags & SVC_FDSET_POLL) ? svc_run_poll() : svc_run_select();
}

/*
 *      This function causes svc_run() to exit by telling it that it has no
 *      more work to do.
 */
void
svc_exit(void)
{
#ifdef _REENTRANT
	extern rwlock_t svc_fd_lock;
#endif

	rwlock_wrlock(&svc_fd_lock);
	svc_fdset_zero();
	rwlock_unlock(&svc_fd_lock);
}
