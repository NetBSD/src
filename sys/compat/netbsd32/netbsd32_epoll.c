/*	$NetBSD: netbsd32_epoll.c,v 1.3 2023/07/30 07:56:15 rin Exp $	*/

/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2007 Roman Divacky
 * Copyright (c) 2014 Dmitry Chagin <dchagin@FreeBSD.org>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: netbsd32_epoll.c,v 1.3 2023/07/30 07:56:15 rin Exp $");

#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/kmem.h>
#include <sys/systm.h>
#include <sys/syscall.h>
#include <sys/syscallargs.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_conv.h>
#include <compat/netbsd32/netbsd32_syscall.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>

int
netbsd32_epoll_create1(struct lwp *l,
    const struct netbsd32_epoll_create1_args *uap, register_t *retval)
{
	/* {
		syscallarg(int)		flags;
	} */
	struct sys_epoll_create1_args ua;

	NETBSD32TO64_UAP(flags);
	return sys_epoll_create1(l, &ua, retval);
}

int
netbsd32_epoll_ctl(struct lwp *l, const struct netbsd32_epoll_ctl_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(int) epfd;
		syscallarg(int) op;
		syscallarg(int) fd;
		syscallarg(netbsd32_epoll_eventp_t) event;
	} */
	struct epoll_event ee, *eep;
	int error;

	if (SCARG(uap, op) != EPOLL_CTL_DEL) {
		struct netbsd32_epoll_event ee32;

		error = copyin(SCARG_P32(uap, event), &ee32, sizeof(ee32));
		if (error != 0)
			return error;

		netbsd32_to_epoll_event(&ee32, &ee);
		eep = &ee;
	} else
		eep = NULL;

	return epoll_ctl_common(l, retval, SCARG(uap, epfd), SCARG(uap, op),
	    SCARG(uap, fd), eep);
}

int
netbsd32_epoll_pwait2(struct lwp *l,
    const struct netbsd32_epoll_pwait2_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) epfd;
		syscallarg(netbsd32_epoll_eventp_t) events;
		syscallarg(int) maxevents;
		syscallarg(netbsd32_timespecp_t) timeout;
		syscallarg(netbsd32_sigsetp_t) sigmask;
	} */
	struct epoll_event *events;
	struct timespec ts, *tsp;
	sigset_t ss, *ssp;
	int error;
	const int maxevents = SCARG(uap, maxevents);

	if (maxevents <= 0 || maxevents >= EPOLL_MAX_EVENTS)
		return EINVAL;

	if (SCARG_P32(uap, timeout) != NULL) {
		struct netbsd32_timespec ts32;

		error = copyin(SCARG_P32(uap, timeout), &ts32, sizeof(ts32));
		if (error != 0)
			return error;

		netbsd32_to_timespec(&ts32, &ts);
		tsp = &ts;
	} else
		tsp = NULL;

	if (SCARG_P32(uap, sigmask) != NULL) {
		error = copyin(SCARG_P32(uap, sigmask), &ss, sizeof(ss));
		if (error != 0)
			return error;

		ssp = &ss;
	} else
		ssp = NULL;

	events = kmem_alloc(maxevents * sizeof(*events), KM_SLEEP);

	error = epoll_wait_common(l, retval, SCARG(uap, epfd), events,
	    maxevents, tsp, ssp);
	if (error != 0 || *retval == 0)
		goto out;

	struct netbsd32_epoll_event *events32 =
	    kmem_alloc(*retval * sizeof(*events32), KM_SLEEP);

	for (int i = 0; i < *retval; i++)
		netbsd32_from_epoll_event(&events[i], &events32[i]);

	error = copyout(events, SCARG_P32(uap, events),
	    *retval * sizeof(*events32));

	kmem_free(events32, *retval * sizeof(*events32));

 out:
	kmem_free(events, maxevents * sizeof(*events));
	return error;
}
